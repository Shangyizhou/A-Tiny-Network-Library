PROJECT_PATH=/home/shang/code/C++/github/my-muduo/
HEADER_PATH=-I ${PROJECT_PATH}/include/mymuduo/base \
			-I ${PROJECT_PATH}/include/mymuduo/net \
			-I ${PROJECT_PATH}/include/mymuduo/net/poller \
			-I ${PROJECT_PATH}/include/mymuduo/logger \
			-I ${PROJECT_PATH}/include/mymuduo/timer 
LIB_PATH=-lmymuduo -lpthread
CFLAGS= -g -Wall ${LIB_PATH} ${HEADER_PATH}

all: EchoServer AsyncLoggingServer

EchoServer: echoServer.cc
	g++ echoServer.cc ${CFLAGS} -o EchoServer

AsyncLoggingServer: echoServerAsyncLogging.cc
	g++ echoServerAsyncLogging.cc ${CFLAGS} -o AsyncLoggingServer

clean:
	rm -r EchoServer AsyncLoggingServer
