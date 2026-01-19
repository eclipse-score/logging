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

//! [`score_log::fmt::ScoreWrite`] writer implementation used by [`crate::ScoreLogger`].

use crate::ffi::LogStream;
use score_log::fmt::{DisplayHint, Error, FormatSpec, Result as FmtResult, ScoreWrite};

/// [`score_log::fmt::ScoreWrite`] writer implementation used by [`crate::ScoreLogger`].
/// Adds values to the log stream with selected formatting.
pub(crate) struct ScoreLoggerWriter<'a> {
    log_stream: LogStream<'a>,
}

impl<'a> ScoreLoggerWriter<'a> {
    pub fn new(log_stream: LogStream<'a>) -> Self {
        Self { log_stream }
    }
}

impl ScoreWrite for ScoreLoggerWriter<'_> {
    fn write_bool(&mut self, v: &bool, _spec: &FormatSpec) -> FmtResult {
        self.log_stream.log_bool(v);
        Ok(())
    }

    fn write_f32(&mut self, v: &f32, _spec: &FormatSpec) -> FmtResult {
        self.log_stream.log_f32(v);
        Ok(())
    }

    fn write_f64(&mut self, v: &f64, _spec: &FormatSpec) -> FmtResult {
        self.log_stream.log_f64(v);
        Ok(())
    }

    fn write_i8(&mut self, v: &i8, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_i8(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                let v_u8 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_hex8(v_u8);
            },
            DisplayHint::Binary => {
                let v_u8 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_bin8(v_u8);
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_i16(&mut self, v: &i16, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_i16(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                let v_u16 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_hex16(v_u16);
            },
            DisplayHint::Binary => {
                let v_u16 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_bin16(v_u16);
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_i32(&mut self, v: &i32, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_i32(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                let v_u32 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_hex32(v_u32);
            },
            DisplayHint::Binary => {
                let v_u32 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_bin32(v_u32);
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_i64(&mut self, v: &i64, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_i64(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => {
                let v_u64 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_hex64(v_u64);
            },
            DisplayHint::Binary => {
                let v_u64 = unsafe { core::mem::transmute(v) };
                self.log_stream.log_bin64(v_u64);
            },
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_u8(&mut self, v: &u8, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_u8(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => self.log_stream.log_hex8(v),
            DisplayHint::Binary => self.log_stream.log_bin8(v),
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_u16(&mut self, v: &u16, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_u16(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => self.log_stream.log_hex16(v),
            DisplayHint::Binary => self.log_stream.log_bin16(v),
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_u32(&mut self, v: &u32, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_u32(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => self.log_stream.log_hex32(v),
            DisplayHint::Binary => self.log_stream.log_bin32(v),
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_u64(&mut self, v: &u64, spec: &FormatSpec) -> FmtResult {
        match spec.get_display_hint() {
            DisplayHint::NoHint => self.log_stream.log_u64(v),
            DisplayHint::LowerHex | DisplayHint::UpperHex => self.log_stream.log_hex64(v),
            DisplayHint::Binary => self.log_stream.log_bin64(v),
            _ => return Err(Error),
        }
        Ok(())
    }

    fn write_str(&mut self, v: &str, _spec: &FormatSpec) -> FmtResult {
        self.log_stream.log_string(v);
        Ok(())
    }
}
