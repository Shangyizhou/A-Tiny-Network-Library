add_executable(EchoServer echoServer.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)

target_link_libraries(EchoServer tiny_network)