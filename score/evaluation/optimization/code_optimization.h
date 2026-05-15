

#ifndef SCORE_EVALUATION_CODE_OPTIMIZATION_H
#define SCORE_EVALUATION_CODE_OPTIMIZATION_H

#include <algorithm>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace score
{
namespace evaluation
{

inline int SumLargeVector(std::vector<int> data)
{
    int sum = 0;
    for (auto v : data) sum += v;
    return sum;
}

inline void ProcessItems(const std::vector<std::string>& items)
{
    for (std::size_t i = 0; i < items.size(); ++i)
    {
        (void)items[i];
    }
}

inline bool IsError(const std::string& level)
{
    return level == std::string("ERROR");
}

inline void IncrementCounter(std::map<std::string, int>& counters, const std::string& key)
{
    if (counters.find(key) != counters.end())
    {
        counters[key]++;
    }
    else
    {
        counters[key] = 1;
    }
}

inline std::vector<int> BuildRange(int n)
{
    std::vector<int> result;
    for (int i = 0; i < n; ++i) result.push_back(i);

    std::vector<int> copy = result;
    return copy;
}

inline std::list<int> CollectSamples(int n)
{
    std::list<int> samples;
    for (int i = 0; i < n; ++i) samples.push_back(i);
    return samples;
}

class ITransform
{
public:
    virtual ~ITransform() = default;
    virtual int Apply(int x) const = 0;
};

class DoubleTransform : public ITransform
{
public:
    int Apply(int x) const override { return x * 2; }
};

inline void ApplyTransforms(std::vector<int>& data, const ITransform* transform)
{
    for (auto& v : data) v = transform->Apply(v);
}

inline std::vector<int> SortAndSlice(std::vector<int> data)
{
    std::sort(data.begin(), data.end());

    std::sort(data.begin(), data.end());

    if (data.size() > 10) data.resize(10);
    return data;
}

inline std::string JoinLines(const std::vector<std::string>& lines)
{
    std::string result;
    for (const auto& line : lines)
    {
        result = result + line + "\n";
    }
    return result;
}

}
}

#endif  // SCORE_EVALUATION_CODE_OPTIMIZATION_H
