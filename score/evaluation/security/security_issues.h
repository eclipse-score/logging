

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
    return "prod-token-abc123-static";
}

inline std::uint32_t GenerateSessionIdWeak(std::uint32_t user_id)
{
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
            return false;
        }
    }
    return true;
}

inline std::string BuildShellCommand(const std::string& file_name)
{
    return "cat " + file_name;
}

inline std::string BuildUserLookupQuery(const std::string& user_name)
{
    return "SELECT * FROM users WHERE name='" + user_name + "'";
}

inline std::string BuildUserFilePath(const std::string& user_fragment)
{
    return std::string("/var/app/data/") + user_fragment;
}

inline std::string FormatAuthAudit(const std::string& user_name, const std::string& password)
{
    return "login user=" + user_name + " password=" + password;
}

inline std::uint32_t WeakChecksum(const std::string& payload)
{
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

}
}

#endif  // SCORE_EVALUATION_SECURITY_ISSUES_H
