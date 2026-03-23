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

/// @file unit_test_quality_test.cpp
/// @brief Deliberately flawed GoogleTest suite for code-review tool evaluation.
///
/// Issues planted (see EVALUATION_GUIDE.md §11 for expected review comments):
///
///  TokenBucket tests
///   [UT-01] Test with no assertion – verifies nothing.
///   [UT-02] Single happy-path test; boundary / negative cases missing.
///   [UT-03] Hard-coded magic numbers in assertions with no explanation.
///   [UT-04] Constructor exception NOT tested.
///
///  StringProcessor tests
///   [UT-05] Copy-paste test body – two tests are byte-for-byte identical.
///   [UT-06] Only one delimiter tested in Split(); empty / multi-token edge cases absent.
///   [UT-07] Test name is misleading / generic ("Test1", "Test2").
///
///  CircularBuffer tests
///   [UT-08] Overwrite-when-full behaviour not tested.
///   [UT-09] Pop() on empty buffer exception NOT tested.
///   [UT-10] Test uses implementation-internal knowledge (white-box) without comment.
///
///  PaymentProcessor / mock tests
///   [UT-11] Mock never sets up EXPECT_CALL – mock methods never verified.
///   [UT-12] Test calls production code and asserts on mock internal state directly
///           (brittle: breaks on any internal refactor).
///   [UT-13] Failure path (gateway returns false) is never exercised.

