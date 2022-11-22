#include "MysqlConn.h"

// 初始化数据库连接
MysqlConn::MysqlConn()
{
    // 分配或初始化与mysql_real_connect()相适应的MYSQL对象
    // 如果mysql是NULL指针，该函数将分配、初始化、并返回新对象
    conn_ = mysql_init(nullptr);
    // 设置字符编码，可以储存中文
    mysql_set_character_set(conn_, "utf8"); 
}

// 释放数据库连接
MysqlConn::~MysqlConn()
{
    if (conn_ != nullptr) {
        mysql_close(conn_);
    }
    // 释放结果集
    freeResult();
}

// 连接数据库
bool MysqlConn::connect(const std::string& user, const std::string& passwd, const std::string dbName, const std::string& ip, const unsigned int& port)
{
    // 尝试与运行在主机上的MySQL数据库引擎建立连接
    MYSQL* ptr = mysql_real_connect(conn_, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    return ptr != nullptr;
}

// 更新数据库：包括 insert update delete 操作
bool MysqlConn::update(const std::string& sql)
{
    if (mysql_query(conn_, sql.c_str()))
    {
        return false;
    }
    return true;
}

// 查询数据库
bool MysqlConn::query(const std::string& sql)
{
    // 查询前确保结果集为空
    freeResult();
    if (mysql_query(conn_, sql.c_str()))
    {
        return false;
    }
    // 储存结果集(这是一个二重指针)
    result_ = mysql_store_result(conn_);
    return true;
}

// 遍历查询得到的结果集
bool MysqlConn::next()
{
    if (result_ != nullptr)
    {
        row_ = mysql_fetch_row(result_);
        if (row_ != nullptr)
        {
            return true;
        }
    }
    return false;
}

// 得到结果集中的字段值
std::string MysqlConn::value(int index)
{
    int rowCount = mysql_num_fields(result_);
    if (index >= rowCount || index < 0)
    {
        // 获取字段索引不合法，返回空字符串
        return std::string();
    }
    // 考虑到储存的可能是二进制字符串，其中含有'\0'
    // 那么我们无法获得完整字符串，因此需要获取字符串头指针和字符串长度
    char* val = row_[index];
    unsigned long length = mysql_fetch_lengths(result_)[index];
    return std::string(val, length);
}

// 事务操作
bool MysqlConn::transaction()
{
    // true  自动提交
    // false 手动提交
    return mysql_autocommit(conn_, false);
}

// 提交事务
bool MysqlConn::commit()
{
    return mysql_commit(conn_);
}

// 事务回滚
bool MysqlConn::rollbock()
{
    return mysql_rollback(conn_);
}

// 刷新起始的空闲时间点
void MysqlConn::refreshAliveTime()
{
    // 获取时间戳
    m_alivetime = std::chrono::steady_clock::now();
}

// 计算连接存活的总时长
long long MysqlConn::getAliveTime()
{
    // 获取时间段（当前时间戳 - 创建时间戳）
    std::chrono::nanoseconds res = std::chrono::steady_clock::now() - m_alivetime;
    // 纳秒 -> 毫秒，高精度向低精度转换需要duration_cast
    std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
    // 返回毫秒数量
    return millsec.count();
}

// 释放结果集
void MysqlConn::freeResult()
{
    if (result_)
    {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}