set(base_SRCS
    Logger.cc
    CurrentThread.cc
    Thread.cc
    Timestamp.cc
    )

# 生成动态库或静态库
add_library(mymuduo_base SHARED ${base_SRCS})

# 将目标文件与库文件进行链接
target_link_libraries(mymuduo_base pthread)

# 将 mymuduo_net 安装到 /user/lib 当中
install(TARGETS mymuduo_base DESTINATION lib)