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

#include <score/span.hpp>
#include <getopt.h>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace
{

void emit_message(const std::string& msg)
{
    std::cerr << msg << "\n";
    score::mw::log::LogError() << "Error in command line:" << msg;
}

void report_error(const std::string& text, const char opt_char, const std::string& arg)
{
    std::string msg = text;

    msg += " option \"";

    //  If a long option, take it as it is.
    //  Otherwise use the single option character.
    if (arg.rfind("--", 0) == 0)
    {
        msg += arg;
    }
    else
    {
        msg += opt_char;
    }

    msg += "\"";

    emit_message(msg);
}

void print_usage(std::string_view program)
{
    std::cout
        << "Usage: " << program
        << " [options]\n"
           "Options:\n"
           "  -h, --help Print this message and exit.\n"
           "  -v, --verbose Display plenty of output to stdout.\n"
           "  -n, --no_adaptive_runtime Do not use the Vector stack. Persistentcy features will not be available.\n"
           "  -V, --version Print the version number of make and exit.\n";
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
bool Options::parse(std::int32_t argc, char* const argv[])
{
    Options& options = Options::get();

    score::cpp::span<char* const> args(argv, static_cast<score::cpp::span<char* const>::size_type>(argc));

    for (std::int32_t arg_index = 1; arg_index < argc; ++arg_index)
    {
        std::string_view argument_token{score::cpp::at(args, arg_index)};

        // Long options: --help, --verbose, --no_adaptive_runtime, --version
        if (argument_token.size() > 2 && argument_token[0] == '-' && argument_token[1] == '-')
        {
            std::string_view long_option_name = argument_token.substr(2);
            if (long_option_name == "help")
            {
                print_usage(args.front());
                options.do_nothing_ = true;
                return true;
            }
            else if (long_option_name == "verbose")
            {
                options.verbose_ = true;
            }
            else if (long_option_name == "no_adaptive_runtime")
            {
                options.no_adaptive_runtime_ = true;
            }
            else if (long_option_name == "version")
            {
                options.print_version_ = true;
                return true;
            }
            else
            {
                report_error("Unknown", '?', score::cpp::at(args, arg_index));
                return false;
            }
        }

        // Short options: -h, -v, -n, -V
        else if (argument_token.size() >= 2 && argument_token[0] == '-')
        {
            // In some "case" below we are not returning directly to support the grouping of options, for example : -vn
            for (size_t short_option_index = 1; short_option_index < argument_token.size(); ++short_option_index)
            {
                char short_option_char = argument_token[short_option_index];
                // using help or version results in exiting the parse function.
                switch (short_option_char)
                {
                    case 'h':
                        print_usage(args.front());
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

                    default:
                        report_error("Unknown", short_option_char, score::cpp::at(args, arg_index));
                        return false;
                }
            }
        }
    }

    return true;
}

Options& Options::get()
{
    static Options options;
    return options;
}

}  // namespace options
}  // namespace logging
}  // namespace score
