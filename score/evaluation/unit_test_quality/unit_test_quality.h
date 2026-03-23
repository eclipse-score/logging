

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

class TokenBucket
{
public:
    explicit TokenBucket(int capacity, int refill_rate)
        : capacity_(capacity), tokens_(capacity), refill_rate_(refill_rate)
    {
        if (capacity <= 0 || refill_rate <= 0)
        {
            throw std::invalid_argument("capacity and refill_rate must be positive");
        }
    }

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

class StringProcessor
{
public:
    static std::string Trim(const std::string& s);

    static std::vector<std::string> Split(const std::string& s, char delimiter);

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
    result.push_back(token);
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

template<typename T>
class CircularBuffer
{
public:
    explicit CircularBuffer(std::size_t capacity)
        : buf_(capacity), head_(0), tail_(0), size_(0), capacity_(capacity)
    {
        if (capacity == 0) throw std::invalid_argument("capacity must be > 0");
    }

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
            head_ = (head_ + 1) % capacity_;
        }
    }

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

    bool RefundLast(const std::string& account)
    {
        if (transaction_log_.empty()) return false;
        const double refund_amount = 1.0;
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

}
}

#endif  // SCORE_EVALUATION_UNIT_TEST_QUALITY_H
