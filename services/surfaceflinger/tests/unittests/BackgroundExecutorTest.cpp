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

#include <gtest/gtest.h>
#include <condition_variable>

#include "BackgroundExecutor.h"

namespace android {

class BackgroundExecutorTest : public testing::Test {};

namespace {

TEST_F(BackgroundExecutorTest, singleProducer) {
    std::mutex mutex;
    std::condition_variable condition_variable;
    bool backgroundTaskComplete = false;

    BackgroundExecutor::getInstance().sendCallbacks(
            {[&mutex, &condition_variable, &backgroundTaskComplete]() {
                std::lock_guard<std::mutex> lock{mutex};
                condition_variable.notify_one();
                backgroundTaskComplete = true;
            }});

    std::unique_lock<std::mutex> lock{mutex};
    condition_variable.wait(lock, [&backgroundTaskComplete]() { return backgroundTaskComplete; });
    ASSERT_TRUE(backgroundTaskComplete);
}

TEST_F(BackgroundExecutorTest, multipleProducers) {
    std::mutex mutex;
    std::condition_variable condition_variable;
    const int backgroundTaskCount = 10;
    int backgroundTaskCompleteCount = 0;

    for (int i = 0; i < backgroundTaskCount; i++) {
        std::thread([&mutex, &condition_variable, &backgroundTaskCompleteCount]() {
            BackgroundExecutor::getInstance().sendCallbacks(
                    {[&mutex, &condition_variable, &backgroundTaskCompleteCount]() {
                        std::lock_guard<std::mutex> lock{mutex};
                        backgroundTaskCompleteCount++;
                        if (backgroundTaskCompleteCount == backgroundTaskCount) {
                            condition_variable.notify_one();
                        }
                    }});
        }).detach();
    }

    std::unique_lock<std::mutex> lock{mutex};
    condition_variable.wait(lock, [&backgroundTaskCompleteCount]() {
        return backgroundTaskCompleteCount == backgroundTaskCount;
    });
    ASSERT_EQ(backgroundTaskCount, backgroundTaskCompleteCount);
}

} // namespace

} // namespace android
