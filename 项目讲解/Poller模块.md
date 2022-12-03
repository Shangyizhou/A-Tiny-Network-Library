# 基类Poller的设计

我们编写网络编程代码的时候少不了使用IO复用系列函数，而muduo也为我们提供了对此的封装。muduo 有 Poller 和 EPollPoller 类分别对应着`epoll`和`poll`。

而我们使用的接口是`Poller`，muduo 以Poller 为虚基类，派生出 Poller 和 EPollPoller 两个子类，用不同的形式实现 IO 复用。

```cpp
class Poller : noncopyable
{
 public:
  // Poller关注的Channel
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /**
   * 需要交给派生类实现的接口
   * 用于监听感兴趣的事件和fd(封装成了channel)
   * 对于Poller是poll，对于EPollerPoller是epoll_wait
   * 最后返回epoll_wait/poll的返回时间
   */
  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

  // 需要交给派生类实现的接口（须在EventLoop所在的线程调用）
  // 更新事件，channel::update->eventloop::updateChannel->Poller::updateChannel
  virtual void updateChannel(Channel* channel) = 0;

  // 需要交给派生类实现的接口（须在EventLoop所在的线程调用）
  // 当Channel销毁时移除此Channel
  virtual void removeChannel(Channel* channel) = 0;
  
  // 需要交给派生类实现的接口
  virtual bool hasChannel(Channel* channel) const;
  
  /** 
   * newDefaultPoller获取一个默认的Poller对象（内部实现可能是epoll或poll）
   * 它的实现并不在 Poller.cc 文件中
   * 如果要实现则可以预料其会包含EPollPoller PollPoller
   * 那么外面就会在基类引用派生类的头文件，这个抽象的设计就不好
   * 所以外面会单独创建一个 DefaultPoller.cc 的文件去实现
   */
  static Poller* newDefaultPoller(EventLoop* loop);

  // 断言是否在创建EventLoop的所在线程
  void assertInLoopThread() const
  {
    ownerLoop_->assertInLoopThread();
  }

 protected:
  // 保存fd => Channel的映射
  typedef std::map<int, Channel*> ChannelMap;
  ChannelMap channels_;

 private:
  EventLoop* ownerLoop_;
};
```

- `ChannelMap channels_` 需要存储从 fd -> channel 的映射
- `ownerLoop_` 定义 Poller 所属的事件循环 EventLoop

**newDefaultPoller**

重写方法靠派生类实现，这里我们可以专注一下 `newDefaultPoler` 方法。

在 muduo 中可以使用此方法获取不同的实例，并且这个方法是在单独的一个 `DefaultPoller.cc` 文件内实现的。正常情况下，我们可能会在 `Poller.cc` 文件中完成该成员函数的实现。但是这并不是一个好的设计，因为 Poller 是一个基类。如果在 `Poller.cc` 文件内实现则势必会在 `Poller.cc`包含 `EPollPoller.h` 等头文件。在一个基类中包含其派生类的头文件，这个设计可以说是很诡异的，这并不是一个好的抽象。

因此，我们专门设置了另一个`DefaultPoller.cc` 文件，在其中包含了 `Poller.h` 和 `EPollPoller.h` 的头文件。这样就让 `Poller.h` 文件显得正常了。

```cpp
#include "muduo/net/Poller.h"
#include "muduo/net/poller/PollPoller.h"
#include "muduo/net/poller/EPollPoller.h"

#include <stdlib.h>

using namespace muduo::net;

// 获取默认的Poller实现方式
Poller* Poller::newDefaultPoller(EventLoop* loop)
{
      if (::getenv("MUDUO_USE_POLL"))
      {
            // poll
            return new PollPoller(loop);
      }
      else
      {
            // epoll
            return new EPollPoller(loop);
      }
}
```
# EPollPoller类设计
```cpp
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    // 默认监听事件数量
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    // 填写活跃的连接
    // EventLoop内部调用此方法，会将发生了事件的Channel填充到activeChannels中
    void fillActiveChannels(int numEvents,
                            ChannelList* activeChannels) const;
    // 更新channel通道，本质是调用了epoll_ctl
    void update(int operation, Channel* channel);
    
    // epoll_event数组
    typedef std::vector<struct epoll_event> EventList;

    // epoll句柄
    int epollfd_;
    // epoll_event数组
    EventList events_;
};
```

