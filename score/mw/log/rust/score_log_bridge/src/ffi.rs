//
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
//

use core::alloc::Layout;
use core::cmp::min;
use core::ffi::c_char;
use core::mem::transmute;
use core::slice::from_raw_parts;
use score_log::fmt::{DisplayHint, Error, FormatSpec, Result as FmtResult, ScoreWrite};

/// Represents severity of a log message.
#[derive(PartialEq, Eq, PartialOrd, Ord, Debug)]
#[repr(u8)]
pub enum LogLevel {
    #[allow(dead_code)]
    Off = 0x00,
    Fatal = 0x01,
    Error = 0x02,
    Warn = 0x03,
    Info = 0x04,
    Debug = 0x05,
    Verbose = 0x06,
}

impl From<score_log::Level> for LogLevel {
    fn from(score_log_level: score_log::Level) -> Self {
        match score_log_level {
            score_log::Level::Fatal => LogLevel::Fatal,
            score_log::Level::Error => LogLevel::Error,
            score_log::Level::Warn => LogLevel::Warn,
            score_log::Level::Info => LogLevel::Info,
            score_log::Level::Debug => LogLevel::Debug,
            score_log::Level::Trace => LogLevel::Verbose,
        }
    }
}

impl From<LogLevel> for score_log::Level {
    fn from(log_level: LogLevel) -> Self {
        match log_level {
            LogLevel::Fatal => score_log::Level::Fatal,
            LogLevel::Error => score_log::Level::Error,
            LogLevel::Warn => score_log::Level::Warn,
            LogLevel::Info => score_log::Level::Info,
            LogLevel::Debug => score_log::Level::Debug,
            LogLevel::Verbose => score_log::Level::Trace,
            _ => panic!("log level not supported"),
        }
    }
}

impl From<LogLevel> for score_log::LevelFilter {
    fn from(level: LogLevel) -> score_log::LevelFilter {
        match level {
            LogLevel::Off => score_log::LevelFilter::Off,
            LogLevel::Fatal => score_log::LevelFilter::Fatal,
            LogLevel::Error => score_log::LevelFilter::Error,
            LogLevel::Warn => score_log::LevelFilter::Warn,
            LogLevel::Info => score_log::LevelFilter::Info,
            LogLevel::Debug => score_log::LevelFilter::Debug,
            LogLevel::Verbose => score_log::LevelFilter::Trace,
        }
    }
}

/// Name of the context.
/// Max 4 bytes containing ASCII characters.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Context {
    data: [c_char; 4],
    size: usize,
}

impl From<&str> for Context {
    /// Create `Context` from `&str`.
    ///
    /// # Panics
    ///
    /// Method will panic if provided `&str` contains non-ASCII characters.
    fn from(value: &str) -> Self {
        // Disallow non-ASCII strings.
        // ASCII characters are single byte in UTF-8.
        assert!(
            value.is_ascii(),
            "provided context contains non-ASCII characters: {value}"
        );

        // Get number of characters.
        let size = min(value.len(), 4);

        // Copy data into array.
        let mut data = [0; _];
        // SAFETY:
        // Copying is safe:
        // - source is a `&str`.
        // - destination is a stack-allocated array of known size.
        // - number of bytes to copy is determined based on source.
        unsafe {
            core::ptr::copy_nonoverlapping(value.as_ptr(), data.as_mut_ptr().cast(), size);
        }

        Self { data, size }
    }
}

impl From<&Context> for &str {
    fn from(value: &Context) -> Self {
        // SAFETY:
        // ASCII characters (`c_char`) are compatible with single byte UTF-8 characters (`u8`).
        // This function reinterprets data containing same information.
        // `[c_char; _]` -> `* const u8` -> `&[u8]` -> `&str`.

        // Characters are reinterpreted from `c_char` (`i8`) to `u8`.
        let data = value.data.as_ptr().cast();
        // Number of bytes is always bound to provided or max allowed size.
        let size = min(value.size, 4);
        unsafe {
            // Create a slice from pointer and size.
            let slice = from_raw_parts(data, size);
            // Create a UTF-8 string from a slice.
            str::from_utf8_unchecked(slice)
        }
    }
}

/// Opaque type representing `Recorder`.
#[repr(C)]
struct RecorderPtr {
    _private: [u8; 0],
}

/// Recorder instance.
pub struct Recorder {
    ptr: *mut RecorderPtr,
}

impl Recorder {
    /// Get recorder instance.
    ///
    /// # Panics
    ///
    /// Method panics if recorder was not properly initialized.
    pub fn new() -> Self {
        // Get recorder and check it's not null.
        // SAFETY: Recorder must be available. Null indicates lack of proper initialization.
        let ptr = unsafe { recorder_get() };
        assert!(!ptr.is_null(), "recorder is not properly initialized");
        Self { ptr }
    }

    pub fn log_level(&self, context: &Context) -> LogLevel {
        // SAFETY: FFI call. Validity of provided objects checked elsewhere.
        unsafe { recorder_log_level(self.ptr, context.data.as_ptr(), context.size) }
    }
}

