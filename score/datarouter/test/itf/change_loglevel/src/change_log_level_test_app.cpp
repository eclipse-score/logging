#include "score/mw/log/logging.h"
#include <unistd.h>

int main()
{
    score::mw::log::LogVerbose() << "Verbose level Enabled !";
    score::mw::log::LogDebug() << "Debug level Enabled !";
    score::mw::log::LogInfo() << "Info level Enabled !";
    score::mw::log::LogWarn() << "Warn level Enabled !";
    score::mw::log::LogError() << "Error level Enabled !";
    score::mw::log::LogFatal() << "Fatal level Enabled !";
    sleep(5);

    return 0;
}
