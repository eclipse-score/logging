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
#include "gtest/gtest.h"

#include "score/os/mocklib/mock_syslog.h"
#include "score/mw/log/configuration/configuration.h"
#include "score/mw/log/detail/syslog/syslog_backend.h"

#include "score/assert_support.hpp"

#include <syslog.h>

#include <limits>

namespace score
{
namespace mw
{
namespace log
{
namespace detail
{
namespace
{

using ::testing::_;
using ::testing::StrEq;

const std::string kDefaultApp{"a1"};
const std::string kDefaultContext{"c1"};
const std::string kDefaultMessage{"default message"};

struct SyslogBackendFixture : ::testing::Test
{
    void SetUp() override
    {
        syslog_mock_ = score::cpp::pmr::make_unique<score::os::MockSyslog>(score::cpp::pmr::get_default_resource());
        syslog_mock_raw_ptr_ = syslog_mock_.get();
    }

  protected:
    void SimulateLogging(LogLevel log_level,
                         const std::string& app_id = kDefaultApp,
                         const std::string& ctx_id = kDefaultContext,
                         const std::string& message = kDefaultMessage)
    {
        SyslogBackend backend(config_.GetNumberOfSlots(), log_record_, app_id, std::move(syslog_mock_));

        auto slot = backend.ReserveSlot();
        EXPECT_TRUE(slot.has_value());

        auto& payload = backend.GetLogRecord(slot.value());
        auto& log_entry = payload.GetLogEntry();
        log_entry.ctx_id = LoggingIdentifier(std::string_view(ctx_id));
        log_entry.log_level = log_level;
        log_entry.payload = ByteVector(message.begin(), message.end());

        backend.FlushSlot(slot.value());
    }

