#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mymuduo/net/Buffer.h"



int main()
{
    Buffer* output = new Buffer;

    // 响应行
    char buf[32];
    memset(buf, '\0', sizeof(buf));
    snprintf(buf, sizeof(buf), "HTTP/1.1 %d ", 200);
    output->append(buf);
    output->append("OK");
    output->append("\r\n");

    std::cout << output->GetBufferAllAsString() << std::endl;

    return 0;
}

// g++ test.cc -g -o test