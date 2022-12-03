# Tiny C++ Network Library

## 项目介绍

本项目是参考 muduo 实现的基于 Reactor 模型的多线程网络库。使用 C++ 11 编写，内部拥有一个小型的 HTTP 服务器案例，可支持GET请求和静态资源的访问并能处理超时连接。内部附有异步日志实现，监控服务端情况。

## 项目特点

- 底层使用 Epoll + LT 模式的 I/O 复用模型，并且结合非阻塞 I/O  实现主从 Reactor 模型。
- 采用「one loop per thread」线程模型，并向上封装线程池避免线程创建，销毁的性能开销。
- 采用 eventfd 作为事件通知描述符，高效派发事件到其他线程执行异步任务。
- 基于双缓冲区实现的异步日志，避免数据落盘时阻塞服务。
- 基于红黑树的定时器管理队列，基于 Linux 的 timerfd 分派定时任务，高效管理定时任务。
- 遵循 RALL 手法使用智能指针管理内存，减小内存泄露风险。
- 利用有限状态机解析 HTTP 请求报文。
- 参照 Nginx 实现了内存池模块，更好管理小块内存空间，减少内存碎片。
- 数据库连接池动态管理连接数量，防止多余连接浪费性能。

## 开发环境

- 操作系统：`Ubuntu 18.04.6 LTS`
- 编译器：`g++ 7.5.0`
- 编辑器：`vscode`
- 版本控制：`git`
- 项目构建：`cmake 3.10.2`

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

## 项目模块讲解

这里的某些模块会配置 muduo 源码讲解，有些使用的是本项目的源码，不过实现思路是一致的。

[Channel模块]()

[EPollPoller模块]()

[EventLoop模块]()

[Buffer模块](./项目讲解/Buffer模块.md)

[定时器模块](./项目讲解/定时器模块.md)

[HTTP模块](./项目讲解/HTTP.md)

[异步日志模块](./项目讲解/异步日志模块.md)

[内存池模块](./项目讲解/内存池模块.md)

[数据库连接池模块](./项目讲解/数据库连接池模块.md)

## 感谢
- 《Linux高性能服务器编程》
- 《Linux多线程服务端编程：使用muduo C++网络库》
- https://github.com/chenshuo/muduo