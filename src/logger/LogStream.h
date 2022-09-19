#ifndef LOG_STREAM_H
#define LOG_STREAM_H

#include "FixedBuffer.h"
#include "noncopyable.h"

#include <string>

/**
 *  比如SourceFile类和时间类就会用到
 *  const char* data_;
 *  int size_;
 */
class GeneralTemplate : noncopyable
{
public:
    GeneralTemplate()
        : data_(nullptr),
          len_(0)
    {}

    explicit GeneralTemplate(const char* data, int len)
        : data_(data),
          len_(len)
    {}

    const char* data_;
    int len_;
};

class LogStream : noncopyable
{
public:
    using Buffer = FixedBuffer<kSmallBuffer>;
    
    void append(const char* data, int len) { buffer_.append(data, len); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

    /**
     * 我们的LogStream需要重载运算符
     */
    LogStream& operator<<(short);
    LogStream& operator<<(unsigned short);
    LogStream& operator<<(int);
    LogStream& operator<<(unsigned int);
    LogStream& operator<<(long);
    LogStream& operator<<(unsigned long);
    LogStream& operator<<(long long);
    LogStream& operator<<(unsigned long long);

    LogStream& operator<<(float v);
    LogStream& operator<<(double v);

    LogStream& operator<<(char c);
    LogStream& operator<<(const void* data);
    LogStream& operator<<(const char* str);
    LogStream& operator<<(const unsigned char* str);
    LogStream& operator<<(const std::string& str);
    LogStream& operator<<(const Buffer& buf);

    // (const char*, int)的重载
    LogStream& operator<<(const GeneralTemplate& g);

private:
    static const int kMaxNumericSize = 48;

    // 对于整型需要特殊处理
    template <typename T>
    void formatInteger(T);

    Buffer buffer_;
};

#endif // LOG_STREAM_H