#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <string.h>

#define MAX_EVENTS 1024
#define BUFFER_SIZE 1024

int epoll_fd;

int coming_events_cnt;
struct epoll_event coming_events[MAX_EVENTS];

void on_request(const int fd)
{
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT;
    event.data.fd = fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        perror("epoll_ctl");
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }
}

void create_epoll()
{
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
}

void destroy_epoll()
{
    close(epoll_fd);
}

void epoll()
{
    coming_events_cnt = epoll_wait(epoll_fd, coming_events, MAX_EVENTS, -1); // block until any event is available
    if (coming_events_cnt == -1)
    {
        perror("epoll_wait");
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }
}

int handle_events()
{
    for (int i = 0; i < coming_events_cnt; i++)
    {
        char buf[BUFFER_SIZE];
        const ssize_t count = read(coming_events[i].data.fd, buf, sizeof(buf));
        if (count == 6 && strncmp(buf, ".exit", 5) == 0)
        {
            return 0;
        }
        if (count > 0)
        {
            write(coming_events[i].data.fd, buf, count);
        }
    }

    return 1;
}

int multimain()
{
    create_epoll();

    on_request(STDIN_FILENO);

    do
    {
        epoll();
    } while (handle_events());

    destroy_epoll();
    return 0;
}
