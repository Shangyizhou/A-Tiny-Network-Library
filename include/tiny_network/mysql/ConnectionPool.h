#ifndef CONNECTION_POOL_H
#define CONNECTION_POOL_H

#include "MysqlConn.h"
#include "json.hpp"
using json = nlohmann::json; 

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class ConnectionPool
{
public:
    static ConnectionPool* getConnectionPool();
    std::shared_ptr<MysqlConn> getConnection();
    ~ConnectionPool();

private:
    ConnectionPool();
    ConnectionPool(const ConnectionPool& obj) = delete;
    ConnectionPool(const ConnectionPool&& obj) = delete;
    ConnectionPool& operator=(const ConnectionPool& obj) = delete;

    bool parseJsonFile();
    void produceConnection();
    void recycleConnection();
    void addConnection();

    // TODO:加上文件路径
    // std::string filePath_;
    std::string ip_;
    std::string user_;
    std::string passwd_;
    std::string dbName_;
    unsigned short port_;
    int minSize_;
    int maxSize_;
    int currentSize_;
    int timeout_;
    int maxIdleTime_;
    std::queue<MysqlConn*> connectionQueue_; // 连接池队列
    std::mutex mutex_; 
    std::condition_variable cond_;
};

#endif // CONNECTION_POOL_H