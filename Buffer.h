#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        :   buffer_(kCheapPrepend + initialSize),
            readerIndex_(kCheapPrepend),
            writerIndex_(kCheapPrepend)
        {}
    
    /**
     * kCheapPrepend | reader | writer |
     * writerIndex_ - readerIndex_
     */
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    /**
     * kCheapPrepend | reader | writer |
     * buffer_.size() - writerIndex_
     */   
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    /**
     * kCheapPrepend | reader | writer |
     * wreaderIndex_
     */    
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    // 需要进行复位操作
    void retrieve(size_t len)
    {
        // 应用只读取可读缓冲区数据的一部分(读取了len的长度)
        if (len < readableBytes())
        {
            // 移动可读缓冲区指针
            readerIndex_ += len;
        }
        // 全部读完 len == readableBytes()
        else
        {
            retrieveAll();
        }
    }

    // 全部读完，则直接将可读缓冲区指针移动到写缓冲区指针那
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // 将onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {   
        // 应用可读取数据的长度
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        // peek()可读数据的起始地址
        std::string result(peek(), len);
        // 上面一句把缓冲区中可读取的数据读取出来，所以要将缓冲区复位
        retrieve(len); 
        return result;
    }

    // buffer_.size() - writeIndex_
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            // 扩容函数
            makeSpace(len);
        }
    }

    // 把[data, data+len]内存上的数据添加到缓冲区中
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);
    // 通过fd发送数据
    ssize_t writeFd(int fd, int *saveErrno);
    
private:
    char* begin()
    {
        // 获取buffer_起始地址
        return &(*buffer_.begin());
    }

    const char* begin() const
    {
        return &(*buffer_.begin());
    }

    // TODO:扩容操作
    void makeSpace(int len)
    {
        /**
         * kCheapPrepend | reader | writer |
         * kCheapPrepend |       len         |
         */
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

#endif // BUFFER_H