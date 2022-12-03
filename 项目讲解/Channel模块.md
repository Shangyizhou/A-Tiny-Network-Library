# 什么是Channel
Channel 对文件描述符和事件进行了一层封装。平常我们写网络编程相关函数，基本就是创建套接字，绑定地址，转变为可监听状态（这部分我们在 Socket 类中实现过了，交给 Acceptor 调用即可），然后接受连接。

但是得到了一个初始化好的 socket 还不够，我们是需要监听这个 socket 上的事件并且处理事件的。比如我们在 Reactor 模型中使用了 epoll 监听该 socket 上的事件，我们还需将需要被监视的套接字和监视的事件注册到 epoll 对象中。

可以想到文件描述符和事件和 IO 函数全都混在在了一起，极其不好维护。而 muduo 中的 Channel 类将文件描述符和其感兴趣的事件（需要监听的事件）封装到了一起。而事件监听相关的代码放到了 Poller/EPollPoller 类中。

# 成员变量
```cpp
/**
* const int Channel::kNoneEvent = 0;
* const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
* const int Channel::kWriteEvent = EPOLLOUT;
*/
static const int kNoneEvent;
static const int kReadEvent;
static const int kWriteEvent;

EventLoop *loop_;   // 当前Channel属于的EventLoop
const int fd_;      // fd, Poller监听对象
int events_;        // 注册fd感兴趣的事件
int revents_;       // poller返回的具体发生的事件
int index_;         // 在Poller上注册的情况

std::weak_ptr<void> tie_;   // 弱指针指向TcpConnection(必要时升级为shared_ptr多一份引用计数，避免用户误删)
bool tied_;  // 标志此 Channel 是否被调用过 Channel::tie 方法

// 保存着事件到来时的回调函数
ReadEventCallback readCallback_; 	// 读事件回调函数
EventCallback writeCallback_;		// 写事件回调函数
EventCallback closeCallback_;		// 连接关闭回调函数
EventCallback errorCallback_;		// 错误发生回调函数
```

- `int fd_`：这个Channel对象照看的文件描述符
- `int events_`：代表fd感兴趣的事件类型集合
- `int revents_`：代表事件监听器实际监听到该fd发生的事件类型集合，当事件监听器监听到一个fd发生了什么事件，通过`Channel::set_revents()`函数来设置revents值。
- `EventLoop* loop_`：这个 Channel 属于哪个EventLoop对象，因为 muduo 采用的是 one loop per thread 模型，所以我们有不止一个 EventLoop。我们的 manLoop 接收新连接，将新连接相关事件注册到线程池中的某一线程的 subLoop 上（轮询）。我们不希望跨线程的处理函数，所以每个 Channel 都需要记录是哪个 EventLoop 在处理自己的事情，这其中还涉及到了线程判断的问题。
- `read_callback_` 、`write_callback_`、`close_callback_`、`error_callback_`：这些是 std::function 类型，代表着这个Channel为这个文件描述符保存的各事件类型发生时的处理函数。比如这个fd发生了可读事件，需要执行可读事件处理函数，这时候Channel类都替你保管好了这些可调用函数。到时候交给 EventLoop 执行即可。
- `index `：我们使用 index 来记录 channel 与 Poller 相关的几种状态，Poller 类会判断当前 channel 的状态然后处理不同的事情。
   - `kNew`：是否还未被poll监视 
   - `kAdded`：是否已在被监视中 
   - `kDeleted`：是否已被移除
