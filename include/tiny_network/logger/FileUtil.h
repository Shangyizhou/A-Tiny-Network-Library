#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <stdio.h>
#include <string>

class FileUtil
{
public:
    explicit FileUtil(std::string& fileName);
    ~FileUtil();

    void append(const char* data, size_t len);

    void flush();

    off_t writtenBytes() const { return writtenBytes_; }

private:    
    size_t write(const char* data, size_t len);

    FILE* fp_;
    char buffer_[64 * 1024]; // fp_的缓冲区
    off_t writtenBytes_; // off_t用于指示文件的偏移量
};

#endif // FILE_UTIL_H