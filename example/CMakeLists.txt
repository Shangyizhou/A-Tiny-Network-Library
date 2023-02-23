# 加载子目录
set(
    echoServer.cc
    echoServerAsync.cc
)

add_executable(echoServer echoServer.cc)

add_executable(echoServerAsync echoServerAsync.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)

target_link_libraries(echoServer tiny_network)
target_link_libraries(echoServerAsync tiny_network)

