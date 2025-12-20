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

#include <atomic>
#include <chrono>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <ftl/finalizer.h>
#include <ftl/ignore.h>

#include "test_framework/hwc3/TimeKeeperThread.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

struct TimeKeeperThread::Passkey final {};

auto TimeKeeperThread::make() -> base::expected<std::unique_ptr<TimeKeeperThread>, std::string> {
    using namespace std::string_literals;

    auto instance = std::make_unique<TimeKeeperThread>(Passkey{});
    if (instance == nullptr) {
        return base::unexpected("Failed to construct the TimeKeeperThread"s);
    }
    if (auto result = instance->init(); !result) {
        return base::unexpected("Failed to initialize the TimeKeeperThread: "s + result.error());
    }
    return instance;
}

TimeKeeperThread::TimeKeeperThread(Passkey passkey) {
    ftl::ignore(passkey);
}

auto TimeKeeperThread::editCallbacks() -> Callbacks& {
    return mCallbacks;
}

void TimeKeeperThread::wakeNow() {
    mSemaphore.release();
}

auto TimeKeeperThread::init() -> base::expected<void, std::string> {
    mThread = std::thread(&TimeKeeperThread::threadMain, this);
    mCleanup = ftl::Finalizer([this]() { stopThread(); });
    return {};
}

void TimeKeeperThread::stopThread() {
    if (mThread.joinable()) {
        mStop = true;
        wakeNow();
        LOG(VERBOSE) << "Waiting for thread to stop...";
        mThread.join();
        LOG(VERBOSE) << "Stopped.";
    }
}

void TimeKeeperThread::threadMain() {
    constexpr auto kNever = TimePoint::max();
    auto targetWakeTime = kNever;
    auto lastSleepEnded = std::chrono::steady_clock::now();

    while (!mStop) {
        if (targetWakeTime == kNever) {
            mSemaphore.acquire();
        } else {
            mSemaphore.try_acquire_until(targetWakeTime);
        }

        const auto currentSleepEnded = std::chrono::steady_clock::now();
        const auto range = TimeInterval{.begin = lastSleepEnded, .end = currentSleepEnded};
        targetWakeTime = mCallbacks.onWakeup(range).value_or(kNever);

        lastSleepEnded = currentSleepEnded;
    }
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
