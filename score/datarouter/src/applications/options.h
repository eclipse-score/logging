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

#ifndef LOGGING_APPLICATIONS_OPTIONS_H_
#define LOGGING_APPLICATIONS_OPTIONS_H_

#include <cstdint>

namespace score
{
namespace logging
{
namespace options
{

/*
Deviation from Rule M3-2-3:
- A type, object or function that is used in multiple translation units shall
be declared in one and only one file.
Justification:
- It's false positive. There's no other declaration of class Options anywhere.
*/
// coverity[autosar_cpp14_m3_2_3_violation : FALSE]
class Options
{
  public:
    // NOLINTNEXTLINE(modernize-avoid-c-arrays): C style array is needed as it has to have main style arguments.
    static bool parse(std::int32_t argc, char* const argv[]);
    static Options& get();

    bool do_nothing() const
    {
        return do_nothing_;
    }
    bool print_version() const
    {
        return print_version_;
    }
    bool verbose() const
    {
        return verbose_;
    }
    bool no_adaptive_runtime() const
    {
        return no_adaptive_runtime_;
    }

  private:
    Options();
    Options(const Options&) = delete;
    Options(Options&&) = delete;
    Options& operator=(const Options&) = delete;
    Options& operator=(Options&&) = delete;

    bool do_nothing_;
    bool print_version_;
    bool verbose_;
    bool no_adaptive_runtime_;
};

}  // namespace options
}  // namespace logging
}  // namespace score

#endif  // LOGGING_APPLICATIONS_OPTIONS_H_