- `kInitEventListSize` 默监听事件的数量
- `epollfd_` 我们使用 epoll_create 创建的指向 epoll 对象的文件描述符（句柄）
- `EventList events_` 返回事件的数组

可以看到，EPollPoller类继承了Poller类，并打算重写这些核心方法。
## 成员函数
### 返回发生事件的 poll 方法
该方法内部调用 epoll_wait 获取发生的事件，并找到这些事件对应的 Channel 并将这些活跃的 Channel 填充入 activeChannels 中，最后返回一个时间戳。
通过 `numEvents` 的值判断事件情况

-  `numEvents > 0`
   - 事件发生，需要调用 `fillActiveChannels` 填充活的 Channel
- `numEvents == 0`
   - 事件超时了，打印日志。（可以设置定时器操作）
- 其他情况则是出错，打印 `LOG_ERROR`日志
```cpp
// 核心函数，其内部不断调用epoll_wait获取发生事件
Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    LOG_TRACE << "fd total count " << channels_.size();

    // 用epoll_wait获取发生事件，事件填充到了events_内部
    // 注意设置了定时器
    int numEvents = ::epoll_wait(epollfd_,
                                &*events_.begin(),
                                static_cast<int>(events_.size()),
                                timeoutMs);
    int savedErrno = errno;
    // 获取当前时间戳
    Timestamp now(Timestamp::now());
    // 有事件产生
    if (numEvents > 0)
    {
        LOG_TRACE << numEvents << " events happened";
        // 填充活跃channel到activeChannels中
        fillActiveChannels(numEvents, activeChannels);
        // TODO:implicit_cast
        if (implicit_cast<size_t>(numEvents) == events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    // 无事件产生
    else if (numEvents == 0)
    {
        LOG_TRACE << "nothing happened";
    }
    else
    {
        // error happens, log uncommon ones
        // 不是终端出错情况
        if (savedErrno != EINTR)
        {
            errno = savedErrno;
            LOG_SYSERR << "EPollPoller::poll()";
        }
    }
    // 返回调用时间戳
    return now;
}
```
### 填写活跃的连接 fillActiveChannels
通过 epollwait 返回的 events 数组内部有指向 channel 的指针，我们可以通过此指针在 EPollPoller 模块获取对 channel 进行操作。
我们需要更新 channel 的返回事件的设置，并且将此 channel 装入 activeChannels。
```cpp
// 填充活跃的channel
void EPollPoller::fillActiveChannels(int numEvents,
                                     ChannelList* activeChannels) const
{
    // 断言活跃的事件数目小于总数
    assert(implicit_cast<size_t>(numEvents) <= events_.size());
    for (int i = 0; i < numEvents; ++i)
    {
        /**
         * 在EPollPoller::update时候通过 events_[i].data 保存了指向 channel 的指针
         * 所以Poller可以通过此 epoll_wait 返回的 epoll_event 获取活跃的chanenl
         */
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
        int fd = channel->fd();
        // 在channelMap中搜索此fd
        ChannelMap::const_iterator it = channels_.find(fd);
        // 断言我们在map中能找到channel并且键值对没有出错
        assert(it != channels_.end());
        assert(it->second == channel);
#endif
        // 更改此 channel 的事件
        channel->set_revents(events_[i].events);
        // 加入activeChannels
        activeChannels->push_back(channel);
    }
}
```
### 更新channel在epoll上的状态
我们获取 channel 在 EPollPoller 上的状态，根据状态进行不同操作。最后调用 update 私有方法。

- 如果此 channel 还没有被添加到 epoll 上或者是之前已经被 epoll 上注销，那么此 channel 接下来会进行添加操作`index == kNew || index == kDeleted`
   - 如果是未添加状态，则需要在 map 上增加此 channel
   - 如果是已删除状态，则直接往后执行
   - 设置 channel 状态为 `kAdded` ，然后调用 `update(EPOLL_CTL_ADD, channel);`
