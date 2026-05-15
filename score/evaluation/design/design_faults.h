

#ifndef SCORE_EVALUATION_DESIGN_FAULTS_H
#define SCORE_EVALUATION_DESIGN_FAULTS_H

#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

struct Order
{
    int         id{0};
    double      total{0.0};
    bool        is_paid{false};
    std::string customer_name;
};

class OrderProcessor
{
public:
    void Apply10PercentDiscount(Order& o) { o.total *= 0.90; }
    void MarkAsPaid(Order& o)             { o.is_paid = true; }
    bool IsFreeShippingEligible(const Order& o) { return o.total > 100.0; }
};

class PipelineStage
{
public:
    PipelineStage() : initialised_(false) {}

    void Init()
    {
        initialised_ = true;
    }

    void Process(const std::string& data)
    {
        if (!initialised_)
        {
            throw std::runtime_error("PipelineStage: Init() not called");
        }
        (void)data;
    }

private:
    bool initialised_;
};

struct Canvas
{
    int width{0};
    int height{0};
    std::vector<int> pixels;
};

class Renderer
{
public:
    void Fill(Canvas& c, int colour)
    {
        c.pixels.assign(static_cast<std::size_t>(c.width * c.height), colour);
    }

    void Resize(Canvas& c, int w, int h)
    {
        c.width  = w;
        c.height = h;
        c.pixels.resize(static_cast<std::size_t>(w * h), 0);
    }
};

class NetworkClient
{
    static constexpr int kTimeoutMs = 5000;
public:
    void Connect() { /* uses kTimeoutMs */ }
};

class FileWatcher
{
    static constexpr int kTimeoutMs = 5000;
public:
    void Watch() { /* uses kTimeoutMs */ }
};

class HealthCheck
{
    static constexpr int kTimeoutMs = 5000;
public:
    void Ping() { /* uses kTimeoutMs */ }
};

class List
{
public:
    virtual ~List()                     = default;
    virtual void Add(int value)          = 0;
    virtual void Remove(int value)       = 0;
    virtual int  Get(int index) const    = 0;
    virtual int  Size() const            = 0;
};

class ReadOnlyList : public List
{
public:
    void Add(int /*value*/) override
    {
        throw std::runtime_error("ReadOnlyList: Add not supported");
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

class AccountLedger;

class AuditEngine
{
public:
    void Audit(AccountLedger& ledger);
};

class AccountLedger
{
    friend class AuditEngine;
private:
    double balance_{0.0};
    std::vector<double> transactions_;
};

inline void AuditEngine::Audit(AccountLedger& ledger)
{
    if (ledger.balance_ < 0) { /* flag */ }
    for (auto t : ledger.transactions_) { (void)t; }
}

inline void ConfigureConnection(
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

inline double CalculateScore(int raw)
{
    if (raw > 255)          return 0.0;
    double norm = raw / 128.0;
    if (norm > 1.75)        return 100.0;
    return norm * 57.14;
}

inline void ExportData(
    const std::string& filename,
    bool               compress,
    bool               encrypt,
    bool               overwrite)
{
    (void)filename; (void)compress; (void)encrypt; (void)overwrite;
}

}
}

#endif  // SCORE_EVALUATION_DESIGN_FAULTS_H
