#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

// 对 epoll 的简单封装
class Epoller {
  public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();
    bool AddFd(int fd, uint32_t events);
    bool ModFd(int fd, uint32_t events);
    bool DelFd(int fd);
    int Wait(int time_out_MS = -1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;

  private:
    // epoll 实例，用 epoll_fd_ 来增改删 fd
    int epoll_fd_;

    // 当有监听事件到来时，将相关事件放到 events 中
    std::vector<struct epoll_event> events_;
};

#endif //__EPOLLER_H__
