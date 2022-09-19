#include "Logging.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include <functional>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "listen socket create err " << errno;
        // LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport) 
    : loop_(loop),
    acceptSocket_(createNonblocking()),
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false)
{
    LOG_DEBUG << "Acceptor create nonblocking socket, [fd = " << acceptChannel_.fd() << "]";
    // LOG_DEBUG("%s:%s:%d Acceptor create nonblocking socket, fd = %d\n", __FILE__, __FUNCTION__, __LINE__, acceptChannel_.fd());
    acceptSocket_.setReuseAddr(reuseport);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(ListenAddr);

    /**
     * TcpServer::start() => Acceptor.listen
     * 有新用户的连接，需要执行一个回调函数
     * 因此向封装了acceptSocket_的channel注册回调函数
     */
    acceptChannel_.setReadCallback(
            std::bind(&Acceptor::handleRead, this));   
}

Acceptor::~Acceptor()
{
    // 把从Poller中感兴趣的事件删除掉
    acceptChannel_.disableAll();    
    // 调用EventLoop->removeChannel => Poller->removeChannel 把Poller的ChannelMap对应的部分删除
    acceptChannel_.remove();       
}

void Acceptor::listen()
{
    // 表示正在监听
    listenning_ = true;
    acceptSocket_.listen();
    // 将acceptChannel的读事件注册到poller
    acceptChannel_.enableReading();
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    // 使用了InetAddress类型定义对象，需要包含头文件
    // 之前为了不加载头文件使用了前置声明
    InetAddress peerAddr;
    // 接受新连接
    int connfd = acceptSocket_.accept(&peerAddr);
    // 确实有新连接到来
    if (connfd >= 0)
    {
        // TcpServer::NewConnectionCallback_
        if (NewConnectionCallback_)
        {
            // 轮询找到subLoop 唤醒并分发当前的新客户端的Channel
            NewConnectionCallback_(connfd, peerAddr); 
        }
        else
        {
            LOG_DEBUG << "no newConnectionCallback() function";
            // LOG_DEBUG("connfd < 0 accept failed");
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "accept() failed";
        // LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);

        // 当前进程的fd已经用完了
        // 可以调整单个服务器的fd上限
        // 也可以分布式部署
        if (errno == EMFILE)
        {
            LOG_ERROR << "sockfd reached limit";
            // LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}