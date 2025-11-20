#include "server_utils.h"

// 信号处理函数，处理程序终止信号
void signal_handler(int signum)
{
    AsyncLog::LOG_INFO("Received signal " + std::to_string(signum) + ".\n");
    AsyncLog::AsyncLog::Instance().Close();
}

// 格式化并写入异步日志
void format_asynclog_write(const char* file_name, const char* func_name, int cur_line, const char* log_mes, AsyncLog::LogLevel level)
{
    std::string strprint = "In file " + std::string(file_name) + " function(" + std::string(func_name) + ")";
    strprint += " Line " + std::to_string(cur_line) + ", " + std::string(log_mes);
    switch (level)
    {
        case AsyncLog::LogLevel::DEBUG:
        {
            AsyncLog::LOG_DEBUG(strprint);
            break;
        }
        case AsyncLog::LogLevel::INFO:
        {
            AsyncLog::LOG_INFO(strprint);
            break;
        }
        case AsyncLog::LogLevel::WARN:
        {
            AsyncLog::LOG_WARN(strprint);
            break;
        }
        case AsyncLog::LogLevel::ERROR:
        {
            AsyncLog::LOG_ERROR(strprint + std::to_string(errno) + ".\n");
            break;
        }
    }
}

// 创建epoll实例，失败时退出程序
int Epoll_create1(int flags)
{
    int rc;

    if ((rc = epoll_create1(flags)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "epoll_create1() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    return rc;
}

// epoll控制操作，失败时退出程序
void Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event)
{
    int rc;

    if ((rc = epoll_ctl(epfd, op, fd, event)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "epoll_ctl() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
}

// 等待epoll事件，失败时退出程序
int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
    int rc;

    if ((rc = epoll_wait(epfd, events, maxevents, timeout)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "epoll_wait() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    return rc;
}

// 创建监听socket
int open_listenfd(const char* port)
{
    struct addrinfo hints;
    struct addrinfo* p;
    struct addrinfo* listp;
    int listenfd, rc, optval = 1;

    // 设置地址信息提示
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;  // 服务器socket，端口为数字

    // 获取地址信息列表
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "open_listenfd getaddrinfo() error: ", AsyncLog::LogLevel::ERROR);
        return -2;
    }

    // 遍历地址列表，尝试绑定
    for (p = listp; p; p = p->ai_next)
    {
        // 创建socket
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) { continue; }
        // 设置SO_REUSEADDR选项
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void*>(&optval), sizeof(int));
        // 尝试绑定
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) { break; }
        // 绑定失败，关闭socket
        if (close(listenfd) < 0)
        {
            fprintf(stderr, "KV storage failed, please check the backend logs.\n");
            format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "open_listenfd close() error: ", AsyncLog::LogLevel::ERROR);
            return -1;
        }
    }
    freeaddrinfo(listp);  // 释放地址信息列表
    if (!p)
    {
        return -1;  // 没有找到可绑定的地址
    }
    // 开始监听
    if (listen(listenfd, SOMAXCONN) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;  // 返回监听socket
}

// 包装函数，创建监听socket失败时退出程序
int Open_listenfd(const char* port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "open_listenfd() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    return rc;
}

