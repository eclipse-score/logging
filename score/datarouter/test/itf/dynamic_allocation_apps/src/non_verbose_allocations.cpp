
#include "score/mw/log/legacy_non_verbose_api/tracing.h"
#include "score/mw/log/logging.h"
#include <array>
#include <chrono>
#include <iostream>
#include <thread>

struct NonVerboseMessage
{
    bool bool_value;
    std::uint8_t uint8_value;
    std::int8_t int8_value;
    std::uint16_t uint16_value;
    std::int16_t int16_value;
    std::uint32_t uint32_value;
    std::int32_t int32_value;
    std::uint64_t uint64_value;
    std::int64_t int64_value;
    float float_value;
    double double_value;
    std::string str_value;
    std::array<int, 5> array_value;
    std::tuple<char, int, float> tuple_value;
    std::pair<int, char> pair_value;
};

SCORE_STRUCT_TRACEABLE(NonVerboseMessage,
                     bool_value,
                     uint8_value,
                     int8_value,
                     uint16_value,
                     int16_value,
                     uint32_value,
                     int32_value,
                     uint64_value,
                     int64_value,
                     float_value,
                     double_value,
                     str_value,
                     array_value,
                     tuple_value,
                     pair_value)

int main()
{
    NonVerboseMessage entry{true,
                            std::uint8_t{5},
                            std::int8_t{5},
                            std::uint16_t{5},
                            std::int16_t{5},
                            std::uint32_t{5},
                            std::int32_t{5},
                            std::uint64_t{5},
                            std::int64_t{5},
                            float{5.2F},
                            double{5.2},
                            std::string{"Foo"},
                            {1, 2, 3, 4, 5},
                            std::make_tuple('a', 10, 15.5),
                            std::make_pair(1, 'c')};
    const auto number_of_iterations = 1000;
    for (uint16_t i = 0; i < number_of_iterations; i++)
    {
        TRACE(entry);
        TraceVerbose(entry);
        TraceDebug(entry);
        TraceInfo(entry);
        TraceWarning(entry);
        TraceError(entry);
        TraceFatal(entry);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
