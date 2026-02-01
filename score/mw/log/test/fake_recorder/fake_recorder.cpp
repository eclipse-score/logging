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
#include "score/mw/log/test/fake_recorder/fake_recorder.h"

#include <bitset>
#include <cstdio>
#include <mutex>
#include <optional>
#include <string>

namespace score
{
namespace mw
{
namespace log
{
namespace test
{
namespace
{
std::mutex gStdoutMutex;

/// Convert unsigned integer to binary string using std::bitset
template <std::size_t n>
std::string ToBinaryString(std::uint64_t value)
{
    return "0b" + std::bitset<n>(value).to_string();
}
}  // namespace

score::cpp::optional<SlotHandle> FakeRecorder::StartRecord(const std::string_view /*context_id*/,
                                                    const LogLevel /*log_level*/) noexcept
{
    return state_.WithLock([](State& s) -> score::cpp::optional<SlotHandle> {
        for (std::size_t i = 0U; i < kMaxSlots; ++i)
        {
            if (!s.in_flight[i].has_value())
            {
                s.in_flight[i].emplace();
                return SlotHandle(static_cast<std::uint8_t>(i));
            }
        }
        return {};
    });
}

void FakeRecorder::StopRecord(const SlotHandle& slot) noexcept
{
    FlushSlot(slot);
}

bool FakeRecorder::IsLogEnabled(const LogLevel& /*log_level*/, const std::string_view /*context*/) const noexcept
{
    return true;
}

void FakeRecorder::Log(const SlotHandle& slot, const bool data) noexcept
{
    AppendToSlot(slot, data ? "true" : "false");
}

void FakeRecorder::Log(const SlotHandle& slot, const std::uint8_t data) noexcept
{
    AppendToSlot(slot, std::to_string(static_cast<unsigned int>(data)));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::uint16_t data) noexcept
{
    AppendToSlot(slot, std::to_string(static_cast<unsigned int>(data)));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::uint32_t data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::uint64_t data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::int8_t data) noexcept
{
    AppendToSlot(slot, std::to_string(static_cast<std::int16_t>(data)));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::int16_t data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::int32_t data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::int64_t data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const float data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const double data) noexcept
{
    AppendToSlot(slot, std::to_string(data));
}

void FakeRecorder::Log(const SlotHandle& slot, const std::string_view data) noexcept
{
    AppendToSlot(slot, data);
}

void FakeRecorder::Log(const SlotHandle& slot, const LogRawBuffer data) noexcept
{
    const auto* begin = data.data();
    AppendToSlot(slot, std::string_view(begin, data.size()));
}

void FakeRecorder::Log(const SlotHandle& slot, const LogSlog2Message /*data*/) noexcept
{
    AppendToSlot(slot, "[LogSlog2Message]");
}

void FakeRecorder::Log(const SlotHandle& slot, const LogHex8 data) noexcept
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "0x%02X", data.value);
    AppendToSlot(slot, buf);
}

void FakeRecorder::Log(const SlotHandle& slot, const LogHex16 data) noexcept
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%04X", data.value);
    AppendToSlot(slot, buf);
}

void FakeRecorder::Log(const SlotHandle& slot, const LogHex32 data) noexcept
{
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", data.value);
    AppendToSlot(slot, buf);
}

void FakeRecorder::Log(const SlotHandle& slot, const LogHex64 data) noexcept
{
    char buf[24];
    std::snprintf(buf, sizeof(buf), "0x%016llX", static_cast<unsigned long long>(data.value));
    AppendToSlot(slot, buf);
}

void FakeRecorder::Log(const SlotHandle& slot, const LogBin8 data) noexcept
{
    AppendToSlot(slot, ToBinaryString<8>(static_cast<std::uint64_t>(data.value)));
}

void FakeRecorder::Log(const SlotHandle& slot, const LogBin16 data) noexcept
{
    AppendToSlot(slot, ToBinaryString<16>(static_cast<std::uint64_t>(data.value)));
}

void FakeRecorder::Log(const SlotHandle& slot, const LogBin32 data) noexcept
{
    AppendToSlot(slot, ToBinaryString<32>(static_cast<std::uint64_t>(data.value)));
}

void FakeRecorder::Log(const SlotHandle& slot, const LogBin64 data) noexcept
{
    AppendToSlot(slot, ToBinaryString<64>(data.value));
}

std::vector<std::string> FakeRecorder::GetRecordedMessages() const noexcept
{
    return state_.WithLock([](const State& s) {
        return s.recorded_messages;
    });
}

void FakeRecorder::ClearRecordedMessages() noexcept
{
    state_.WithLock([](State& s) {
        s.recorded_messages.clear();
        for (auto& slot : s.in_flight)
        {
            slot.reset();
        }
    });
}

void FakeRecorder::AppendToSlot(const SlotHandle& slot, const std::string_view text) noexcept
{
    const auto idx = static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder());
    state_.WithLock([idx, text](State& s) {
        if (!s.in_flight[idx].has_value())
        {
            return;
        }
        s.in_flight[idx]->append(text);
    });
}

void FakeRecorder::FlushSlot(const SlotHandle& slot) noexcept
{
    const auto idx = static_cast<std::size_t>(slot.GetSlotOfSelectedRecorder());
    std::string msg;

    state_.WithLock([&msg, idx](State& s) {
        if (!s.in_flight[idx].has_value())
        {
            return;
        }

        msg = std::move(*s.in_flight[idx]);
        s.in_flight[idx].reset();
    });

    if (msg.empty())
    {
        return;
    }

    {
        std::lock_guard<std::mutex> out_lock(gStdoutMutex);
        std::fwrite(msg.c_str(), 1U, msg.size(), stdout);
        std::fwrite("\n", 1U, 1U, stdout);
        std::fflush(stdout);
    }

    state_.WithLock([&msg](State& s) {
        s.recorded_messages.push_back(std::move(msg));
    });
}

}  // namespace test
}  // namespace log
}  // namespace mw
}  // namespace score
