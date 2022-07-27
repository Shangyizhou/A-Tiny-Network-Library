#include <iostream>
#include <thread>
#include <memory>

class EventLoop;
__thread EventLoop *t_loopInThisThread = nullptr;

class EventLoop 
{
public:
    EventLoop() {
        if (!t_loopInThisThread) {
            t_loopInThisThread = this;
        } else {
            printf("this thread exist eventLoop\n");
            exit(1);
        }
    }
};                                                                                                                                                                                                                                                                       

void callback()
{
    printf("create another thread!\n");
    EventLoop loop;
    printf("thread destroy!\n");
}

int main()
{
    EventLoop loop;

    std::thread thread(callback);
    thread.join();

    EventLoop loop2;
    // this thread exist eventLoop

    return 0;    
}