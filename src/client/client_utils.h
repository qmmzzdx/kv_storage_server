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

ssize_t read_full(int fd, char* usrbuf, size_t n);

ssize_t write_all(int fd, const char* usrbuf, size_t n);

int open_clientfd(const char* hostname, const char* port);

int Open_clientfd(const char* hostname, const char* port);

int32_t on_response(const uint8_t* data, uint32_t size);

ssize_t send_request(int fd, const std::vector<std::string>& cmd);

ssize_t recv_request(int fd);

#endif
