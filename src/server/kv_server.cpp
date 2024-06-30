#include "server_utils.h"
#include "../utils/asynclog.h"

int main(int argc, char* argv[])
{
    int fd = Open_listenfd("1234");
    std::vector<std::unique_ptr<ConnectionNode>> fd_to_connection;

    fd_set_nb(fd);
    int epoll_fd = Epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event ev;
    ev.events = EPOLLIN, ev.data.fd = fd;
    Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    while (true)
    {
        std::vector<struct epoll_event> events(SOMAXCONN);
        int ret = Epoll_wait(epoll_fd, events.data(), events.size(), TIMEOUT_VAL);
        for (int i = 0; i < ret; ++i)
        {
            if (events[i].data.fd == fd)
            {
                int connfd = accept_new_connection(fd_to_connection, fd);
                ev.events = EPOLLIN, ev.data.fd = connfd;
                Epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connfd, &ev);
            }
            else
            {
                auto& conn = fd_to_connection[events[i].data.fd];
                connection_io(conn);
                if (conn->state == STATE_END)
                {
                    Epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, nullptr);
                    close(conn->fd);
                    fd_to_connection[conn->fd].reset(nullptr);
                }
                else
                {
                    ev.events = (conn->state == STATE_REQ ? EPOLLIN : EPOLLOUT) | EPOLLERR;
                    ev.data.fd = conn->fd;
                    Epoll_ctl(epoll_fd, EPOLL_CTL_MOD, conn->fd, &ev);
                }
            }
        }
    }
    close(fd);
    close(epoll_fd);
    AsyncLog::Instance().Close();

    return 0;
}
