#include "HttpContext.h"
#include "Buffer.h"

// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end)
{
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');

    // 不是最后一个空格，并且成功获取了method并设置到request_
    if (space != end && request_.setMethod(start, space))
    {
        // 跳过空格
        start = space+1;
        // 继续寻找下一个空格
        space = std::find(start, end, ' ');
        if (space != end)
        {
            // 查看是否有请求参数
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                // 设置访问路径
                request_.setPath(start, question);
                // 设置访问变量
                request_.setQuery(question, space);
            }
            else
            {
                request_.setPath(start, space);
            }
            start = space+1;
            // 获取最后的http版本
            succeed = (end-start == 8 && std::equal(start, end-1, "HTTP/1."));
            if (succeed)
            {
                if (*(end-1) == '1')
                {
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if (*(end-1) == '0')
                {
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else
                {
                    succeed = false;
                }
            }
        }
    }  
    return succeed;
}

// return false if any error
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
    bool ok = false;
    bool hasMore = true;
    while (hasMore)
    {
        // 请求行状态
        if (state_ == kExpectRequestLine)
        {
            // 找到 \r\n 位置
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                // 从可读区读取请求行
                // [peek(), crlf + 2) 是一行
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    // readerIndex 向后移动位置直到 crlf + 2
                    buf->retrieveUntil(crlf + 2);
                    // 状态转移，接下来解析请求头
                    state_ = kExpectHeaders;
                }
                else
                {
                    hasMore = false;
                }
            }
            else
            {
                hasMore = false;
            }
        }
        // 解析请求头
        else if (state_ == kExpectHeaders)
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                // 找到 : 位置
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    // 添加状态首部
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else // colon == crlf 说明没有找到 : 了，直接返回 end
                {
                    // empty line, end of header
                    // FIXME:
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2);
            }
            else
            {
                hasMore = false;
            }
        }
        // 解析请求体，可以看到这里没有做出处理，只支持GET请求
        else if (state_ == kExpectBody)
        {
            // FIXME:
        }
    }
    return ok;
}
