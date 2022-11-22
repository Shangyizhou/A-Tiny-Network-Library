#include "Timer.h"
#include <iostream>
#include <unistd.h>
using std::cout;
using std::endl;

long fibonacci(unsigned n)
{
    if (n < 2) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main()
{
    cxxtimer::Timer timer;
    timer.start();
    
    // sleep(2);
    fibonacci(42);

    timer.stop();

    cout << timer.count<std::chrono::seconds>() << endl;
    cout << timer.count<std::chrono::milliseconds>() << endl;
    cout << timer.count<std::chrono::microseconds>() << endl;

    // 重新启动定时器，之前累计的时间不会重置为 0
    // 但是基准时间会重新获取，调用 std::chrnon::steady_clock::now()
    timer.start();

    sleep(1);

    timer.stop();

    cout << timer.count<std::chrono::seconds>() << endl;
    cout << timer.count<std::chrono::milliseconds>() << endl;
    cout << timer.count<std::chrono::microseconds>() << endl;

    return 0;

}