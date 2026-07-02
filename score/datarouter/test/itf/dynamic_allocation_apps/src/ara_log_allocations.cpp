
#include "score/mw/log/logger.h"
#include "score/mw/log/logging.h"
#include <ara/core/error_code.h>
#include <ara/core/error_domain.h>
#include <chrono>
#include <iostream>
#include <thread>

auto constexpr kMsg = "";
const auto kArrayOfChar = "Hello";
template <typename T>
void DoLog(T value)
{
    const auto number_of_iterations = 100;
    for (int i = 0; i < number_of_iterations; i++)
    {
        score::mw::log::Logger& logger{score::mw::log::CreateLogger(std::to_string(i), kMsg)};

        logger.LogFatal() << value;
        logger.LogError() << value;
        logger.LogWarn() << value;
        logger.LogInfo() << value;
        logger.LogDebug() << value;
        logger.LogVerbose() << value;
        std::ignore = logger.IsLogEnabled(score::mw::log::LogLevel::kError);
        std::ignore = logger.IsEnabled(score::mw::log::LogLevel::kError);

        score::mw::log::LogFatal() << value;
        score::mw::log::LogError() << value;
        score::mw::log::LogWarn() << value;
        score::mw::log::LogInfo() << value;
        score::mw::log::LogDebug() << value;
        score::mw::log::LogFatal() << value;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
using ErrorDomain = ara::core::ErrorDomain;
using CodeType = ErrorDomain::CodeType;  // std::int32_t
using SupportDataType = ErrorDomain::SupportDataType;
using IdType = ErrorDomain::IdType;
using ErrorCode = ara::core::ErrorCode;
class SimpleErrorDomain : public ara::core::ErrorDomain
{
  public:
    constexpr SimpleErrorDomain(IdType id, const char* name, const char* message, const char* exception) noexcept
        : ErrorDomain(id), m_name{name}, m_message{message}, m_exception{exception}
    {
    }
    virtual ~SimpleErrorDomain() = default;
    const char* Name() const noexcept override
    {
        return m_name;
    }
    const char* Message(CodeType) const noexcept override
    {
        return m_message;
    }
    void ThrowAsException(const ErrorCode&) const noexcept(false) override
    {
        std::cerr << m_exception << std::endl;
    }
    const char* m_name;
    const char* m_message;
    const char* m_exception;
};
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
    DoLog(kArrayOfChar);
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

    ara::core::ErrorDomain::IdType idtype_test{300U};
    const char* name_test = "name_test";
    const char* message_test = "message_test";
    const char* exception_test = "exception_test";
    SimpleErrorDomain errordomain_test{idtype_test, name_test, message_test, exception_test};
    CodeType codetype_test{100};
    SupportDataType supportdatatype_test{200};
    const char* usermessage_test = "usermessage_test";
    ErrorCode errorcode_test{codetype_test, errordomain_test, supportdatatype_test, usermessage_test};
    DoLog(codetype_test);
    return 0;
}
