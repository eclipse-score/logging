

#ifndef SCORE_EVALUATION_NON_IDIOMATIC_CPP_H
#define SCORE_EVALUATION_NON_IDIOMATIC_CPP_H

#include <cstdio>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

#define MAX_ITEMS 64           // [NI-03]: prefer constexpr int kMaxItems = 64;
#define PI_VALUE  3.14159265   // [NI-03]: prefer constexpr double kPi = 3.14159265;

int g_request_count = 0;

inline double ToDouble(int value)
{
    return (double)value;
}

inline void* GetRawPtr(int* p)
{
    return (void*)p;
}

inline bool IsNull(int* ptr)
{
    return ptr == NULL;
}

void SetPointer(int** pp)
{
    *pp = NULL;
}

struct LegacyBuffer
{
    int  data[MAX_ITEMS];
    int  size;

    void Clear()
    {
        for (int i = 0; i < MAX_ITEMS; ++i)
        {
            data[i] = 0;
        }
    }
};

inline std::string FormatMessage(int code, const std::string& text)
{
    char buf[256];
    sprintf(buf, "Code=%d Msg=%s", code, text.c_str());
    return std::string(buf);
}

inline int SumVector(const std::vector<int>& vec)
{
    int total = 0;
    for (int i = 0; i < (int)vec.size(); ++i)
    {
        total += vec[i];
    }
    return total;
}

inline bool HasPrefix(std::string text, std::string prefix)
{
    return text.substr(0, prefix.size()) == prefix;
}

inline void PrintLines(const std::vector<std::string>& lines)
{
    for (const auto& line : lines)
    {
        std::cout << line << std::endl;
    }
}

inline bool ParseInt(const std::string& str, int* result)
{
    if (str.empty() || result == NULL)
    {
        return false;
    }
    *result = std::stoi(str);
    return true;
}

}
}

#endif  // SCORE_EVALUATION_NON_IDIOMATIC_CPP_H
