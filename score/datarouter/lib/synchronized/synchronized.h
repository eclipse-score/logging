/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#ifndef SCORE_PAS_LOGGING_LIB_SYNCHRONIZED_SYNCHRONIZED_H_
#define SCORE_PAS_LOGGING_LIB_SYNCHRONIZED_SYNCHRONIZED_H_

#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>

namespace score
{
namespace platform
{
namespace datarouter
{

/**
 * @brief Small helper providing serialized access to an object.
 *
 * Synchronized<T, Mutex> wraps a T protected by a Mutex (std::mutex by default).
 *
 * NOTE:
 *   - std::mutex is non-recursive. Locking the same mutex multiple times
 *     from the same thread has undefined behaviour
 *     (see https://en.cppreference.com/w/cpp/thread/mutex/lock).
 *   - Therefore, calling lock() / with_lock() re-entrantly on the same
 *     Synchronized instance is not supported.
 *
 * The intended usage is simple, non-reentrant critical sections:
 *   Synchronized<Foo> f;
 *   f.with_lock([](Foo& x){ x.do_something(); });
 */
template <typename T, typename Mutex = std::mutex>
class Synchronized
{
  public:
    template <typename... Args>
    explicit Synchronized(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        : obj(std::forward<Args>(args)...)
    {
    }

    // Explicitly non-copyable and non-movable (by design)
    Synchronized(const Synchronized&) = delete;
    Synchronized(Synchronized&&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;
    Synchronized& operator=(Synchronized&&) = delete;

    [[nodiscard]] auto lock()
    {
        auto unlocker = [ul = std::unique_lock(mut)](T*) mutable {};
        return std::unique_ptr<T, decltype(unlocker)>(&obj, std::move(unlocker));
    }

    [[nodiscard]] auto lock() const
    {
        auto unlocker = [ul = std::unique_lock(mut)](const T*) mutable {};
        return std::unique_ptr<const T, decltype(unlocker)>(&obj, std::move(unlocker));
    }

    template <typename Func>
    auto with_lock(Func&& f)
    {
        auto guard = lock();
        return std::invoke(std::forward<Func>(f), *guard);
    }

    template <typename Func>
    auto with_lock(Func&& f) const
    {
        auto guard = lock();
        return std::invoke(std::forward<Func>(f), *guard);
    }

  private:
    mutable Mutex mut;
    T obj;
};

}  // namespace datarouter
}  // namespace platform
}  // namespace score

#endif  // SCORE_PAS_LOGGING_LIB_SYNCHRONIZED_SYNCHRONIZED_H_
