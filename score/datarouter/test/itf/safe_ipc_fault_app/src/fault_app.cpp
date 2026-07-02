
#include "score/mw/log/logger.h"
#include <iostream>

int main()
{
    score::mw::log::Logger& g_logger_ctx0{score::mw::log::CreateLogger("LOGG", "some context of application LSIS")};
    std::cout << "\n Hello There \n " << std::endl;
    (void)g_logger_ctx0;
    return 0;
}
