#include "../Timestamp.h"

int main()
{
    // 打印当前时间
    std::cout << Timestamp::now().toString() << std::endl; // 2022/07/22 00:49:45

    return 0;
}