这是 muduo 里面的回声服务器代码，我们从此作为入口，查看 TcpServer 的处理流程。
```cpp
int main()
{
    // 这个EventLoop就是main EventLoop，即负责循环事件监听处理新用户连接事件的事件循环器。第一章概述篇的图2里面的EventLoop1就是我们的main EventLoop。
    EventLoop loop;
    
    // InetAddress其实是对socket编程中的sockaddr_in进行封装，使其变为更友好简单的接口而已。
    InetAddress addr(4567);
    
    // EchoServer类，自己等一下往下翻一下。
    EchoServer server(&loop, addr, "EchoServer-01");
    
    // 启动TcpServer服务器
    server.start(); 
    
    // 执行EventLoop::loop()函数，这个函数在概述篇的EventLoop小节有提及，自己去看一下！！
    loop.loop(); 
    return 0;
}
```
可以看到，我们需要额外创建一个 EventLoop 对象，并且设置好该服务器的监听端口。在创建 EchoServer 的时候需要传入这个 EventLoop 对象和监听端口完成初始化操作。

而启动 TcpServer 需要显示调用 start 函数，此时才能确保其可以监听新连接并且创建线程池。而真正执行事件还需要开启 EventLoop，否则内部会阻塞在 epoll_wait 上。

# TcpServer构造函数都做了什么
构造 TcpServer 对象会完成如下事情

1. 让 loop_ 成员指向用户传入的 EventLoop，此处会对 EventLoop 进行一个检查，如果为 nullptr 则会打印 LOG_FATAL 日志并结束程序。这里的 loop 就是 BaseLoop/MainReactor。
2. 创建 Acceptor 对象，为其分配其所在 EventLoop 和它需要监听的地址（用户传入），其默认操作采用端口复用。并且设置其当有新连接的事件产生时，其调用 `TcpServer::newConnection`。
3. 开启线程池：这里的线程指的是 EventLoopThread，正好对应着 one loop per thread。线程池里的线程负责运行 SubLoop/SubReactor，Acceptor 监听到的新连接就会被轮询派发到 SubReactor 中。
4. 初始化回调函数和一些变量。
```cpp
// 检查传入的 baseLoop 指针是否有意义
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << "mainLoop is null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(),
    messageCallback_(),
    writeCompleteCallback_(),
    threadInitCallback_(),
    started_(0),
    nextConnId_(1)    
{
    // 当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生执行handleRead()调用TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}
```
# TcpServer启动后做的事情

1. 开启 TcpServer 的线程池，其内部会创建一定数量的 EventLoopThread 线程。
2. 运行 Acceptor::listen 函数，此步骤正式开启服务器的监听。
```cpp
// 开启服务器监听
void TcpServer::start()
{
    if (started_++ == 0)
    {
        // 启动底层的lopp线程池
        threadPool_->start(threadInitCallback_);
        // acceptor_.get()绑定时候需要地址
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}
```
# TcpServer是如何处理新连接的
先前说过，TcpServer 的构造函数创建了 Acceptor 对象用于监听新连接，且设置新连接到来的回调函数为 `TcpServer::newConnection`。那么这个函数又做了哪些操作呢？

1. TcpServer 会从线程池中选择一个 SubLoop/SubReactor，这里只实现了简单的负载均衡，没有按照权重选择。
2. 生成此连接的名字，其形式为`传入姓名 + IP + port + ConnectionID`。
3. 获取负责该连接的 socket 的 IP 和 port，将其封装成 `InetAddress` 对象，这个是服务端本地的。
4. 创建 TcpConnection 对象，今后就由它来负责这个连接的大小事情。我们需要向其传入刚得到的 SubLoop、此连接的姓名、此连接的 socket、此连接的对端 InetAddress。
5. 设置此 TcpConnection 的各种回调函数，都为开发者自主传入的。
6. 设置此 TcpConnection 关闭连接的回调函数为 `TcpServer::removeConnection`。
7. 在 MainReactor 运行 `TcpConnection::connectEstablished` 函数构建新连接，注意此函数是 MainReactor 所在线程调用的，但是这个函数是 SubReactor 上的，因此涉及到跨线程调用。
```cpp
// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // 轮询算法 选择一个subLoop 来管理connfd对应的channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    // 提示信息
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    // 这里没有设置为原子类是因为其只在mainloop中执行 不涉及线程安全问题
    ++nextConnId_;  
    // 新连接名字
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_.c_str() << "] - new connection [" << connName.c_str() << "] from " << peerAddr.toIpPort().c_str();
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr() failed";
    }

    InetAddress localAddr(local);
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer => TcpConnection的，至于Channel绑定的则是TcpConnection设置的四个，handleRead,handleWrite... 这下面的回调用于handlexxx函数中
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));

    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
}
```
# TcpServer销毁时都做了什么

1. 循环遍历 TcpServer 管理的 TcpConnection，取消指向 TcpConnection 的智能指针的管理权。这样当 conn 出了其作用域，即可释放智能指针指向的对象
2. 跨线程让 SubEventLoop 调用 TcpConnection::connectDestroyed 函数。
```cpp
TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        item.second.reset();    
        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}
```
# TcpServer的成员
```cpp
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop,
                const InetAddress &ListenAddr,
                const std::string &nameArg,
                Option option = kNoReusePort);
    ~TcpServer();

    // 设置回调函数(用户自定义的函数传入)
    void setThreadInitCallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    // 设置底层subLoop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();
    
    EventLoop* getLoop() const { return loop_; }

    const std::string name() { return name_; }

    const std::string ipPort() { return ipPort_; }

private:
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    /**
     * key:     std::string
     * value:   std::shared_ptr<TcpConnection> 
     */
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    
    EventLoop *loop_;                    // 用户定义的baseLoop
    const std::string ipPort_;           // 传入的IP地址和端口号
    const std::string name_;             // TcpServer名字
    std::unique_ptr<Acceptor> acceptor_; // Acceptor对象负责监视
    
    std::shared_ptr<EventLoopThreadPool> threadPool_; // 线程池

    ConnectionCallback  connectionCallback_;        // 有新连接时的回调函数
    MessageCallback messageCallback_;               // 有读写消息时的回调函数
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后的回调函数

    ThreadInitCallback threadInitCallback_;  // loop线程初始化的回调函数
    std::atomic_int started_;                // TcpServer

    int nextConnId_;            // 连接索引
    ConnectionMap connections_; // 保存所有的连接
};
```
