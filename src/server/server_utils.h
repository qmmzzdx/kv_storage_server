#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <iostream>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "../utils/asynclog.h"
#include "../utils/kv_constant.h"

// 连接节点结构体，管理客户端连接状态
struct ConnectionNode
{
    int fd = -1;                            // 文件描述符
    uint32_t state = 0;                     // 连接状态
    size_t rbuf_size = 0;                   // 读缓冲区数据大小
    uint8_t rbuf[4 + MAX_MSG] = {};         // 读缓冲区
    size_t wbuf_size = 0;                   // 写缓冲区数据大小
    size_t wbuf_sent = 0;                   // 已发送字节数
    uint8_t wbuf[4 + MAX_MSG] = {};         // 写缓冲区
};

// 跳表节点类
class Node
{
public:
    Node() {}

    Node(const int64_t& _key, const std::string& _value, const int level) : key(_key), value(_value)
    {
        node_ptr_list.resize(level + 1);  // 初始化指针数组
    }

    ~Node() {}

    int64_t get_key() const { return key; }           // 获取键
    std::string get_value() const { return value; }   // 获取值
    void set_value(const std::string _value) { value = _value; }  // 设置值

    std::vector<std::shared_ptr<Node>> node_ptr_list;  // 指向不同层的指针数组

private:
    int64_t key;        // 节点键
    std::string value;  // 节点值
};

// 跳表类，实现有序集合
class SkipList
{
public:
    SkipList(int _max_level = 18) : max_level(_max_level), skiplist_level(0), element_count(0)
    {
        header = std::make_shared<Node>(int64_t(), std::string(), max_level);  // 创建头节点
    }

    ~SkipList() {}

    // 生成随机层数
    int get_random_level()
    {
        int cur_level = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::bernoulli_distribution distrib(0.25);  // 25%概率增加层数

        while (distrib(gen))
        {
            ++cur_level;
        }
        return std::min(cur_level, max_level);  // 不超过最大层数
    }

    // 创建新节点
    std::shared_ptr<Node> create_node(const int64_t& _key, const std::string& _value, const int _level)
    {
        return std::make_shared<Node>(_key, _value, _level);
    }

    // 插入节点
    bool insert(const int64_t& _key, const std::string& _value)
    {
        std::shared_ptr<Node> cur = header;
        std::vector<std::shared_ptr<Node>> update(max_level + 1);  // 更新路径数组

        // 从最高层开始查找插入位置
        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
            update[i] = cur;  // 记录每层的插入位置
        }
        cur = cur->node_ptr_list[0];
        if (cur == nullptr || cur->get_key() != _key)  // 键不存在，可以插入
        {
            int random_level = get_random_level();
            if (skiplist_level < random_level)  // 更新跳表层级
            {
                for (int i = skiplist_level + 1; i <= random_level; ++i) { update[i] = header; }
                skiplist_level = random_level;
            }
            auto new_node = create_node(_key, _value, random_level);
            for (int i = 0; i <= random_level; ++i)  // 更新各层指针
            {
                new_node->node_ptr_list[i] = update[i]->node_ptr_list[i];
                update[i]->node_ptr_list[i] = new_node;
            }
            ++element_count;
            return true;
        }
        return false;  // 键已存在
    }

    // 搜索节点
    bool search(const int64_t& _key)
    {
        std::shared_ptr<Node> cur = header;

        // 从最高层开始搜索
        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
        }
        cur = cur->node_ptr_list[0];
        return cur != nullptr && cur->get_key() == _key;  // 检查是否找到
    }

    // 删除节点
    bool cancel(const int64_t& _key)
    {
        std::shared_ptr<Node> cur = header;
        std::vector<std::shared_ptr<Node>> update(max_level + 1);  // 更新路径数组

        // 从最高层开始查找删除位置
        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
            update[i] = cur;  // 记录每层的删除位置
        }
        cur = cur->node_ptr_list[0];
        if (cur != nullptr && cur->get_key() == _key)  // 找到要删除的节点
        {
            for (int i = 0; i <= skiplist_level; ++i)  // 更新各层指针
            {
                if (update[i]->node_ptr_list[i] != cur) { break; }
                update[i]->node_ptr_list[i] = cur->node_ptr_list[i];
            }
            // 更新跳表层级
            while (skiplist_level > 0 && header->node_ptr_list[skiplist_level] == nullptr) { --skiplist_level; }
            --element_count;
            return true;
        }
        return false;  // 节点不存在
    }

    int size() { return element_count; }  // 返回元素数量

