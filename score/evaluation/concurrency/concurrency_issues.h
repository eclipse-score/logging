

#ifndef SCORE_EVALUATION_CONCURRENCY_ISSUES_H
#define SCORE_EVALUATION_CONCURRENCY_ISSUES_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace score
{
namespace evaluation
{

class UnsafeCounter
{
public:
    void Increment() { ++count_; }
    int  Get() const { return count_; }

private:
    int count_{0};
};

class ManualLockUser
{
public:
    void Write(const std::string& data)
    {
        mtx_.lock();
        ProcessData(data);
        mtx_.unlock();
    }

private:
    static void ProcessData(const std::string& /*data*/)
    {
    }

    std::mutex mtx_;
};

class BrokenLazyInit
{
public:
    void EnsureInitialised()
    {
        if (!initialised_)
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (!initialised_)
            {
                resource_ = 42;
                initialised_ = true;
            }
        }
    }

    int GetResource() const { return resource_; }

private:
    bool       initialised_{false};
    int        resource_{0};
    std::mutex mtx_;
};

class DeadlockProne
{
public:
    void TransferAB(int amount)
    {
        std::lock_guard<std::mutex> la(mtx_a_);
        std::lock_guard<std::mutex> lb(mtx_b_);
        balance_a_ -= amount;
        balance_b_ += amount;
    }

    void TransferBA(int amount)
    {
        std::lock_guard<std::mutex> lb(mtx_b_);
        std::lock_guard<std::mutex> la(mtx_a_);
        balance_b_ -= amount;
        balance_a_ += amount;
    }

private:
    std::mutex mtx_a_;
    std::mutex mtx_b_;
    int balance_a_{1000};
    int balance_b_{1000};
};

class SpuriousWakeupBug
{
public:
    void Wait()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        if (!ready_) cv_.wait(lk);
    }

    void Signal()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        ready_ = true;
        cv_.notify_one();
    }

private:
    std::mutex              mtx_;
    std::condition_variable cv_;
    bool                    ready_{false};
};

inline void StartDetachedThread()
{
    std::string local_config = "config_data";

    std::thread t([&local_config]()
    {
        (void)local_config;
    });
    t.detach();
}

class TocTouAtomic
{
public:
    void ConsumeIfAvailable()
    {
        if (count_.load() > 0)
        {
            --count_;
        }
    }

    void Produce() { ++count_; }

private:
    std::atomic<int> count_{0};
};

}
}

#endif  // SCORE_EVALUATION_CONCURRENCY_ISSUES_H