#include "score/evaluation/unit_test_quality/unit_test_quality.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace score
{
namespace evaluation
{
namespace test
{

// ===========================================================================
// TokenBucket tests
// ===========================================================================

// ---------------------------------------------------------------------------
// [UT-01] Test body contains no assertion – the test always passes regardless
//         of what TryConsume() actually returns.
// ---------------------------------------------------------------------------
TEST(TokenBucketTest, ConsumeDoesNotCrash)
{
    TokenBucket bucket(10, 2);
    bucket.TryConsume(3);   // [UT-01] return value discarded; no EXPECT/ASSERT
}

// ---------------------------------------------------------------------------
// [UT-02] Only the "enough tokens" path is exercised.
//         Missing: consume more than available, consume exactly 0, consume
//         exactly capacity, multiple consecutive consumes, refill cap at capacity.
// ---------------------------------------------------------------------------
TEST(TokenBucketTest, ConsumeSucceedsWhenEnoughTokens)
{
    TokenBucket bucket(10, 2);
    EXPECT_TRUE(bucket.TryConsume(5));
    EXPECT_EQ(bucket.AvailableTokens(), 5);
}

// ---------------------------------------------------------------------------
// [UT-03] Magic literals 10, 2, 7, 3 with no explanation of what they represent
//         or why these specific values exercise meaningful behaviour.
// ---------------------------------------------------------------------------
TEST(TokenBucketTest, RefillAddsTokens)
{
    TokenBucket bucket(10, 2);   // [UT-03] why 10 capacity, 2 refill?
    bucket.TryConsume(7);        // [UT-03] why consume 7?
    bucket.Refill();
    EXPECT_EQ(bucket.AvailableTokens(), 5);  // [UT-03] 10-7+2=5, but not documented
}

// ---------------------------------------------------------------------------
// [UT-04] The constructor throws std::invalid_argument for non-positive arguments,
//         but this is never tested.
//         Missing: EXPECT_THROW(TokenBucket(0, 1), std::invalid_argument);
//                  EXPECT_THROW(TokenBucket(5, 0), std::invalid_argument);
//                  EXPECT_THROW(TokenBucket(-1, 1), std::invalid_argument);
// ---------------------------------------------------------------------------
TEST(TokenBucketTest, ConstructWithValidArgs)
{
    EXPECT_NO_THROW(TokenBucket(5, 1));
    // [UT-04] Only the non-throwing path tested; negative/zero args never tested.
}

// ===========================================================================
// StringProcessor tests
// ===========================================================================

// ---------------------------------------------------------------------------
// [UT-05] Two tests have identical bodies – copy-paste error; one of them
//         covers nothing new and gives false confidence.
// ---------------------------------------------------------------------------
TEST(StringProcessorTest, TrimRemovesLeadingSpaces)
{
    EXPECT_EQ(StringProcessor::Trim("  hello"), "hello");
}

TEST(StringProcessorTest, TrimRemovesTrailingSpaces)
{
    EXPECT_EQ(StringProcessor::Trim("  hello"), "hello");  // [UT-05] identical to above
    // Should be: EXPECT_EQ(StringProcessor::Trim("hello  "), "hello");
}

// ---------------------------------------------------------------------------
// [UT-06] Split() tested with only a single simple case.
//         Missing edge cases: empty string, delimiter at start/end, consecutive
//         delimiters, string with no delimiter present, single-character string.
// ---------------------------------------------------------------------------
TEST(StringProcessorTest, SplitOnComma)
{
    const auto parts = StringProcessor::Split("a,b,c", ',');
    EXPECT_EQ(parts.size(), 3u);
    // [UT-06] Only the happy path; none of the boundary conditions checked.
}

// ---------------------------------------------------------------------------
// [UT-07] Generic, non-descriptive test names give no indication of what
//         behaviour is being verified or what the failure means.
// ---------------------------------------------------------------------------
TEST(StringProcessorTest, Test1)   // [UT-07] what does Test1 verify?
{
    EXPECT_EQ(StringProcessor::ToUpperCase("hello"), "HELLO");
}

TEST(StringProcessorTest, Test2)   // [UT-07] what does Test2 verify?
{
    EXPECT_EQ(StringProcessor::ToUpperCase("World"), "WORLD");
}

// ===========================================================================
// CircularBuffer tests
// ===========================================================================

// ---------------------------------------------------------------------------
// [UT-08] Full-capacity overwrite behaviour is never tested.
//         A CircularBuffer should silently overwrite the oldest element when
//         Push() is called on a full buffer.  This contract is untested.
// ---------------------------------------------------------------------------
TEST(CircularBufferTest, PushAndPopSingleElement)
{
    CircularBuffer<int> cb(3);
    cb.Push(42);
    EXPECT_EQ(cb.Pop(), 42);
    // [UT-08] Never fills the buffer to capacity, never verifies overwrite.
}

// ---------------------------------------------------------------------------
// [UT-09] Pop() on an empty buffer throws std::underflow_error – never tested.
//         Missing: EXPECT_THROW(cb.Pop(), std::underflow_error);
// ---------------------------------------------------------------------------
TEST(CircularBufferTest, IsEmptyAfterConstruction)
{
    CircularBuffer<int> cb(4);
    EXPECT_TRUE(cb.IsEmpty());
    EXPECT_EQ(cb.Size(), 0u);
    // [UT-09] Destructive empty-pop exception path never exercised.
}

// ---------------------------------------------------------------------------
// [UT-10] The test inspects head_/tail_ indirectly via Size() in a way that
//         assumes the internal layout of the circular buffer (white-box testing
//         without any comment explaining why white-box is justified here).
// ---------------------------------------------------------------------------
TEST(CircularBufferTest, SizeTracksCorrectly)
{
    CircularBuffer<int> cb(3);
    cb.Push(1);
    cb.Push(2);
    EXPECT_EQ(cb.Size(), 2u);
    cb.Pop();
    // [UT-10] Using Size() to infer internal cursor positions without comment.
    EXPECT_EQ(cb.Size(), 1u);
    cb.Push(3);
    cb.Push(4);
    EXPECT_EQ(cb.Size(), 3u);   // [UT-10] relies on exact wrap-around behaviour
}

// ===========================================================================
// PaymentProcessor / Mock tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Mock gateway for PaymentProcessor.
// ---------------------------------------------------------------------------
class MockPaymentGateway : public IPaymentGateway
{
public:
    MOCK_METHOD(bool, Charge, (const std::string& account, double amount), (override));
    MOCK_METHOD(bool, Refund, (const std::string& account, double amount), (override));
};

// ---------------------------------------------------------------------------
// [UT-11] EXPECT_CALL is never set up, so the mock framework cannot verify
//         that Charge() was actually called with the expected arguments.
//         The test passes even if ProcessPayment() never calls gateway_.Charge().
// ---------------------------------------------------------------------------
TEST(PaymentProcessorTest, ProcessPaymentCallsGateway)
{
    MockPaymentGateway mock_gateway;
    // [UT-11] No EXPECT_CALL(...) – the mock records nothing and verifies nothing.
    ON_CALL(mock_gateway, Charge(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(true));

    PaymentProcessor processor(mock_gateway);
    EXPECT_TRUE(processor.ProcessPayment("ACC-001", 50.0));
}

// ---------------------------------------------------------------------------
// [UT-12] The test asserts on transaction_log_.size() == 1 via TransactionLog(),
//         which is an implementation detail.  If the log is ever refactored
//         (e.g., entries stored differently), the test breaks for the wrong reason.
//         The relevant contract is "payment succeeds", not "internal log has size 1".
// ---------------------------------------------------------------------------
TEST(PaymentProcessorTest, TransactionLogUpdatedAfterCharge)
{
    MockPaymentGateway mock_gateway;
    EXPECT_CALL(mock_gateway, Charge("ACC-002", 25.0)).WillOnce(::testing::Return(true));

    PaymentProcessor processor(mock_gateway);
    processor.ProcessPayment("ACC-002", 25.0);

    // [UT-12] Testing internal state (log size) rather than observable behaviour.
    EXPECT_EQ(processor.TransactionLog().size(), 1u);
}

// ---------------------------------------------------------------------------
// [UT-13] The gateway's failure path (Charge returns false) is never tested.
//         Missing: a test where Charge() returns false and ProcessPayment()
//         must return false and must NOT append to the transaction log.
// ---------------------------------------------------------------------------
TEST(PaymentProcessorTest, ProcessPaymentWithInvalidAccount)
{
    MockPaymentGateway mock_gateway;
    PaymentProcessor   processor(mock_gateway);

    // [UT-13] Tests only empty-account guard; gateway-failure path absent.
    EXPECT_FALSE(processor.ProcessPayment("", 10.0));
    EXPECT_FALSE(processor.ProcessPayment("ACC-003", 0.0));
    // Missing: mock returns false → ProcessPayment should return false.
}

}  // namespace test
}  // namespace evaluation
}  // namespace score
