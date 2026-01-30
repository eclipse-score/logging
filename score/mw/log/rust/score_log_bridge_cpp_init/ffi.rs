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
use core::slice::from_raw_parts;
use score_log_bridge::ScoreLoggerBuilder;

#[no_mangle]
extern "C" fn set_default_logger(
    context_ptr: *const c_char,
    context_size: usize,
    show_module: *const bool,
    show_file: *const bool,
    show_line: *const bool,
) {
    let mut builder = ScoreLoggerBuilder::new();

    // Set parameters if non-null (option-like).
    if !context_ptr.is_null() {
        let context = unsafe {
            let slice = from_raw_parts(context_ptr.cast(), context_size);
            str::from_utf8(slice).expect("provided context is not a valid UTF-8 string")
        };
        builder = builder.context(context);
    }

    if !show_module.is_null() {
        builder = builder.show_module(unsafe { *show_module });
    }

    if !show_file.is_null() {
        builder = builder.show_file(unsafe { *show_file });
    }

    if !show_line.is_null() {
        builder = builder.show_line(unsafe { *show_line });
    }

    builder.set_as_default_logger();
}
