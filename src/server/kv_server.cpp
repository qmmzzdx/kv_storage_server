#include "server_utils.h"

// 服务器主函数
int main(int argc, char* argv[])
{
    // 设置信号处理函数，处理程序终止信号
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 创建监听socket
    int fd = Open_listenfd("1234");
    std::vector<std::unique_ptr<ConnectionNode>> fd_to_connection;

    // 设置监听socket为非阻塞模式
    fd_set_nb(fd);

    // 创建epoll实例
    int epoll_fd = Epoll_create1(EPOLL_CLOEXEC);

    // 设置监听socket的epoll事件
    struct epoll_event ev;
    ev.events = EPOLLIN, ev.data.fd = fd;
    Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);

    // 主事件循环
    while (true)
    {
        // 等待事件发生
        std::vector<struct epoll_event> events(SOMAXCONN);
        int ret = Epoll_wait(epoll_fd, events.data(), events.size(), TIMEOUT_VAL);

        // 处理所有就绪的事件
        for (int i = 0; i < ret; ++i)
        {
            if (events[i].data.fd == fd)
            {
                // 新的客户端连接请求
                int connfd = accept_new_connection(fd_to_connection, fd);
                ev.events = EPOLLIN, ev.data.fd = connfd;
                Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else
            {
                // 处理客户端连接的数据
                auto& conn = fd_to_connection[events[i].data.fd];
                connection_io(conn);

                if (conn->state == STATE_END)
                {
                    // 连接结束，清理资源
                    Epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
                    close(conn->fd);
                    fd_to_connection[conn->fd].reset(nullptr);
                }
                else
                {
                    // 根据连接状态更新epoll监听事件
                    ev.events = (conn->state == STATE_REQ ? EPOLLIN : EPOLLOUT) | EPOLLERR;
                    ev.data.fd = conn->fd;
                    Epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev);
                }
            }
        }
    }

    // 清理资源（实际上不会执行到这里，因为上面是无限循环）
    close(fd);
    close(epoll_fd);

    return 0;
}
