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

#include "score/mw/log/runtime.h"
#include "score/mw/log/slot_handle.h"

using namespace score::mw::log;
using namespace score::mw::log::detail;

// Verify configuration at compile time.
// `SlotHandle` is expected to be allocated on stack to reduce performance overhead.
// Size and alignment of `SlotHandle` (C++) and `SlotHandleStorage` (Rust) must match.
// Those parameters must be:
// - managed by build system (using defines and features)
// - cross-checked between `ffi.rs` and `adapter.cpp`
#if defined(x86_64_linux) || defined(arm64_qnx) || defined(x86_64_qnx)
// Expected size and alignment of `SlotHandle`.
static_assert(sizeof(SlotHandle) == 24);
static_assert(alignof(SlotHandle) == 8);
#else
#error "Unknown configuration, unable to check layout"
#endif

extern "C" {
/// @brief Get current recorder from runtime.
/// @return Current recorder.
Recorder* recorder_get() { return &Runtime::GetRecorder(); }

/// @brief Start recording log message.
/// @param recorder     Recorder.
/// @param context      Message context name.
/// @param context_size Message context name size.
/// @param log_level    Message log level.
/// @param slot         `SlotHandle`-sized buffer.
/// @return `slot` if acquired, `nullptr` otherwise.
SlotHandle* recorder_start(Recorder* recorder, const char* context, size_t context_size,
                           LogLevel log_level, SlotHandle* slot) {
    auto start_result{recorder->StartRecord(std::string_view{context, context_size}, log_level)};
    if (start_result) {
        return new (slot) SlotHandle{*start_result};
    } else {
        return nullptr;
    }
}

/// @brief Get current log level for provided context.
/// @param recorder     Recorder.
/// @param context      Message context name.
/// @param context_size Message context name size.
/// @return Current log level.
LogLevel recorder_log_level(const Recorder* recorder, const char* context, size_t context_size) {
    auto first{static_cast<uint8_t>(LogLevel::kOff)};
    auto last{static_cast<uint8_t>(LogLevel::kVerbose)};
    // Reversed order - `kOff` always seem to report true.
    for (uint8_t i{last}; i > first; --i) {
        auto current = static_cast<LogLevel>(i);
        if (recorder->IsLogEnabled(current, context)) {
            return current;
        }
    }

    // Fall-back.
    return LogLevel::kOff;
}

/// @brief Stop recording log message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
void recorder_stop(Recorder* recorder, SlotHandle* slot) { recorder->StopRecord(*slot); }

/// @brief Add bool value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_bool(Recorder* recorder, SlotHandle* slot, const bool* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add f32 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_f32(Recorder* recorder, SlotHandle* slot, const float* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add f64 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_f64(Recorder* recorder, SlotHandle* slot, const double* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add string value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_string(Recorder* recorder, SlotHandle* slot, const char* value, size_t size) {
    recorder->Log(*slot, std::string_view{value, size});
}

/// @brief Add i8 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_i8(Recorder* recorder, SlotHandle* slot, const int8_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add i16 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_i16(Recorder* recorder, SlotHandle* slot, const int16_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add i32 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_i32(Recorder* recorder, SlotHandle* slot, const int32_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add i64 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_i64(Recorder* recorder, SlotHandle* slot, const int64_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add u8 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_u8(Recorder* recorder, SlotHandle* slot, const uint8_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add u16 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_u16(Recorder* recorder, SlotHandle* slot, const uint16_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add u32 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_u32(Recorder* recorder, SlotHandle* slot, const uint32_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add u64 value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_u64(Recorder* recorder, SlotHandle* slot, const uint64_t* value) {
    recorder->Log(*slot, *value);
}

/// @brief Add 8-bit binary value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_bin8(Recorder* recorder, SlotHandle* slot, const uint8_t* value) {
    recorder->Log(*slot, LogBin8{*value});
}

/// @brief Add 16-bit binary value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_bin16(Recorder* recorder, SlotHandle* slot, const uint16_t* value) {
    recorder->Log(*slot, LogBin16{*value});
}

/// @brief Add 32-bit binary value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_bin32(Recorder* recorder, SlotHandle* slot, const uint32_t* value) {
    recorder->Log(*slot, LogBin32{*value});
}

/// @brief Add 64-bit binary value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_bin64(Recorder* recorder, SlotHandle* slot, const uint64_t* value) {
    recorder->Log(*slot, LogBin64{*value});
}

/// @brief Add 8-bit hexadecimal value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_hex8(Recorder* recorder, SlotHandle* slot, const uint8_t* value) {
    recorder->Log(*slot, LogHex8{*value});
}

/// @brief Add 16-bit hexadecimal value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_hex16(Recorder* recorder, SlotHandle* slot, const uint16_t* value) {
    recorder->Log(*slot, LogHex16{*value});
}

/// @brief Add 32-bit hexadecimal value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_hex32(Recorder* recorder, SlotHandle* slot, const uint32_t* value) {
    recorder->Log(*slot, LogHex32{*value});
}

/// @brief Add 64-bit hexadecimal value to message.
/// @param recorder Recorder.
/// @param slot     Acquired slot.
/// @param value    Value.
void log_hex64(Recorder* recorder, SlotHandle* slot, const uint64_t* value) {
    recorder->Log(*slot, LogHex64{*value});
}

/// @brief Get size of `SlotHandle`.
/// @return Size.
size_t slot_handle_size() { return sizeof(SlotHandle); }

/// @brief Get alignment of `SlotHandle`.
/// @return Alignment.
size_t slot_handle_alignment() { return alignof(SlotHandle); }
}
