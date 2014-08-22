#include "FileWriter.h"

#include <exception>
#include <boost/format.hpp>
#include <Windows.h>

using namespace std;
using boost::str;
using boost::format;
using boost::wformat;

FileWriter::FileWriter(std::wstring fileName, const std::shared_ptr<concurrent_queue<std::string>>& queue) :
    queue_(queue),
    exit_(false)
{
    file_ = CreateFile(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_ == INVALID_HANDLE_VALUE)
    {
        throw new exception(str(format("Could not create file %1%") % string(fileName.begin(), fileName.end())).c_str());
    }

    thread_ = unique_ptr<thread>(new thread(&FileWriter::ThreadFunc, this));
}

FileWriter::~FileWriter(void)
{
    exit_ = true;
    queue_->push(""); // awake thread to exit
    thread_->join();
    CloseHandle(file_);
}

void FileWriter::ThreadFunc()
{
    while (!exit_)
    {
        string msg = queue_->wait_pop();

        if (msg.size() > 0)
        {
            DWORD written;
            WriteFile(file_, msg.c_str(), msg.size(), &written, nullptr);
        }
    }
}
