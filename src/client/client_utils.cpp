#include "client_utils.h"

// 从文件描述符读取指定数量的字节，确保读取完整
ssize_t read_full(int fd, char* usrbuf, size_t n)
{
    size_t nleft = n;        // 剩余需要读取的字节数
    ssize_t nread;           // 单次读取的字节数
    char* bufp = usrbuf;     // 当前缓冲区位置

    // 循环读取直到读取完指定数量的字节
    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            // 处理中断错误，继续读取
            if (errno == EINTR) { nread = 0; }
            else { return -1; }  // 其他错误返回-1
        }
        else if (nread == 0) { break; }  // EOF，连接关闭
        nleft -= nread, bufp += nread;   // 更新剩余字节数和缓冲区位置
    }
    return n - nleft;  // 返回实际读取的字节数
}

// 向文件描述符写入指定数量的字节，确保写入完整
ssize_t write_all(int fd, const char* usrbuf, size_t n)
{
    size_t nleft = n;          // 剩余需要写入的字节数
    ssize_t nwritten;          // 单次写入的字节数
    const char* bufp = usrbuf; // 当前缓冲区位置

    // 循环写入直到写入完指定数量的字节
    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            // 处理中断错误，继续写入
            if (errno == EINTR) { nwritten = 0; }
            else { return -1; }  // 其他错误返回-1
        }
        nleft -= nwritten, bufp += nwritten;  // 更新剩余字节数和缓冲区位置
    }
    return n;  // 写入成功返回请求的字节数
}

// 创建客户端socket并连接到服务器
int open_clientfd(const char* hostname, const char* port)
{
    int clientfd, rc;
    struct addrinfo hints;
    struct addrinfo* p;
    struct addrinfo* listp;

    // 设置地址信息提示
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;  // TCP socket
    hints.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG;  // 端口为数字，仅配置地址

    // 获取地址信息列表
    if ((rc = getaddrinfo(hostname, port, &hints, &listp)) != 0)
    {
        fprintf(stderr, "getaddrinfo() failed.\n");
        return -2;
    }

    // 遍历地址列表，尝试连接
    for (p = listp; p; p = p->ai_next)
    {
        // 创建socket
        if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) { continue; }
        // 尝试连接
        if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1) { break; }
        // 连接失败，关闭socket
        if (close(clientfd) < 0)
        {
            fprintf(stderr, "close error: %s.\n", strerror(errno));
            return -1;
        }
    }
    freeaddrinfo(listp);  // 释放地址信息列表
    return p ? clientfd : -1;  // 返回连接成功的socket或-1
}

// 包装函数，连接失败时退出程序
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

// 解析服务器响应数据
int32_t on_response(const uint8_t* data, uint32_t size)
{
    if (size < 1)
    {
        fprintf(stderr, "bad response.\n");
        return -1;
    }
    int32_t res = -1;

    // 根据响应类型进行解析
    switch (data[0])
    {
        case SERIAL_NIL:  // 空值响应
        {
            fprintf(stderr, "(nil)\n");
            res = 1;  // 只消耗类型字节
            break;
        }
        case SERIAL_ERR:  // 错误响应
        {
            // 检查数据长度是否足够
            if (size < 1 + 8)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t code = 0, len = 0;
            memcpy(&code, &data[1], 4), memcpy(&len, &data[5], 4);  // 提取错误码和消息长度
            if (size < 1 + 8 + len)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            printf("(err) %u %.*s\n", code, len, &data[9]);  // 打印错误信息
            res = 1 + 8 + len;  // 类型 + 错误码(4) + 长度(4) + 消息内容
            break;
        }
        case SERIAL_STR:  // 字符串响应
        {
            if (size < 1 + 4)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);  // 提取字符串长度
            if (size < 1 + 4 + len)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            printf("(str) %.*s\n", len, &data[5]);  // 打印字符串内容
            res = 1 + 4 + len;  // 类型 + 长度(4) + 字符串内容
            break;
        }
        case SERIAL_INT:  // 整数响应
        {
            if (size < 1 + 8)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            int64_t val = 0;
            memcpy(&val, &data[1], 8);  // 提取整数值
            printf("(int) %ld\n", val);  // 打印整数值
            res = 1 + 8;  // 类型 + 整数值(8)
            break;
        }
        case SERIAL_ARR:  // 数组响应
        {
            if (size < 1 + 4)
            {
                fprintf(stderr, "bad response.\n");
                res = -1;
                break;
            }
            uint32_t len = 0;
            memcpy(&len, &data[1], 4);  // 提取数组长度
            printf("(arr) len = %u\n", len);  // 打印数组长度
            size_t arr_bytes = 5;  // 已处理的字节数（类型 + 数组长度）

            // 递归解析数组中的每个元素
            for (uint32_t i = 0; i < len; ++i)
            {
                int32_t rv = on_response(&data[arr_bytes], size - arr_bytes);
                if (rv < 0) { return rv; }  // 解析失败
                arr_bytes += static_cast<size_t>(rv);  // 更新已处理字节数
            }
            printf("(arr) end\n");
            res = static_cast<int32_t>(arr_bytes);  // 返回总共处理的字节数
            break;
        }
        default:  // 未知响应类型
        {
            fprintf(stderr, "bad response.\n");
            res = -1;
            break;
        }
    }
    return res;
}

// 发送请求到服务器
ssize_t send_request(int fd, const std::vector<std::string>& cmd)
{
    // 计算消息总长度
    uint32_t len = 4;  // 字符串数量字段
    for (const auto& s : cmd)
    {
        len += 4 + s.size();  // 每个字符串：长度字段 + 字符串内容
    }
    if (len > MAX_MSG) { return -1; }  // 检查消息长度限制

    // 构建发送缓冲区
    char wbuf[4 + MAX_MSG];
    memcpy(&wbuf[0], &len, 4);  // 写入消息总长度
    uint32_t n = static_cast<uint32_t>(cmd.size());
    memcpy(&wbuf[4], &n, 4);    // 写入字符串数量

    // 写入每个字符串
    size_t cur = 8;  // 当前写入位置（已写入总长度和字符串数量）
    for (const auto& s : cmd)
    {
        uint32_t p = static_cast<uint32_t>(s.size());
        memcpy(&wbuf[cur], &p, 4);                    // 写入字符串长度
        memcpy(&wbuf[cur + 4], s.c_str(), s.size());  // 写入字符串内容
        cur += 4 + s.size();                          // 更新写入位置
    }
    return write_all(fd, wbuf, 4 + len);  // 发送完整消息
}

// 接收服务器响应
ssize_t recv_request(int fd)
{
    char rbuf[4 + MAX_MSG + 1];  // 接收缓冲区

    // 读取消息长度字段
    errno = 0;
    ssize_t ret = read_full(fd, rbuf, 4);
    if (ret < 0)
    {
        fprintf(stderr, "%s.\n", errno == 0 ? "read() EOF" : "read() error");
        return ret;
    }

    // 提取消息长度
    uint32_t len = 0;
    memcpy(&len, rbuf, 4);
    if (len > MAX_MSG)
    {
        fprintf(stderr, "command is too long.\n");
        return -1;
    }

    // 读取消息内容
    ret = read_full(fd, &rbuf[4], len);
    if (ret < 0)
    {
        fprintf(stderr, "read() error.\n");
        return ret;
    }

    // 解析响应
    int32_t rv = on_response(reinterpret_cast<uint8_t*>(&rbuf[4]), len);
    if (rv > 0 && static_cast<uint32_t>(rv) != len)
    {
        fprintf(stderr, "bad response.\n");  // 响应长度不匹配
        rv = -1;
    }
    return rv;
}
