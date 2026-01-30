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

#pragma once

#include <optional>
#include <string>

namespace score::mw::log::rust
{

/// @brief
/// Builder for logger used by Rust libraries.
///
/// @note
/// If parameter is not set explicitly then Rust-side default is used.
/// Only global logger setup is allowed.
/// `config` method is not exposed.
class ScoreLoggerBuilder final
{
  public:
    /// @brief Set context for the logger.
    /// @param context
    /// Context name.
    /// Only ASCII characters are allowed.
    /// Max 4 characters are used. Rest of the provided string will be trimmed.
    /// @return This builder.
    ScoreLoggerBuilder& Context(const std::string& context) noexcept;

    /// @brief Show module name in logs.
    /// @param show_module Value to set.
    /// @return This builder.
    ScoreLoggerBuilder& ShowModule(bool show_module) noexcept;

    /// @brief Show file name in logs.
    /// @param show_module Value to set.
    /// @return This builder.
    ScoreLoggerBuilder& ShowFile(bool show_file) noexcept;

    /// @brief Show line number in logs.
    /// @param show_module Value to set.
    /// @return This builder.
    ScoreLoggerBuilder& ShowLine(bool show_line) noexcept;

    /// @brief Initialize default logger with provided parameters.
    void SetAsDefaultLogger() noexcept;

  private:
    std::optional<std::string> context_;
    std::optional<bool> show_module_;
    std::optional<bool> show_file_;
    std::optional<bool> show_line_;
};

}  // namespace score::mw::log::rust
