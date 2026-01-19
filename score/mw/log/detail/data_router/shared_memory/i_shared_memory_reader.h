#ifndef BMW_MW_LOG_WAIT_FREE_I_SHARED_MEMORY_READER
#define BMW_MW_LOG_WAIT_FREE_I_SHARED_MEMORY_READER

#include "score/mw/log/detail/data_router/shared_memory/common.h"
#include "score/mw/log/detail/wait_free_producer_queue/alternating_reader.h"

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

struct TypeRegistration
{
    /*
      Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
      particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per the
      design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly; this is
      additionally guaranteed by the build system(bazel) visibility
  */
    // coverity[autosar_cpp14_m11_0_1_violation]
    TypeIdentifier type_id{};
    // coverity[autosar_cpp14_m11_0_1_violation]
    score::cpp::span<Byte> registration_data;
};

using TypeRegistrationCallback = score::cpp::callback<void(const TypeRegistration&), 64>;

struct SharedMemoryRecord
{ /*
     Maintaining compatibility and avoiding performance overhead outweighs POD Type (class) based design for this
     particular struct. The Type is simple and does not require invariance (interface OR custom behavior) as per the
     design. Moreover the type is ONLY used internally under the namespace detail and NOT exposed publicly; this is
     additionally guaranteed by the build system(bazel) visibility
 */
    // coverity[autosar_cpp14_m11_0_1_violation]
    BufferEntryHeader header;
    // coverity[autosar_cpp14_m11_0_1_violation]
    score::cpp::span<Byte> payload;
};

using NewRecordCallback = score::cpp::callback<void(const SharedMemoryRecord&), 64>;

class ISharedMemoryReader
{
  public:
    virtual ~ISharedMemoryReader() = default;

    virtual std::optional<Length> Read(const TypeRegistrationCallback& type_registration_callback,
                                       const NewRecordCallback& new_message_callback) noexcept = 0;

    virtual std::optional<Length> PeekNumberOfBytesAcquiredInBuffer(
        const std::uint32_t acquired_buffer_count_id) const noexcept = 0;

    virtual std::optional<Length> ReadDetached(const TypeRegistrationCallback& type_registration_callback,
                                               const NewRecordCallback& new_message_callback) noexcept = 0;

    virtual Length GetNumberOfDropsWithBufferFull() const noexcept = 0;
    virtual Length GetNumberOfDropsWithInvalidSize() const noexcept = 0;
    virtual Length GetNumberOfDropsWithTypeRegistrationFailed() const noexcept = 0;
    virtual Length GetSizeOfDropsWithBufferFull() const noexcept = 0;

    virtual Length GetRingBufferSizeBytes() const noexcept = 0;

    virtual bool IsBlockReleasedByWriters(const std::uint32_t block_count) noexcept = 0;

    virtual std::optional<Length> NotifyAcquisitionSetReader(const ReadAcquireResult& acquire_result) noexcept = 0;
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // BMW_MW_LOG_WAIT_FREE_I_SHARED_MEMORY_READER
