#pragma once

#include <string>
#include <vector>
#include <thread>
#include <memory>
#include "concurrent_queue.h"

class FileReader
{
public:
    FileReader(std::wstring fileName, const std::shared_ptr<concurrent_queue<std::string>>& queue);
    ~FileReader();

private:
    FileReader(const FileReader&);
    FileReader& operator=(const FileReader&);

    void LogError(const std::wstring& msg);
    void ProcessNewLine(const std::string& line);
    bool WaitForEvent();
    void Read();
    void ProcessLinesInBuffer(bool flush);
    void ThreadFunc();

private:
    static const int BuffLen = 1024*100;

    std::wstring fileName_;
    const std::shared_ptr<concurrent_queue<std::string>>& queue_;
    std::unique_ptr<std::thread> thread_;
    HANDLE changeNotificationHandle_;
    HANDLE exitEventHandle_;
    DWORD lastPos_;
    std::unique_ptr<char[]> buff_;
    unsigned int buffPos_;
};
