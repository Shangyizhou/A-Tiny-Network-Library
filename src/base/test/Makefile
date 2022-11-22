PROJECT_PATH=/home/shang/code/C++/github-project/student-work-project/my-muduo
HEADER_PATH=-I ${PROJECT_PATH}/src/base \
			-I ${PROJECT_PATH}/src/net \
			-I ${PROJECT_PATH}/src/net/poller \
			-I ${PROJECT_PATH}/src/logger \
			-I ${PROJECT_PATH}/src/timer 
LIB_PATH=-L${PROJECT_PATH}/lib -ltiny_network -lpthread
CFLAGS= -g -Wall ${LIB_PATH} ${HEADER_PATH}

all: ThreadPool

ThreadPool: ThreadPool.cc
	g++ ThreadPool.cc ${CFLAGS} -o ThreadPool

clean:
	rm -r ThreadPool 
