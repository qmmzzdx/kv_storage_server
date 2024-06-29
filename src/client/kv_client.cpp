#include <iostream>
#include <iomanip>
#include <sstream>
#include "client_utils.h"

void process_request(int fd, const std::vector<std::string>& cmd)
{
    if (send_request(fd, cmd) < 0 || recv_request(fd) < 0) { return; }
}

int main(int argc, char* argv[])
{
    int clientfd = Open_clientfd("127.0.0.1", "1234");

    std::string s, t;
    std::vector<std::string> cmd;
    while (std::cout << "KV storage(designed by mzzdx)> ", std::getline(std::cin >> std::ws, s))
    {
        cmd.clear();
        std::stringstream sin(s);
        while (sin >> t) { cmd.push_back(t); }
        process_request(clientfd, cmd);
    }
    close(clientfd);

    return 0;
}
