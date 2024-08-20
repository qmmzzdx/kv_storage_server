#include "client_utils.h"

ssize_t read_full(int fd, char* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char* bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            if (errno == EINTR) { nread = 0; }
            else { return -1; }
        }
        else if (nread == 0) { break; }
        nleft -= nread, bufp += nread;
    }
    return n - nleft;
}

ssize_t write_all(int fd, const char* usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    const char* bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            if (errno == EINTR) { nwritten = 0; }
            else { return -1; }
        }
        nleft -= nwritten, bufp += nwritten;
    }
    return n;
}

int open_clientfd(const char* hostname, const char* port)
{
    int clientfd, rc;
    struct addrinfo hints;
    struct addrinfo* p;
    struct addrinfo* listp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo() failed.\n");
        return -2;
    }
    for (p = listp; p; p = p->ai_next)
    {
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) { continue; }
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) { break; }
        if (close(clientfd) < 0)
        {
            fprintf(stderr, "close error: %s.\n", strerror(errno));
            return -1;
        }
    }
    freeaddrinfo(listp);
    return p ? clientfd : -1;
}

int Open_clientfd(const char* hostname, const char* port)
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0)
    {
        fprintf(stderr, "Open_clientfd error: %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return rc;
}

int32_t on_response(const uint8_t* data, uint32_t size)
{
    if (size < 1)
    {
        fprintf(stderr, "bad response.\n");
        return -1;
    }
    int32_t res = -1;
    switch (data[0])
    {
        case SERIAL_NIL:
        {
            fprintf(stderr, "(nil)\n");
            res = 1;
            break;
        }
        case SERIAL_ERR:
        {
            if (size < 1 + 8)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t code = 0, len = 0;
            memcpy(&code, &data[1], 4), memcpy(&len, &data[5], 4);
            if (size < 1 + 8 + len)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            printf("(err) %u %.*s\n", code, len, &data[9]);
            res = 1 + 8 + len;
            break;
        }
        case SERIAL_STR:
        {
            if (size < 1 + 4)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);
            if (size < 1 + 4 + len)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            printf("(str) %.*s\n", len, &data[5]);
            res = 1 + 4 + len;
            break;
        }
        case SERIAL_INT:
        {
            if (size < 1 + 8)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            int64_t val = 0;
            memcpy(&val, &data[1], 8);
            printf("(int) %ld\n", val);
            res = 1 + 8;
            break;
        }
        case SERIAL_ARR:
        {
            if (size < 1 + 4)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);
            printf("(arr) len = %u\n", len);
            size_t arr_bytes = 5;
            for (uint32_t i = 0; i < len; ++i)
            {
                int32_t rv = on_response(&data[arr_bytes], size - arr_bytes);
                if (rv < 0) { return rv; }
                arr_bytes += static_cast<size_t>(rv);
            }
            printf("(arr) end\n");
            res = static_cast<int32_t>(arr_bytes);
            break;
        }
        default:
        {
            fprintf(stderr, "bad response.\n");
            res = -1;
            break;
        }
    }
    return res;
}

ssize_t send_request(int fd, const std::vector<std::string>& cmd)
{
    uint32_t len = 4;
    for (const auto& s : cmd)
    {
        len += 4 + s.size();
    }
    if (len > MAX_MSG) { return -1; }
    char wbuf[4 + MAX_MSG];
    memcpy(&wbuf[0], &len, 4);
    uint32_t n = static_cast<uint32_t>(cmd.size());
    memcpy(&wbuf[4], &n, 4);

    size_t cur = 8;
    for (const auto& s : cmd)
    {
        uint32_t p = static_cast<uint32_t>(s.size());
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.c_str(), s.size());
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len);
}

ssize_t recv_request(int fd)
{
    char rbuf[4 + MAX_MSG + 1];

    errno = 0;
    ssize_t ret = read_full(fd, rbuf, 4);
    if (ret < 0)
    {
        fprintf(stderr, "%s.\n", errno == 0 ? "read() EOF" : "read() error");
        return ret;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > MAX_MSG)
    {
        fprintf(stderr, "command is too long.\n");
        return -1;
    }
    ret = read_full(fd, &rbuf[4], len);
    if (ret < 0)
    {
        fprintf(stderr, "read() error.\n");
        return ret;
    }

    int32_t rv = on_response(reinterpret_cast<uint8_t*>(&rbuf[4]), len);
    if (rv > 0 && static_cast<uint32_t>(rv) != len)
    {
        fprintf(stderr, "bad response.\n");
        rv = -1;
    }
    return rv;
}
