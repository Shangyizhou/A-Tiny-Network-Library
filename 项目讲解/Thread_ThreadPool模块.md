

muduo 实现了 Thread 类封装了线程的基本操作，这里使用 C++ 11 的操作代替。然后将 Thread，EventLoop 放在一块实现了 EventLoopThread，这对应着 one loop per thread，一个线程有一个事件循环。

既然有了线程，那么为了节省创建线程的开销，muduo 还继续向上封装了 EventLoopThreadPool，即事件循环线程池。此类被 TcpServer 所调用，创建一系列线程并执行对应的线程初始化回调函数。 

因为EventLoopThredPool、EventLoopThread、Thread联系紧密，本文打算从线程池创建开始逐步自定向下讲解这几个类之间的调用关系。

# 从EchoServer入手查看调用过程
大致流程如下图：

![1658586767(1).png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1658586771575-1db8502a-a993-450b-9ec2-71e13100dd41.png#averageHue=%23faf9f8&clientId=u58c03f9a-a418-4&crop=0&crop=0&crop=1&crop=1&from=paste&height=673&id=DjeQQ&margin=%5Bobject%20Object%5D&name=1658586767%281%29.png&originHeight=841&originWidth=1717&originalType=binary&ratio=1&rotation=0&showTitle=false&size=327411&status=done&style=none&taskId=u11da66af-2b71-43d6-9b29-30d8dec67b1&title=&width=1373.6)

muduo 编程得一个入门案例：

```cpp
int main()
{
    EventLoop loop;
    InetAddress addr(4567);    
    EchoServer server(&loop, addr, "EchoServer-01");
    //启动TcpServer服务器
    server.start();      
    loop.loop(); //执行EventLoop::loop()函数，这个函数在概述篇的EventLoop小节有提及，自己去看一下！！
    
    return 0;
}
```
从启动服务器这一行`server.start();`，我们深入进去查看。
```cpp
// 启动服务器
void TcpServer::start()
{
    if (started_.getAndSet(1) == 0)
    {
        // 开启线程池
        threadPool_->start(threadInitCallback_);
        
        assert(!acceptor_->listening());
        // 将Acceptor::listen加入loop.runInLoop开启监听
        loop_->runInLoop(
        	std::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}
```
这里看到了开启线程池的代码了，`threadPool_->start(threadInitCallback_);`。里面传入了一个线程池初始化的回调函数，这个函数是用户自己自定义的。继续查看线程池的创建函数`EventLoopThreadPool::start`
```cpp
void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
	/**省略***/
    
    // 创建一定数量线程
    for (int i = 0; i < numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        // 创建EventLoopThread，传入线程初始化的回调函数
        EventLoopThread* t = new EventLoopThread(cb, buf);
        // 线程容器加入此线程
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // EventLoop容器加入此线程的EventLoop
        // 此处已经生成了新的线程
        loops_.push_back(t->startLoop());
    }
    // 单线程运行
    if (numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}
```
其内部创建一定数量的线程，并且标注上每一个线程的名字。根据名字和创建`EventLoopThread`，并将其加入容器进行管理。这里不直接创建`Thread`变量，而是间接创建`EventLoopThread`，事件循环和线程由它来管理（相当于向上层封装了一下）。
`loops_.push_back(t->startLoop());`此处中的`startLoop`函数会创建新的线程，新线程执行任务创建一个的`EventLoop`并返回。线程池类加入新的`EventLoop`到容器中管理，我们继续往下查看`startLoop()`：
```cpp
// startLoop() => Thread::start() => pthread_create()
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    // 调用startLoop即开启一个新线程，创建新线程时指定了执行的函数和需要的参数
    thread_.start();

    EventLoop* loop = NULL;
    {
        // 等待新线程执行threadFunc完毕，所以使用cond_.wait
        MutexLockGuard lock(mutex_);
        /**
         * threadFunc会被前几行创建的新线程所执行
         * 我们需要等待这个函数执行完毕，否则loop_指针会指向无意义的地方
         * 所以使用了条件变量
         */
        while (loop_ == NULL)
        {
            cond_.wait();
        }
        loop = loop_;
    }

    return loop;
}
```
内部调用`thread_.start();`用于开启一个新的线程，其实际调用了`pthread_create`函数。我们创建新的线程，其实也就是创建`subReactor`，其内部也应有一个`EventLoop`。而这个`EventLoop`的创建过程在`EventLoopThread::threadFunc`函数完成的。`Thread`的某个变量绑定了此函数，并在`Thread`的初始化过程中调用。从而完成了新`EventLoop`的创建。
```cpp
// 这个函数会在Thread内部执行
void EventLoopThread::threadFunc()
{
    // 创建一个临时对象
    EventLoop loop;

    // 如果设置了回调函数则执行
    if (callback_)
    {
        callback_(&loop);
    }

    {
        // 确定生成了EventLoop对象，且loop_指向了有意义的地址则唤醒
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        // 唤醒阻塞
        cond_.notify();
    }
    // 执行事件循环
    loop.loop();
    // assert(exiting_);
    // 执行下面的代码说明事件循环结束，我们要防止loop_变成悬空指针
    // 这部分需要加锁
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}
```
生成`EventLoop`之后就直接执行了`loop.loop()`，如果不主动结束那么会一直进行事件循环。
# EventLoopThreadPool详解
## EventLoopThreadPool重要成员
```cpp
class EventLoopThreadPool : noncopyable
{
public:
    // 开启线程池
    typedef std::function<void(EventLoop*)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
    ~EventLoopThreadPool();
    // 设置线程数量
    void setThreadNum(int numThreads) { numThreads_ = numThreads; } 
    // 开启线程池
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    // valid after calling start()
    /// round-robin
    // 轮询获取下一个线程
    EventLoop* getNextLoop();

    /// with the same hash code, it will always return the same EventLoop
    EventLoop* getLoopForHash(size_t hashCode);

    // 获取所有EventLoop
    std::vector<EventLoop*> getAllLoops();

    // 是否已经开启
    bool started() const
    { return started_; }

    // 返回名字
    const string& name() const
    { return name_; }

private:
    /**
     * 主线程的事件循环驱动，TcpServer所在的事件驱动循环，创建TcpServer传入的EventLoop
     * 通常只负责监听客户端连接请求
     */
    EventLoop* baseLoop_;  // mainLoop
    string name_; // 线程池姓名
    bool started_; // 是否开启
    int numThreads_; // 线程数量
    int next_; // 标记下次应该取出哪个线程，采用round_robin
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // 存储线程池中所有的线程
    std::vector<EventLoop*> loops_; // 存储线程池中所有的EventLoop
};
```
## 线程池中简单的负载均衡
`mainReactor`每次监听到新连接，并创建好负责连接的对象后会将其分派到某个`subReactor`。这是通过简单的轮询算法实现的。
```cpp
EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);
    // 新创建的EventLoop指针，先指向baseloop
    EventLoop* loop = baseLoop_;

    if (!loops_.empty())
    {
        // round-robin
        // 获取下一个线程
        loop = loops_[next_];
        ++next_;
        // 防止超出界限
        if (implicit_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}
```
# EventLoopThread详解
muduo 强调按照 「one loop per thread」。即，一个线程中只能有一个事件循环。这在代码中也是有体现的，muduo 的将`EventLoop`和`Thread`类对象组合到一起作为`EventLoopThread`的成员变量。

这个`EventLoopThread`的结构天然的对应着「one loop per thread」。

## EventLoopThread重要变量
```cpp
class EventLoopThread : noncopyable
{
public:
	// 线程初始化回调函数
    typedef std::function<void(EventLoop*)> ThreadInitCallback; 
	
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const string& name = string());
    ~EventLoopThread();
    // 开启事件循环
    EventLoop* startLoop(); 

private:
	// 交给Thread执行，在栈上创建EventLoop并执行事件循环
    void threadFunc();

    EventLoop* loop_ GUARDED_BY(mutex_); // EventLoop对象
    bool exiting_;  // 事件循环线程是否停止执行loop.loop()
    Thread thread_; // 自定义线程类对象
    MutexLock mutex_;
    Condition cond_ GUARDED_BY(mutex_);
    ThreadInitCallback callback_; // 线程初始化回调函数
};
```
这里重点关注`thread_`，其在初始化的时候就被绑定了`EventLoop`的成员函数`EventLoopThread::threadFunc`。所以我们执行`EventLoopThread::startLoop()`的时候，`Thread`可以调用`EventLoopThread`的成员方法。
```cpp
EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
  : loop_(NULL),
    exiting_(false),
    thread_(std::bind(&EventLoopThread::threadFunc, this), name), // 新线程绑定此函数
    mutex_(),
    cond_(mutex_),
    callback_(cb) // 传入的线程初始化回调函数，用户自定义
{
}
```
## 开启事件循环的细节
```cpp
EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    // 调用startLoop即开启一个新线程，创建新线程时指定了执行的函数和需要的参数
    thread_.start();

    EventLoop* loop = NULL;
    {
        // 等待新线程执行threadFunc完毕，所以使用cond_.wait
        MutexLockGuard lock(mutex_);
        /**
         * threadFunc会被前几行创建的新线程所执行
         * 我们需要等待这个函数执行完毕，否则loop_指针会指向无意义的地方
         * 所以使用了条件变量
         */
        while (loop_ == NULL)
        {
            cond_.wait();
        }
        loop = loop_;
    }

    return loop;
}
```
注意`thread_.start();` 这里，此处已经开启了一个新的线程，新的线程会执行指定的函数。在此函数内部，会执行`EventLoopThread::threadFun`函数。其会生成一个`EventLoop`对象并让`loop_ = loop;`
但是这里是异步执行的：

1. 当前线程会往下执行访问`loop_`变量
2. 开启的新线程往下执行会调用`EventLoopThread::threadFun`函数生成`EventLoop`。

如果还未生成`EventLoop`时候，当前线程就访问了`loop_`变量，那么就是访问了不合法的地址。这是会造成程序崩溃的！因此，我们使用了条件变量，并增加了判断语句，当`loop_ == NULL`时候继续阻塞等待。
## Thread::start()真正开始创建线程
`thread_.start`内部的操作是什么样子的？这里放出关键的代码
```cpp
/**
 * 内部调用pthread_create创建新的线程
 * Thread::start => startThread => data.runInThread => func_
 */
void Thread::start()
{
    started_ = true;
    detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
    {
        started_ = false;
        delete data; // or no delete?
        LOG_SYSFATAL << "Failed in pthread_create";
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0);
    }
}

/**
 * 创建线程时会将其作为新线程执行的函数
 * 向其传入需要的参数
 */
void* startThread(void* obj)
{
    // 传入参数
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}
```
可以看见内部调用了`pthread_create`创建了新线程，新线程指定运行`&detail::startThread`函数。而`&detail::startThread`函数内部将参数进行强制类型转换后又继续调用`data->runInThread();`

这个内部就会真正的调用`EventLoopThread::threadFun`了。最开始的时候`EventLoopThread::threadFun`就被绑定到了`Thread`类的成员变量中。

```cpp
Thread::Thread(ThreadFunc func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(std::move(func)),
    name_(n),
    latch_(1)
{
      setDefaultName();
}
```

