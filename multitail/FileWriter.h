#pragma once

#include <string>
#include <memory>
#include <thread>
#include "concurrent_queue.h"

class FileWriter
{
public:
    FileWriter(std::wstring fileName, const std::shared_ptr<concurrent_queue<std::string>>& queue);
    ~FileWriter(void);

private:
    FileWriter(const FileWriter&);
    FileWriter& operator=(const FileWriter&);

    void ThreadFunc();

private:
    const std::shared_ptr<concurrent_queue<std::string>>& queue_;
    std::unique_ptr<std::thread> thread_;
    HANDLE file_;
    volatile bool exit_;
};
