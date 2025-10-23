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

use core::ffi::c_char;

use log::{Level, LevelFilter};

// Opaque type representing the C++ logger ptr
#[repr(C)]
pub(crate) struct Logger {
    _private: [u8; 0], // Opaque
}

pub(crate) fn mw_log_logger_level(logger: *const Logger) -> LevelFilter {
    let level = unsafe { mw_log_logger_level_internal(logger) };
    log_level_from_ffi(level)
}

pub(crate) fn mw_log_is_log_level_enabled(logger: *const Logger, level: Level) -> bool {
    let level_byte = match level {
        Level::Error => 0x02,
        Level::Warn => 0x03,
        Level::Info => 0x04,
        Level::Debug => 0x05,
        Level::Trace => 0x06,
    };
    unsafe { mw_log_is_log_level_enabled_internal(logger, level_byte) }
}

/// Get the max log level from C++ as a LevelFilter directly
fn log_level_from_ffi(level: u8) -> LevelFilter {
    match level {
        0x00 => LevelFilter::Off,
        0x01 => LevelFilter::Error, // Currently Fatal treated as Error
        0x02 => LevelFilter::Error,
        0x03 => LevelFilter::Warn,
        0x04 => LevelFilter::Info,
        0x05 => LevelFilter::Debug,
        0x06 => LevelFilter::Trace, // Verbose is Trace
        _ => LevelFilter::Info,     // fallback
    }
}

extern "C" {

    pub(crate) fn mw_log_create_logger(context: *const c_char) -> *mut Logger;
    pub(crate) fn mw_log_error_logger(logger: *const Logger, message: *const c_char, len: u32);
    pub(crate) fn mw_log_warn_logger(logger: *const Logger, message: *const c_char, len: u32);
    pub(crate) fn mw_log_info_logger(logger: *const Logger, message: *const c_char, len: u32);
    pub(crate) fn mw_log_debug_logger(logger: *const Logger, message: *const c_char, len: u32);
    pub(crate) fn mw_log_verbose_logger(logger: *const Logger, message: *const c_char, len: u32);

    fn mw_log_is_log_level_enabled_internal(logger: *const Logger, level: u8) -> bool;
    fn mw_log_logger_level_internal(logger: *const Logger) -> u8;

}
