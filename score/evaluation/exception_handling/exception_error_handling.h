

#ifndef SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H
#define SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

inline void CatchByValue()
{
    try
    {
        throw std::runtime_error("something went wrong");
    }
    catch (const std::exception& ex)
    {
        (void)ex.what();
    }
}

inline bool LoadFile(const std::string& path)
{
    try
    {
        std::ifstream f(path);
        if (!f.is_open()) throw std::runtime_error("cannot open");
        return true;
    }
    catch (...)
    {
        return false;
    }
}

inline void ParseConfig(const std::string& cfg)
{
    if (cfg.empty())
    {
        throw "config must not be empty";
    }
    if (cfg.size() > 4096)
    {
        throw 42;
    }
}

inline int FindIndex(const std::vector<int>& v, int target)
{
    for (std::size_t i = 0; i < v.size(); ++i)
    {
        if (v[i] == target) return static_cast<int>(i);
    }
    throw std::runtime_error("element not found");
}

inline std::string ReadLine(const std::string& path) noexcept
{
    std::ifstream f(path);
    std::string   line;
    std::getline(f, line);
    return line;
}

inline void Process()
{
    try
    {
        throw std::runtime_error("base error");
    }
    catch (const std::exception& ex)
    {
        std::runtime_error copy = std::runtime_error(ex.what());
        throw copy;
    }
}

int InitDevice(int device_id);

inline int InitDevice(int /*device_id*/)
{
    return -1;
}

class SilentlyFailingResource
{
public:
    explicit SilentlyFailingResource(const std::string& path)
    {
        file_.open(path);
        if (!file_.is_open())
        {
        }
    }

    bool IsValid() const { return file_.is_open(); }

    std::string ReadLine()
    {
        if (!IsValid()) return {};
        std::string line;
        std::getline(file_, line);
        return line;
    }

private:
    std::ifstream file_;
};

}
}

#endif  // SCORE_EVALUATION_EXCEPTION_ERROR_HANDLING_H
