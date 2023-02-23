#include <iostream>
#include <memory>
#include <thread>
#include "MysqlConn.h"
#include "ConnectionPool.h"
using namespace std;
// 1. 单线程: 使用/不使用连接池
// 2. 多线程: 使用/不使用连接池

// 非连接池
void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        MysqlConn conn;
        conn.connect("root", "123456", "test", "127.0.0.1");
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into user values(%d, 'zhang san', '221B')", i);
        conn.update(sql);
    }
}

// 连接池
void op2(ConnectionPool* pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        shared_ptr<MysqlConn> conn = pool->getConnection();
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "insert into user values(%d, 'zhang san', '221B')", i);
        conn->update(sql);
    }
}

// 非连接池查询
void op3(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        MysqlConn conn;
        conn.connect("root", "123456", "test", "127.0.0.1");
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "select * from user");
        conn.query(sql);
    }
}

// 连接池查询
void op4(ConnectionPool* pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        shared_ptr<MysqlConn> conn = pool->getConnection();
        char sql[1024] = { 0 };
        snprintf(sql, sizeof(sql), "select * from user");
        conn->query(sql);
    }
}

// 单线程
void test1()
{
#if 0
    // 非连接池, 单线程, 用时: 34127689958 纳秒, 34127 毫秒
    steady_clock::time_point begin = steady_clock::now();
    op1(0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "非连接池, 单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;
#else
    // 连接池, 单线程, 用时: 19413483633 纳秒, 19413 毫秒
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    steady_clock::time_point begin = steady_clock::now();
    op2(pool, 0, 5000);
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

// 多线程
void test2()
{
#if 0
    // 非连接池, 多线程, 用时: 15702495964 纳秒, 15702 毫秒
    MysqlConn conn;
    conn.connect("root", "123456", "test", "127.0.0.1");
    steady_clock::time_point begin = steady_clock::now();
    std::thread t1(op1, 0, 1000);
    std::thread t2(op1, 1000, 2000);
    std::thread t3(op1, 2000, 3000);
    std::thread t4(op1, 3000, 4000);
    std::thread t5(op1, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "非连接池, 多单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#else
    // 连接池, 多线程, 用时: 6076443405 纳秒, 6076 毫秒
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    steady_clock::time_point begin = steady_clock::now();
    std::thread t1(op2, pool, 0, 1000);
    std::thread t2(op2, pool, 1000, 2000);
    std::thread t3(op2, pool, 2000, 3000);
    std::thread t4(op2, pool, 3000, 4000);
    std::thread t5(op2, pool, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 多单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;

#endif
}

void test3()
{
    // 连接池, 多线程, 用时: 6076443405 纳秒, 6076 毫秒
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    steady_clock::time_point begin = steady_clock::now();
    std::thread t1(op4, pool, 0, 1000);
    std::thread t2(op4, pool, 1000, 2000);
    std::thread t3(op4, pool, 2000, 3000);
    std::thread t4(op4, pool, 3000, 4000);
    std::thread t5(op4, pool, 4000, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    steady_clock::time_point end = steady_clock::now();
    auto length = end - begin;
    cout << "连接池, 多单线程, 用时: " << length.count() << " 纳秒, "
        << length.count() / 1000000 << " 毫秒" << endl;
}

// 查询测试
int query()
{
    MysqlConn conn;
    conn.connect("root", "123456", "test", "127.0.0.1");
    string sql = "insert into user values(1, 'zhang san', '221B')";
    bool flag = conn.update(sql);
    cout << "flag value:  " << flag << endl;

    sql = "select * from user";
    conn.query(sql);
    // 从结果集中取出一行
    while (conn.next())
    {
        // 打印每行字段值
        cout << conn.value(0) << ", "
            << conn.value(1) << ", "
            << conn.value(2) << ", "
            << conn.value(3) << endl;
    }
    return 0;
}

int main()
{   
    // 测试MysqlConn的增删改查操作
    // query(); 

    // 测试在单线程环境下，使用连接池和不使用连接池性能差距
    // test1(); 
    
    // 测试在多线程环境下，使用连接池和不使用连接池性能差距
    test3();  

    return 0;
}