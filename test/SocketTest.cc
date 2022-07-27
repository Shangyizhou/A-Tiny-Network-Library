#include "../Socket.h"
#include "../InetAddress.h"
#include "../Logger.h"

//将信息输出到标准错误流,并关闭程序
void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_FATAL("%s:%s:%d socket() failed", __FILE__, __FUNCTION__, __LINE__);
    }

    InetAddress serv_address(8080);

    Socket socket(sockfd);
    socket.bindAddress(serv_address);// 绑定地址
    socket.listen();            // 监听状态

    socket.shutdownWrite();     // 设置半关闭
    socket.setTcpNoDelay(true); // 设置Nagle算法
    socket.setReuseAddr(true);  // 设置地址复用
    socket.setReusePort(true);  // 设置端口复用
    socket.setKeepAlive(true);  

    LOG_INFO("start to accept new connection");
    LOG_INFO("the server address:%s", serv_address.toIpPort().c_str());

    InetAddress clnt_address;
    int connfd = socket.accept(&clnt_address);     
    if (connfd < 0) {
        LOG_FATAL("%s:%s:%d accept() failed", __FILE__, __FUNCTION__, __LINE__);
    }

    LOG_INFO("accept a new connection connfd:%d!", connfd);

    return 0;
}