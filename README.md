# Tiny C++ Network Library



| **Part Ⅰ**            | **Part Ⅱ**            | **Part Ⅲ**            | **Part Ⅳ**            | **Part Ⅴ**            | **Part Ⅵ**            | **Part Ⅶ**            |
| --------------------- | --------------------- | --------------------- | --------------------- | --------------------- | --------------------- | --------------------- |
| [项目介绍](#项目介绍) | [项目特点](#项目特点) | [开发环境](#开发环境) | [并发模型](#并发模型) | [构建项目](#构建项目) | [运行案例](#运行案例) | [模块讲解](#模块讲解) |

## 项目介绍

本项目是参考 muduo 实现的基于 Reactor 模型的多线程网络库。使用 C++ 11 编写去除 muduo 对 boost 的依赖，内部实现了一个小型的 HTTP 服务器，可支持 GET 请求和静态资源的访问，且附有异步日志监控服务端情况。

项目已经实现了 Channel 模块、Poller 模块、事件循环模块、HTTP 模块、定时器模块、异步日志模块、内存池模块、数据库连接池模块。   

## 项目特点

- 底层使用 Epoll + LT 模式的 I/O 复用模型，并且结合非阻塞 I/O  实现主从 Reactor 模型。
- 采用「one loop per thread」线程模型，并向上封装线程池避免线程创建和销毁带来的性能开销。
- 采用 eventfd 作为事件通知描述符，方便高效派发事件到其他线程执行异步任务。
- 基于自实现的双缓冲区实现异步日志，由后端线程负责定时向磁盘写入前端日志信息，避免数据落盘时阻塞网络服务。
- 基于红黑树实现定时器管理结构，内部使用 Linux 的 timerfd 通知到期任务，高效管理定时任务。
- 遵循 RAII 手法使用智能指针管理内存，减小内存泄露风险。
- 利用有限状态机解析 HTTP 请求报文。
- 参照 Nginx 实现了内存池模块，更好管理小块内存空间，减少内存碎片。
- 数据库连接池可以动态管理连接数量，及时生成或销毁连接，保证连接池性能。

## 开发环境

- 操作系统：`Ubuntu 18.04.6 LTS`
- 编译器：`g++ 7.5.0`
- 编辑器：`vscode`
- 版本控制：`git`
- 项目构建：`cmake 3.10.2`

## 并发模型

![image.png](https://cdn.nlark.com/yuque/0/2022/png/26752078/1670853134528-c88d27f2-10a2-46d3-b308-48f7632a2f09.png?x-oss-process=image%2Fresize%2Cw_937%2Climit_0)

项目采用主从 Reactor 模型，MainReactor 只负责监听派发新连接，在 MainReactor 中通过 Acceptor 接收新连接并轮询派发给 SubReactor，SubReactor 负责此连接的读写事件。

调用 TcpServer 的 start 函数后，会内部创建线程池。每个线程独立的运行一个事件循环，即 SubReactor。MainReactor 从线程池中轮询获取 SubReactor 并派发给它新连接，处理读写事件的 SubReactor 个数一般和 CPU 核心数相等。使用主从 Reactor 模型有诸多优点：

1. 响应快，不必为单个同步事件所阻塞，虽然 Reactor 本身依然是同步的；
2. 可以最大程度避免复杂的多线程及同步问题，并且避免多线程/进程的切换；
3. 扩展性好，可以方便通过增加 Reactor 实例个数充分利用 CPU 资源；
4. 复用性好，Reactor 模型本身与具体事件处理逻辑无关，具有很高的复用性；

## 构建项目

安装Cmake

```shell
sudo apt-get update
sudo apt-get install cmake
```

下载项目

```shell
git clone git@github.com:Shangyizhou/tiny-network.git
```

执行脚本构建项目

```shell
cd ./tiny-network && bash build.sh
```

## 运行案例

这里以一个简单的回声服务器作为案例，`EchoServer`默认监听端口为`8080`。

```shell
cd ./example
./EchoServer
```

执行情况：

![img](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663561528671-14461537-2593-4d52-b8da-da0c79248374.png)

`http`模块有一个小型的`HTTP`服务器案例，也可以执行。其默认监听`8080`：

```shell
cd ./src/http && ./HttpServer
```

![img](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663577836134-85f389cc-a3ac-4b55-8bb8-05d751633e39.png)

## 模块讲解

这里的某些模块会配置 muduo 源码讲解，有些使用的是本项目的源码，不过实现思路是一致的。

[Channel模块](./项目讲解/Channel模块.md)

[Poller模块](./项目讲解/Poller模块.md)

[EventLoop模块](./项目讲解/EventLoop模块.md)

[Buffer模块](./项目讲解/Buffer模块.md)

[定时器模块](./项目讲解/定时器模块.md)

[HTTP模块](./项目讲解/HTTP.md)

[异步日志模块](./项目讲解/异步日志模块.md)

[内存池模块](./项目讲解/内存池模块.md)

[数据库连接池模块](./项目讲解/数据库连接池模块.md)

## 优化计划

1. 计划使用 std::chrono 实现底层时间戳
2. 使用优先级队列管理定时器结构
3. 覆盖更多的单元测试

## 感谢
- 《Linux高性能服务器编程》
- 《Linux多线程服务端编程：使用muduo C++网络库》
- https://github.com/chenshuo/muduo