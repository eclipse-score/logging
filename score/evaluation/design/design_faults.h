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

/// @file design_faults.h
/// @brief Software-design-level faults for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §8 for expected review comments):
///   [DF-01] Anemic domain model – all business logic outside the class that owns the data.
///   [DF-02] Temporal coupling – callers must invoke Init() before Use() with no enforcement.
///   [DF-03] Feature envy – method operates almost entirely on another class's fields.
///   [DF-04] Shotgun surgery – a single concept (timeout) is duplicated across several classes.
///   [DF-05] Refused bequest – subclass ignores/breaks inherited public interface.
///   [DF-06] Inappropriate intimacy – two classes directly access each other's private via friend.
///   [DF-07] Long parameter list – function with 8 positional arguments.
///   [DF-08] Magic numbers scattered throughout without named constants.
///   [DF-09] Boolean trap – function with multiple bool parameters hiding intent.
///   [DF-10] Circular include dependency risk – explicit comment calling it out.

#ifndef SCORE_EVALUATION_DESIGN_FAULTS_H
#define SCORE_EVALUATION_DESIGN_FAULTS_H

#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

// ---------------------------------------------------------------------------
// [DF-01] Anemic domain model – Order holds data, OrderProcessor holds all logic.
// ---------------------------------------------------------------------------
struct Order          // [DF-01] pure data bag; no behaviour; violates domain-model principle
{
    int         id{0};
    double      total{0.0};
    bool        is_paid{false};
    std::string customer_name;
};

class OrderProcessor  // [DF-01] all logic here; tight coupling to Order internals
{
public:
    void Apply10PercentDiscount(Order& o) { o.total *= 0.90; }
    void MarkAsPaid(Order& o)             { o.is_paid = true; }
    bool IsFreeShippingEligible(const Order& o) { return o.total > 100.0; }
};

// ---------------------------------------------------------------------------
// [DF-02] Temporal coupling – Init() must be called before Process().
// ---------------------------------------------------------------------------
class PipelineStage
{
public:
    PipelineStage() : initialised_(false) {}

    void Init()
    {
        // [DF-02] Nothing stops a caller from calling Process() without Init().
        initialised_ = true;
    }

    void Process(const std::string& data)
    {
        if (!initialised_)
        {
            throw std::runtime_error("PipelineStage: Init() not called");
            // Enforcement is runtime-only; should be enforced at construction time.
        }
        (void)data;
    }

private:
    bool initialised_;
};

// ---------------------------------------------------------------------------
// [DF-03] Feature envy – Renderer accesses Canvas fields directly.
// ---------------------------------------------------------------------------
struct Canvas         // data holder
{
    int width{0};
    int height{0};
    std::vector<int> pixels;
};

class Renderer        // [DF-03] operates almost entirely on Canvas's internals
{
public:
    void Fill(Canvas& c, int colour)
    {
        c.pixels.assign(static_cast<std::size_t>(c.width * c.height), colour);
        // All work done with Canvas data; Fill() should be a method of Canvas.
    }

    void Resize(Canvas& c, int w, int h)
    {
        c.width  = w;
        c.height = h;
        c.pixels.resize(static_cast<std::size_t>(w * h), 0);
        // Again: should be Canvas::Resize().
    }
};

// ---------------------------------------------------------------------------
// [DF-04] Shotgun surgery – timeout value duplicated across unrelated classes.
// ---------------------------------------------------------------------------
class NetworkClient
{
    static constexpr int kTimeoutMs = 5000;   // [DF-04] duplicated magic value
public:
    void Connect() { /* uses kTimeoutMs */ }
};

class FileWatcher
{
    static constexpr int kTimeoutMs = 5000;   // [DF-04] same constant, no shared source
public:
    void Watch() { /* uses kTimeoutMs */ }
};

class HealthCheck
{
    static constexpr int kTimeoutMs = 5000;   // [DF-04] three places to change if requirement changes
public:
    void Ping() { /* uses kTimeoutMs */ }
};

// ---------------------------------------------------------------------------
// [DF-05] Refused bequest – ReadOnlyList claims to be a List but rejects mutations.
// ---------------------------------------------------------------------------
class List
{
public:
    virtual ~List()                     = default;
    virtual void Add(int value)          = 0;
    virtual void Remove(int value)       = 0;
    virtual int  Get(int index) const    = 0;
    virtual int  Size() const            = 0;
};

class ReadOnlyList : public List       // [DF-05] refuses inherited mutation contract
{
public:
    void Add(int /*value*/) override
    {
        throw std::runtime_error("ReadOnlyList: Add not supported");  // breaks LSP too
    }
    void Remove(int /*value*/) override
    {
        throw std::runtime_error("ReadOnlyList: Remove not supported");
    }
    int Get(int /*index*/) const override { return 0; }
    int Size() const override { return static_cast<int>(data_.size()); }

private:
    std::vector<int> data_;
};

// ---------------------------------------------------------------------------
// [DF-06] Inappropriate intimacy via friend class.
// ---------------------------------------------------------------------------
class AccountLedger;

class AuditEngine
{
public:
    void Audit(AccountLedger& ledger);  // accesses private members via friend
};

class AccountLedger
{
    friend class AuditEngine;   // [DF-06] tight coupling; expose only what's needed via accessors
private:
    double balance_{0.0};
    std::vector<double> transactions_;
};

inline void AuditEngine::Audit(AccountLedger& ledger)
{
    // Direct access to private members breaks encapsulation.
    if (ledger.balance_ < 0) { /* flag */ }
    for (auto t : ledger.transactions_) { (void)t; }
}

// ---------------------------------------------------------------------------
// [DF-07] Long parameter list – 8 positional arguments.
// ---------------------------------------------------------------------------
inline void ConfigureConnection(           // [DF-07]: wrap in a ConnectionConfig struct
    const std::string& host,
    int                port,
    int                timeout_ms,
    int                retry_count,
    bool               use_tls,
    bool               verify_cert,
    const std::string& username,
    const std::string& password)
{
    (void)host; (void)port; (void)timeout_ms; (void)retry_count;
    (void)use_tls; (void)verify_cert; (void)username; (void)password;
}

// ---------------------------------------------------------------------------
// [DF-08] Magic numbers throughout logic without named constants.
// ---------------------------------------------------------------------------
inline double CalculateScore(int raw)
{
    if (raw > 255)          return 0.0;    // [DF-08]: what is 255?
    double norm = raw / 128.0;             // [DF-08]: what is 128?
    if (norm > 1.75)        return 100.0;  // [DF-08]: what is 1.75?
    return norm * 57.14;                   // [DF-08]: what is 57.14?
}

// ---------------------------------------------------------------------------
// [DF-09] Boolean trap – multiple bool parameters hide call-site intent.
// ---------------------------------------------------------------------------
inline void ExportData(                    // [DF-09]: use enum class or named-parameter idiom
    const std::string& filename,
    bool               compress,           // caller: ExportData("out", true, false, true) – unreadable
    bool               encrypt,
    bool               overwrite)
{
    (void)filename; (void)compress; (void)encrypt; (void)overwrite;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_DESIGN_FAULTS_H
