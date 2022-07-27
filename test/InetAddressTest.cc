#include "../InetAddress.h"
#include <iostream>

int main()
{
    // 127.0.0.1:8080
    InetAddress address(8080);

    std::cout << address.toIp() << std::endl;       // 127.0.0.1
    std::cout << address.toPort() << std::endl;     // 8080
    std::cout << address.toIpPort() << std::endl;   // 127.0.0.1:8080

    std::cout << "new server address" << std::endl;

    // sockaddr_in *tmp_addr = address.getSockAddr(); 必须为const类型
    const sockaddr_in *tmp_addr = address.getSockAddr();

    sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(3000);

    address.setSockAddr(server_addr);
    std::cout << address.toIp() << std::endl;       // 0.0.0.0
    std::cout << address.toPort() << std::endl;     // 3000
    std::cout << address.toIpPort() << std::endl;   // 0.0.0.0:3000
    
    return 0;
}