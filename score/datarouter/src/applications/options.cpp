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

#include "options.h"

#include "score/mw/log/logging.h"
#include "score/os/getopt.h"

#include <array>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace
{

void EmitMessage(const std::string& msg)
{
    std::cerr << msg << "\n";
    score::mw::log::LogError() << "Error in command line:" << msg;
}

void PrintUsage(std::string_view program)
{
    std::cout
        << "Usage: " << program
        << " [options]\n"
           "Options:\n"
           "  -h, --help Print this message and exit.\n"
           "  -v, --verbose Display plenty of output to stdout.\n"
           "  -n, --no_adaptive_runtime Do not use the Vector stack. Persistentcy features will not be available.\n"
           "  -V, --version Print the version number of make and exit.\n"
           "  -c, --config <path> Path to the channel configuration (log-channels.json). Defaults to the "
           "built-in path when omitted.\n"
           "  -C, --nv-config <path> Path to the non-verbose configuration (class-id.json). Defaults to the "
           "built-in path when omitted.\n";
}

}  // namespace

namespace score
{
namespace logging
{
namespace options
{

Options::Options() : do_nothing_(false), print_version_(false), verbose_(false), no_adaptive_runtime_(false) {}

// NOLINTNEXTLINE(modernize-avoid-c-arrays): C style array is needed as it has to have main style arguments.
bool Options::Parse(std::int32_t argc, char* const argv[])
{
    Options& options = Options::Get();

    // Suppress getopt's built-in stderr output; diagnostics are emitted via EmitMessage.
    opterr = 0;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays): required by the getopt_long API.
    static const std::array<::option, 7> kLongOptions{{
        {"help", no_argument, nullptr, 'h'},
        {"verbose", no_argument, nullptr, 'v'},
        {"no_adaptive_runtime", no_argument, nullptr, 'n'},
        {"version", no_argument, nullptr, 'V'},
        {"config", required_argument, nullptr, 'c'},
        {"nv-config", required_argument, nullptr, 'C'},
        {nullptr, 0, nullptr, 0},
    }};

    score::os::Getopt& getopt_instance = score::os::Getopt::instance();
    std::int32_t longindex{0};
    std::int32_t opt{};

    // Leading ':' in optstring causes getopt_long to return ':' (not '?') on missing arguments.
    while ((opt = getopt_instance.getopt_long(argc, argv, ":hvnVc:C:", kLongOptions.data(), &longindex)) != -1)
    {
        switch (opt)
        {
            case 'h':
                PrintUsage(argv[0]);
                options.do_nothing_ = true;
                return true;

            case 'v':
                options.verbose_ = true;
                break;

            case 'n':
                options.no_adaptive_runtime_ = true;
                break;

            case 'V':
                options.print_version_ = true;
                return true;

            case 'c':  // -c / --config
            {
                const std::string_view value{optarg};
                if (value.empty())
                {
                    EmitMessage(R"(Missing value for option "-c/--config")");
                    return false;
                }
                options.config_path_ = std::string{value};
                break;
            }

            case 'C':  // -C / --nv-config
            {
                const std::string_view value{optarg};
                if (value.empty())
                {
                    EmitMessage(R"(Missing value for option "-C/--nv-config")");
                    return false;
                }
                options.nv_config_path_ = std::string{value};
                break;
            }

            case ':':
                EmitMessage(std::string{"Missing value for option \""} +
                            std::string{argv[getopt_instance.getoptind() - 1]} + "\"");
                return false;

            default:  // '?'
                EmitMessage(std::string{"Unknown option \""} + std::string{argv[getopt_instance.getoptind() - 1]} +
                            "\"");
                return false;
        }
    }

    return true;
}

Options& Options::Get()
{
    static Options options;
    return options;
}

}  // namespace options
}  // namespace logging
}  // namespace score