// SAFETY:
// `Runtime::GetRecorder()` is thread-safe in this use-case.
// Refer to `runtime.h` for more details.
unsafe impl Send for Recorder {}

// SAFETY:
// `Runtime::GetRecorder()` is thread-safe in this use-case.
// Refer to `runtime.h` for more details.
unsafe impl Sync for Recorder {}

/// Opaque storage type representing `SlotHandle`.
///
/// `SlotHandle` is expected to be allocated on stack to reduce performance overhead.
/// Size and alignment of `SlotHandle` (C++) and `SlotHandleStorage` (Rust) must match.
/// Those parameters must be:
/// - managed by build system (using defines and features)
/// - cross-checked between `ffi.rs` and `adapter.cpp`
#[cfg(any(feature = "x86_64_linux", feature = "arm64_qnx"))]
#[repr(C, align(8))]
pub struct SlotHandleStorage {
    _private: [u8; 24],
}

impl SlotHandleStorage {
    /// Returns an unsafe mutable pointer to this object.
    pub fn as_mut_ptr(&mut self) -> *mut SlotHandleStorage {
        self as *mut SlotHandleStorage
    }

    /// Rust-side layout of this object.
    pub fn layout_rust() -> Layout {
        Layout::new::<Self>()
    }

    /// C++-side layout of this object.
    pub fn layout_cpp() -> Layout {
        // SAFETY: parameter-less FFI calls.
        let size = unsafe { slot_handle_size() };
        let align = unsafe { slot_handle_alignment() };
        Layout::from_size_align(size, align).expect("Invalid SlotHandle layout, size: {size}, alignment: {align}")
    }
}

impl Default for SlotHandleStorage {
    /// Create storage for `SlotHandle`.
    fn default() -> Self {
        Self { _private: [0; _] }
    }
}

/// Single log message.
/// Adds values to the log message with selected formatting.
/// Message is flushed on drop.
pub struct LogMessage<'a> {
    recorder: &'a Recorder,
    slot: SlotHandleStorage,
}

impl<'a> LogMessage<'a> {
    pub fn new(recorder: &'a Recorder, context: &Context, log_level: LogLevel) -> Result<Self, ()> {
        // Start record.
        // `SlotHandle` is allocated on stack.
        let mut slot = SlotHandleStorage::default();
        // SAFETY: FFI call.
        // Provided pointers are valid and checked elsewhere.
        // `slot` is stack-allocated above.
        // If result is null - logging is not performed.
        let slot_result = unsafe {
            recorder_start(
                recorder.ptr,
                context.data.as_ptr(),
                context.size,
                log_level,
                slot.as_mut_ptr(),
            )
        };

        // Return without value if failed to acquire slot.
        if slot_result.is_null() {
            return Err(());
        }

        Ok(Self { recorder, slot })
    }
}

