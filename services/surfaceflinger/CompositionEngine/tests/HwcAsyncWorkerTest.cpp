/*
 * Copyright 2024 The Android Open Source Project
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

#include <future>

#include <compositionengine/impl/HwcAsyncWorker.h>
#include <gtest/gtest.h>

namespace android::compositionengine {
namespace {

using namespace std::chrono_literals;

// For the edge case tests below, how much real time should be spent trying to reproduce edge cases
// problems in a loop.
//
// Larger values mean problems are more likely to be detected, at the cost of making the unit test
// run slower.
//
// As we expect the tests to be run continuously, even a short loop will eventually catch
// problems, though not necessarily from changes in the same build that introduce them.
constexpr auto kWallTimeForEdgeCaseTests = 5ms;

TEST(HwcAsyncWorker, continuousTasksEdgeCase) {
    // Ensures that a single worker that is given multiple tasks in short succession will run them.

    impl::HwcAsyncWorker worker;
    const auto endTime = std::chrono::steady_clock::now() + kWallTimeForEdgeCaseTests;
    while (std::chrono::steady_clock::now() < endTime) {
        auto f1 = worker.send([] { return false; });
        EXPECT_FALSE(f1.get());
        auto f2 = worker.send([] { return true; });
        EXPECT_TRUE(f2.get());
    }
}

TEST(HwcAsyncWorker, constructAndDestroyEdgeCase) {
    // Ensures that newly created HwcAsyncWorkers can be immediately destroyed.

    const auto endTime = std::chrono::steady_clock::now() + kWallTimeForEdgeCaseTests;
    while (std::chrono::steady_clock::now() < endTime) {
        impl::HwcAsyncWorker worker;
    }
}

TEST(HwcAsyncWorker, newlyCreatedRunsTasksEdgeCase) {
    // Ensures that newly created HwcAsyncWorkers will run a task if given one immediately.

    const auto endTime = std::chrono::steady_clock::now() + kWallTimeForEdgeCaseTests;
    while (std::chrono::steady_clock::now() < endTime) {
        impl::HwcAsyncWorker worker;
        auto f = worker.send([] { return true; });
        f.get();
    }
}

} // namespace
} // namespace android::compositionengine
