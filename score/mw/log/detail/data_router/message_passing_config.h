#ifndef SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_CONFIG_H
#define SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_CONFIG_H

#include <cstdint>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{

/// \brief Configuration constants for message passing between DataRouter and logging clients.
struct MessagePassingConfig
{
    /// \brief Maximum message size in bytes (1 byte for message ID + 16 bytes for payload).
    static constexpr std::uint32_t kMaxMessageSize{17U};

    /// \brief Maximum number of messages in receiver queue.
    /// \note Value not used at the moment of integration with message passing library. May change in the future.
    static constexpr std::uint32_t kMaxReceiverQueueSize{0U};

    /// \brief Number of pre-allocated connections.
    static constexpr std::uint32_t kPreAllocConnections{0U};

    /// \brief Maximum number of queued notifications.
    static constexpr std::uint32_t kMaxQueuedNotifies{0U};

    /// \brief Maximum reply size in bytes.
    static constexpr std::uint32_t kMaxReplySize{0U};

    /// \brief Maximum notification size in bytes.
    static constexpr std::uint32_t kMaxNotifySize{0U};

    /// \brief The DataRouter receiver endpoint identifier.
    static constexpr const char* kDatarouterReceiverIdentifier{"/logging.datarouter_recv"};

    /// \brief Start index of random filename part in the shared memory file name.
    /// \note The file name format is "/logging.shm_XXXXXX" where X is random.
    static constexpr std::size_t kRandomFilenameStartIndex{13U};
};

}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_DETAIL_DATA_ROUTER_MESSAGE_PASSING_CONFIG_H
