#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "TimerQueue.h"
#include <functional>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

class Channel;
class Poller;
// 事件循环类 主要包含了两大模块，channel poller
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前线程同步调用函数
    void runInLoop(Functor cb);
    /**
     * 把cb放入队列，唤醒loop所在的线程执行cb
     * 
     * 实例情况：
     * 在mainLoop中获取subLoop指针，然后调用相应函数
     * 在queueLoop中发现当前的线程不是创建这个subLoop的线程，将此函数装入subLoop的pendingFunctors容器中
     * 之后mainLoop线程会调用subLoop::wakeup向subLoop的eventFd写数据，以此唤醒subLoop来执行pengdingFunctors
     */
    void queueInLoop(Functor cb);

    // 用来唤醒loop所在的线程
    void wakeup();

    // EventLoop的方法 => Poller
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // 判断EventLoop是否在自己的线程
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    /**
     * 定时任务相关函数
     */
    void runAt(Timestamp timestamp, Functor&& cb) {
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }

    void runAfter(double waitTime, Functor&& cb) {
        Timestamp time(addTime(Timestamp::now(), waitTime)); 
        runAt(time, std::move(cb));
    }

    void runEvery(double interval, Functor&& cb) {
        Timestamp timestamp(addTime(Timestamp::now(), interval)); 
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }
private:
    void handleRead();
    void doPendingFunctors();

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;  // 原子操作，通过CAS实现
    std::atomic_bool quit_;     // 标志退出事件循环
    std::atomic_bool callingPendingFunctors_; // 标志当前loop是否有需要执行的回调操作
    const pid_t threadId_;      // 记录当前loop所在线程的id
    Timestamp pollReturnTime_;  // poller返回发生事件的channels的返回时间
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;
    
    /**
     * TODO:eventfd用于线程通知机制，libevent和我的webserver是使用sockepair
     * 作用：当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 
     * 通过该成员唤醒subLoop处理Channel
     */
    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;            // 活跃的Channel
    Channel* currentActiveChannel_;         // 当前处理的活跃channel
    std::mutex mutex_;                      // 用于保护pendingFunctors_线程安全操作
    std::vector<Functor> pendingFunctors_;  // 存储loop跨线程需要执行的所有回调操作
};


#endif // EVENTLOOP_H