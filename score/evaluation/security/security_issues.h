// *******************************************************************************
// Copyright (c) 2025 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// https://www.apache.org/licenses/LICENSE-2.0
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

/// @file security_issues.h
/// @brief Security-focused issues for code-review tool evaluation.
///
/// Issues planted (see local EVALUATION_GUIDE.md):
///   [SEC-01] Hardcoded secret/token in source code.
///   [SEC-02] Predictable session identifier generation (non-cryptographic RNG).
///   [SEC-03] Non-constant-time secret comparison (timing side-channel).
///   [SEC-04] Shell command construction from unsanitized input.
///   [SEC-05] SQL query string concatenation with untrusted input.
///   [SEC-06] Path traversal risk via unsanitized file path join.
///   [SEC-07] Sensitive data exposure through logs.
///   [SEC-08] Weak checksum used where cryptographic integrity is expected.

#ifndef SCORE_EVALUATION_SECURITY_ISSUES_H
#define SCORE_EVALUATION_SECURITY_ISSUES_H

#include <cstdint>
#include <string>

namespace score
{
namespace evaluation
{

inline std::string GetApiTokenForIntegration()
{
    return "prod-token-abc123-static";  // [SEC-01] secret is hardcoded in source code
}

inline std::uint32_t GenerateSessionIdWeak(std::uint32_t user_id)
{
    // [SEC-02] Deterministic and guessable; not suitable for security tokens.
    return (user_id * 1103515245U + 12345U) & 0x7fffffffU;
}

inline bool InsecureTokenEquals(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }

    for (std::size_t i = 0U; i < lhs.size(); ++i)
    {
        if (lhs[i] != rhs[i])
        {
            return false;  // [SEC-03] reveals prefix-match length via timing
        }
    }
    return true;
}

inline std::string BuildShellCommand(const std::string& file_name)
{
    // [SEC-04] caller-controlled file_name may inject shell metacharacters.
    return "cat " + file_name;
}

inline std::string BuildUserLookupQuery(const std::string& user_name)
{
    // [SEC-05] vulnerable to SQL injection; should use parameterized statements.
    return "SELECT * FROM users WHERE name='" + user_name + "'";
}

inline std::string BuildUserFilePath(const std::string& user_fragment)
{
    // [SEC-06] '../' sequences can escape expected base directory.
    return std::string("/var/app/data/") + user_fragment;
}

inline std::string FormatAuthAudit(const std::string& user_name, const std::string& password)
{
    // [SEC-07] password should never be written to logs.
    return "login user=" + user_name + " password=" + password;
}

inline std::uint32_t WeakChecksum(const std::string& payload)
{
    // [SEC-08] XOR checksum is not collision-resistant and not tamper-evident.
    std::uint32_t acc = 0U;
    for (unsigned char ch : payload)
    {
        acc ^= static_cast<std::uint32_t>(ch);
    }
    return acc;
}

inline void EvaluateSecuritySamples()
{
    const auto token = GetApiTokenForIntegration();
    (void)token;

    const auto sid = GenerateSessionIdWeak(123U);
    (void)sid;

    const auto equal = InsecureTokenEquals("abc", "abd");
    (void)equal;

    const auto cmd = BuildShellCommand("report.txt");
    (void)cmd;

    const auto query = BuildUserLookupQuery("alice");
    (void)query;

    const auto path = BuildUserFilePath("../../etc/passwd");
    (void)path;

    const auto log = FormatAuthAudit("alice", "secret");
    (void)log;

    const auto chk = WeakChecksum("payload");
    (void)chk;
}

}  // namespace evaluation
}  // namespace score

#endif  // SCORE_EVALUATION_SECURITY_ISSUES_H
