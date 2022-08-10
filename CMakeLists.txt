# 指定使用 CMake 的最低版本号
cmake_minimum_required(VERSION 3.0)

# 设置项目名称
project(mymuduo)

# mymuduo最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# "${CMAKE_CXX_FLAGS} -g -std=c++11"

# 设置调试信息
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11" )

# 加载子目录
add_subdirectory(mymuduo/base)
add_subdirectory(mymuduo/net)

# 定义参与编译的源文件
# aux_source_directory(. SRC_LIST)

# 编译动态库
# add_library(mymuduo SHARED ${SRC_LIST})

