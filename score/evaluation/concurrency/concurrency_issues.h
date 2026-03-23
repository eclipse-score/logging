// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/parameters/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

/// @file concurrency_issues.h
/// @brief Concurrency and thread-safety issues for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §9 for expected review comments):
///   [CI-01] Data race: shared counter incremented without synchronisation.
///   [CI-02] Mutex locked but not protected by RAII lock guard – leak on exception.
///   [CI-03] Double-checked locking without std::atomic – broken on modern hardware.
///   [CI-04] Deadlock: two mutexes acquired in inconsistent order across two functions.
///   [CI-05] Condition variable spurious wakeup not handled (while replaced by if).
///   [CI-06] Detached thread writing to stack variable of the spawning scope.
///   [CI-07] std::atomic<int> used for a compound check-then-act – still a TOCTOU race.

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

// ---------------------------------------------------------------------------
// [CI-01] Data race on shared counter – no synchronisation.
// ---------------------------------------------------------------------------
class UnsafeCounter
{
public:
    void Increment() { ++count_; }   // [CI-01] non-atomic, non-locked; data race
    int  Get() const { return count_; }

private:
    int count_{0};   // should be std::atomic<int> or guarded by a mutex
};

// ---------------------------------------------------------------------------
// [CI-02] Manual mutex lock/unlock without RAII – leak on exception.
// ---------------------------------------------------------------------------
class ManualLockUser
{
public:
    void Write(const std::string& data)
    {
        mtx_.lock();          // [CI-02] if anything between lock and unlock throws,
        ProcessData(data);    //         the mutex is never unlocked → deadlock.
        mtx_.unlock();        //         Use std::lock_guard<std::mutex> lk(mtx_);
    }

private:
    static void ProcessData(const std::string& /*data*/)
    {
        // may throw
    }

    std::mutex mtx_;
};

// ---------------------------------------------------------------------------
// [CI-03] Double-checked locking without proper atomics / memory fences.
// ---------------------------------------------------------------------------
class BrokenLazyInit
{
public:
    // [CI-03] initialised_ is a plain bool – reads/writes are not atomic.
    //         A second thread may observe initialised_ == true before the
    //         constructor of resource_ has completed (memory reorder).
    //         Use std::atomic<bool> with acquire/release semantics or call_once.
    void EnsureInitialised()
    {
        if (!initialised_)               // first check, no lock
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (!initialised_)           // second check, under lock
            {
                resource_ = 42;
                initialised_ = true;     // reorder: may be visible before resource_ = 42
            }
        }
    }

    int GetResource() const { return resource_; }

private:
    bool       initialised_{false};  // [CI-03] should be std::atomic<bool>
    int        resource_{0};
    std::mutex mtx_;
};

// ---------------------------------------------------------------------------
// [CI-04] Deadlock: mutexes acquired in opposite order in two different paths.
// ---------------------------------------------------------------------------
class DeadlockProne
{
public:
    // Thread A calls TransferAB, Thread B calls TransferBA simultaneously → deadlock.
    void TransferAB(int amount)
    {
        std::lock_guard<std::mutex> la(mtx_a_);   // locks A first
        std::lock_guard<std::mutex> lb(mtx_b_);   // then B
        balance_a_ -= amount;
        balance_b_ += amount;
    }

    void TransferBA(int amount)
    {
        std::lock_guard<std::mutex> lb(mtx_b_);   // locks B first  [CI-04]
        std::lock_guard<std::mutex> la(mtx_a_);   // then A – opposite order
        balance_b_ -= amount;
        balance_a_ += amount;
    }

private:
    std::mutex mtx_a_;
    std::mutex mtx_b_;
    int balance_a_{1000};
    int balance_b_{1000};
};

// ---------------------------------------------------------------------------
// [CI-05] Condition variable spurious wakeup handled with 'if' instead of 'while'.
// ---------------------------------------------------------------------------
class SpuriousWakeupBug
{
public:
    void Wait()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        // [CI-05] Using if: spurious wakeup proceeds even when ready_ is false.
        //         Must use while (!ready_) or the predicate overload of wait().
        if (!ready_) cv_.wait(lk);
        // Proceed – but ready_ might still be false after a spurious wakeup.
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

// ---------------------------------------------------------------------------
// [CI-06] Detached thread capturing reference to local stack variable.
// ---------------------------------------------------------------------------
inline void StartDetachedThread()
{
    std::string local_config = "config_data";

    // [CI-06] local_config is on the stack of StartDetachedThread().
    //         After the function returns, the thread holds a dangling reference.
    std::thread t([&local_config]()
    {
        // Accessing local_config after StartDetachedThread() returns → UB.
        (void)local_config;
    });
    t.detach();
    // Function returns here; local_config destroyed; thread still running.
}

// ---------------------------------------------------------------------------
// [CI-07] Compound check-then-act on std::atomic is still a TOCTOU race.
// ---------------------------------------------------------------------------
class TocTouAtomic
{
public:
    // [CI-07] Even though count_ is atomic, the if-check and decrement are
    //         two separate operations.  Another thread can decrement between the
    //         check and the decrement, driving count_ below zero.
    void ConsumeIfAvailable()
    {
        if (count_.load() > 0)    // check
        {
            --count_;             // act – not atomic with the check
        }
    }

    void Produce() { ++count_; }

private:
    std::atomic<int> count_{0};
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_CONCURRENCY_ISSUES_H
