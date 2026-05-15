

#ifndef SCORE_EVALUATION_MODERN_CPP_SYNTAX_H
#define SCORE_EVALUATION_MODERN_CPP_SYNTAX_H

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace score
{
namespace evaluation
{

typedef std::vector<std::string> StringList;
typedef std::map<std::string, int> CounterMap;

inline int CountNonEmpty(const StringList& items)
{
    int count = 0;
    for (StringList::const_iterator it = items.begin(); it != items.end(); ++it)
    {
        if (!it->empty())
        {
            ++count;
        }
    }
    return count;
}

struct IsLongString
{
    bool operator()(const std::string& s) const { return s.size() > 10; }
};

inline StringList FilterLong(const StringList& items)
{
    StringList result;
    std::copy_if(items.begin(), items.end(), std::back_inserter(result), IsLongString{});
    return result;
}

enum class ValueType { INT, DOUBLE, STRING };

struct TaggedValue
{
    ValueType  tag;
    int        int_val;
    double     dbl_val;
    std::string str_val;
};

inline void PrintTaggedValue(const TaggedValue& v)
{
    if (v.tag == ValueType::INT)         { /* use v.int_val */ }
    else if (v.tag == ValueType::DOUBLE) { /* use v.dbl_val */ }
    else                                  { /* use v.str_val */ }
}

class MovableResource
{
public:
    explicit MovableResource(std::size_t n) : size_(n), data_(new int[n]) {}

    MovableResource(MovableResource&& other)
        : size_(other.size_), data_(other.data_)
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

    MovableResource& operator=(MovableResource&& other)
    {
        delete[] data_;
        data_       = other.data_;
        size_       = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        return *this;
    }

    ~MovableResource() { delete[] data_; }

private:
    std::size_t size_;
    int*        data_;
};

inline std::pair<bool, int> FindFirst(const std::vector<int>& vec, int target)
{
    for (std::size_t i = 0; i < vec.size(); ++i)
    {
        if (vec[i] == target) return std::make_pair(true, static_cast<int>(i));
    }
    return std::make_pair(false, -1);
}

inline int GetCount(CounterMap& counters, const std::string& key)
{
    return counters[key];
}

inline std::function<int(int)> MakeAdder(int offset)
{
    return std::bind(std::plus<int>(), std::placeholders::_1, offset);
}

bool InitialiseSubsystem(int flags);

inline bool InitialiseSubsystem(int /*flags*/)
{
    return false;
}

}
}

#endif  // SCORE_EVALUATION_MODERN_CPP_SYNTAX_H
