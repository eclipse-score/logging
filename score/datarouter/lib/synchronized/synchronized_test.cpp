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

#include "../synchronized/synchronized.h"

#include "gtest/gtest.h"
#include <thread>
#include <vector>

using score::platform::datarouter::Synchronized;

namespace test
{

struct TestStruct
{
    int value;

    int sum() const
    {
        return value + 50;
    }
};

struct MoveOnlyObject
{
    int value;

    MoveOnlyObject(int v) : value(v) {}
    MoveOnlyObject(const MoveOnlyObject&) = delete;
    MoveOnlyObject& operator=(const MoveOnlyObject&) = delete;
    MoveOnlyObject(MoveOnlyObject&&) = default;
    MoveOnlyObject& operator=(MoveOnlyObject&&) = default;
};

struct NoCopyNoMoveObject
{
    int value;

    NoCopyNoMoveObject(int v) : value(v) {}
    NoCopyNoMoveObject(const NoCopyNoMoveObject&) = delete;
    NoCopyNoMoveObject& operator=(const NoCopyNoMoveObject&) = delete;
    NoCopyNoMoveObject(NoCopyNoMoveObject&&) = delete;
    NoCopyNoMoveObject& operator=(NoCopyNoMoveObject&&) = delete;
};

struct DefaultConstructibleObject
{
    int value;

    DefaultConstructibleObject() : value(42) {}
    DefaultConstructibleObject(int v) : value(v) {}
};

struct ParameterizedObject
{
    int x, y;
    std::string name;

    ParameterizedObject(int x_val, int y_val, const std::string& n) : x(x_val), y(y_val), name(n) {}

    int sum() const
    {
        return x + y;
    }
    const std::string& get_name() const
    {
        return name;
    }
};

// Basic test fixture for synchronized utility
class SynchronizedUtilityTest : public ::testing::Test
{
  protected:
    void TestSynchronizedWrapper()
    {
        Synchronized<int> sync_value(42);

        int result = sync_value.with_lock([](const auto& value) noexcept {
            return value;
        });
        EXPECT_EQ(result, 42);

        sync_value.with_lock([](auto& value) noexcept {
            value = 100;
        });

        result = sync_value.with_lock([](const auto& value) noexcept {
            return value;
        });
        EXPECT_EQ(result, 100);
    }
};

// Test the synchronized wrapper template
TEST_F(SynchronizedUtilityTest, TestSynchronizedWrapperTemplate)
{
    TestSynchronizedWrapper();
}

// Test for thread safety with multiple threads accessing a synchronized wrapper
TEST_F(SynchronizedUtilityTest, TestThreadSafety)
{
    Synchronized<int> counter(0);
    const int num_threads = 10;
    const int increments_per_thread = 1000;

    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i)
    {
        threads.push_back(std::thread([&counter]() noexcept {
            for (int j = 0; j < increments_per_thread; ++j)
            {
                counter.with_lock([](auto& value) noexcept {
                    ++value;
                });
            }
        }));
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    int result = counter.with_lock([](auto& value) noexcept {
        return value;
    });

    EXPECT_EQ(result, num_threads * increments_per_thread);
}

TEST_F(SynchronizedUtilityTest, LockMethodBasic)
{
    Synchronized<int> sync_int(42);

    {
        auto locked_ptr = sync_int.lock();
        EXPECT_EQ(*locked_ptr, 42);
        *locked_ptr = 100;
        EXPECT_EQ(*locked_ptr, 100);
    }

    sync_int.with_lock([](int& value) {
        EXPECT_EQ(value, 100);
    });
}

// Comprehensive TestStruct tests covering additional scenarios
TEST_F(SynchronizedUtilityTest, TestStructLockReturnValue)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    {
        auto locked_ptr = sync_struct.lock();
        ASSERT_TRUE(locked_ptr);
        EXPECT_EQ(locked_ptr->value, 42);

        locked_ptr->value = 100;
        EXPECT_EQ(locked_ptr->value, 100);

        EXPECT_EQ((*locked_ptr).value, 100);
        EXPECT_EQ((*locked_ptr).sum(), 150);
    }
}

TEST_F(SynchronizedUtilityTest, TestStructConstLockBehavior)
{
    const Synchronized<TestStruct> const_sync_struct(TestStruct{42});

    {
        auto const_locked_ptr = const_sync_struct.lock();
        EXPECT_EQ(const_locked_ptr->value, 42);
        // const_locked_ptr->value = 100;  // Should not compile
        EXPECT_EQ((*const_locked_ptr).value, 42);
        EXPECT_EQ((*const_locked_ptr).sum(), 92);
    }

    const_sync_struct.with_lock([](const TestStruct& s) noexcept {
        EXPECT_EQ(s.value, 42);
    });
}

TEST_F(SynchronizedUtilityTest, TestStructWithLockVariations)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    sync_struct.with_lock([](TestStruct& s) noexcept {
        s.value = 100;
    });

    int result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 100);

    sync_struct.with_lock([](TestStruct& s) noexcept {
        s.value = s.value * 2;
    });

    result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 200);
}

TEST_F(SynchronizedUtilityTest, TestStructMemberFunctionPointers)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    auto result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 42);
}

TEST_F(SynchronizedUtilityTest, TestStructFreeFunctionPointers)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    auto process_struct = [](TestStruct& s) noexcept {
        s.value = 999;
    };

    sync_struct.with_lock(process_struct);

    auto result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 999);
}