// 接受客户端连接，失败时退出程序
int Accept(int fd, struct sockaddr* addr, socklen_t* addrlen)
{
    int rc;

    if ((rc = accept(fd, addr, addrlen)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "accept() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    return rc;
}

// 设置文件描述符为非阻塞模式
void fd_set_nb(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 4, "fcntl() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;  // 添加非阻塞标志
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "fcntl() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
}

// 序列化NIL值到输出缓冲区
void out_nil(std::string& out)
{
    out.push_back(SERIAL_NIL);
}

// 序列化字符串值到输出缓冲区
void out_str(std::string& out, const std::string& val)
{
    out.push_back(SERIAL_STR);  // 类型标识
    uint32_t len = static_cast<uint32_t>(val.size());
    out.append(reinterpret_cast<const char*>(&len), 4);  // 长度字段
    out.append(val);  // 字符串内容
}

// 序列化整数值到输出缓冲区
void out_int(std::string& out, int64_t val)
{
    out.push_back(SERIAL_INT);  // 类型标识
    out.append(reinterpret_cast<const char*>(&val), 8);  // 整数值
}

// 序列化错误信息到输出缓冲区
void out_err(std::string& out, int32_t code, const std::string& msg)
{
    out.push_back(SERIAL_ERR);  // 类型标识
    out.append(reinterpret_cast<const char*>(&code), 4);  // 错误码
    uint32_t len = static_cast<uint32_t>(msg.size());
    out.append(reinterpret_cast<const char*>(&len), 4);  // 消息长度
    out.append(msg);  // 错误消息
}

// 序列化数组到输出缓冲区
void out_arr(std::string& out, uint32_t n)
{
    out.push_back(SERIAL_ARR);  // 类型标识
    out.append(reinterpret_cast<const char*>(&n), 4);  // 数组长度
}

// 解析客户端请求
int parse_request(const uint8_t* buf, size_t len, std::vector<std::string>& out)
{
    if (len < 4) { return -1; }  // 检查最小长度
    uint32_t n = 0;
    memcpy(&n, buf, 4);  // 提取字符串数量
    if (n > MAX_ARGS) { return -1; }  // 检查参数数量限制

    size_t pos = 4;  // 当前位置（跳过字符串数量字段）
    while (n--)
    {
        if (pos + 4 > len) { return -1; }  // 检查长度字段是否完整
        uint32_t sz = 0;
        memcpy(&sz, &buf[pos], 4);  // 提取字符串长度
        if (pos + 4 + sz > len) { return -1; }  // 检查字符串内容是否完整
        // 提取字符串内容
        out.push_back(std::string(buf + pos + 4, buf + pos + 4 + sz));
        pos += 4 + sz;  // 更新当前位置
    }
    return pos != len ? -1 : 0;  // 检查是否解析了所有数据
}

// 接受新的客户端连接
int accept_new_connection(std::vector<std::unique_ptr<ConnectionNode>>& fd_to_connection, int fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    // 接受客户端连接
    int connfd = Accept(fd, reinterpret_cast<struct sockaddr*>(&client_addr), &socklen);

    // 设置新连接为非阻塞模式
    fd_set_nb(connfd);
    // 创建连接节点
    std::unique_ptr<ConnectionNode> conn = std::make_unique<ConnectionNode>();
    conn->fd = connfd;
    conn->state = STATE_REQ;  // 初始状态为请求读取
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    // 确保连接向量足够大
    if (fd_to_connection.size() <= static_cast<size_t>(conn->fd))
    {
        fd_to_connection.resize(conn->fd + 1);
    }
    fd_to_connection[conn->fd] = std::move(conn);
    return connfd;
}

// 判断命令字符串是否匹配
bool judge_cmd(const std::string& word, const char* cmd)
{
    return 0 == strcasecmp(word.c_str(), cmd);  // 不区分大小写比较
}

// 字符串转换为整数
bool str_to_int(const std::string& s, int64_t& out)
{
    char* endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);  // 转换为整数
    return endp == s.c_str() + s.size();  // 检查是否完全转换
}

// 处理keys命令：返回所有键
void do_keys(const std::vector<std::string>& cmd, std::string& out)
{
    // 计算总键数并输出数组
    out_arr(out, static_cast<uint32_t>(KvStroageData::Instance().hsize() + KvStroageData::Instance().zsize()));
    // 输出所有键
    for (auto& key : KvStroageData::Instance().get_all_keys()) { out_str(out, key); }
}

// 处理get命令：获取字符串值
void do_get(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    if (gmap.count(cmd[2]))
    {
        out_str(out, gmap[cmd[2]]);  // 找到键，返回字符串值
        return;
    }
    out_nil(out);  // 键不存在，返回NIL
}

// 处理set命令：设置字符串值
void do_set(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    gmap[cmd[2]] = cmd[3];  // 设置键值对
    out_nil(out);  // 返回NIL表示成功
}

// 处理del命令：删除键
void do_del(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    int ret = gmap.count(cmd[2]);  // 检查键是否存在
    if (ret) { gmap.erase(cmd[2]); }  // 存在则删除
    out_int(out, ret);  // 返回删除结果（1成功，0失败）
}

// 处理zadd命令：添加有序集合成员
void do_zadd(const std::vector<std::string>& cmd, std::string& out)
{
    int64_t score = 0;
    if (!str_to_int(cmd[2], score))  // 转换分数
    {
        out_err(out, ERR_TYPE, "expect score number");  // 分数格式错误
        return;
    }
    auto& zset = KvStroageData::Instance().hzset_ref();
    auto& zlist = KvStroageData::Instance().hzlist_ref();
    // 检查成员和分数是否已存在
    if (!zset.count(cmd[3]) && !zlist.search(score))
    {
        zset[cmd[3]] = score;  // 添加到哈希表
        zlist.insert(score, cmd[3]);  // 添加到跳表
        out_int(out, 1);  // 返回成功
        return;
    }
    out_err(out, ERR_ARG, "key or value already exists");  // 成员或分数已存在
}

// 处理zrem命令：删除有序集合成员
void do_zrem(const std::vector<std::string>& cmd, std::string& out)
{
    auto& zset = KvStroageData::Instance().hzset_ref();
    int ret = zset.count(cmd[2]);  // 检查成员是否存在
    if (ret)
    {
        auto val = zset[cmd[2]];  // 获取分数
        auto& zlist = KvStroageData::Instance().hzlist_ref();
        if (zlist.cancel(val)) { zset.erase(cmd[2]); }  // 从跳表删除并从哈希表删除
        else { ret = 0; }  // 删除失败
    }
    out_int(out, ret);  // 返回删除结果
}

// 处理zscore命令：获取有序集合成员分数
void do_zscore(const std::vector<std::string>& cmd, std::string& out)
{
    auto& zset = KvStroageData::Instance().hzset_ref();
    if (zset.count(cmd[2]))
    {
        out_int(out, zset[cmd[2]]);  // 返回分数
        return;
    }
    out_err(out, ERR_ARG, "key don't exists");  // 成员不存在
}

// 处理zcard命令：获取有序集合元素数量
void do_zcard(const std::vector<std::string>& cmd, std::string& out)
{
    out_int(out, KvStroageData::Instance().zsize());  // 返回集合大小
}

// 处理客户端请求，路由到相应的命令处理函数
void do_request(std::vector<std::string>& cmd, std::string& out)
{
    // 根据命令类型路由到相应的处理函数
    if (cmd.size() == 1 && judge_cmd(cmd[0], "keys"))
    {
        do_keys(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_keys operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "get") && judge_cmd(cmd[1], "str"))
    {
        do_get(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_get operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 4 && judge_cmd(cmd[0], "set") && judge_cmd(cmd[1], "str"))
    {
        do_set(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_set operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "del") && judge_cmd(cmd[1], "str"))
    {
        do_del(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_del operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 4 && judge_cmd(cmd[0], "zadd") && judge_cmd(cmd[1], "zset"))
    {
        do_zadd(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_zadd operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "zrem") && judge_cmd(cmd[1], "zset"))
    {
        do_zrem(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_zrem operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "zscore") && judge_cmd(cmd[1], "zset"))
    {
        do_zscore(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_zscore operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    else if (cmd.size() == 2 && judge_cmd(cmd[0], "zcard") && judge_cmd(cmd[1], "zset"))
    {
        do_zcard(cmd, out);
        format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "execute do_zcard operation.\n", AsyncLog::LogLevel::INFO);
        return;
    }
    out_err(out, ERR_UNKNOWN, "Unknown cmd");  // 未知命令
    format_asynclog_write(__FILE__, __func__, __LINE__ - 1, "Unknown cmd.\n", AsyncLog::LogLevel::WARN);
}

// 尝试处理一个完整的请求
bool try_one_request(std::unique_ptr<ConnectionNode>& conn)
{
    if (conn->rbuf_size < 4) { return false; }  // 检查是否有足够数据读取消息长度
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);  // 提取消息长度
    if (len > MAX_MSG)
    {
        fprintf(stderr, "command is too long.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "command is too long.\n", AsyncLog::LogLevel::WARN);
        conn->state = STATE_END;  // 消息过长，结束连接
        return false;
    }
    if (4 + len > conn->rbuf_size) { return false; }  // 检查是否收到完整消息

    // 解析请求
    std::vector<std::string> cmd;
    if (parse_request(&conn->rbuf[4], len, cmd) < 0)
    {
        fprintf(stderr, "bad request.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "bad request.\n", AsyncLog::LogLevel::WARN);
        conn->state = STATE_END;  // 解析失败，结束连接
        return false;
    }

    // 处理请求并生成响应
    std::string out;
    do_request(cmd, out);

    // 检查响应大小
    if (4 + out.size() > MAX_MSG)
    {
        out.clear();
        out_err(out, ERR_TOO_BIG, "response is too big.");  // 响应过大
    }

    // 准备发送响应
    uint32_t wlen = static_cast<uint32_t>(out.size());
    memcpy(&conn->wbuf[0], &wlen, 4);  // 写入响应长度
    memcpy(&conn->wbuf[4], out.c_str(), out.size());  // 写入响应内容
    conn->wbuf_size = 4 + wlen;  // 设置写入缓冲区大小

    // 移动剩余数据到缓冲区开头
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain > 0)
    {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain, conn->state = STATE_RES;  // 更新状态为响应写入
    state_res(conn);  // 尝试发送响应
    return conn->state == STATE_REQ;  // 返回是否准备好处理下一个请求
}

// 尝试填充读取缓冲区
bool try_fill_buffer(std::unique_ptr<ConnectionNode>& conn)
{
    if (conn->rbuf_size >= sizeof(conn->rbuf))  // 检查缓冲区是否已满
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "read() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read = 0;
    // 循环读取数据，处理中断
    do
    {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;  // 剩余缓冲区容量
        bytes_read = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (bytes_read < 0 && errno == EINTR);  // 处理中断

    if (bytes_read < 0)
    {
        if (errno == EAGAIN) { return false; }  // 非阻塞，没有数据可读
        fprintf(stderr, "read() error.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 4, "read() error: ", AsyncLog::LogLevel::ERROR);
        conn->state = STATE_END;  // 读取错误，结束连接
        return false;
    }
    if (bytes_read == 0)
    {
        // 连接关闭
        fprintf(stderr, "read() %s from client %d.\n", conn->rbuf_size > 0 ? "unexpected EOF" : "EOF", conn->fd);
        std::string log_message = "read() " + std::string(conn->rbuf_size > 0 ? "unexpected EOF" : "EOF") + " from client " + std::to_string(conn->fd) + ".\n";
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, log_message.c_str(), AsyncLog::LogLevel::WARN);
        conn->state = STATE_END;  // 连接关闭，结束连接
        return false;
    }
    conn->rbuf_size += static_cast<size_t>(bytes_read);  // 更新缓冲区大小
    if (conn->rbuf_size > sizeof(conn->rbuf))  // 检查缓冲区溢出
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "read() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    // 处理所有完整的请求
    while (try_one_request(conn)) { continue; }
    return conn->state == STATE_REQ;  // 返回连接状态
}

// 尝试刷新写入缓冲区
bool try_flush_buffer(std::unique_ptr<ConnectionNode>& conn)
{
    ssize_t bytes_written = 0;
    // 循环写入数据，处理中断
    do
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;  // 剩余待发送数据
        bytes_written = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (bytes_written < 0 && errno == EINTR);  // 处理中断

    if (bytes_written < 0)
    {
        if (errno == EAGAIN) { return false; }  // 非阻塞，暂时无法写入
        fprintf(stderr, "write() error.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 4, "write() error: ", AsyncLog::LogLevel::ERROR);
        conn->state = STATE_END;  // 写入错误，结束连接
        return false;
    }
    conn->wbuf_sent += static_cast<size_t>(bytes_written);  // 更新已发送字节数
    if (conn->wbuf_sent > conn->wbuf_size)  // 检查发送字节数是否超出
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "write() error: ", AsyncLog::LogLevel::ERROR);
        exit(EXIT_FAILURE);
    }
    if (conn->wbuf_sent == conn->wbuf_size)  // 检查是否发送完成
    {
        conn->state = STATE_REQ;  // 切换回请求读取状态
        conn->wbuf_sent = 0;  // 重置发送位置
        conn->wbuf_size = 0;  // 重置缓冲区大小
        return false;  // 发送完成
    }
    return true;  // 还有数据待发送
}

// 处理请求读取状态
void state_req(std::unique_ptr<ConnectionNode>& conn)
{
    while (try_fill_buffer(conn)) { continue; }  // 持续填充缓冲区直到无法读取
}

// 处理响应写入状态
void state_res(std::unique_ptr<ConnectionNode>& conn)
{
    while (try_flush_buffer(conn)) { continue; }  // 持续刷新缓冲区直到无法写入
}

// 处理连接I/O操作
void connection_io(std::unique_ptr<ConnectionNode>& conn)
{
    switch (conn->state)
    {
        case STATE_REQ:  // 请求读取状态
        {
            state_req(conn);
            break;
        }
        case STATE_RES:  // 响应写入状态
        {
            state_res(conn);
            break;
        }
        case STATE_END:  // 连接结束状态
        {
            fprintf(stderr, "end to read and write.\n");
            format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "end to read and write.\n", AsyncLog::LogLevel::INFO);
            break;
        }
        default:  // 未知状态
        {
            fprintf(stderr, "unexpected state.\n");
            format_asynclog_write(__FILE__, __func__, __LINE__ - 3, "unexpected state.\n", AsyncLog::LogLevel::WARN);
            break;
        }
    }
}
