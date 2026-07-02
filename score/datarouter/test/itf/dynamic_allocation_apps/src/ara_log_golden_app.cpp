
#include "score/mw/log/logging.h"
#include <iostream>

int main()
{
    score::mw::log::LogFatal() << 1;

    return 0;
}
