/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "test_framework/core/AsyncFunction.h"

namespace android::surfaceflinger::tests::end2end {
namespace {

template <typename T>
using AsyncFunction = test_framework::core::AsyncFunction<T>;

template <typename T>
using AsyncFunctionStd = test_framework::core::AsyncFunctionStd<T>;

template <typename T>
using AsyncFunctionFtl = test_framework::core::AsyncFunctionFtl<T>;

TEST(AsyncFunction, DefaultConstructionOfVoidFunctionIsANoOpWhenInvoked) {
    const AsyncFunctionStd<void(int)> afn;
    afn(123);
}

TEST(AsyncFunction, DefaultConstructionOfIntFunctionIsANoOpWhenInvoked) {
    const AsyncFunctionStd<int(int)> afn;
    const auto result = afn(123);
    EXPECT_FALSE(result.has_value());
}

TEST(AsyncFunction, NulledVoidFunctionIsANoOpWhenInvoked) {
    int callArg = 0;
    AsyncFunctionStd<void(int)> afn{[&](auto value) { callArg = value; }};
    afn.set(nullptr)();

    EXPECT_EQ(callArg, 0);
    afn(345);
    EXPECT_EQ(callArg, 0);
    EXPECT_TRUE(std::is_void_v<decltype(afn(345))>);
}

TEST(AsyncFunction, NulledIntFunctionIsANoOpWhenInvoked) {
    int callArg = 0;
    AsyncFunctionStd<int(int)> afn{[&](auto value) {
        callArg = value;
        return value + 123;
    }};
    afn.set(nullptr)();

    EXPECT_EQ(callArg, 0);
    const auto result = afn(345);
    EXPECT_EQ(callArg, 0);
    EXPECT_FALSE(result.has_value());
}

TEST(AsyncFunction, VoidFunctionWorksWhenInvoked) {
    int callArg = 0;
    const AsyncFunctionStd<void(int)> afn{[&](auto value) { callArg = value; }};

    EXPECT_EQ(callArg, 0);
    afn(345);
    EXPECT_EQ(callArg, 345);
    EXPECT_TRUE(std::is_void_v<decltype(afn(345))>);
}

TEST(AsyncFunction, IntFunctionWorksWhenInvoked) {
    int callArg = 0;
    const AsyncFunctionStd<int(int)> afn{[&](auto value) {
        callArg = value;
        return value + 123;
    }};

    EXPECT_EQ(callArg, 0);
    const auto result = afn(345);
    EXPECT_EQ(callArg, 345);
    EXPECT_TRUE(result == 468);
}

TEST(AsyncFunction, NulledIntFtlFunctionIsANoOpWhenInvoked) {
    int callArg = 0;
    AsyncFunctionFtl<int(int)> afn{[&](auto value) {
        callArg = value;
        return value + 123;
    }};
    afn.set(nullptr)();

    EXPECT_EQ(callArg, 0);
    const auto result = afn(345);
    EXPECT_EQ(callArg, 0);
    EXPECT_FALSE(result.has_value());
}

TEST(AsyncFunction, IntFtlFunctionWorksWhenInvoked) {
    int callArg = 0;
    const AsyncFunctionFtl<int(int)> afn{[&](auto value) {
        callArg = value;
        return value + 123;
    }};

    EXPECT_EQ(callArg, 0);
    const auto result = afn(345);
    EXPECT_EQ(callArg, 345);
    EXPECT_TRUE(result == 468);
}

TEST(AsyncFunction, CalledFunctionCanReplaceItselfWithoutHanging) {
    AsyncFunctionStd<void()> afn;
    afn.set([&] { afn.set([] {})(); })();
    afn();
}

TEST(AsyncFunction, FinalizerDestroysCaptureWhenManuallyInvoked) {
    auto shared = std::make_shared<int>(0);
    const std::weak_ptr<int> weak = shared;
    AsyncFunctionStd<void()> afn = [context = std::move(shared)] {};

    EXPECT_FALSE(weak.expired());
    auto finalizer = afn.clear();
    EXPECT_FALSE(weak.expired());
    finalizer();
    EXPECT_TRUE(weak.expired());
}

TEST(AsyncFunction, FinalizerDestroysCaptureWhenDestroyed) {
    auto shared = std::make_shared<int>(0);
    const std::weak_ptr<int> weak = shared;

    {
        AsyncFunctionStd<void()> afn = [context = std::move(shared)] {};
        EXPECT_FALSE(weak.expired());
        std::ignore = afn.clear();
        EXPECT_TRUE(weak.expired());
    }
}

TEST(AsyncFunctionStd, CanSafelyReplaceFunctionViaAssignmentWhileBeingCalled) {
    // This has a good but not guaranteed chance of catching a problem, but with continuous runs of
    // this test we should eventually see a failure. Increase this value to increase the chance of
    // catching errors when running locally.
    static constexpr auto kRunFor = std::chrono::milliseconds(10);

    // This test ensures that the wrapper does what it says it does:
    // 1) Allows the function to be replaced at any time.
    // 2) Includes a barrier that the replaced function is no longer used past a point that can be
    //    controlled by the caller so that a wait can be done outside of any locks that might be
    //    acquired by the replaced function,

    // We intentionally use a mutex to guard `state` rather than using an atomic integer.
    std::mutex mutex;
    AsyncFunctionStd<void()> afn;  // GUARDED_BY(mutex)
    int state = 0;                 // GUARDED_BY(mutex)

    auto setState = [&](int value) {
        const std::lock_guard lock(mutex);
        state = value;
    };

    auto setStateTemporarily = [&](int value) {
        static constexpr auto kDelayBeforeSwitching = std::chrono::microseconds(10);
        setState(value);
        std::this_thread::sleep_for(kDelayBeforeSwitching);
        setState(0);
    };

    auto getState = [&] -> int {
        const std::lock_guard lock(mutex);
        return state;
    };

    auto expectStateIsZeroOr = [&](int expected) {
        const auto value = getState();
        EXPECT_TRUE(value == 0 || value == expected)
                << "Expected zero or " << expected << " but observed " << value;
    };

    auto setAsyncFunction = [&](auto newFunction) -> AsyncFunctionStd<void()>::Finalizer {
        const std::lock_guard lock(mutex);
        return afn.set(newFunction);
    };

    // Continuously invoke the current function via the wrapper in a separate thread, for kRunFor
    // time.
    const std::future<void> future = std::async(std::launch::async, [&afn] {
        const auto endAt = std::chrono::steady_clock::now() + kRunFor;
        while (std::chrono::steady_clock::now() < endAt) {
            afn();
        }
    });

    // While the callback is being called, switch the callback between two functions.
    // The first callback sets the state to either 0 or 1.
    // The second callback sets the state to either 0 or 2.
    constexpr auto kWaitFor = std::chrono::microseconds(0);
    while (future.wait_for(kWaitFor) != std::future_status::ready) {
        auto cleanup1 = setAsyncFunction([&] { setStateTemporarily(1); });
        cleanup1();
        expectStateIsZeroOr(1);
        setAsyncFunction([&] { setStateTemporarily(2); })();
        expectStateIsZeroOr(2);
    }
}

}  // namespace
}  // namespace android::surfaceflinger::tests::end2end
