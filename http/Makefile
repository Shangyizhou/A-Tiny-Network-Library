LIBS=-lmymuduo -lpthread

server: HttpServer_test.cc HttpServer.cc HttpContext.cc HttpResponse.cc
	g++ HttpServer_test.cc HttpServer.cc HttpContext.cc HttpResponse.cc $(LIBS) -o HttpServer
	
clean:
	rm -r HttpServer
