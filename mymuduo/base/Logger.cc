#include "Logger.h"
#include "Timestamp.h"

// Logger& Logger::instance()
// {
//     static Logger logger;
//     return logger;
// }

// 设置日志级别
void Logger::setLogLevel(int Level)
{
    logLevel_ = Level;
}

// 写日志
void Logger::log(std::string msg)
{
    switch (logLevel_)
    {
        case INFO:
            std::cout << "[INFO]\t";
            break;
        case ERROR:
            std::cout << "[ERROR]\t";
            break;
        case FATAL:
            std::cout << "[FATAL]\t";
            break;
        case DEBUG:
            std::cout << "[DEBUG]\t";
            break; 
        default:
            break;                                    
    }
    // TODO: 将时间设置成陈硕的方法
    // 打印时间和信息
    // 打印时间和msg

    std::cout << Timestamp::now().toString() << " : " << msg << std::endl;
}