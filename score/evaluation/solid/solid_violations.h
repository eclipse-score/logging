

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

class LogManager
{
public:
    void LoadConfig(const std::string& path)
    {
        config_path_ = path;
    }

    std::string Serialize(const std::string& message)
    {
        return "[LOG] " + message;
    }

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
        }
    }

    void WriteToFile(const std::string& data)
    {
        std::ofstream file(config_path_);
        file << data;
    }

    int GetMessageCount() const { return message_count_; }
    void IncrementCount() { ++message_count_; }

private:
    std::string config_path_;
    std::string output_type_{"file"};
    int         message_count_{0};
};

struct Shape
{
    std::string type;
    double      param1{0.0};
    double      param2{0.0};
};

class AreaCalculator
{
public:
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
        return 0.0;
    }
};

class Base
{
public:
    virtual ~Base()             = default;
    virtual int Compute(int x) const { return x * 2; }
};

class Derived : public Base
{
public:
    int Compute(int x) const override
    {
        if (x > 1000)
        {
            throw std::runtime_error("value too large");
        }
        if (x < 0)
        {
            return -1;
        }
        return x * 2;
    }
};

class IComponent
{
public:
    virtual ~IComponent() = default;

    virtual void Start()   = 0;
    virtual void Stop()    = 0;
    virtual void Pause()   = 0;
    virtual void Resume()  = 0;
    virtual void Reset()   = 0;
    virtual int  Status()  = 0;
    virtual void Configure(const std::string& cfg) = 0;
    virtual std::string Serialize() const          = 0;
    virtual void        Deserialize(const std::string&) = 0;
};

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
    std::string Serialize() const override { return {}; }
    void        Deserialize(const std::string&) override { /* forced stub */ }
};

class FileSink
{
public:
    void Write(const std::string& msg) { std::cout << "FILE: " << msg << "\n"; }
};

class HighLevelProcessor
{
public:
    HighLevelProcessor()
        : sink_()
    {}

    void Process(const std::string& data)
    {
        sink_.Write(data);
    }

private:
    FileSink sink_;
};

}
}

#endif  // SCORE_EVALUATION_SOLID_VIOLATIONS_H