    LogRecord log_record_{};
    Configuration config_{};
    score::cpp::pmr::unique_ptr<score::os::MockSyslog> syslog_mock_{};
    score::os::MockSyslog* syslog_mock_raw_ptr_;
};

TEST_F(SyslogBackendFixture, SyslogOpenlog)
{
    RecordProperty("Description", "Verifies the backend opens the syslog connection on construction.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _));

    SyslogBackend unit(config_.GetNumberOfSlots(), log_record_, config_.GetAppId(), std::move(syslog_mock_));
}

TEST_F(SyslogBackendFixture, SyslogOpenlogWithCapacityBiggerThanTheMaximum)
{
    RecordProperty("Description", "Verifies backend construction with slots' capacity bigger than the maximum.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    auto capacity = std::numeric_limits<SlotIndex>::max() + 1;
    config_.SetNumberOfSlots(capacity);
    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _));

    SyslogBackend unit(config_.GetNumberOfSlots(), log_record_, config_.GetAppId(), std::move(syslog_mock_));
}

TEST_F(SyslogBackendFixture, ReserveSlotShouldAcquireSlot)
{
    RecordProperty("Description", "Verifies the ability of reserving slot.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _));

    SyslogBackend unit(config_.GetNumberOfSlots(), log_record_, config_.GetAppId(), std::move(syslog_mock_));

    auto slot = unit.ReserveSlot();
    EXPECT_TRUE(slot.has_value());
}

TEST_F(SyslogBackendFixture, LevelOffProducesNoEmit)
{
    RecordProperty("Description", "A kOff level shall not be emitted to syslog.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(_, _)).Times(0);

    SimulateLogging(LogLevel::kOff);
}

TEST_F(SyslogBackendFixture, FatalLog)
{
    RecordProperty("Description", "Verifies the ability of logging fatal message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_CRIT, _)).Times(1);

    SimulateLogging(LogLevel::kFatal);
}

TEST_F(SyslogBackendFixture, ErrorLog)
{
    RecordProperty("Description", "Verifies the ability of logging error message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_ERR, _)).Times(1);

    SimulateLogging(LogLevel::kError);
}

TEST_F(SyslogBackendFixture, WarningLog)
{
    RecordProperty("Description", "Verifies the ability of logging warning message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_WARNING, _)).Times(1);

    SimulateLogging(LogLevel::kWarn);
}

TEST_F(SyslogBackendFixture, InfoLog)
{
    RecordProperty("Description", "Verifies the ability of logging info message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_INFO, _)).Times(1);

    SimulateLogging(LogLevel::kInfo);
}

TEST_F(SyslogBackendFixture, DebugLog)
{
    RecordProperty("Description", "Verifies the ability of logging debug message.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_DEBUG, _)).Times(1);

    SimulateLogging(LogLevel::kDebug);
}

TEST_F(SyslogBackendFixture, VerboseLog)
{
    RecordProperty("Description", "Verifies verbose maps to LOG_DEBUG (syslog has no separate verbose level).");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_DEBUG, _)).Times(1);

    SimulateLogging(LogLevel::kVerbose);
}

TEST_F(SyslogBackendFixture, MessageShouldContainAppCtxPayload)
{
    RecordProperty("Description", "Verifies log message contains application id, context id and payload.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_DEBUG, StrEq("MyAp,MyCt: Hello World"))).Times(1);

    SimulateLogging(LogLevel::kVerbose, "MyAp", "MyCt", "Hello World");
}

TEST_F(SyslogBackendFixture, BackendShouldHandleEmptyPayload)
{
    RecordProperty("Description", "Verifies the ability of the backend of handling empty payload.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_DEBUG, StrEq(",: "))).Times(1);

    SimulateLogging(LogLevel::kVerbose, "", "", "");
}

TEST_F(SyslogBackendFixture, LongIdentifiersShouldBeCropped)
{
    RecordProperty("Description",
                   "Verifies that the application or context IDs should be cropped if it exceeds 4 characters length.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(LOG_DEBUG, StrEq("1234,4567: "))).Times(1);

    SimulateLogging(LogLevel::kVerbose, "12345", "45678", "");
}

TEST_F(SyslogBackendFixture, NoSlotAvailableShouldReturnEmptyHandle)
{
    RecordProperty("Description", "Verifies returning empty handler in case of no available slots.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    SyslogBackend backend(config_.GetNumberOfSlots(), log_record_, config_.GetAppId(), std::move(syslog_mock_));

    for (std::size_t i = 0; i < config_.GetNumberOfSlots(); ++i)
    {
        EXPECT_TRUE(backend.ReserveSlot().has_value());
    }

    EXPECT_FALSE(backend.ReserveSlot().has_value());
}

TEST_F(SyslogBackendFixture, TooMuchSlotsRequestedShallBeTruncated)
{
    RecordProperty("Description", "Verifies requesting too much slots shall be truncated.");
    RecordProperty("TestingTechnique", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    const auto kMaxSlotCount = std::numeric_limits<SlotIndex>::max();
    const std::size_t kSlotNumberOverflow = static_cast<std::size_t>(kMaxSlotCount) + 2UL;

    SyslogBackend backend(kSlotNumberOverflow, log_record_, config_.GetAppId(), std::move(syslog_mock_));

    for (std::size_t i = 0; i < kMaxSlotCount; ++i)
    {
        EXPECT_TRUE(backend.ReserveSlot().has_value());
    }

    EXPECT_FALSE(backend.ReserveSlot().has_value());
}

TEST_F(SyslogBackendFixture, ToSyslogPriorityInvalidLevel)
{
    RecordProperty("Description", "Tests ToSyslogPriority with an invalid log level, which must not be emitted.");
    RecordProperty("TestingTechnique", "Boundary value analysis");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    EXPECT_CALL(*syslog_mock_raw_ptr_, openlog(_, _, _)).Times(1);
    EXPECT_CALL(*syslog_mock_raw_ptr_, MockedSyslog(_, _)).Times(0);

    // Pass a log level greater than GetMaxLogLevelValue() to trigger the `else` branch (kInvalid, no emit).
    LogLevel invalid_log_level = static_cast<LogLevel>(static_cast<std::uint8_t>(LogLevel::kVerbose) + 1);
    SimulateLogging(invalid_log_level);
}

}  // namespace
}  // namespace detail
}  // namespace log
}  // namespace mw
}  // namespace score
