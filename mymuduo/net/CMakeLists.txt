set(net_SRCS
  Acceptor.cc
  Buffer.cc
  Channel.cc
  EventLoop.cc
  EventLoopThread.cc
  EventLoopThreadPool.cc
  InetAddress.cc
  poller/DefaultPoller.cc
  poller/EPollPoller.cc
  poller/Poller.cc
  Socket.cc
  TcpConnection.cc
  TcpServer.cc
  )

include_directories(${PROJECT_SOURCE_DIR}/mymuduo/base)
include_directories(${PROJECT_SOURCE_DIR}/mymuduo/net)
include_directories(${PROJECT_SOURCE_DIR}/mymuduo/net/poller)

add_library(mymuduo_net SHARED ${net_SRCS})
target_link_libraries(mymuduo_net mymuduo_base)

# 将 mymuduo_net 安装到 /user/lib 当中
install(TARGETS mymuduo_net DESTINATION lib)