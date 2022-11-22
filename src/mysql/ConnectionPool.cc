#include "ConnectionPool.h"

#include <fstream>
#include <thread>
#include <assert.h>

ConnectionPool* ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool;
    return &pool;
}

ConnectionPool::ConnectionPool()
{
    // assert();
    parseJsonFile();

    for (int i = 0; i < minSize_; ++i)
    {
        addConnection();
        currentSize_++;
    }
    // 开启新线程执行任务
    std::thread producer(&ConnectionPool::produceConnection, this);
    std::thread recycler(&ConnectionPool::recycleConnection, this);
    // 设置线程分离，不阻塞在此处
    producer.detach();
    recycler.detach();
}

ConnectionPool::~ConnectionPool()
{
    // 释放队列里管理的MySQL连接资源
    while (!connectionQueue_.empty())
    {
        MysqlConn* conn = connectionQueue_.front();
        connectionQueue_.pop();
        delete conn;
        currentSize_--;
    }
}

// 解析JSON配置文件
bool ConnectionPool::parseJsonFile()
{
    std::ifstream file("conf.json");
    json conf = json::parse(file);

    ip_ = conf["ip"];
    user_ = conf["userName"];
    passwd_ = conf["password"];
    dbName_ = conf["dbName"];
    port_ = conf["port"];
    minSize_ = conf["minSize"];
    maxSize_ = conf["maxSize"];
    timeout_ = conf["timeout"];
    maxIdleTime_ = conf["maxIdleTime"];
    return true;
}

void ConnectionPool::produceConnection()
{
    while (true)
    {
        // RALL手法封装的互斥锁，初始化即加锁，析构即解锁
        std::unique_lock<std::mutex> locker(mutex_);
        while (!connectionQueue_.empty())
        {
            // 还有可用连接则不创建
            cond_.wait(locker);
        } 
        
        // 还没达到连接最大限制
        if (currentSize_ < maxSize_)
        {
            addConnection();
            currentSize_++;
            // 唤醒被阻塞的线程
            cond_.notify_all();
        }       
    }
}

// 销毁多余的数据库连接
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        // 周期性的做检测工作，每500毫秒（0.5s）执行一次
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        std::lock_guard<std::mutex> locker(mutex_);
        // 连接数位于(maxSize, maxSize]时候，可能会有空闲连接等待太久
        while (connectionQueue_.size() > minSize_)
        {
            MysqlConn* conn = connectionQueue_.front();
            if (conn->getAliveTime() >= maxIdleTime_)
            {
                // 存在时间超过设定值则销毁
                connectionQueue_.pop();
                delete conn;
                currentSize_--;
            }
            else
            {
                // 按照先进先出顺序，前面的没有超过后面的肯定也没有
                break;
            }
        }
    }
}

void ConnectionPool::addConnection()
{
    MysqlConn* conn = new MysqlConn;
    conn->connect(user_, passwd_, dbName_, ip_, port_);
    conn->refreshAliveTime();    // 刷新起始的空闲时间点
    connectionQueue_.push(conn); // 记录新连接
    currentSize_++;
}

// 获取连接
std::shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    std::unique_lock<std::mutex> locker(mutex_);
    if (connectionQueue_.empty())
    {
        while (connectionQueue_.empty())
        {
            // 如果为空，需要阻塞一段时间，等待新的可用连接
            if (std::cv_status::timeout == cond_.wait_for(locker, std::chrono::milliseconds(timeout_)))
            {
                // std::cv_status::timeout 表示超时
                if (connectionQueue_.empty())
                {
                    continue;
                }
            }
        }
    }
    
    // 有可用的连接
    // 如何还回数据库连接？
    // 使用共享智能指针并规定其删除器
    // 规定销毁后调用删除器，在互斥的情况下更新空闲时间并加入数据库连接池
    std::shared_ptr<MysqlConn> connptr(connectionQueue_.front(), 
        [this](MysqlConn* conn) {
            std::lock_guard<std::mutex> locker(mutex_);
            conn->refreshAliveTime();
            connectionQueue_.push(conn);
        });
    connectionQueue_.pop();
    cond_.notify_all();
    return connptr;
}