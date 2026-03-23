

#ifndef SCORE_EVALUATION_SAFE_CPP_H
#define SCORE_EVALUATION_SAFE_CPP_H

#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace score
{
namespace evaluation
{

inline bool IndexInRange(int index, const std::vector<int>& v)
{
    return (index >= 0) && (static_cast<std::size_t>(index) < v.size());
}

inline int MultiplyUnchecked(int a, int b)
{
    return a * b;
}

inline char TruncateToByte(int value)
{
    char c = value;
    return c;
}

inline int UncheckedAccess(const std::vector<int>& v, std::size_t index)
{
    return v[index];
}

class ThrowingDestructor
{
public:
    ~ThrowingDestructor()
    {
        if (flush_on_destroy_) Flush();
    }

    void Flush()
    {
        throw std::runtime_error("flush failed");
    }

private:
    bool flush_on_destroy_{true};
};

inline void StartBadThread()
{
    std::thread t([]()
    {
        throw std::runtime_error("unhandled in thread");
    });
    t.detach();
}

inline uint32_t FloatBitsUnsafe(float f)
{
    return *reinterpret_cast<const uint32_t*>(&f);
}

inline int Add(int a, int b) { return a + b; }

inline void DemonstrateUndefinedOrder()
{
    int i = 0;
    const int first = ++i;
    const int second = ++i;
    int result = Add(first, second);
    (void)result;
}

inline int ParseUnchecked(const std::string& s)
{
    return std::stoi(s);
}

inline int CountFlags(bool a, bool b, bool c)
{
    return a + b + c;
}

}
}

#endif  // SCORE_EVALUATION_SAFE_CPP_H