- `kNoneEvent`、`kReadEvent`、`kWriteEvent`：事件状态设置会使用的变量
# 成员函数
## 设置此 Channel 对于事件的回调函数
```cpp
// 设置回调函数对象
// 使用右值引用，延长了临时cb对象的生命周期，避免了拷贝操作
void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
void setWriteCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
void setCloseCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
void setErrorCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
```
## 设置 Channel 感兴趣的事件到 Poller
```cpp
// 设置fd相应的事件状态，update()其本质调用epoll_ctl
void enableReading() { events_ |= kReadEvent; update(); }     // 设置读事件到poll对象中
void disableReading() { events_ &= ~kReadEvent; update(); }   // 从poll对象中移除读时间
void enableWriting() { events_ |= kWriteEvent; update(); }    // 设置写事件到poll对象中
void disableWriting() { events_ &= ~kWriteEvent; update(); }  // 从poll对象中移除写时间
void disableAll() { events_ = kNoneEvent; update(); }         // 关闭所有事件
bool isWriting() const { return events_ & kWriteEvent; }      // 是否关注写事件
bool isReading() const { return events_ & kReadEvent; }       // 是否关注读事件
```
设置好该 Channel 的监视事件的类型，调用 update 私有函数向 Poller 注册。实际调用 epoll_ctl
```cpp
/**
 * 当改变channel所表示fd的events事件后
 * update负责在poller里面更改fd相应的事件epoll_ctl 
 */
void Channel::update()
{
    // 通过该channel所属的EventLoop，调用poller对应的方法，注册fd的events事件
    loop_->updateChannel(this);
}
```
## 更新Channel关注的事件
```cpp
// 更新该fd相关事件
void Channel::update()
{
  // 设置该channel状态为已加入EventLoop
  addedToLoop_ = true;
  // 调用EventLoop::updateChannel，传入channel指针
  // EventLoop::updateChannel => Poller::updateChannel
  loop_->updateChannel(this);
}
```
## 移除操作
```cpp
// 从epoll对象中移除该fd
void Channel::remove()
{
  // 断言无事件处理
  assert(isNoneEvent());  
  // 设置该Channel没有被添加到eventLoop
  addedToLoop_ = false;   
  // 调用EventLoop::removeChannel，传入channel指针
  // EventLoop::removeChannel => Poller::removeChannel
  loop_->removeChannel(this);
}
```
## 用于增加TcpConnection生命周期的tie方法（防止用户误删操作）
```cpp
// 在TcpConnection建立得时候会调用
void Channel::tie(const std::shared_ptr<void> &obj)
{
    // weak_ptr 指向 obj
    tie_ = obj;
    // 设置tied_标志
    tied_ = true;
}
```
```cpp
// fd得到poller通知以后，去处理事件
void Channel::handleEvent(Timestamp receiveTime)
{
    /**
     * 调用了Channel::tie得会设置tid_=true
     * 而TcpConnection::connectEstablished会调用channel_->tie(shared_from_this());
     * 所以对于TcpConnection::channel_ 需要多一份强引用的保证以免用户误删TcpConnection对象
     */
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        else 
        {
            handleEventWithGuard(receiveTime);
        }

    }
}
```
用户使用muduo库的时候，会利用到TcpConnection。用户可以看见 TcpConnection，如果用户注册了要监视的事件和处理的回调函数，并在处理 subLoop 处理过程中「误删」了 TcpConnection 的话会发生什么呢？

总之，EventLoop 肯定不能很顺畅的运行下去。毕竟它的生命周期小于 TcpConnection。为了防止用户误删的情况，TcpConnection 在创建之初 `TcpConnection::connectEstablished` 会调用此函数来提升对象生命周期。

实现方案是在处理事件时，如果对被调用了`tie()`方法的Channel对象，我们让一个共享型智能指针指向它，在处理事件期间延长它的生命周期。哪怕外面「误删」了此对象，也会因为多出来的引用计数而避免销毁操作。

```cpp
// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected); // 建立连接，设置一开始状态为连接态
    /**
     * channel_->tie(shared_from_this());
     * tie相当于在底层有一个强引用指针记录着，防止析构
     * 为了防止TcpConnection这个资源被误删掉，而这个时候还有许多事件要处理
     * channel->tie 会进行一次判断，是否将弱引用指针变成强引用，变成得话就防止了计数为0而被析构得可能
     */
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的EPOLLIN读事件

    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}
```
注意，传递的是 this 指针，所以是在 Channel 的内部增加对 TcpConnection 对象的引用计数（而不是 Channel 对象）。这里体现了 shared_ptr 的一处妙用，可以通过引用计数来控制变量的生命周期。巧妙地在内部增加一个引用计数，假设在外面误删，也不会因为引用计数为 0 而删除对象。
> `weak_ptr.lock()` 会返回 `shared_ptr`（如果 weak_ptr 不为空）。

## 根据相应事件执行Channel保存的回调函数
我们的Channel里面保存了许多回调函数，这些都是在对应的事件下被调用的。用户提前设置写好此事件的回调函数，并绑定到Channel的成员里。等到事件发生时，Channel自然的调用事件处理方法。借由回调操作实现了异步的操作。
```cpp
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // 标志，此时正在处理各个事件
    eventHandling_ = true;
    LOG_TRACE << reventsToString();
    // 对端关闭事件
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (logHup_)
        {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
        }
        // 内部储存function，这是判断是否注册了处理函数，有则直接调用
        if (closeCallback_) closeCallback_();
    }
    // fd不是一个打开的文件
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }
    // 发生了错误，且fd不可一个可以打开的文件
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }
    // 读事件 且是高优先级读且发生了挂起
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }
    // 写事件
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}
```
# 参考
[万字长文梳理Muduo库核心代码及优秀编程细节思想剖析](https://zhuanlan.zhihu.com/p/495016351)

