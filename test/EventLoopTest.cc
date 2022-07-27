#include <mymuduo/EventLoop.h>
#include <thread>

void callback()
{
    printf("callback(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    EventLoop anotherLoop;
}

int main(int argc, char *argv[])
{
    EventLoop loop;
    printf("main(): pid = %d, tid = %d\n", getpid(), CurrentThread::tid());    

    // 创建新线程执行callback function
    // 符合one loop per thread
    std::thread thraed(callback);

    // 在一个线程内创建多个EventLoop，触发FATAL，结束进程
    callback();
    
    return 0;
}