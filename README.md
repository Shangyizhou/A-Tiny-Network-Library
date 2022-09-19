# Tiny C++ Network Library

## Introduction

本项目是参考 muduo 实现的基于 Reactor 模型的多线程网络库。使用 C++ 11 编写，内部拥有一个小型的 HTTP 服务器案例，可支持GET请求和静态资源的访问并能处理超时连接。内部附有异步日志实现，监控服务端情况。

## Feature

- 底层使用 Epoll + LT 模式的 I/O 复用模型，并且结合非阻塞 I/O  实现 Reactor 模型
- 采用「one loop per thread」线程模型，并向上封装线程池避免线程创建，销毁的性能开销
- 采用 eventfd 作为事件通知描述符，高效派发事件到其他线程执行异步任务
- 基于双缓冲区实现的异步日志，避免数据落盘时阻塞服务
- 基于红黑树的定时器管理队列，高效管理定时任务
- 遵循RALL手法使用智能指针管理内存，减小内存泄露风险
- 利用有限状态机解析 HTTP 请求报文
- 服务器支持优雅断开连接操作

## Development Environment

- 操作系统：`Ubuntu 18.04.6 LTS`
- 编译器：`g++ 7.5.0`
- 编辑器：`vscode`
- 版本控制：`git`
- 项目构建：`cmake 3.10.2`

## Build

安装Cmake

```shell
sudo apt-get update
sudo apt-get install cmake
```

下载项目

```shell
git clone 
```

执行脚本构建项目

```shell
bash build.sh
```

## Running Examples

这里以一个简单的回声服务器作为案例，`EchoServer`默认监听端口为`8080`。

```shell
cd /example && ./EchoServer
```

执行情况：

![img](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663561528671-14461537-2593-4d52-b8da-da0c79248374.png)

`http`模块有一个小型的`HTTP`服务器案例，也可以执行。其默认监听`8080`：

```shell
./HttpServer
```

![img](https://cdn.nlark.com/yuque/0/2022/png/26752078/1663577836134-85f389cc-a3ac-4b55-8bb8-05d751633e39.png)

## Thanks For
- 《Linux高性能服务器编程》
- 《Linux多线程服务端编程：使用muduo C++网络库》
- https://github.com/chenshuo/muduo