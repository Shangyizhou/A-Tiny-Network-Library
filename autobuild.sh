#!/bin/bash

set -e

SOURCE_DIR=`pwd`
SRC_BASE=${SOURCE_DIR}/mymuduo/base
SRC_NET=${SOURCE_DIR}/mymuduo/net

# 如果没有build目录 创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -fr `pwd`/build/*
cd `pwd`/build &&
    cmake .. &&
    make

# 将base头文件拷贝到/usr/include/mymuduo_base
if [ ! -d /usr/include/mymuduo/base ]; then
    mkdir -p /usr/include/mymuduo/base
fi

cd ${SRC_BASE}

for header in `ls *.h`y
do
    cp $header /usr/include/mymuduo/base
done

# 将net头文件拷贝到/usr/include/mymuduo_base
if [ ! -d /usr/include/mymuduo/net ]; then
    mkdir -p /usr/include/mymuduo/net
fi

if [ ! -d /usr/include/mymuduo/net/poller ]; then
    mkdir -p /usr/include/mymuduo/net/poller
fi

cd ${SRC_NET}

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo/net
done

cd ${SRC_NET}/poller

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo/net/poller
done

cd ${SOURCE_DIR}
cp ${SOURCE_DIR}/lib/libmymuduo_base.so /usr/lib
cp ${SOURCE_DIR}/lib/libmymuduo_net.so /usr/lib

ldconfig