- 如果已经在 poller 上注册的状态，则要进行删除或修改操作，需要判断此 channel 是否还有监视的事情（是否还要事件要等着处理）
   - 如果没有则直接删除，调用 `update(EPOLL_CTL_DEL, channel);` 并重新设置状态为 `kDeleted`
   - 如果还有要监视的事情，则说明要进行修改（MOD）操作，调用 `update(EPOLL_CTL_MOD, channel);`
```cpp
// 更新channel在epoll上的状态
void EPollPoller::updateChannel(Channel* channel)
{
    // 断言是当前EventLoop所属的线程调用
    Poller::assertInLoopThread();
    // 获取channel在epoll这里的状态，是新的未被监视事件的，还是正被监视事件的，还是已经从监视中删除了
    const int index = channel->index();
    LOG_TRACE << "fd = " << channel->fd()
    << " events = " << channel->events() << " index = " << index;
    // 如果是未被监视过的或者已经从监视中删除的情况
    if (index == kNew || index == kDeleted)
    {
        // a new one, add with EPOLL_CTL_ADD
        int fd = channel->fd();
        // 未被监视的则要将其加入到 map<fd, channel*> 中
        if (index == kNew)
        {
            // 断言在当前的map中找不到此fd
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        }
        // 如果是已经被删除的
        else // index == kDeleted
        {
            // 被删除则从epoll对象里被删除了，但是map中仍然留存
            assert(channels_.find(fd) != channels_.end());
            // 继续断言，该键值对应该是对应的
            assert(channels_[fd] == channel);
        }
        // 重新更改channel状态，设置为被监视状态
        channel->set_index(kAdded);
        // 调用EPollPoller::update()，此处正式调用epoll_wait将事件注册到内核中
        update(EPOLL_CTL_ADD, channel);
    }
    // kAdded状态，说明正被监视
    else
    {
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        (void)fd;
        // 断言可以在map找到
        assert(channels_.find(fd) != channels_.end());
        // 断言键值对相等
        assert(channels_[fd] == channel);
        // 断言channel在poller的装填
        assert(index == kAdded);
        // 如果该channel没有处理事件了，则代表要从epoll对象中注销掉了
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            // 重新设置channel状态
            channel->set_index(kDeleted);
        }
        // 如果还有处理事件，则更改监视事件的类型
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}
```
接着往下看 update 操作，其本质是调用 `epoll_ctl` 函数，而里面的操作由之前的 `updateChannel` 所指定。
```cpp
void EPollPoller::update(int operation, Channel* channel)
{
    // 获取一个epoll_event
    struct epoll_event event;
    memZero(&event, sizeof event);
    event.events = channel->events();
    // event的成员保存channel指针
    event.data.ptr = channel;
    int fd = channel->fd();
    LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
    << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
    // 向内核注册或注销监视fd和监视事件或者更改监视事件类型
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        // 删除操作则记录日志
        if (operation == EPOLL_CTL_DEL)
        {
            // 日志输出
            LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
        else
        {
            // 日志输出
            LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
        }
    }
}
```
### 从 epoll 中移除监视的channel
```cpp
// 从 epoll 中移除监视的channel
void EPollPoller::removeChannel(Channel* channel)
{
    // 断言在所属EventLoop的创建线程中
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE << "fd = " << fd;
    // 根据channel找到fd
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    // 断言此channel无事件关注
    assert(channel->isNoneEvent());
    // 获取此channel在EPollPoller对象中的状态
    int index = channel->index();
    // 断言是添加状态或删除状态
    assert(index == kAdded || index == kDeleted);
    // 在当前map中删除此映射
    size_t n = channels_.erase(fd);
    (void)n;
    assert(n == 1);

    // 如果之前是添加操作则从epoll对象中删除
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    // 重新设置此channel在epoll中的监视状态为未被监视
    channel->set_index(kNew);
}
```

1. 断言此`channel`已经没有可关注的事件了。
2. 从`unordered_map<fd, Channel*> channels`中删除此`channel`，根据`fd`删除。
3. 如果之前`channel`的状态是添加状态，则还需调用`update`方法，将此`channel`从`epoll`对象中删除
4. `channel`设置其状态为未被监视状态