/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_MW_LOG_TEST_FAKE_RECORDER_FAKE_RECORDER_H
#define SCORE_MW_LOG_TEST_FAKE_RECORDER_FAKE_RECORDER_H

#include "score/mw/log/recorder.h"
#include "score/datarouter/lib/synchronized/synchronized.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace score
{
namespace mw
{
namespace log
{
namespace test
{

class FakeRecorder final : public score::mw::log::Recorder
{
  public:
    FakeRecorder() = default;
    ~FakeRecorder() noexcept override = default;

    FakeRecorder(const FakeRecorder&) = delete;
    FakeRecorder& operator=(const FakeRecorder&) = delete;
    FakeRecorder(FakeRecorder&&) = delete;
    FakeRecorder& operator=(FakeRecorder&&) = delete;

    // Recorder interface implementation
    score::cpp::optional<SlotHandle> StartRecord(std::string_view context_id, LogLevel log_level) noexcept override;
    void StopRecord(const SlotHandle& slot) noexcept override;
    bool IsLogEnabled(const LogLevel& log_level, const std::string_view context) const noexcept override;

    void Log(const SlotHandle& slot, bool data) noexcept override;
    void Log(const SlotHandle& slot, std::uint8_t data) noexcept override;
    void Log(const SlotHandle& slot, std::uint16_t data) noexcept override;
    void Log(const SlotHandle& slot, std::uint32_t data) noexcept override;
    void Log(const SlotHandle& slot, std::uint64_t data) noexcept override;
    void Log(const SlotHandle& slot, std::int8_t data) noexcept override;
    void Log(const SlotHandle& slot, std::int16_t data) noexcept override;
    void Log(const SlotHandle& slot, std::int32_t data) noexcept override;
    void Log(const SlotHandle& slot, std::int64_t data) noexcept override;
    void Log(const SlotHandle& slot, float data) noexcept override;
    void Log(const SlotHandle& slot, double data) noexcept override;
    void Log(const SlotHandle& slot, std::string_view data) noexcept override;
    void Log(const SlotHandle& slot, LogHex8 data) noexcept override;
    void Log(const SlotHandle& slot, LogHex16 data) noexcept override;
    void Log(const SlotHandle& slot, LogHex32 data) noexcept override;
    void Log(const SlotHandle& slot, LogHex64 data) noexcept override;
    void Log(const SlotHandle& slot, LogBin8 data) noexcept override;
    void Log(const SlotHandle& slot, LogBin16 data) noexcept override;
    void Log(const SlotHandle& slot, LogBin32 data) noexcept override;
    void Log(const SlotHandle& slot, LogBin64 data) noexcept override;
    void Log(const SlotHandle& slot, const LogRawBuffer data) noexcept override;
    void Log(const SlotHandle& slot, const LogSlog2Message data) noexcept override;

    std::vector<std::string> GetRecordedMessages() const noexcept;

    void ClearRecordedMessages() noexcept;

  private:
    void AppendToSlot(const SlotHandle& slot, std::string_view text) noexcept;
    void FlushSlot(const SlotHandle& slot) noexcept;

    static constexpr std::size_t kMaxSlots{256U};

    struct State
    {
        std::array<std::optional<std::string>, kMaxSlots> in_flight{};
        std::vector<std::string> recorded_messages{};
    };

    score::platform::datarouter::Synchronized<State> state_{};
};

}  // namespace test
}  // namespace log
}  // namespace mw
}  // namespace score

#endif  // SCORE_MW_LOG_TEST_FAKE_RECORDER_FAKE_RECORDER_H
