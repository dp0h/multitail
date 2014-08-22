#include "FileReader.h"

#include <exception>
#include <boost/format.hpp>
#include <io.h>
#include <stdio.h>
#include <Windows.h>

using namespace std;
using boost::str;
using boost::format;
using boost::wformat;

static DWORD GetFileSize(const wstring& fileName);
static wstring ExtractFilePath(const wstring& fileName);


FileReader::FileReader(wstring fileName, const shared_ptr<concurrent_queue<string>>& queue):
    fileName_(fileName),
    queue_(queue),
    lastPos_(0),
    buff_(new char[BuffLen]),
    buffPos_(0)
{
    wstring path = ExtractFilePath(fileName_);

    if (path == L"")
        throw new exception(str(format("Path for input file %1% is not defined.") % string(fileName_.begin(), fileName_.end())).c_str());

    if ((exitEventHandle_ = ::CreateEvent(nullptr, TRUE, FALSE, nullptr)) == nullptr)
        throw new exception("Could not create exit event");

    if ((changeNotificationHandle_ = ::FindFirstChangeNotification(path.c_str(), FALSE, FILE_NOTIFY_CHANGE_SIZE)) == INVALID_HANDLE_VALUE)
    {
        CloseHandle(exitEventHandle_);
        throw new exception(str(format("Could not register file change notification for %1%") % string(fileName_.begin(), fileName_.end())).c_str());
    }

    thread_ = unique_ptr<thread>(new thread(&FileReader::ThreadFunc, this));
}

FileReader::~FileReader(void)
{
    SetEvent(exitEventHandle_);
    thread_->join();
    FindCloseChangeNotification(changeNotificationHandle_);
    CloseHandle(exitEventHandle_);
}

void FileReader::LogError(const wstring& error)
{
    wcerr << error << endl;
}

void FileReader::ProcessNewLine(const string& line)
{
    queue_->push(line);
}

bool FileReader::WaitForEvent()
{
    HANDLE handles[2];
    handles[0] = exitEventHandle_;
    handles[1] = changeNotificationHandle_;

    DWORD res = WaitForMultipleObjects(2, handles, FALSE, INFINITE);

    switch (res)
    {
    case WAIT_OBJECT_0:
        return false;
    case WAIT_OBJECT_0 + 1:
        return true;
    default:
        LogError(str(wformat(L"WaitForMultipleObjects failed, error: %1%") % res));
        return false;
    }
}

void FileReader::Read()
{
    HANDLE file = CreateFile(fileName_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        LogError(L"Failed to open a file");
        return;
    }

    if (lastPos_ != 0 && SetFilePointer(file, lastPos_, nullptr, FILE_BEGIN) != lastPos_)
        LogError(L"Failed to set file pointer.");

    DWORD bytesRead;
    while (ReadFile(file, (LPVOID)&buff_[buffPos_], BuffLen - buffPos_, &bytesRead, nullptr) && bytesRead > 0)
    {
        buffPos_ += bytesRead;
        ProcessLinesInBuffer(false);
    }

    ProcessLinesInBuffer(true);
    lastPos_ = SetFilePointer(file, 0, 0, FILE_END);

    CloseHandle(file);
}

void FileReader::ProcessLinesInBuffer(bool flush)
{
    char* pos;
    while ((pos = (char*)memchr(buff_.get(), '\n', buffPos_)) != nullptr)
    {
        ProcessNewLine(string(buff_.get(), pos + 1));
        memmove(buff_.get(), pos + 1, buffPos_ - (pos - buff_.get()) - 1);
        buffPos_ -= ((pos - buff_.get()) + 1);
    }

    if ((flush && buffPos_ > 0) || buffPos_ == BuffLen)
    {
        ProcessNewLine(string(buff_.get(), buff_.get() + buffPos_));
        buffPos_ = 0;
    }
}

 void FileReader::ThreadFunc()
{
    Read();

    while (true)
    {
        if (!WaitForEvent())
            return;

        auto fileSize = GetFileSize(fileName_);
        if (fileSize > lastPos_)
        {
            Read();
        }
        else if (fileSize < lastPos_)
        {
            lastPos_ = buffPos_ = 0;
            Read();
        }

        if (!FindNextChangeNotification(changeNotificationHandle_))
        {
            LogError(L"FindNextChangeNotification failed");
            return;
        }
    }
}

static DWORD GetFileSize(const wstring& fileName)
{
    HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    DWORD dwFileSize = GetFileSize(hFile, nullptr);
    CloseHandle(hFile);
    return dwFileSize;
}

static wstring ExtractFilePath(const wstring& fileName)
{
    wchar_t drive[_MAX_DRIVE];
    wchar_t path[_MAX_PATH];
    wchar_t newPath[_MAX_PATH];

    _wsplitpath_s(fileName.c_str(), drive, _MAX_DRIVE, path, _MAX_PATH, nullptr, 0, nullptr, 0);
    _wmakepath_s(newPath, drive, path, nullptr, nullptr);

    return wstring(newPath);
}
