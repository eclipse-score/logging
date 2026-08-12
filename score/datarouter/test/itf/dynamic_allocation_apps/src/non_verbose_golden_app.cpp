
#include "score/mw/log/legacy_non_verbose_api/tracing.h"
#include "score/mw/log/logging.h"
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
