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

struct ConnectionNode
{
    int fd = -1;
    uint32_t state = 0;
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + MAX_MSG] = {};
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + MAX_MSG] = {};
};

class Node
{
public:
    Node() {}

    Node(const int64_t& _key, const std::string& _value, const int level) : key(_key), value(_value)
    {
        node_ptr_list.resize(level + 1);
    }

    ~Node() {}

    int64_t get_key() const { return key; }

    std::string get_value() const { return value; }

    void set_value(const std::string _value) { value = _value; }

    std::vector<std::shared_ptr<Node>> node_ptr_list;

private:
    int64_t key;
    std::string value;
};

class SkipList
{
public:
    SkipList(int _max_level = 18) : max_level(_max_level), skiplist_level(0), element_count(0)
    {
        header = std::make_shared<Node>(int64_t(), std::string(), max_level);
    }

    ~SkipList() {}

    int get_random_level()
    {
        int cur_level = 0;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::bernoulli_distribution distrib(0.25);

        while (distrib(gen))
        {
            ++cur_level;
        }
        return std::min(cur_level, max_level);
    }

    std::shared_ptr<Node> create_node(const int64_t& _key, const std::string& _value, const int _level)
    {
        return std::make_shared<Node>(_key, _value, _level);
    }

    bool insert(const int64_t& _key, const std::string& _value)
    {
        std::shared_ptr<Node> cur = header;
        std::vector<std::shared_ptr<Node>> update(max_level + 1);

        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
            update[i] = cur;
        }
        cur = cur->node_ptr_list[0];
        if (cur == nullptr || cur->get_key() != _key)
        {
            int random_level = get_random_level();
            if (skiplist_level < random_level)
            {
                for (int i = skiplist_level + 1; i <= random_level; ++i) { update[i] = header; }
                skiplist_level = random_level;
            }
            auto new_node = create_node(_key, _value, random_level);
            for (int i = 0; i <= random_level; ++i)
            {
                new_node->node_ptr_list[i] = update[i]->node_ptr_list[i];
                update[i]->node_ptr_list[i] = new_node;
            }
            ++element_count;
            return true;
        }
        return false;
    }

    bool search(const int64_t& _key)
    {
        std::shared_ptr<Node> cur = header;

        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
        }
        cur = cur->node_ptr_list[0];
        return cur != nullptr && cur->get_key() == _key;
    }

    bool cancel(const int64_t& _key)
    {
        std::shared_ptr<Node> cur = header;
        std::vector<std::shared_ptr<Node>> update(max_level + 1);

        for (int i = skiplist_level; i >= 0; --i)
        {
            while (cur->node_ptr_list[i] != nullptr && cur->node_ptr_list[i]->get_key() < _key) { cur = cur->node_ptr_list[i]; }
            update[i] = cur;
        }
        cur = cur->node_ptr_list[0];
        if (cur != nullptr && cur->get_key() == _key)
        {
            for (int i = 0; i <= skiplist_level; ++i)
            {
                if (update[i]->node_ptr_list[i] != cur) { break; }
                update[i]->node_ptr_list[i] = cur->node_ptr_list[i];
            }
            while (skiplist_level > 0 && header->node_ptr_list[skiplist_level] == nullptr) { --skiplist_level; }
            --element_count;
            return true;
        }
        return false;
    }

    int size() { return element_count; }

private:
    int max_level;
    int skiplist_level;
    int element_count;
    std::shared_ptr<Node> header;
};

class KvStroageData
{
public:
    static KvStroageData& Instance()
    {
        static KvStroageData instance;
        return instance;
    }

    ~KvStroageData() {}

    size_t hsize() { return hstrhash.size(); }
    size_t zsize() { return zsethash.size(); }

    std::vector<std::string> get_all_keys()
    {
        std::vector<std::string> keys;
        for (auto& x : hstrhash) { keys.emplace_back(x.first); }
        for (auto& x : zsethash) { keys.emplace_back(x.first); }
        return keys;
    }

    std::unordered_map<std::string, std::string>& hstrhash_ref() { return hstrhash; }

    std::unordered_map<std::string, int64_t>& hzset_ref() { return zsethash; }

    SkipList& hzlist_ref() { return zsetlist; }

private:
    std::unordered_map<std::string, std::string> hstrhash;
    SkipList zsetlist;
    std::unordered_map<std::string, int64_t> zsethash;

    KvStroageData() {}
    KvStroageData(const KvStroageData&) = delete;
    KvStroageData& operator=(const KvStroageData&) = delete;
};

void signal_handler(int signum);

void format_asynclog_write(const char* file_name, const char* func_name, int cur_line, const char* log_mes, AsyncLog::LogLevel level);

int Epoll_create1(int flags);

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);

int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);

int open_listenfd(const char* port);

int Open_listenfd(const char* port);

int Accept(int fd, struct sockaddr* addr, socklen_t* addrlen);

void fd_set_nb(int fd);

void out_nil(std::string& out);

void out_str(std::string& out, const std::string& val);

void out_int(std::string& out, int64_t val);

void out_err(std::string& out, int32_t code, const std::string& msg);

void out_arr(std::string& out, uint32_t n);

int parse_request(const uint8_t* buf, size_t len, std::vector<std::string>& out);

int accept_new_connection(std::vector<std::unique_ptr<ConnectionNode>>& fd_to_connection, int fd);

bool judge_cmd(const std::string& word, const char* cmd);

bool str_to_int(const std::string& s, int64_t& out);

void do_keys(const std::vector<std::string>& cmd, std::string& out);

void do_get(const std::vector<std::string>& cmd, std::string& out);

void do_set(const std::vector<std::string>& cmd, std::string& out);

void do_del(const std::vector<std::string>& cmd, std::string& out);

void do_zadd(const std::vector<std::string>& cmd, std::string& out);

void do_zrem(const std::vector<std::string>& cmd, std::string& out);

void do_zscore(const std::vector<std::string>& cmd, std::string& out);

void do_zcard(const std::vector<std::string>& cmd, std::string& out);

void do_request(std::vector<std::string>& cmd, std::string& out);

bool try_one_request(std::unique_ptr<ConnectionNode>& conn);

bool try_fill_buffer(std::unique_ptr<ConnectionNode>& conn);

bool try_flush_buffer(std::unique_ptr<ConnectionNode>& conn);

void state_req(std::unique_ptr<ConnectionNode>& conn);

void state_res(std::unique_ptr<ConnectionNode>& conn);

void connection_io(std::unique_ptr<ConnectionNode>& conn);

#endif
