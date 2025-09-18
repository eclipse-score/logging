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

mod mw_log_ffi;

use crate::mw_log_ffi::*;

use core::ffi::c_char;
use core::fmt::{self, Write};
use mw_log::{Level, Log, Metadata, Record};
use std::ffi::CString;
use std::mem::MaybeUninit;

const MSG_SIZE: usize = 512;

/// Builder for the MwLogger
pub struct MwLoggerBuilder {
    context: Option<CString>,
}

impl MwLoggerBuilder {
    pub fn new() -> Self {
        Self { context: None }
    }

    /// Builds the MwLogger with the specified context and configuration and returns it.
    pub fn build<const SHOW_MODULE: bool, const SHOW_FILE: bool, const SHOW_LINE: bool>(
        self,
    ) -> MwLogger {
        let context_cstr = self.context.unwrap_or(CString::new("DFLT").unwrap());
        let c_logger_ptr = unsafe { mw_log_create_logger(context_cstr.as_ptr() as *const _) };
        MwLogger {
            ptr: c_logger_ptr,
            log_fn: log::<SHOW_MODULE, SHOW_FILE, SHOW_LINE>,
        }
    }

    /// Builds and sets the MwLogger as the default logger with the specified configuration.
    pub fn set_as_default_logger<
        const SHOW_MODULE: bool,
        const SHOW_FILE: bool,
        const SHOW_LINE: bool,
    >(
        self,
    ) {
        let logger = self.build::<SHOW_MODULE, SHOW_FILE, SHOW_LINE>();
        mw_log::set_max_level(mw_log_logger_level(logger.ptr));
        mw_log::set_boxed_logger(Box::new(logger))
            .expect("Failed to initialize MwLogger as default logger - logger may already be set");
    }

    /// Sets the context for currently build logger.
    pub fn with_context(mut self, context: &str) -> Self {
        self.context = Some(CString::new(context).expect(
            "Failed to create CString:
             input contains null bytes",
        ));
        self
    }
}

/// A simple buffer writer that implements `core::fmt::Write`
/// and writes into a fixed-size(`BUF_SIZE) buffer.
/// Used in `Log` implementation to format log messages
/// before passing them to the underlying C++ logger.
struct BufWriter<const BUF_SIZE: usize> {
    buf: [MaybeUninit<u8>; BUF_SIZE],
    pos: usize,
}

impl<const BUF_SIZE: usize> BufWriter<BUF_SIZE> {
    fn new() -> Self {
        Self {
            buf: [MaybeUninit::uninit(); BUF_SIZE],
            pos: 0,
        }
    }

    /// Returns a slice to filled part of buffer. This is not null-terminated.
    fn as_slice(&self) -> &[c_char] {
        // SAFETY: We only expose already initialized part of the buffer
        unsafe { core::slice::from_raw_parts(self.buf.as_ptr().cast::<c_char>(), self.len()) }
    }

    /// Returns the current length of the filled part of the buffer.
    fn len(&self) -> usize {
        self.pos
    }

    /// Reverts the current position (consumes) by `cnt` bytes, saturating at 0.
    fn revert_pos(&mut self, cnt: usize) {
        self.pos = self.pos.saturating_sub(cnt);
    }

    // Finds the right index (around requested `index`) to trim utf8 string to a valid char boundary
    fn floor_char_boundary(view: &str, index: usize) -> usize {
        if index >= view.len() {
            view.len()
        } else {
            let lower_bound = index.saturating_sub(3);
            let new_index = view.as_bytes()[lower_bound..=index]
                .iter()
                .rposition(|b| Self::is_utf8_char_boundary(*b));

            // SAFETY: we know that the character boundary will be within four bytes
            unsafe { lower_bound + new_index.unwrap_unchecked() }
        }
    }

    const fn is_utf8_char_boundary(i: u8) -> bool {
        // This is bit magic equivalent to: b < 128 || b >= 192
        (i as i8) >= -0x40
    }
}

impl<const BUF_SIZE: usize> Write for BufWriter<BUF_SIZE> {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        let bytes = s.as_bytes();
        let to_write = bytes.len().min(self.buf.len() - self.pos - 1); // Cutting write, instead error when we don't fit
        let bounded_to_write = Self::floor_char_boundary(s, to_write);

        if bounded_to_write == 0 {
            return Err(fmt::Error);
        }

        let dest = self.buf[self.pos..self.pos + bounded_to_write]
            .as_mut_ptr()
            .cast::<u8>();

        unsafe { core::ptr::copy_nonoverlapping(bytes.as_ptr(), dest, bounded_to_write) };

        self.pos += bounded_to_write;
        Ok(())
    }
}

pub struct MwLogger {
    ptr: *const Logger,
    log_fn: fn(&mut BufWriter<MSG_SIZE>, &Record),
}

// SAFETY: The underlying C++ logger is known to be safe to change thread
unsafe impl Send for MwLogger {}

// SAFETY: The underlying C++ logger is known to be thread-safe.
unsafe impl Sync for MwLogger {}

impl MwLogger {
    fn write_log(&self, level: Level, msg: &BufWriter<MSG_SIZE>) {
        let slice = msg.as_slice();

        unsafe {
            match level {
                Level::Error => mw_log_error_logger(self.ptr, slice.as_ptr(), slice.len() as u32),
                Level::Warn => mw_log_warn_logger(self.ptr, slice.as_ptr(), slice.len() as u32),
                Level::Info => mw_log_info_logger(self.ptr, slice.as_ptr(), slice.len() as u32),
                Level::Debug => mw_log_debug_logger(self.ptr, slice.as_ptr(), slice.len() as u32),
                Level::Trace => mw_log_verbose_logger(self.ptr, slice.as_ptr(), slice.len() as u32),
            }
        }
    }
}

impl Log for MwLogger {
    fn enabled(&self, metadata: &Metadata) -> bool {
        mw_log_is_log_level_enabled(self.ptr, metadata.level())
    }
    fn log(&self, record: &Record) {
        if !self.enabled(record.metadata()) {
            return;
        }

        let mut msg_writer = BufWriter::<MSG_SIZE>::new();
        (self.log_fn)(&mut msg_writer, record);

        self.write_log(record.level(), &msg_writer);
    }

    fn flush(&self) {
        // No-op for this logger, as it does not buffer logs
    }
}

fn log<const SHOW_MODULE: bool, const SHOW_FILE: bool, const SHOW_LINE: bool>(
    msg_writer: &mut BufWriter<MSG_SIZE>,
    record: &Record,
) {
    if SHOW_FILE || SHOW_LINE || SHOW_MODULE {
        let _ = write!(msg_writer, "[");
        if SHOW_MODULE {
            if let Some(module) = record.module_path() {
                let _ = write!(msg_writer, "{}:", module);
            }
        }
        if SHOW_FILE {
            if let Some(file) = record.file() {
                let _ = write!(msg_writer, "{}:", file);
            }
        }
        if SHOW_LINE {
            if let Some(line) = record.line() {
                let _ = write!(msg_writer, "{}:", line);
            }
        }

        msg_writer.revert_pos(1);
        let _ = write!(msg_writer, "] ");
    }

    let _ = msg_writer.write_fmt(*record.args());
}
