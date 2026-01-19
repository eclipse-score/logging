#ifndef PAS_LOGGING_LOGPARSERMOCK_H_
#define PAS_LOGGING_LOGPARSERMOCK_H_

#include "logparser/i_logparser.h"

#include <functional>
#include <optional>
#include <string>

#include <gmock/gmock.h>

namespace score
{
namespace platform
{
namespace internal
{

class LogParserMock : public ILogParser
{
  public:
    ~LogParserMock() = default;

    MOCK_METHOD(void, set_filter_factory, (FilterFunctionFactory factory), (override));

    MOCK_METHOD(void, add_incoming_type, (const bufsize_t map_index, const std::string& params), (override));
    MOCK_METHOD(void, AddIncomingType, (const score::mw::log::detail::TypeRegistration&), (override));

    MOCK_METHOD(void, add_type_handler, (const std::string& typeName, TypeHandler& handler), (override));
    MOCK_METHOD(void, add_global_handler, (AnyHandler & handler), (override));

    MOCK_METHOD(void, remove_type_handler, (const std::string& typeName, TypeHandler& handler), (override));
    MOCK_METHOD(void, remove_global_handler, (AnyHandler & handler), (override));

    MOCK_METHOD(bool, is_type_hndl_registered, (const std::string& typeName, const TypeHandler& handler), (override));
    MOCK_METHOD(bool, is_glb_hndl_registered, (const AnyHandler& handler), (override));

    MOCK_METHOD(void, reset_internal_mapping, (), (override));
    MOCK_METHOD(void, parse, (timestamp_t timestamp, const char* data, bufsize_t size), (override));
    MOCK_METHOD(void, Parse, (const score::mw::log::detail::SharedMemoryRecord& record), (override));
};

}  // namespace internal
}  // namespace platform
}  // namespace score

#endif  // PAS_LOGGING_LOGPARSERMOCK_H_
