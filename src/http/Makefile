header_path = -I/home/shang/code/C++/github/my-muduo/mymuduo/base -I/home/shang/code/C++/github/my-muduo/mymuduo/net
LIBS=-lmymuduo_base -lmymuduo_net -lpthread
CFLAGS= -g -Wall

all:server test

server: HttpServer_test.cc HttpServer.cc HttpContext.cc HttpResponse.cc
	g++ HttpServer_test.cc HttpServer.cc HttpContext.cc HttpResponse.cc ${CFLAGS} ${header_path} ${LIBS} -o HttpServer

clean:
	rm -r HttpServer

