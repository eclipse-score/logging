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

#include "applications/datarouter_feature_config.h"
#include "gmock/gmock.h"
#include "stub_config_session.h"
#include "stub_config_session_factory.h"
#include <gtest/gtest.h>

using namespace testing;

namespace test
{

class StubConfigSessionUT : public Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    // Helper to create a test handle for stub mode
    score::platform::datarouter::ConfigSessionHandleType createTestHandle(int value = 42)
    {
        // In stub mode, can use simple int constructor
        return score::platform::datarouter::ConfigSessionHandleType{value};
    }
};

TEST_F(StubConfigSessionUT, Constructor_AcceptsHandleAndHandler)
{
    // Arrange
    auto handle = createTestHandle();
    auto handler = [](const std::string& /*cmd*/) -> std::string {
        return "response";
    };

    // Act & Assert - Should not throw
    EXPECT_NO_THROW({ score::logging::daemon::StubConfigSession session(std::move(handle), handler); });
}

TEST_F(StubConfigSessionUT, Constructor_AcceptsAnyHandlerType)
{
    // Arrange
    auto handle = createTestHandle();

    // Test with lambda
    auto lambda_handler = [](const std::string&) {
        return std::string{};
    };

    // Test with function pointer
    auto func_ptr_handler = +[](const std::string&) {
        return std::string{};
    };

    // Test with std::function
    std::function<std::string(const std::string&)> std_func_handler = [](const std::string&) {
        return std::string{};
    };

    // Act & Assert - All should construct successfully
    EXPECT_NO_THROW({
        score::logging::daemon::StubConfigSession session1(handle, lambda_handler);
        score::logging::daemon::StubConfigSession session2(handle, func_ptr_handler);
        score::logging::daemon::StubConfigSession session3(handle, std_func_handler);
    });
}

TEST_F(StubConfigSessionUT, Tick_ReturnsTrue)
{
    // Arrange
    auto handle = createTestHandle();
    auto handler = [](const std::string&) {
        return std::string{};
    };
    score::logging::daemon::StubConfigSession session(std::move(handle), handler);

    // Act
    bool result = session.tick();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(StubConfigSessionUT, OnCommand_DoesNothing)
{
    // Arrange
    auto handle = createTestHandle();
    auto handler = [](const std::string&) {
        return std::string{};
    };
    score::logging::daemon::StubConfigSession session(std::move(handle), handler);

    // Act & Assert - Should not throw or crash
    EXPECT_NO_THROW({
        session.on_command("test command");
        session.on_command("");
        session.on_command("another command");
    });
}

TEST_F(StubConfigSessionUT, OnClosedByPeer_DoesNothing)
{
    // Arrange
    auto handle = createTestHandle();
    auto handler = [](const std::string&) {
        return std::string{};
    };
    score::logging::daemon::StubConfigSession session(std::move(handle), handler);

    // Act & Assert - Should not throw or crash
    EXPECT_NO_THROW({
        session.on_closed_by_peer();
        session.on_closed_by_peer();  // Can be called multiple times
    });
}

TEST_F(StubConfigSessionUT, IsSessionInterface)
{
    // Arrange
    auto handle = createTestHandle();
    auto handler = [](const std::string&) {
        return std::string{};
    };

    // Act - Create session and cast to base interface
    std::unique_ptr<score::logging::ISession> session =
        std::make_unique<score::logging::daemon::StubConfigSession>(std::move(handle), handler);

    // Assert - Should be able to call interface methods
    EXPECT_NO_THROW({
        EXPECT_TRUE(session->tick());
        session->on_command("test");
        session->on_closed_by_peer();
    });
}

// ================================================================================================
// StubConfigSessionFactory Tests
// ================================================================================================

class StubConfigSessionFactoryUT : public Test
{
  protected:
    void SetUp() override {}
    void TearDown() override {}

    // Helper to create a test handle for stub mode
    score::platform::datarouter::ConfigSessionHandleType createTestHandle(int value = 42)
    {
        // In stub mode, can use simple int constructor
        return score::platform::datarouter::ConfigSessionHandleType{value};
    }
};

TEST_F(StubConfigSessionFactoryUT, CreateConcreteSession_ReturnsValidSession)
{
    // Arrange
    score::logging::daemon::StubConfigSessionFactory factory;
    auto handle = createTestHandle(123);
    auto handler = [](const std::string& /*cmd*/) -> std::string {
        return "test_response";
    };

    // Act
    auto session = factory.CreateConcreteSession(std::move(handle), handler);

    // Assert
    ASSERT_NE(session, nullptr);
    EXPECT_TRUE(session->tick());
}

TEST_F(StubConfigSessionFactoryUT, CreateConfigSession_ReturnsValidSession)
{
    // Arrange
    score::logging::daemon::StubConfigSessionFactory factory;
    auto handle = createTestHandle(456);
    auto handler = [](const std::string& /*cmd*/) -> std::string {
        return "config_response";
    };

    // Act
    auto session = factory.CreateConfigSession(std::move(handle), handler);

    // Assert
    ASSERT_NE(session, nullptr);
    EXPECT_TRUE(session->tick());
}

TEST_F(StubConfigSessionFactoryUT, CreateConfigSession_WorksWithDifferentHandlerTypes)
{
    // Arrange
    score::logging::daemon::StubConfigSessionFactory factory;
    auto handle = createTestHandle(789);

    // Test with lambda
    auto lambda_handler = [](const std::string& /*cmd*/) -> std::string {
        return "lambda";
    };

    // Test with std::function
    std::function<std::string(const std::string&)> std_func_handler = [](const std::string& /*cmd*/) -> std::string {
        return "std_function";
    };

    // Act & Assert
    EXPECT_NO_THROW({
        auto session1 = factory.CreateConfigSession(handle, lambda_handler);
        ASSERT_NE(session1, nullptr);

        auto session2 = factory.CreateConfigSession(handle, std_func_handler);
        ASSERT_NE(session2, nullptr);
    });
}

TEST_F(StubConfigSessionFactoryUT, MultipleSessionCreation)
{
    // Arrange
    score::logging::daemon::StubConfigSessionFactory factory;
    auto handler = [](const std::string& /*cmd*/) -> std::string {
        return "response";
    };

    // Act - Create multiple sessions
    std::vector<std::unique_ptr<score::logging::ISession>> sessions;
    for (int i = 0; i < 5; ++i)
    {
        auto handle = createTestHandle(i);
        sessions.push_back(factory.CreateConfigSession(std::move(handle), handler));
    }

    // Assert - All sessions should be valid and functional
    for (const auto& session : sessions)
    {
        ASSERT_NE(session, nullptr);
        EXPECT_TRUE(session->tick());
        EXPECT_NO_THROW({
            session->on_command("test");
            session->on_closed_by_peer();
        });
    }
}

}  // namespace test
