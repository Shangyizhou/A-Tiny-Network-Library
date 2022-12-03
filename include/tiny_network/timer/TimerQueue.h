#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H

#include "Timestamp.h"
#include "Channel.h"

#include <vector>
#include <set>

class EventLoop;
class Timer;

class TimerQueue
{
public:
    using TimerCallback = std::function<void()>;

    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    // 插入定时器（回调函数，到期时间，是否重复）
    void addTimer(TimerCallback cb,
                  Timestamp when,
                  double interval);
    
private:
    using Entry = std::pair<Timestamp, Timer*>; // 以时间戳作为键值获取定时器
    using TimerList = std::set<Entry>;          // 底层使用红黑树管理，自动按照时间戳进行排序

    // 在本loop中添加定时器
    // 线程安全
    void addTimerInLoop(Timer* timer);

    // 定时器读事件触发的函数
    void handleRead();

    // 重新设置timerfd_
    void resetTimerfd(int timerfd_, Timestamp expiration);
    
    // 移除所有已到期的定时器
    // 1.获取到期的定时器
    // 2.重置这些定时器（销毁或者重复定时任务）
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    // 插入定时器的内部方法
    bool insert(Timer* timer);

    EventLoop* loop_;           // 所属的EventLoop
    const int timerfd_;         // timerfd是Linux提供的定时器接口
    Channel timerfdChannel_;    // 封装timerfd_文件描述符
    // Timer list sorted by expiration
    TimerList timers_;          // 定时器队列（内部实现是红黑树）

    bool callingExpiredTimers_; // 标明正在获取超时定时器
};

#endif // TIMER_QUEUE_H