private:
    int max_level;                       // 最大层数
    int skiplist_level;                  // 当前层数
    int element_count;                   // 元素数量
    std::shared_ptr<Node> header;        // 头节点
};

// KV存储数据类，单例模式
class KvStroageData
{
public:
    static KvStroageData& Instance()  // 获取单例实例
    {
        static KvStroageData instance;
        return instance;
    }

    ~KvStroageData() {}

    size_t hsize() { return hstrhash.size(); }  // 字符串哈希表大小
    size_t zsize() { return zsethash.size(); }  // 有序集合哈希表大小

    // 获取所有键
    std::vector<std::string> get_all_keys()
    {
        std::vector<std::string> keys;
        for (auto& x : hstrhash) { keys.emplace_back(x.first); }  // 添加字符串键
        for (auto& x : zsethash) { keys.emplace_back(x.first); }  // 添加有序集合键
        return keys;
    }

    std::unordered_map<std::string, std::string>& hstrhash_ref() { return hstrhash; }  // 字符串哈希表引用
    std::unordered_map<std::string, int64_t>& hzset_ref() { return zsethash; }         // 有序集合哈希表引用
    SkipList& hzlist_ref() { return zsetlist; }                                        // 跳表引用

private:
    std::unordered_map<std::string, std::string> hstrhash;  // 字符串存储哈希表
    SkipList zsetlist;                                      // 有序集合跳表
    std::unordered_map<std::string, int64_t> zsethash;      // 有序集合哈希表

    KvStroageData() {}  // 私有构造函数
    KvStroageData(const KvStroageData&) = delete;  // 禁止拷贝
    KvStroageData& operator=(const KvStroageData&) = delete;  // 禁止赋值
};

// 函数声明

// 信号处理函数
void signal_handler(int signum);

// 格式化异步日志写入
void format_asynclog_write(const char* file_name, const char* func_name, int cur_line, const char* log_mes, AsyncLog::LogLevel level);

// epoll相关函数
int Epoll_create1(int flags);
void Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);

// 网络相关函数
int open_listenfd(const char* port);
int Open_listenfd(const char* port);
int Accept(int fd, struct sockaddr* addr, socklen_t* addrlen);
void fd_set_nb(int fd);

// 序列化函数
void out_nil(std::string& out);
void out_str(std::string& out, const std::string& val);
void out_int(std::string& out, int64_t val);
void out_err(std::string& out, int32_t code, const std::string& msg);
void out_arr(std::string& out, uint32_t n);

// 请求解析函数
int parse_request(const uint8_t* buf, size_t len, std::vector<std::string>& out);
int accept_new_connection(std::vector<std::unique_ptr<ConnectionNode>>& fd_to_connection, int fd);

// 命令处理辅助函数
bool judge_cmd(const std::string& word, const char* cmd);
bool str_to_int(const std::string& s, int64_t& out);

// 命令处理函数
void do_keys(const std::vector<std::string>& cmd, std::string& out);
void do_get(const std::vector<std::string>& cmd, std::string& out);
void do_set(const std::vector<std::string>& cmd, std::string& out);
void do_del(const std::vector<std::string>& cmd, std::string& out);
void do_zadd(const std::vector<std::string>& cmd, std::string& out);
void do_zrem(const std::vector<std::string>& cmd, std::string& out);
void do_zscore(const std::vector<std::string>& cmd, std::string& out);
void do_zcard(const std::vector<std::string>& cmd, std::string& out);

// 请求路由函数
void do_request(std::vector<std::string>& cmd, std::string& out);

// 连接状态处理函数
bool try_one_request(std::unique_ptr<ConnectionNode>& conn);
bool try_fill_buffer(std::unique_ptr<ConnectionNode>& conn);
bool try_flush_buffer(std::unique_ptr<ConnectionNode>& conn);
void state_req(std::unique_ptr<ConnectionNode>& conn);
void state_res(std::unique_ptr<ConnectionNode>& conn);
void connection_io(std::unique_ptr<ConnectionNode>& conn);

#endif
