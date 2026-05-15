

#include "score/evaluation/unit_test_quality/unit_test_quality.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace score
{
namespace evaluation
{
namespace test
{

TEST(TokenBucketTest, ConsumeDoesNotCrash)
{
    TokenBucket bucket(10, 2);
    bucket.TryConsume(3);
}

TEST(TokenBucketTest, ConsumeSucceedsWhenEnoughTokens)
{
    TokenBucket bucket(10, 2);
    EXPECT_TRUE(bucket.TryConsume(5));
    EXPECT_EQ(bucket.AvailableTokens(), 5);
}

TEST(TokenBucketTest, RefillAddsTokens)
{
    TokenBucket bucket(10, 2);
    bucket.TryConsume(7);
    bucket.Refill();
    EXPECT_EQ(bucket.AvailableTokens(), 5);
}

TEST(TokenBucketTest, ConstructWithValidArgs)
{
    EXPECT_NO_THROW(TokenBucket(5, 1));
}

TEST(StringProcessorTest, TrimRemovesLeadingSpaces)
{
    EXPECT_EQ(StringProcessor::Trim("  hello"), "hello");
}

TEST(StringProcessorTest, TrimRemovesTrailingSpaces)
{
    EXPECT_EQ(StringProcessor::Trim("  hello"), "hello");
}

TEST(StringProcessorTest, SplitOnComma)
{
    const auto parts = StringProcessor::Split("a,b,c", ',');
    EXPECT_EQ(parts.size(), 3u);
}

TEST(StringProcessorTest, Test1)
{
    EXPECT_EQ(StringProcessor::ToUpperCase("hello"), "HELLO");
}

TEST(StringProcessorTest, Test2)
{
    EXPECT_EQ(StringProcessor::ToUpperCase("World"), "WORLD");
}

TEST(CircularBufferTest, PushAndPopSingleElement)
{
    CircularBuffer<int> cb(3);
    cb.Push(42);
    EXPECT_EQ(cb.Pop(), 42);
}

TEST(CircularBufferTest, IsEmptyAfterConstruction)
{
    CircularBuffer<int> cb(4);
    EXPECT_TRUE(cb.IsEmpty());
    EXPECT_EQ(cb.Size(), 0u);
}

TEST(CircularBufferTest, SizeTracksCorrectly)
{
    CircularBuffer<int> cb(3);
    cb.Push(1);
    cb.Push(2);
    EXPECT_EQ(cb.Size(), 2u);
    cb.Pop();
    EXPECT_EQ(cb.Size(), 1u);
    cb.Push(3);
    cb.Push(4);
    EXPECT_EQ(cb.Size(), 3u);
}

class MockPaymentGateway : public IPaymentGateway
{
public:
    MOCK_METHOD(bool, Charge, (const std::string& account, double amount), (override));
    MOCK_METHOD(bool, Refund, (const std::string& account, double amount), (override));
};

TEST(PaymentProcessorTest, ProcessPaymentCallsGateway)
{
    MockPaymentGateway mock_gateway;
    ON_CALL(mock_gateway, Charge(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(true));

    PaymentProcessor processor(mock_gateway);
    EXPECT_TRUE(processor.ProcessPayment("ACC-001", 50.0));
}

TEST(PaymentProcessorTest, TransactionLogUpdatedAfterCharge)
{
    MockPaymentGateway mock_gateway;
    EXPECT_CALL(mock_gateway, Charge("ACC-002", 25.0)).WillOnce(::testing::Return(true));

    PaymentProcessor processor(mock_gateway);
    processor.ProcessPayment("ACC-002", 25.0);

    EXPECT_EQ(processor.TransactionLog().size(), 1u);
}

TEST(PaymentProcessorTest, ProcessPaymentWithInvalidAccount)
{
    MockPaymentGateway mock_gateway;
    PaymentProcessor   processor(mock_gateway);

    EXPECT_FALSE(processor.ProcessPayment("", 10.0));
    EXPECT_FALSE(processor.ProcessPayment("ACC-003", 0.0));
}

}
}
}
