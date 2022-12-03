#ifndef HTTP_HTTPCONTEXT_H
#define HTTP_HTTPCONTEXT_H

#include "HttpRequest.h"

class Buffer;

class HttpContext
{
public:
    // HTTP请求状态
    enum HttpRequestParseState
    {
        kExpectRequestLine, // 解析请求行状态
        kExpectHeaders,     // 解析请求头部状态
        kExpectBody,        // 解析请求体状态
        kGotAll,            // 解析完毕状态
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    bool parseRequest(Buffer* buf, Timestamp receiveTime);

    bool gotAll() const { return state_ == kGotAll; }

    // 重置HttpContext状态，异常安全
    void reset()
    {
        state_ = kExpectRequestLine;
        /**
         * 构造一个临时空HttpRequest对象，和当前的成员HttpRequest对象交换置空
         * 然后临时对象析构
         */
        HttpRequest dummy;
        request_.swap(dummy);
    }

    const HttpRequest& request() const { return request_; }

    HttpRequest& request() { return request_; }

private:
    bool processRequestLine(const char *begin, const char *end);

    HttpRequestParseState state_;
    HttpRequest request_;
};

#endif // HTTP_HTTPCONTEXT_H