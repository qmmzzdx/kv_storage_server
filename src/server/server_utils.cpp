#include "server_utils.h"
#include "../utils/asynclog.h"

void signal_handler(int signum)
{
    AsyncLog::LOG_INFO("Received signal " + std::to_string(signum) + ".");
    AsyncLog::AsyncLog::Instance().Close();
}

int Epoll_create1(int flags)
{
    int rc;

    if ((rc = epoll_create1(flags)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "epoll_create1() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    return rc;
}

void Epoll_ctl(int epfd, int op, int fd, struct epoll_event* event)
{
    int rc;

    if ((rc = epoll_ctl(epfd, op, fd, event)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "epoll_ctl() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
}

int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
{
    int rc;

    if ((rc = epoll_wait(epfd, events, maxevents, timeout)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "epoll_wait() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    return rc;
}

int open_listenfd(const char* port)
{
    struct addrinfo hints;
    struct addrinfo* p;
    struct addrinfo* listp;
    int listenfd, rc, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
    if ((rc = getaddrinfo(NULL, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "getaddrinfo failed (port " + std::string(port) + ").\n";
        AsyncLog::LOG_ERROR(strprint);
        return -2;
    }
    for (p = listp; p; p = p->ai_next)
    {
        if ((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) { continue; }
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void*>(&optval), sizeof(int));
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) { break; }
        if (close(listenfd) < 0)
        {
            fprintf(stderr, "KV storage failed, please check the backend logs.\n");
            std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
            strprint += "open_listenfd close failed: " + std::string(strerror(errno)) + ".\n";
            AsyncLog::LOG_ERROR(strprint);
            return -1;
        }
    }
    freeaddrinfo(listp);
    if (!p)
    {
        return -1;
    }
    if (listen(listenfd, SOMAXCONN) < 0)
    {
        close(listenfd);
        return -1;
    }
    return listenfd;
}

int Open_listenfd(const char* port)
{
    int rc;

    if ((rc = open_listenfd(port)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "Open_listenfd error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    return rc;
}

int Accept(int fd, struct sockaddr* addr, socklen_t* addrlen)
{
    int rc;

    if ((rc = accept(fd, addr, addrlen)) < 0)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "Accept error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    return rc;
}

void fd_set_nb(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "fcntl error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "fcntl error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
}

void out_nil(std::string& out)
{
    out.push_back(SERIAL_NIL);
}

void out_str(std::string& out, const std::string& val)
{
    out.push_back(SERIAL_STR);
    uint32_t len = static_cast<uint32_t>(val.size());
    out.append(reinterpret_cast<const char*>(&len), 4);
    out.append(val);
}

void out_int(std::string& out, int64_t val)
{
    out.push_back(SERIAL_INT);
    out.append(reinterpret_cast<const char*>(&val), 8);
}

void out_err(std::string& out, int32_t code, const std::string& msg)
{
    out.push_back(SERIAL_ERR);
    out.append(reinterpret_cast<const char*>(&code), 4);
    uint32_t len = static_cast<uint32_t>(msg.size());
    out.append(reinterpret_cast<const char*>(&len), 4);
    out.append(msg);
}

void out_arr(std::string& out, uint32_t n)
{
    out.push_back(SERIAL_ARR);
    out.append(reinterpret_cast<const char*>(&n), 4);
}

int parse_request(const uint8_t* buf, size_t len, std::vector<std::string>& out)
{
    if (len < 4) { return -1; }
    uint32_t n = 0;
    memcpy(&n, buf, 4);
    if (n > MAX_ARGS) { return -1; }

    size_t pos = 4;
    while (n--)
    {
        if (pos + 4 > len) { return -1; }
        uint32_t sz = 0;
        memcpy(&sz, &buf[pos], 4);
        if (pos + 4 + sz > len) { return -1; }
        out.push_back(std::string(buf + pos + 4, buf + pos + 4 + sz));
        pos += 4 + sz;
    }
    return pos != len ? -1 : 0;
}

int accept_new_connection(std::vector<std::unique_ptr<ConnectionNode>>& fd_to_connection, int fd)
{
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = Accept(fd, reinterpret_cast<struct sockaddr*>(&client_addr), &socklen);

    fd_set_nb(connfd);
    std::unique_ptr<ConnectionNode> conn = std::make_unique<ConnectionNode>();
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    if (fd_to_connection.size() <= static_cast<size_t>(conn->fd))
    {
        fd_to_connection.resize(conn->fd + 1);
    }
    fd_to_connection[conn->fd] = std::move(conn);
    return connfd;
}

bool judge_cmd(const std::string& word, const char* cmd)
{
    return 0 == strcasecmp(word.c_str(), cmd);
}

bool str_to_int(const std::string& s, int64_t& out)
{
    char* endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

void do_keys(const std::vector<std::string>& cmd, std::string& out)
{
    out_arr(out, static_cast<uint32_t>(KvStroageData::Instance().hsize() + KvStroageData::Instance().zsize()));
    for (auto& key : KvStroageData::Instance().get_all_keys()) { out_str(out, key); }
}

void do_get(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    if (gmap.count(cmd[2]))
    {
        out_str(out, gmap[cmd[2]]);
        return;
    }
    out_nil(out);
}

void do_set(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    gmap[cmd[2]] = cmd[3];
    out_nil(out);
}

void do_del(const std::vector<std::string>& cmd, std::string& out)
{
    auto& gmap = KvStroageData::Instance().hstrhash_ref();
    int ret = gmap.count(cmd[2]);
    if (ret) { gmap.erase(cmd[2]); }
    out_int(out, ret);
}

void do_zadd(const std::vector<std::string>& cmd, std::string& out)
{
    int64_t score = 0;
    if (!str_to_int(cmd[2], score))
    {
        out_err(out, ERR_TYPE, "expect score number");
        return;
    }
    auto& zset = KvStroageData::Instance().hzset_ref();
    auto& zlist = KvStroageData::Instance().hzlist_ref();
    if (!zset.count(cmd[3]) && !zlist.search(score))
    {
        zset[cmd[3]] = score;
        zlist.insert(score, cmd[3]);
        out_int(out, 1);
        return;
    }
    out_err(out, ERR_ARG, "key or value already exists");
}

void do_zrem(const std::vector<std::string>& cmd, std::string& out)
{
    auto& zset = KvStroageData::Instance().hzset_ref();
    int ret = zset.count(cmd[2]);
    if (ret)
    {
        auto val = zset[cmd[2]];
        auto& zlist = KvStroageData::Instance().hzlist_ref();
        if (zlist.cancel(val)) { zset.erase(cmd[2]); }
        else { ret = 0; }
    }
    out_int(out, ret);
}

void do_zscore(const std::vector<std::string>& cmd, std::string& out)
{
    auto& zset = KvStroageData::Instance().hzset_ref();
    if (zset.count(cmd[2]))
    {
        out_int(out, zset[cmd[2]]);
        return;
    }
    out_err(out, ERR_ARG, "key don't exists");
}

void do_zcard(const std::vector<std::string>& cmd, std::string& out)
{
    out_int(out, KvStroageData::Instance().zsize());
}

void do_request(std::vector<std::string>& cmd, std::string& out)
{
    if (cmd.size() == 1 && judge_cmd(cmd[0], "keys"))
    {
        do_keys(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_keys operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "get") && judge_cmd(cmd[1], "str"))
    {
        do_get(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_get operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 4 && judge_cmd(cmd[0], "set") && judge_cmd(cmd[1], "str"))
    {
        do_set(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_set operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "del") && judge_cmd(cmd[1], "str"))
    {
        do_del(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_del operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 4 && judge_cmd(cmd[0], "zadd") && judge_cmd(cmd[1], "zset"))
    {
        do_zadd(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_zadd operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "zrem") && judge_cmd(cmd[1], "zset"))
    {
        do_zrem(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_zrem operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 3 && judge_cmd(cmd[0], "zscore") && judge_cmd(cmd[1], "zset"))
    {
        do_zscore(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_zscore operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    else if (cmd.size() == 2 && judge_cmd(cmd[0], "zcard") && judge_cmd(cmd[1], "zset"))
    {
        do_zcard(cmd, out);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
        strprint += "execute do_zcard operation.\n";
        AsyncLog::LOG_INFO(strprint);
        return;
    }
    out_err(out, ERR_UNKNOWN, "Unknown cmd");
    std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 1) + ", ";
    strprint += "Unknown cmd.\n";
    AsyncLog::LOG_WARN(strprint);
}

bool try_one_request(std::unique_ptr<ConnectionNode>& conn)
{
    if (conn->rbuf_size < 4) { return false; }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > MAX_MSG)
    {
        fprintf(stderr, "command is too long.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "command is too long.\n";
        AsyncLog::LOG_WARN(strprint);
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size) { return false; }

    std::vector<std::string> cmd;
    if (parse_request(&conn->rbuf[4], len, cmd) < 0)
    {
        fprintf(stderr, "bad request.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "bad request.\n";
        AsyncLog::LOG_WARN(strprint);
        conn->state = STATE_END;
        return false;
    }
    std::string out;
    do_request(cmd, out);
    if (4 + out.size() > MAX_MSG)
    {
        out.clear();
        out_err(out, ERR_TOO_BIG, "response is too big.");
    }
    uint32_t wlen = static_cast<uint32_t>(out.size());
    memcpy(&conn->wbuf[0], &wlen, 4);
    memcpy(&conn->wbuf[4], out.c_str(), out.size());
    conn->wbuf_size = 4 + wlen;

    size_t remain = conn->rbuf_size - 4 - len;
    if (remain > 0)
    {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain, conn->state = STATE_RES;
    state_res(conn);
    return conn->state == STATE_REQ;
}

bool try_fill_buffer(std::unique_ptr<ConnectionNode>& conn)
{
    if (conn->rbuf_size >= sizeof(conn->rbuf))
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "read() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read = 0;
    do
    {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        bytes_read = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while (bytes_read < 0 && errno == EINTR);
    if (bytes_read < 0)
    {
        if (errno == EAGAIN) { return false; }
        fprintf(stderr, "read() error.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 4) + ", ";
        strprint += "read() error.\n";
        AsyncLog::LOG_WARN(strprint);
        conn->state = STATE_END;
        return false;
    }
    if (bytes_read == 0)
    {
        fprintf(stderr, "read() %s from client %d.\n", conn->rbuf_size > 0 ? "unexpected EOF" : "EOF", conn->fd);
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "read() " + std::string(conn->rbuf_size > 0 ? "unexpected EOF" : "EOF") + " from client " + std::to_string(conn->fd) + ".\n";
        AsyncLog::LOG_WARN(strprint);
        conn->state = STATE_END;
        return false;
    }
    conn->rbuf_size += static_cast<size_t>(bytes_read);
    if (conn->rbuf_size > sizeof(conn->rbuf))
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "read() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    while (try_one_request(conn)) { continue; }
    return conn->state == STATE_REQ;
}

bool try_flush_buffer(std::unique_ptr<ConnectionNode>& conn)
{
    ssize_t bytes_written = 0;
    do
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        bytes_written = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
    } while (bytes_written < 0 && errno == EINTR);
    if (bytes_written < 0)
    {
        if (errno == EAGAIN) { return false; }
        fprintf(stderr, "write() error.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 4) + ", ";
        strprint += "write() error.\n";
        AsyncLog::LOG_WARN(strprint);
        conn->state = STATE_END;
        return false;
    }
    conn->wbuf_sent += static_cast<size_t>(bytes_written);
    if (conn->wbuf_sent > conn->wbuf_size)
    {
        fprintf(stderr, "KV storage failed, please check the backend logs.\n");
        std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
        strprint += "write() error: " + std::to_string(errno) + ".\n";
        AsyncLog::LOG_ERROR(strprint);
        exit(EXIT_FAILURE);
    }
    if (conn->wbuf_sent == conn->wbuf_size)
    {
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }
    return true;
}

void state_req(std::unique_ptr<ConnectionNode>& conn)
{
    while (try_fill_buffer(conn)) { continue; }
}

void state_res(std::unique_ptr<ConnectionNode>& conn)
{
    while (try_flush_buffer(conn)) { continue; }
}

void connection_io(std::unique_ptr<ConnectionNode>& conn)
{
    switch (conn->state)
    {
        case STATE_REQ:
        {
            state_req(conn);
            break;
        }
        case STATE_RES:
        {
            state_res(conn);
            break;
        }
        case STATE_END:
        {
            fprintf(stderr, "end to read and write.\n");
            std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
            strprint += "end to read and write.\n";
            AsyncLog::LOG_INFO(strprint);
            break;
        }
        default:
        {
            fprintf(stderr, "unexpected state.\n");
            std::string strprint = "In file " + std::string(__FILE__) + " function(" + std::string(__func__) + ")" + " Line " + std::to_string(__LINE__ - 3) + ", ";
            strprint += "unexpected state.\n";
            AsyncLog::LOG_WARN(strprint);
            break;
        }
    }
}
