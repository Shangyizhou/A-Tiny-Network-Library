#include <cstdio>

#include "../Timestamp.h"
#include "../Logger.h"

int main()
{
    // 不传入额外参数的测试语句
    LOG_INFO("this is a test for[no args]");

    // 传入额外参数的测试语句，打印所属文件，调用所在函数和所在的行数
    LOG_INFO("%s:%s:%d this is a test for INFO", __FILE__, __FUNCTION__, __LINE__);

    // #define MUDEBUG 才可以输出 DEBUG 日志
    LOG_DEBUG("%s:%s:%d this is a test for DEBUG", __FILE__, __FUNCTION__, __LINE__);

    LOG_ERROR("%s:%s:%d this is a test for ERROR",  __FILE__, __FUNCTION__, __LINE__);
    LOG_FATAL("%s:%s:%d this is a test for FATAL",  __FILE__, __FUNCTION__, __LINE__);

    // LOG_FATAL会调用exit()结束进程，看不到这条语句
    LOG_INFO("if LOG_FATAL, you will not see this sentence");


    return 0;
}