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

//! C++-based backend for `score_log`.

#![warn(missing_docs)]
#![warn(clippy::std_instead_of_core)]
#![warn(clippy::alloc_instead_of_core)]

mod ffi;
mod score_logger;
mod score_logger_writer;

pub use crate::score_logger::{ScoreLogger, ScoreLoggerBuilder};