TEST_F(SynchronizedUtilityTest, TestStructLockNonConst)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    {
        auto locked_ptr = sync_struct.lock();
        locked_ptr->value = 77;
        EXPECT_EQ(locked_ptr->value, 77);
    }

    auto result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 77);
}

TEST_F(SynchronizedUtilityTest, TestStructLockConst)
{
    const Synchronized<TestStruct> const_sync_struct(TestStruct{42});

    {
        auto const_locked_ptr = const_sync_struct.lock();
        EXPECT_EQ(const_locked_ptr->value, 42);
        // const_sync_struct->value = 100;  // Should not compile
    }

    const_sync_struct.with_lock([](const TestStruct& s) noexcept {
        EXPECT_EQ(s.value, 42);
    });
}

TEST_F(SynchronizedUtilityTest, TestStructLambdaCaptureModes)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    int external_value = 10;

    // Test lambda with value capture
    sync_struct.with_lock([external_value](TestStruct& s) noexcept {
        s.value = external_value * 5;
    });

    auto result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 50);

    // Test lambda with reference capture
    int multiplier = 4;
    sync_struct.with_lock([&multiplier](TestStruct& s) noexcept {
        s.value = s.value * multiplier;
    });

    result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 200);
}

TEST_F(SynchronizedUtilityTest, TestStructVoidReturningLambdas)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    sync_struct.with_lock([](TestStruct& s) noexcept -> void {
        s.value = 123;
    });

    auto result = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 123);
}

TEST_F(SynchronizedUtilityTest, TestStructCopyVsMoveSemantics)
{
    // Test construction from temporary (move semantics)
    Synchronized<TestStruct> sync_struct1(TestStruct{42});
    auto val = sync_struct1.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val, 42);

    // Test construction from lvalue (copy semantics)
    TestStruct temp_struct{55};
    Synchronized<TestStruct> sync_struct2(temp_struct);
    auto val1 = sync_struct2.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val1, 55);

    // Test in-place construction
    Synchronized<TestStruct> sync_struct3(TestStruct{77});
    auto val2 = sync_struct3.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val2, 77);
}

// Test for move-only objects
TEST_F(SynchronizedUtilityTest, TestMoveOnlyObject)
{
    Synchronized<MoveOnlyObject> sync_obj(150);

    auto result = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(result, 150);

    sync_obj.with_lock([](auto& obj) noexcept {
        obj.value = 300;
    });

    auto val = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(val, 300);
}

// Test for objects that don't allow copy or move
TEST_F(SynchronizedUtilityTest, TestNoCopyNoMoveObject)
{
    Synchronized<NoCopyNoMoveObject> sync_obj(250);

    auto result = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(result, 250);

    sync_obj.with_lock([](auto& obj) noexcept {
        obj.value = 500;
    });

    auto val = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(val, 500);
}

// Test for default-constructible objects
TEST_F(SynchronizedUtilityTest, TestDefaultConstructedObject)
{
    Synchronized<DefaultConstructibleObject> sync_obj{};

    auto result = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(result, 42);

    sync_obj.with_lock([](auto& obj) noexcept {
        obj.value = 100;
    });

    auto val = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.value;
    });
    EXPECT_EQ(val, 100);
}

// Test for parameterized construction
TEST_F(SynchronizedUtilityTest, TestParameterizedConstruction)
{
    Synchronized<ParameterizedObject> sync_obj(10, 20, "test_object");

    auto result = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.sum();
    });
    EXPECT_EQ(result, 30);

    auto name = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.get_name();
    });
    EXPECT_EQ(name, "test_object");

    sync_obj.with_lock([](auto& obj) noexcept {
        obj.x = 15;
        obj.y = 25;
    });

    result = sync_obj.with_lock([](const auto& obj) noexcept {
        return obj.sum();
    });
    EXPECT_EQ(result, 40);
}

TEST_F(SynchronizedUtilityTest, TestConstOperations)
{
    const Synchronized<TestStruct> const_sync_struct(TestStruct{42});

    auto result = const_sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(result, 42);

    {
        auto const_locked = const_sync_struct.lock();
        EXPECT_EQ(const_locked->value, 42);
    }
}

TEST_F(SynchronizedUtilityTest, LockAndWithLock)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    {
        auto locked_ptr = sync_struct.lock();
        locked_ptr->value = 150;
        EXPECT_EQ(locked_ptr->value, 150);
    }

    sync_struct.with_lock([](TestStruct& s) noexcept {
        s.value = 200;
    });

    auto result = sync_struct.with_lock([](const TestStruct& s) {
        return s.sum();
    });
    EXPECT_EQ(result, 250);
    auto val1 = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val1, 200);

    EXPECT_NO_THROW({
        sync_struct.with_lock([](const TestStruct& s) noexcept {
            EXPECT_EQ(s.value, 200);
        });
    });

    auto val2 = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val2, 200);
}

TEST_F(SynchronizedUtilityTest, TestStructExceptionSafetyDetailed)
{
    Synchronized<TestStruct> sync_struct(TestStruct{42});

    sync_struct.with_lock([](TestStruct& s) noexcept {
        s.value = 100;
    });

    EXPECT_NO_THROW({
        sync_struct.with_lock([](TestStruct& s) noexcept {
            s.value = 999;
            EXPECT_EQ(s.value, 999);
        });
    });

    auto val = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val, 999);

    sync_struct.with_lock([](TestStruct& s) noexcept {
        s.value = 200;
    });
    auto val2 = sync_struct.with_lock([](const TestStruct& s) noexcept {
        return s.value;
    });
    EXPECT_EQ(val2, 200);
}

}  // namespace test
