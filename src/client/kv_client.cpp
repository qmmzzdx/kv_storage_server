#include <iostream>
#include <iomanip>
#include <sstream>
#include "client_utils.h"

// 处理单个请求：发送命令并接收响应
void process_request(int fd, const std::vector<std::string>& cmd)
{
    if (send_request(fd, cmd) < 0 || recv_request(fd) < 0) { return; }
}

// 客户端主函数
int main(int argc, char* argv[])
{
    // 连接到服务器
    int clientfd = Open_clientfd("127.0.0.1", "1234");

    std::string s, t;
    std::vector<std::string> cmd;

    // 主循环：读取用户输入并处理命令
    while (std::cout << "KV storage(designed by mzzdx)> ", std::getline(std::cin >> std::ws, s))
    {
        cmd.clear();
        std::stringstream sin(s);

        // 解析输入的命令参数
        while (sin >> t) { cmd.push_back(t); }

        // 处理请求
        process_request(clientfd, cmd);
    }

    // 关闭客户端连接
    close(clientfd);

    return 0;
}
