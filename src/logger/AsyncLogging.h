#ifndef ASYNC_LOGGING_H
#define ASYNC_LOGGING_H

#include "noncopyable.h"
#include "Thread.h"
#include "FixedBuffer.h"
#include "LogStream.h"
#include "LogFile.h"


#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>

class AsyncLogging
{
public:
    AsyncLogging(const std::string& basename,
                 off_t rollSize,
                 int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // 前端调用 append 写入日志
    void append(const char* logling, int len);

    void start()
    {
        running_ = true;
        thread_.start();
    }

    void stop()
    {
        running_ = false;
        cond_.notify_one();
        thread_.join();
    }

private:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::unique_ptr<Buffer>>;
    using BufferPtr = BufferVector::value_type;

    void threadFunc();

    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};

#endif // ASYNC_LOGGING_H