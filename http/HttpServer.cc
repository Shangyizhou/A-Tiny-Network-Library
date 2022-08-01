#include "HttpServer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpContext.h"

#include <memory>

/**
 * 默认的http回调函数
 * 设置响应状态码，响应信息并关闭连接
 */
void defaultHttpCallback(const HttpRequest&, HttpResponse* resp)
{
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(EventLoop *loop,
                      const InetAddress &listenAddr,
                      const std::string &name,
                      TcpServer::Option option)
  : server_(loop, listenAddr, name, option),
    httpCallback_(defaultHttpCallback)
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void HttpServer::start()
{
    LOG_INFO("HttpServer[%s] starts listening on %s", 
            server_.name().c_str(), 
            server_.ipPort().c_str());
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO("***********HttpServer::onConnection\n");
    if (conn->connected())
    {
      LOG_INFO("%s:%s:%d new Connection arrived\n", __FILE__, __FUNCTION__, __LINE__);
    }
}

// 有消息到来时的业务处理
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
    LOG_INFO("HttpServer::onMessage\n");
    std::unique_ptr<HttpContext> context(new HttpContext);

    const char* info = buf->retrieveAllAsString().c_str();
    LOG_INFO("The buf is %s\n", info);
    // 进行状态机解析
    // 错误则发送 BAD REQUEST 半关闭
    if (!context->parseRequest(buf, receiveTime))
    {
        LOG_INFO("parseRequest failed!\n");
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }

    // 如果成功解析
    if (context->gotAll())
    {
        LOG_INFO("parseRequest success!\n");
        onRequest(conn, context->request());
        context->reset();
    }
}

void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    const std::string& connection = req.getHeader("Connection");
    // 判断长连接还是短连接
    bool close = connection == "close" ||
        (req.version() == HttpRequest::kHttp10 && connection != "Keep-Alive");
    // 响应信息
    HttpResponse response(close);
    // httpCallback_ 由用户传入，怎么写响应体由用户决定
    httpCallback_(req, &response);
    Buffer buf;
    response.appendToBuffer(&buf);
    // TODO:需要重载 TcpConnection::send 使其可以接收一个缓冲区
    conn->send(&buf);
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}