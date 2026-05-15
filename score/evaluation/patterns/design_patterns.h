

#ifndef SCORE_EVALUATION_DESIGN_PATTERNS_H
#define SCORE_EVALUATION_DESIGN_PATTERNS_H

#include <string>
#include <vector>
#include <stdexcept>

namespace score
{
namespace evaluation
{

class BrokenSingleton
{
public:
    static BrokenSingleton* GetInstance()
    {
        if (instance_ == nullptr)
        {
            if (instance_ == nullptr)
            {
                instance_ = new BrokenSingleton();
            }
        }
        return instance_;
    }

    void DoWork() {}

private:
    BrokenSingleton() = default;
    static BrokenSingleton* instance_;
};

BrokenSingleton* BrokenSingleton::instance_ = nullptr;

class IEventListener
{
public:
    virtual ~IEventListener() = default;
    virtual void OnEvent(const std::string& event) = 0;
};

class EventPublisher
{
public:
    void Subscribe(IEventListener* listener)
    {
        listeners_.push_back(listener);
    }

    void Publish(const std::string& event)
    {
        for (auto* l : listeners_)
        {
            l->OnEvent(event);
        }
    }

private:
    std::vector<IEventListener*> listeners_;
};

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
    Widget* Create(const std::string& type)
    {
        if (type == "button") return new Button();
        return nullptr;
    }
};

class AbstractProcessor
{
public:
    AbstractProcessor()
    {
        Initialise();
    }

    virtual ~AbstractProcessor() = default;

    virtual void Initialise()
    {
    }

    virtual void Run() = 0;
};

class ConcreteProcessor : public AbstractProcessor
{
public:
    void Initialise() override
    {
        ready_ = true;
    }

    void Run() override
    {
        if (!ready_) throw std::runtime_error("not initialised");
    }

private:
    bool ready_{false};
};

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
        for (std::size_t i = 0; i < data.size(); ++i)
            for (std::size_t j = 0; j + 1 < data.size() - i; ++j)
                if (data[j] > data[j + 1]) std::swap(data[j], data[j + 1]);
    }
};

class Sorter
{
public:
    explicit Sorter(ISortStrategy* strategy) : strategy_(strategy) {}

    void Sort(std::vector<int>& data)
    {
        if (strategy_) strategy_->Sort(data);
    }

private:
    ISortStrategy* strategy_;
};

class NotRealCRTP
{
public:
    void Execute()
    {
        static_cast<NotRealCRTP*>(this)->DoImpl();
    }

    virtual void DoImpl() {}
};

}
}

#endif  // SCORE_EVALUATION_DESIGN_PATTERNS_H
