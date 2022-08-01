#include <mymuduo/Acceptor.h>
#include <mymuduo/InetAddress.h>
#include <mymuduo/EventLoop.h>

int main()
{
    EventLoop loop;
    InetAddress addr(3000);
    Acceptor acceptor(&loop, addr, false);
    loop.loop();


    return 0;
}