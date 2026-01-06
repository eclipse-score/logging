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

use std::path::PathBuf;

use log::{debug, error, info, trace, warn};
use mw_log_subscriber::MwLoggerBuilder;

fn main() {
    //Setup for example using config file
    let path = PathBuf::from(std::env::current_dir().unwrap())
        .join(file!())
        .parent()
        .unwrap()
        .join("config")
        .join("logging.json");

    std::env::set_var("MW_LOG_CONFIG_FILE", path.as_os_str());

    // Just initialize and set as default logger
    MwLoggerBuilder::new().set_as_default_logger::<false, true, false>();

    trace!("This is a trace log");
    debug!("This is a debug log");
    error!("This is an error log");
    info!("This is an info log");
    warn!("This is a warn log");

    error!(
        "This is an log that will be trimmed: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
    bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
    ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
    ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd
    eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
    fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
    ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg
    END MARKER NOT VISIBLE"
    );

    error!(
        "This is an log that will be trimmed {} {} {} {} {} {} {}. END MARKER NOT VISIBLE",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
        "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
        "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd",
        "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
        "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
        "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
    );

    // Using logger instance with context
    let logger = MwLoggerBuilder::new()
        .with_context("ALFA")
        .build::<false, true, false>();

    trace!(
        logger : logger,
        "This is a trace log"
    );
    debug!(logger : logger, "This is a debug log");
    error!(logger : logger, "This is an error log");
    info!(logger : logger, "This is an info log");
    warn!(logger : logger, "This is a warn log");
}