impl ScoreWrite for LogMessage<'_> {
    #[inline]
    fn write_bool(&mut self, v: &bool, _spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI call. Reference is reinterpreted as a pointer of the same type.
        unsafe { log_bool(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) };
        Ok(())
    }

    #[inline]
    fn write_f32(&mut self, v: &f32, _spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI call. Reference is reinterpreted as a pointer of the same type.
        unsafe { log_f32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) };
        Ok(())
    }

    #[inline]
    fn write_f64(&mut self, v: &f64, _spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI call. Reference is reinterpreted as a pointer of the same type.
        unsafe { log_f64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) };
        Ok(())
    }

    #[inline]
    fn write_i8(&mut self, v: &i8, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_i8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u8 = transmute(v);
                    log_hex8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            DisplayHint::Binary => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u8 = transmute(v);
                    log_bin8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_i16(&mut self, v: &i16, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_i16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u16 = transmute(v);
                    log_hex16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            DisplayHint::Binary => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u16 = transmute(v);
                    log_bin16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_i32(&mut self, v: &i32, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_i32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u32 = transmute(v);
                    log_hex32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            DisplayHint::Binary => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u32 = transmute(v);
                    log_bin32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_i64(&mut self, v: &i64, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_i64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u64 = transmute(v);
                    log_hex64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            DisplayHint::Binary => {
                // SAFETY: source and destination types are of the same length.
                unsafe {
                    let v: &u64 = transmute(v);
                    log_bin64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _);
                }
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_u8(&mut self, v: &u8, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_u8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => unsafe {
                log_hex8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _)
            },
            DisplayHint::Binary => unsafe { log_bin8(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_u16(&mut self, v: &u16, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_u16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => unsafe {
                log_hex16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _)
            },
            DisplayHint::Binary => unsafe { log_bin16(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_u32(&mut self, v: &u32, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_u32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => unsafe {
                log_hex32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _)
            },
            DisplayHint::Binary => unsafe { log_bin32(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_u64(&mut self, v: &u64, spec: &FormatSpec) -> FmtResult {
        // SAFETY: FFI calls. Reference is reinterpreted as a pointer of the same type.
        match spec.get_display_hint() {
            DisplayHint::NoHint => unsafe { log_u64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            DisplayHint::LowerHex | DisplayHint::UpperHex => unsafe {
                log_hex64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _)
            },
            DisplayHint::Binary => unsafe { log_bin64(self.recorder.ptr, self.slot.as_mut_ptr(), v as *const _) },
            _ => return Err(Error),
        }
        Ok(())
    }

    #[inline]
    fn write_str(&mut self, v: &str, _spec: &FormatSpec) -> FmtResult {
        // Get string as pointer and size.
        let v_ptr = v.as_ptr().cast();
        let v_size = v.len();

        // SAFETY: FFI call. String is reinterpreted into `c_char` pointer and number of bytes.
        unsafe {
            log_string(self.recorder.ptr, self.slot.as_mut_ptr(), v_ptr, v_size);
        }
        Ok(())
    }
}

impl Drop for LogMessage<'_> {
    fn drop(&mut self) {
        // SAFETY: FFI call. Validity of objects is checked elsewhere.
        unsafe {
            recorder_stop(self.recorder.ptr, self.slot.as_mut_ptr());
        }
    }
}

unsafe extern "C" {
    fn recorder_get() -> *mut RecorderPtr;
    fn recorder_start(
        recorder: *mut RecorderPtr,
        context: *const c_char,
        context_size: usize,
        log_level: LogLevel,
        slot: *mut SlotHandleStorage,
    ) -> *mut SlotHandleStorage;
    fn recorder_stop(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage);
    fn recorder_log_level(recorder: *const RecorderPtr, context: *const c_char, context_size: usize) -> LogLevel;
    fn log_bool(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const bool);
    fn log_f32(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const f32);
    fn log_f64(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const f64);
    fn log_string(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const c_char, size: usize);
    fn log_i8(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const i8);
    fn log_i16(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const i16);
    fn log_i32(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const i32);
    fn log_i64(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const i64);
    fn log_u8(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u8);
    fn log_u16(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u16);
    fn log_u32(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u32);
    fn log_u64(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u64);
    fn log_bin8(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u8);
    fn log_bin16(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u16);
    fn log_bin32(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u32);
    fn log_bin64(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u64);
    fn log_hex8(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u8);
    fn log_hex16(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u16);
    fn log_hex32(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u32);
    fn log_hex64(recorder: *mut RecorderPtr, slot: *mut SlotHandleStorage, value: *const u64);
    fn slot_handle_size() -> usize;
    fn slot_handle_alignment() -> usize;
}

#[cfg(test)]
mod tests {
    use crate::ffi::{Context, LogLevel};

    #[test]
    fn test_log_level_from_score_log_level() {
        let levels = [
            (score_log::Level::Fatal, LogLevel::Fatal),
            (score_log::Level::Error, LogLevel::Error),
            (score_log::Level::Warn, LogLevel::Warn),
            (score_log::Level::Info, LogLevel::Info),
            (score_log::Level::Debug, LogLevel::Debug),
            (score_log::Level::Trace, LogLevel::Verbose),
        ];

        for (score_log_level, ffi_level) in levels.into_iter() {
            assert_eq!(LogLevel::from(score_log_level), ffi_level);
        }
    }

    #[test]
    fn test_score_log_level_from_log_level() {
        let levels = [
            (score_log::Level::Fatal, LogLevel::Fatal),
            (score_log::Level::Error, LogLevel::Error),
            (score_log::Level::Warn, LogLevel::Warn),
            (score_log::Level::Info, LogLevel::Info),
            (score_log::Level::Debug, LogLevel::Debug),
            (score_log::Level::Trace, LogLevel::Verbose),
        ];

        for (score_log_level, ffi_level) in levels.into_iter() {
            assert_eq!(score_log::Level::from(ffi_level), score_log_level);
        }
    }

    #[test]
    fn test_score_log_level_filter_from_log_level() {
        let levels = [
            (score_log::LevelFilter::Off, LogLevel::Off),
            (score_log::LevelFilter::Fatal, LogLevel::Fatal),
            (score_log::LevelFilter::Error, LogLevel::Error),
            (score_log::LevelFilter::Warn, LogLevel::Warn),
            (score_log::LevelFilter::Info, LogLevel::Info),
            (score_log::LevelFilter::Debug, LogLevel::Debug),
            (score_log::LevelFilter::Trace, LogLevel::Verbose),
        ];

        for (score_log_level_filter, ffi_level) in levels.into_iter() {
            assert_eq!(score_log::LevelFilter::from(ffi_level), score_log_level_filter);
        }
    }

    #[test]
    fn test_context_single_char() {
        let context = Context::from("X");
        assert_eq!(context.size, 1);
        assert_eq!(context.data, [88, 0, 0, 0]);
    }

    #[test]
    fn test_context_four_chars() {
        let context = Context::from("TEST");
        assert_eq!(context.size, 4);
        assert_eq!(context.data, [84, 69, 83, 84]);
    }

    #[test]
    fn test_context_trimmed() {
        let context = Context::from("trimmed");
        assert_eq!(context.size, 4);
        assert_eq!(context.data, [116, 114, 105, 109]);
    }

    #[test]
    #[should_panic(expected = "provided context contains non-ASCII characters: ")]
    fn test_context_non_ascii() {
        let _ = Context::from("❌");
    }
}
