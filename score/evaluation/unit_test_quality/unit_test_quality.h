// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

/// @file unit_test_quality.h
/// @brief Production source code used as the subject under test for §11.
///
/// The class designs here are intentionally crafted to expose missing or
/// weak test coverage that a code-review tool should flag in the
/// accompanying test file (unit_test_quality_test.cpp).

#ifndef SCORE_EVALUATION_UNIT_TEST_QUALITY_H
#define SCORE_EVALUATION_UNIT_TEST_QUALITY_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// TokenBucket – simple rate limiter used by [UT-01..UT-04].
// ---------------------------------------------------------------------------
class TokenBucket
{
public:
    /// @param capacity   Maximum number of tokens the bucket can hold.
    /// @param refill_rate Tokens added per Refill() call (must be > 0).
    explicit TokenBucket(int capacity, int refill_rate)
        : capacity_(capacity), tokens_(capacity), refill_rate_(refill_rate)
    {
        if (capacity <= 0 || refill_rate <= 0)
        {
            throw std::invalid_argument("capacity and refill_rate must be positive");
        }
    }

    /// Try to consume @p n tokens. Returns true on success, false if not enough tokens.
    bool TryConsume(int n)
    {
        if (n <= 0) return true;
        if (tokens_ >= n)
        {
            tokens_ -= n;
            return true;
        }
        return false;
    }

    /// Refill the bucket; tokens are capped at capacity.
    void Refill()
    {
        tokens_ = std::min(tokens_ + refill_rate_, capacity_);
    }

    int AvailableTokens() const noexcept { return tokens_; }
    int Capacity()        const noexcept { return capacity_; }

private:
    int capacity_;
    int tokens_;
    int refill_rate_;
};

// ---------------------------------------------------------------------------
// StringProcessor – used by [UT-05..UT-07].
// ---------------------------------------------------------------------------
class StringProcessor
{
public:
    /// Trims leading and trailing ASCII whitespace.
    static std::string Trim(const std::string& s);

    /// Splits @p s on the single-character @p delimiter.
    static std::vector<std::string> Split(const std::string& s, char delimiter);

    /// Returns @p s with every ASCII letter converted to upper-case.
    static std::string ToUpperCase(const std::string& s);
};

inline std::string StringProcessor::Trim(const std::string& s)
{
    const std::string ws = " \t\r\n";
    const auto first = s.find_first_not_of(ws);
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(ws);
    return s.substr(first, last - first + 1);
}

inline std::vector<std::string> StringProcessor::Split(const std::string& s, char delimiter)
{
    std::vector<std::string> result;
    std::string token;
    for (char c : s)
    {
        if (c == delimiter)
        {
            result.push_back(token);
            token.clear();
        }
        else
        {
            token += c;
        }
    }
    result.push_back(token);  // last token (may be empty)
    return result;
}

inline std::string StringProcessor::ToUpperCase(const std::string& s)
{
    std::string out = s;
    for (char& c : out)
    {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
    }
    return out;
}

// ---------------------------------------------------------------------------
// CircularBuffer<T> – fixed-size FIFO used by [UT-08..UT-10].
// ---------------------------------------------------------------------------
template<typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(std::size_t capacity)
        : buf_(capacity), head_(0), tail_(0), size_(0), capacity_(capacity)
    {
        if (capacity == 0) throw std::invalid_argument("capacity must be > 0");
    }

    /// Push an element; overwrites oldest element when full.
    void Push(const T& value)
    {
        buf_[tail_] = value;
        tail_ = (tail_ + 1) % capacity_;
        if (size_ < capacity_)
        {
            ++size_;
        }
        else
        {
            head_ = (head_ + 1) % capacity_;  // overwrite: advance head
        }
    }

    /// Pop the oldest element; throws std::underflow_error when empty.
    T Pop()
    {
        if (size_ == 0) throw std::underflow_error("CircularBuffer is empty");
        T value = buf_[head_];
        head_   = (head_ + 1) % capacity_;
        --size_;
        return value;
    }

    bool        IsEmpty()   const noexcept { return size_ == 0; }
    bool        IsFull()    const noexcept { return size_ == capacity_; }
    std::size_t Size()      const noexcept { return size_; }
    std::size_t Capacity()  const noexcept { return capacity_; }

private:
    std::vector<T>  buf_;
    std::size_t     head_;
    std::size_t     tail_;
    std::size_t     size_;
    std::size_t     capacity_;
};

// ---------------------------------------------------------------------------
// PaymentProcessor – used by [UT-11..UT-13] (dependency injection / mocking).
// ---------------------------------------------------------------------------
class IPaymentGateway
{
public:
    virtual ~IPaymentGateway() = default;
    virtual bool Charge(const std::string& account, double amount) = 0;
    virtual bool Refund(const std::string& account, double amount) = 0;
};

class PaymentProcessor
{
public:
    explicit PaymentProcessor(IPaymentGateway& gateway) : gateway_(gateway) {}

    /// Returns true and records the transaction if the charge succeeds.
    bool ProcessPayment(const std::string& account, double amount)
    {
        if (account.empty() || amount <= 0.0) return false;
        if (gateway_.Charge(account, amount))
        {
            transaction_log_.push_back("CHARGE " + account);
            return true;
        }
        return false;
    }

    /// Refunds the last successful charge; no-op if log is empty.
    bool RefundLast(const std::string& account)
    {
        if (transaction_log_.empty()) return false;
        const double refund_amount = 1.0;  // simplified: always refunds 1.0
        return gateway_.Refund(account, refund_amount);
    }

    const std::vector<std::string>& TransactionLog() const noexcept
    {
        return transaction_log_;
    }

private:
    IPaymentGateway&         gateway_;
    std::vector<std::string> transaction_log_;
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_UNIT_TEST_QUALITY_H
