#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class concurrent_queue
{
public:
    void push(const T& item)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(item);
        lock.unlock();
        cond_.notify_one();
    }

    T wait_pop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty())
        {
            cond_.wait(lock);
        }
        auto ret = queue_.front();
        queue_.pop();
        return ret;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
};
