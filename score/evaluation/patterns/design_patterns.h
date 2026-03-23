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

/// @file design_patterns.h
/// @brief Incorrect or missing design-pattern usage for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §5 for expected review comments):
///   [DP-01] Singleton implemented without thread-safety (double-checked locking broken).
///   [DP-02] Observer pattern leaks subscribers (raw pointers, no unsubscribe).
///   [DP-03] Factory that returns raw owning pointer instead of unique_ptr.
///   [DP-04] Template Method pattern broken by calling virtual from constructor.
///   [DP-05] Strategy pattern stored by raw pointer (lifetime not managed).
///   [DP-06] CRTP misuse – static_cast to derived in base non-template method.

#ifndef SCORE_EVALUATION_DESIGN_PATTERNS_H
#define SCORE_EVALUATION_DESIGN_PATTERNS_H

#include <string>
#include <vector>
#include <stdexcept>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [DP-01] Non-thread-safe Singleton (classic broken double-checked locking).
// ---------------------------------------------------------------------------
class BrokenSingleton
{
public:
    // [DP-01] Not thread-safe: two threads can both see instance_ == nullptr
    //         and construct two objects. Use Meyers singleton (local static)
    //         or std::call_once instead.
    static BrokenSingleton* GetInstance()
    {
        if (instance_ == nullptr)          // first check – no lock
        {
            // <- context switch here allows second thread in
            if (instance_ == nullptr)      // second check – still no lock
            {
                instance_ = new BrokenSingleton();
            }
        }
        return instance_;
    }

    void DoWork() {}

private:
    BrokenSingleton() = default;
    static BrokenSingleton* instance_;     // raw owning pointer; never deleted
};

BrokenSingleton* BrokenSingleton::instance_ = nullptr;

// ---------------------------------------------------------------------------
// [DP-02] Observer with raw subscriber pointers – no lifetime management.
// ---------------------------------------------------------------------------
class IEventListener
{
public:
    virtual ~IEventListener() = default;
    virtual void OnEvent(const std::string& event) = 0;
};

class EventPublisher
{
public:
    // [DP-02] Raw pointer stored; if listener is destroyed before publisher,
    //         invoking it is undefined behaviour. Use weak_ptr or a token/handle.
    void Subscribe(IEventListener* listener)
    {
        listeners_.push_back(listener);     // no duplicate check, no ownership
    }

    void Publish(const std::string& event)
    {
        for (auto* l : listeners_)
        {
            l->OnEvent(event);              // potential use-after-free
        }
    }

private:
    std::vector<IEventListener*> listeners_;   // raw, non-owning, no Unsubscribe
};

// ---------------------------------------------------------------------------
// [DP-03] Factory returning raw owning pointer.
// ---------------------------------------------------------------------------
class Widget
{
public:
    virtual ~Widget() = default;
    virtual void Draw() const = 0;
};

class Button : public Widget
{
public:
    void Draw() const override {}
};

class WidgetFactory
{
public:
    // [DP-03] Returns raw owning pointer – caller must delete; leaks on exception.
    //         Should return std::unique_ptr<Widget>.
    Widget* Create(const std::string& type)
    {
        if (type == "button") return new Button();
        return nullptr;                     // returning nullptr without documentation
    }
};

// ---------------------------------------------------------------------------
// [DP-04] Calling virtual function from constructor (Template Method anti-pattern).
// ---------------------------------------------------------------------------
class AbstractProcessor
{
public:
    AbstractProcessor()
    {
        Initialise();   // [DP-04] virtual call in ctor: always calls AbstractProcessor::Initialise
                        //         even if a derived class overrides it. Derived vtable not set yet.
    }

    virtual ~AbstractProcessor() = default;

    virtual void Initialise()
    {
        // base initialisation only – derived override never reached from ctor
    }

    virtual void Run() = 0;
};

class ConcreteProcessor : public AbstractProcessor
{
public:
    void Initialise() override
    {
        // Never called from AbstractProcessor constructor!
        ready_ = true;
    }

    void Run() override
    {
        if (!ready_) throw std::runtime_error("not initialised");  // always throws
    }

private:
    bool ready_{false};
};

// ---------------------------------------------------------------------------
// [DP-05] Strategy stored as raw non-owning pointer – lifetime not managed.
// ---------------------------------------------------------------------------
class ISortStrategy
{
public:
    virtual ~ISortStrategy() = default;
    virtual void Sort(std::vector<int>& data) = 0;
};

class BubbleSort : public ISortStrategy
{
public:
    void Sort(std::vector<int>& data) override
    {
        // naive bubble sort
        for (std::size_t i = 0; i < data.size(); ++i)
            for (std::size_t j = 0; j + 1 < data.size() - i; ++j)
                if (data[j] > data[j + 1]) std::swap(data[j], data[j + 1]);
    }
};

class Sorter
{
public:
    // [DP-05] Raw pointer – no ownership semantics; strategy may be destroyed
    //         before Sorter. Should use shared_ptr or accept by reference with
    //         documented lifetime requirements.
    explicit Sorter(ISortStrategy* strategy) : strategy_(strategy) {}

    void Sort(std::vector<int>& data)
    {
        if (strategy_) strategy_->Sort(data);
    }

private:
    ISortStrategy* strategy_;   // non-owning raw pointer, lifetime unmanaged
};

// ---------------------------------------------------------------------------
// [DP-06] CRTP base casting to derived in a non-template base method.
// ---------------------------------------------------------------------------
class NotRealCRTP
{
public:
    // [DP-06] This is NOT CRTP: NotRealCRTP is not a template, and the cast
    //         is unconditional and unsafe.  True CRTP requires
    //         template<typename Derived> class Base { ... static_cast<Derived*>(this) }.
    void Execute()
    {
        // Unsafe downcast without any type check – UB if actual type is wrong.
        static_cast<NotRealCRTP*>(this)->DoImpl();
    }

    virtual void DoImpl() {}
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_DESIGN_PATTERNS_H
