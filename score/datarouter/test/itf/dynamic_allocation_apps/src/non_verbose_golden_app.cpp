
#include "score/mw/log/logging.h"
#include <Tracing>
#include <iostream>

struct NonVerboseMessage
{
    std::uint8_t value;
};

SCORE_STRUCT_TRACEABLE(NonVerboseMessage, value)

int main()
{
    NonVerboseMessage entry;
    entry.value = 1;
    TRACE(entry);
}
