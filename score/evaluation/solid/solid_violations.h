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

/// @file solid_violations.h
/// @brief Intentional SOLID principle violations for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §1 for expected review comments):
///   [SV-01] SRP – God class: LogManager handles configuration, serialisation, routing and I/O.
///   [SV-02] OCP – switch/if-else type dispatch prevents extension without modification.
///   [SV-03] LSP – derived class Derived::compute() silently returns a different result for x==0.
///   [SV-04] ISP – fat interface IComponent forces implementors to stub unused methods.
///   [SV-05] DIP – HighLevelProcessor directly constructs its low-level dependency (concrete class).

#ifndef SCORE_EVALUATION_SOLID_VIOLATIONS_H
#define SCORE_EVALUATION_SOLID_VIOLATIONS_H

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [SV-01] SRP violation – LogManager does far too many things in one class.
// ---------------------------------------------------------------------------
class LogManager
{
public:
    // Configuration concern
    void LoadConfig(const std::string& path)
    {
        config_path_ = path;
        // ... parse YAML/JSON inline, mixed with routing logic
    }

    // Serialisation concern
    std::string Serialize(const std::string& message)
    {
        return "[LOG] " + message;  // format chosen here, not in a dedicated formatter
    }

    // Routing concern
    void RouteMessage(const std::string& message)
    {
        std::string serialized = Serialize(message);
        if (output_type_ == "file")
        {
            WriteToFile(serialized);
        }
        else if (output_type_ == "console")
        {
            std::cout << serialized << "\n";
        }
        else if (output_type_ == "network")
        {
            // TODO: implement network routing
        }
    }

    // I/O concern
    void WriteToFile(const std::string& data)
    {
        std::ofstream file(config_path_);
        file << data;
    }

    // Statistics concern mixed in too
    int GetMessageCount() const { return message_count_; }
    void IncrementCount() { ++message_count_; }

private:
    std::string config_path_;
    std::string output_type_{"file"};
    int         message_count_{0};
};

// ---------------------------------------------------------------------------
// [SV-02] OCP violation – adding a new shape requires modifying AreaCalculator.
// ---------------------------------------------------------------------------
struct Shape
{
    std::string type;
    double      param1{0.0};
    double      param2{0.0};
};

class AreaCalculator
{
public:
    // Every new shape type requires touching this function.
    double Calculate(const Shape& s)
    {
        if (s.type == "circle")
        {
            return 3.14159 * s.param1 * s.param1;
        }
        else if (s.type == "rectangle")
        {
            return s.param1 * s.param2;
        }
        else if (s.type == "triangle")
        {
            return 0.5 * s.param1 * s.param2;
        }
        // Adding "ellipse" forces modification of this closed function.
        return 0.0;
    }
};

// ---------------------------------------------------------------------------
// [SV-03] LSP violation – Derived::Compute changes the semantic contract.
// ---------------------------------------------------------------------------
class Base
{
public:
    virtual ~Base()             = default;
    /// Contract: returns x * 2 for all non-negative x.
    virtual int Compute(int x) const { return x * 2; }
};

class Derived : public Base
{
public:
    /// Breaks contract: returns 0 when x == 0 instead of 0 (coincidentally fine),
    /// but also silently returns -1 for x < 0 where Base would return a negative even number.
    /// More critically: throws when x > 1000 – Base never throws (violated Liskov).
    int Compute(int x) const override
    {
        if (x > 1000)
        {
            throw std::runtime_error("value too large");  // Base does not throw
        }
        if (x < 0)
        {
            return -1;  // Different semantics from Base
        }
        return x * 2;
    }
};

// ---------------------------------------------------------------------------
// [SV-04] ISP violation – single fat interface forces implementors to stub.
// ---------------------------------------------------------------------------
class IComponent
{
public:
    virtual ~IComponent() = default;

    virtual void Start()   = 0;
    virtual void Stop()    = 0;
    virtual void Pause()   = 0;   // not all components support pause
    virtual void Resume()  = 0;   // not all components support resume
    virtual void Reset()   = 0;   // not all components support reset
    virtual int  Status()  = 0;
    virtual void Configure(const std::string& cfg) = 0;
    virtual std::string Serialize() const          = 0;  // mixing I/O into a lifecycle interface
    virtual void        Deserialize(const std::string&) = 0;
};

// ConcreteComponent is forced to implement irrelevant methods.
class ConcreteComponent : public IComponent
{
public:
    void Start()   override { /* real logic */ }
    void Stop()    override { /* real logic */ }
    void Pause()   override { /* not supported – stub */ }
    void Resume()  override { /* not supported – stub */ }
    void Reset()   override { /* not supported – stub */ }
    int  Status()  override { return 0; }
    void Configure(const std::string&) override { /* real logic */ }
    std::string Serialize() const override { return {}; }  // forced stub
    void        Deserialize(const std::string&) override { /* forced stub */ }
};

// ---------------------------------------------------------------------------
// [SV-05] DIP violation – high-level module depends on a concrete class.
// ---------------------------------------------------------------------------
class FileSink           // low-level, concrete
{
public:
    void Write(const std::string& msg) { std::cout << "FILE: " << msg << "\n"; }
};

class HighLevelProcessor // high-level, but coupled to FileSink directly
{
public:
    HighLevelProcessor()
        : sink_()          // constructs concrete dependency internally – no injection
    {}

    void Process(const std::string& data)
    {
        // Can never be tested with a mock, or swapped for a network sink
        sink_.Write(data);
    }

private:
    FileSink sink_;        // depends on concrete class, not on an abstraction
};

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_SOLID_VIOLATIONS_H
