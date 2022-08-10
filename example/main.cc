#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define BUF_SIZE 100
#define EPOLL_SIZE 50
void error_handling(const char *message);

int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        error_handling("createNonblocking() failed");
    }
    return sockfd;
}

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;
    int str_len, i;
    char buf[BUF_SIZE];

    struct epoll_event *ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

    if (argc != 2)
    {
        printf("Usage : %s <port> \n", argv[0]);
        exit(1);
    }
    serv_sock = createNonblocking();
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    // 向操作系统申请内存并获取epoll_fd
    epfd = epoll_create(EPOLL_SIZE); //可以忽略这个参数，填入的参数为操作系统参考
    // 最多可以关注 EPOLL_SIZE 个事件
    ep_events = (epoll_event *)malloc(sizeof(struct epoll_event) * EPOLL_SIZE);


    event.events = EPOLLIN;     // 监视读数据事件
    event.data.fd = serv_sock;  // 需要监视的文件描述符
    // 向操作系统中添加改事件
    epoll_ctl(epfd, EPOLL_CTL_ADD, serv_sock, &event); //例程epfd 中添加文件描述符 serv_sock，目的是监听 enevt 中的事件

    while (1)
    {
        // 返回发生的事件个数
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1); //获取改变了的文件描述符，返回数量
        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        // 这个循环的所有文件描述符都是发生了事件的，效率得到了提高
        for (i = 0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock) //客户端请求连接时
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept4(serv_sock, (struct sockaddr *)&clnt_adr, &adr_sz, SOCK_NONBLOCK | SOCK_CLOEXEC);
                // 我们也需要监视这个跟客户端通信的套接字
                event.events = EPOLLIN;
                event.data.fd = clnt_sock; //把客户端套接字添加进去
                epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sock, &event);
                printf("connected client : %d \n", clnt_sock);
            }
            else //是客户端套接字时
            {
                str_len = read(ep_events[i].data.fd, buf, BUF_SIZE);
                if (str_len == 0)
                {
                    // 客户端关闭了连接，我们也通过 epoll_ctl 告诉操作系统不用关注此文件描述符的事件了
                    epoll_ctl(epfd, EPOLL_CTL_DEL, ep_events[i].data.fd, NULL); //从epoll中删除套接字
                    close(ep_events[i].data.fd);
                    printf("closed client : %d \n", ep_events[i].data.fd);
                }
                else
                {
                    write(ep_events[i].data.fd, buf, str_len);
                }
            }
        }
    }
    close(serv_sock);
    close(epfd);

    return 0;
}

void error_handling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}