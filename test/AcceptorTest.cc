#include <mymuduo/net/Acceptor.h>
#include <mymuduo/net/InetAddress.h>
#include <mymuduo/net/EventLoop.h>
#include <functional>

void NewConnection(int sockfd, const InetAddress&)
{
    printf("NewConnection come in\n");
}

int main()
{
    EventLoop loop;
    InetAddress addr(3000);
    Acceptor acceptor(&loop, addr, false);
    acceptor.setNewConnectionCallback(std::bind(&NewConnection, std::placeholders::_1, std::placeholders::_2));
    loop.runInLoop(std::bind(&Acceptor::listen, &acceptor));
    loop.loop();

    return 0;
}

// g++ AcceptorTest.cc -lmymuduo_net -lmymuduo_base -o AcceptorTest