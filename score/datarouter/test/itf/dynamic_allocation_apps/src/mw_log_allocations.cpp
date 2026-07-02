
#include "score/mw/log/log_types.h"
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"
#include <chrono>
#include <iostream>
#include <thread>
const std::string_view kContextStringView;
template <typename T>
void DoLog(T value)
{
    const auto number_of_iterations = 100;
    for (int i = 0; i < number_of_iterations; i++)
    {
        auto logger{score::mw::log::CreateLogger(std::to_string(i))};
        logger.LogError() << value;
        logger.LogWarn() << value;
        logger.LogInfo() << value;
        logger.LogDebug() << value;
        logger.LogVerbose() << value;
        std::ignore = logger.IsLogEnabled(score::mw::log::LogLevel::kError);
        std::ignore = logger.IsEnabled(score::mw::log::LogLevel::kError);
        std::ignore = logger.GetContext();
        std::ignore = score::mw::log::GetDefaultContextId();
        score::mw::log::LogFatal() << value;
        score::mw::log::LogError() << value;
        score::mw::log::LogWarn() << value;
        score::mw::log::LogInfo() << value;
        score::mw::log::LogDebug() << value;
        score::mw::log::LogVerbose() << value;

        score::mw::log::LogFatal(kContextStringView) << value;
        score::mw::log::LogError(kContextStringView) << value;
        score::mw::log::LogWarn(kContextStringView) << value;
        score::mw::log::LogInfo(kContextStringView) << value;
        score::mw::log::LogDebug(kContextStringView) << value;
        score::mw::log::LogFatal(kContextStringView) << value;
        std::ignore = score::mw::log::GetDefaultLogRecorder();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
int main()
{
    DoLog(true);
    DoLog(std::uint8_t{5});
    DoLog(std::int8_t{5});
    DoLog(std::uint16_t{5});
    DoLog(std::int16_t{5});
    DoLog(std::uint32_t{5});
    DoLog(std::int32_t{5});
    DoLog(std::uint64_t{5});
    DoLog(std::int64_t{5});
    DoLog(float{5.2F});
    DoLog(double{5.2});
    DoLog(std::string_view{"Foo"});
    DoLog(std::string{"Foo"});
    const score::mw::log::LogHex8 hex8_value{0xFF};
    DoLog(hex8_value);
    const score::mw::log::LogHex16 hex16_value{0xFFFF};
    DoLog(hex16_value);
    const score::mw::log::LogHex32 hex32_value{0xFFFFFF};
    DoLog(hex32_value);
    const score::mw::log::LogHex64 hex64_value{0xFFFFFFFF};
    DoLog(hex64_value);
    const score::mw::log::LogBin16 bin16_value{0xFFFF};
    DoLog(bin16_value);
    const score::mw::log::LogBin8 bin8_value{0xFF};
    DoLog(bin8_value);
    const score::mw::log::LogBin32 bin32_value{0xFFFFFF};
    DoLog(bin32_value);
    const score::mw::log::LogBin64 bin64_value{0xFFFFFFFF};
    DoLog(bin64_value);
    const score::mw::log::LogRawBuffer raw_buffer_value{nullptr, 0};
    DoLog(raw_buffer_value);
    return 0;
}
