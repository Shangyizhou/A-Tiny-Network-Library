#ifndef EPOLLPOLLER_H
#define EPOLLPOLLER_H

#include <vector>
#include <sys/epoll.h>
#include <unistd.h>

#include "Logging.h"
#include "Poller.h"
#include "Timestamp.h"

/**
 * epoll_create
 * epoll_ctl
 * epoll_wait
 */
class EPollPoller : public Poller
{
    using EventList = std::vector<epoll_event>;
public:
    EPollPoller(EventLoop *Loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    // 默认监听事件数量
    static const int kInitEventListSize = 16; 

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel *channel);

    int epollfd_;       // epoll_create在内核创建空间返回的fd
    EventList events_;  // 用于存放epoll_wait返回的所有发生的事件的文件描述符
};

#endif // EPOLLPOLLER_H