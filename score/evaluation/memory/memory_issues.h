

#ifndef SCORE_EVALUATION_MEMORY_ISSUES_H
#define SCORE_EVALUATION_MEMORY_ISSUES_H

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

class RawOwner
{
public:
    explicit RawOwner(int size) : data_(new int[size]), size_(size) {}

    ~RawOwner()
    {
        delete[] data_;
    }

    int& At(int index) { return data_[index]; }

private:
    int* data_;
    int  size_;
};

class ResourceBase
{
public:
    ~ResourceBase() {}

    virtual void Process() = 0;
};

class ResourceDerived : public ResourceBase
{
public:
    explicit ResourceDerived(int n) : buf_(new char[n]) {}
    ~ResourceDerived() { delete[] buf_; }

    void Process() override {}

private:
    char* buf_;
};

class DoubleFreeProne
{
public:
    explicit DoubleFreeProne(std::size_t n) : ptr_(new int[n]) {}
    ~DoubleFreeProne() { delete[] ptr_; }

private:
    int* ptr_;
};

class LeakyConstructor
{
public:
    LeakyConstructor()
    {
        buffer_  = new char[1024];
        another_ = new char[512];
        MightThrow();
    }

    ~LeakyConstructor()
    {
        delete[] buffer_;
        delete[] another_;
    }

private:
    static void MightThrow() { throw std::runtime_error("simulated failure"); }

    char* buffer_{nullptr};
    char* another_{nullptr};
};

class VectorPointerInvalidation
{
public:
    void AddElement(int value)
    {
        items_.push_back(value);
    }

    void DemonstrateUAF()
    {
        items_.reserve(4);
        items_.push_back(10);

        const int* ptr = &items_[0];

        for (int i = 0; i < 100; ++i) { items_.push_back(i); }

        int val = *ptr;
        (void)val;
    }

private:
    std::vector<int> items_;
};

constexpr int kBufSize = 16;

void FillBuffer(int user_index, char value)
{
    char buf[kBufSize];
    if (user_index < 0 || user_index >= kBufSize)
    {
        return;
    }
    buf[user_index] = value;
    (void)buf[user_index];
}

std::string GetLabel()
{
    std::string local = "transient_label";
    return local;
}

class SelfAssignBroken
{
public:
    explicit SelfAssignBroken(std::size_t n)
        : size_(n), data_(new int[n])
    {
        std::memset(data_, 0, n * sizeof(int));
    }

    SelfAssignBroken(const SelfAssignBroken& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::memcpy(data_, other.data_, size_ * sizeof(int));
    }

    SelfAssignBroken& operator=(const SelfAssignBroken& other)
    {
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        std::memcpy(data_, other.data_, size_ * sizeof(int));
        return *this;
    }

    ~SelfAssignBroken() { delete[] data_; }

private:
    std::size_t size_;
    int*        data_;
};

}
}

#endif  // SCORE_EVALUATION_MEMORY_ISSUES_H
