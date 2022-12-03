add_executable(MysqlPoolTest MysqlPoolTest.cc)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/mysql/test)

target_link_libraries(MysqlPoolTest tiny_network)
