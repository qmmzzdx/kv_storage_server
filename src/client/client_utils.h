#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <string>
#include <vector>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include "../utils/kv_constant.h"

// 从文件描述符读取指定数量的字节，确保读取完整
ssize_t read_full(int fd, char* usrbuf, size_t n);

// 向文件描述符写入指定数量的字节，确保写入完整
ssize_t write_all(int fd, const char* usrbuf, size_t n);

// 创建客户端socket并连接到服务器
int open_clientfd(const char* hostname, const char* port);

// 包装函数，连接失败时退出程序
int Open_clientfd(const char* hostname, const char* port);

// 解析服务器响应数据
int32_t on_response(const uint8_t* data, uint32_t size);

// 发送请求到服务器
ssize_t send_request(int fd, const std::vector<std::string>& cmd);

// 接收服务器响应
ssize_t recv_request(int fd);

#endif
