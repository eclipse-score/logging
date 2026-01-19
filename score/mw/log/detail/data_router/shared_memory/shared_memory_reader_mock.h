#ifndef BMW_MW_LOG_SHARED_MEMORY_READER_MOCK
#define BMW_MW_LOG_SHARED_MEMORY_READER_MOCK

#include "score/mw/log/detail/data_router/shared_memory/i_shared_memory_reader.h"

#include <gmock/gmock.h>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief The factory is responsible for creating the shared memory file and instantiating the SharedMemoryReader
class ISharedMemoryReaderMock : public ISharedMemoryReader
{
  public:
    MOCK_METHOD(std::optional<Length>,
                Read,
                (const TypeRegistrationCallback& type_registration_callback,
                 const NewRecordCallback& new_message_callback),
                (noexcept, override));

    MOCK_METHOD(std::optional<Length>,
                PeekNumberOfBytesAcquiredInBuffer,
                (const std::uint32_t acquired_buffer_count_id),
                (const, noexcept, override));

    MOCK_METHOD(std::optional<Length>,
                ReadDetached,
                (const TypeRegistrationCallback& type_registration_callback,
                 const NewRecordCallback& new_message_callback),
                (noexcept, override));

    MOCK_METHOD(Length, GetNumberOfDropsWithBufferFull, (), (const, noexcept, override));
    MOCK_METHOD(Length, GetNumberOfDropsWithInvalidSize, (), (const, noexcept, override));
    MOCK_METHOD(Length, GetNumberOfDropsWithTypeRegistrationFailed, (), (const, noexcept, override));
    MOCK_METHOD(Length, GetSizeOfDropsWithBufferFull, (), (const, noexcept, override));
    MOCK_METHOD(Length, GetRingBufferSizeBytes, (), (const, noexcept, override));
    MOCK_METHOD(bool, IsBlockReleasedByWriters, (const std::uint32_t block_count), (noexcept, override));
    MOCK_METHOD(std::optional<Length>,
                NotifyAcquisitionSetReader,
                (const ReadAcquireResult& acquire_result),
                (noexcept, override));
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  //  BMW_MW_LOG_SHARED_MEMORY_READER_MOCK
