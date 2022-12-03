# TcpConnection构造时做的事情
在 TcpServer::newConnection 函数中，会创建 TcpConnection 对象并让共享智能指针接管。TcpServer 会向其传入 SubLoop、此连接的姓名、此连接的 socket、此连接的对端 InetAddress peerAddress。现在了看一下 TcpConnection 构造函数会做的事情：

1. 检查传入的 EventLoop 是否正确，并赋值成员。
2. 根据收到的此连接的 socket 生成 Socket 对象。
3. 将此 socket 封装成 Channel。
4. 设置 TcpConnection 的读事件发生、写事件完成、连接关闭事件发生、错误事件发生的回调函数。
```cpp
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M 避免发送太快对方接受太慢
{
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO << "TcpConnection::ctor[" << name_.c_str() << "] at fd =" << sockfd;
    socket_->setKeepAlive(true);
}
```
# 正式建立连接所做的事情
TcpServer 创建 TcpConnection 之后会跨线程调用 TcpConnection::connectEstablished，在这里会做如下事情：

1. 设置此连接为正连接状态
2. 为此 Channel 调用 tie 函数，让 Channel 内部的弱指针 tie_ 指向此 TcpConnection 对象。内部的指向增加了一次引用计数，防止 TcpConnection 被误删。
3. 向 poller 注册 channel 的 EPOLLIN 读事件
4. 若用户传入了连接相关事件的回调函数，那么执行。
```cpp
void TcpConnection::connectEstablished()
{
    setState(kConnected); // 建立连接，设置一开始状态为连接态
    /**
     * TODO:tie
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
# TcpConnection销毁时做的事情

1. 若此状态为连接状态，那么设置其为非连接装填。
2. 取消 Channel 所感兴趣的所有事件。
3. 若用户传入了连接相关事件的回调函数，那么执行（此部分是连接销毁）。
4. 将此 Channel 从 epoll 中删除。
```cpp
// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除掉
}
```
# 读写相关操作

...