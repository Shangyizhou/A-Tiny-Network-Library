#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
// #include <memory>
// #include <thread>
// #include <mutex>
#include <assert.h>
#include "noncopyable.h"

#define MUDEBUG

// 使用宏让使用更便利
// TODO: 宏的学习 do while 的用法
#define LOG_INFO(logmsgFormat, ...)                         \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(INFO);                           \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while(0)                                              

#define LOG_ERROR(logmsgFormat, ...)                        \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(ERROR);                          \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...)                        \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(FATAL);                          \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
        exit(1);                                            \
    } while(0) 

// DEBUG 平时应不开启，信息比较多影响流程和增加服务器负担
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                        \
    do                                                      \
    {                                                       \
        Logger &logger = Logger::instance();                \
        logger.setLogLevel(DEBUG);                          \
        char buf[1024] = {0};                               \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);   \
        logger.log(buf);                                    \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别 INFO ERROR FATAL DEBUG
enum LogLevel
{
    INFO,   // 普通信息
    ERROR,  // 错误信息
    FATAL,  // core信息
    DEBUG,  // 调试信息
};

class Logger : noncopyable
{
public:
    // 静态方法获取日志唯一的实例对象
    static Logger& instance()
    {
        // C++11保证静态局部对象是线程安全的
        static Logger log; // 静态局部变量
        return log;
    }

    // 获取日志唯一的实例对象
    // static Logger& instance();

    // 设置日志级别
    void setLogLevel(int Level);
    // 写日志
    void log(std::string msg);

private:
    // Logger()
    // {
    //     file_.open("log.txt", std::ios::out | std::ios::app);
    // }
    Logger() = default;
	Logger(const Logger& other) = delete;
    Logger(Logger&&) = delete;
	Logger& operator=(const Logger&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    int logLevel_;
    // std::ofstream file_; // 日志文件
};

#endif // LOGGER_H