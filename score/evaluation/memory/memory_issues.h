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

/// @file memory_issues.h
/// @brief Intentional memory-management issues for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §2 for expected review comments):
///   [MI-01] Raw owning pointer – manual new/delete, no RAII.
///   [MI-02] Missing virtual destructor on polymorphic base → UB on delete.
///   [MI-03] Double-free risk in destructor after copy construction.
///   [MI-04] Memory leak on exception path inside constructor.
///   [MI-05] Use-after-free: pointer stored after vector reallocation.
///   [MI-08] Self-assignment not handled in copy-assignment operator.

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

// ---------------------------------------------------------------------------
// [MI-01] Raw owning pointer – should use std::unique_ptr.
// ---------------------------------------------------------------------------
class RawOwner
{
public:
    explicit RawOwner(int size) : data_(new int[size]), size_(size) {}

    ~RawOwner()
    {
        delete[] data_;   // will leak if constructor throws after allocation
    }

    int& At(int index) { return data_[index]; }  // no bounds check

private:
    int* data_;           // owning raw pointer
    int  size_;
};

// ---------------------------------------------------------------------------
// [MI-02] Missing virtual destructor – UB when Base* points to Derived.
// ---------------------------------------------------------------------------
class ResourceBase
{
public:
    // No virtual destructor: deleting via base pointer causes undefined behaviour.
    ~ResourceBase() {}    // [MI-02]: should be virtual ~ResourceBase()

    virtual void Process() = 0;
};

class ResourceDerived : public ResourceBase
{
public:
    explicit ResourceDerived(int n) : buf_(new char[n]) {}
    ~ResourceDerived() { delete[] buf_; }   // never called via ResourceBase*

    void Process() override {}

private:
    char* buf_;
};

// ---------------------------------------------------------------------------
// [MI-03] Double-free via implicit copy – no copy constructor or Rule-of-Five.
// ---------------------------------------------------------------------------
class DoubleFreeProne
{
public:
    explicit DoubleFreeProne(std::size_t n) : ptr_(new int[n]) {}
    ~DoubleFreeProne() { delete[] ptr_; }

    // No user-defined copy constructor: compiler-generated does a shallow copy.
    // Copying two instances then destructing both → double free of ptr_.

private:
    int* ptr_;  // raw owning pointer without Rule-of-Five
};

// ---------------------------------------------------------------------------
// [MI-04] Memory leak on exception path.
// ---------------------------------------------------------------------------
class LeakyConstructor
{
public:
    LeakyConstructor()
    {
        buffer_  = new char[1024];
        another_ = new char[512];  // if this throws, buffer_ leaks
        // Should use unique_ptr or initialise via constructor-body members.
        MightThrow();               // if this throws, both allocations leak
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

// ---------------------------------------------------------------------------
// [MI-05] Use-after-free / dangling pointer after vector resize.
// ---------------------------------------------------------------------------
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

        const int* ptr = &items_[0];  // pointer into vector storage

        // Causes reallocation → ptr is now dangling
        for (int i = 0; i < 100; ++i) { items_.push_back(i); }

        // [MI-05] Reading through invalidated pointer – undefined behaviour.
        int val = *ptr;
        (void)val;
    }

private:
    std::vector<int> items_;
};

// ---------------------------------------------------------------------------
// Safe buffer write helper (compiler-detectable overflow condition removed).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Safe value return helper (compiler-detectable dangling-reference condition removed).
// ---------------------------------------------------------------------------
std::string GetLabel()
{
    std::string local = "transient_label";
    return local;
}

// ---------------------------------------------------------------------------
// [MI-08] Self-assignment not handled in copy-assignment operator.
// ---------------------------------------------------------------------------
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

    // [MI-08] Self-assignment deletes data_ before copying from it.
    SelfAssignBroken& operator=(const SelfAssignBroken& other)
    {
        delete[] data_;                             // danger: if other == *this, data_ gone
        size_ = other.size_;
        data_ = new int[size_];
        std::memcpy(data_, other.data_, size_ * sizeof(int));  // reads freed memory on self-assign
        return *this;
    }

    ~SelfAssignBroken() { delete[] data_; }

private:
    std::size_t size_;
    int*        data_;
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_MEMORY_ISSUES_H
