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

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <semaphore>
#include <string>
#include <thread>

#include <android-base/expected.h>
#include <ftl/finalizer.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/core/TimeInterval.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

// Interface to a Timekeeper Thread.
//
// A Timekeeper thread spends most of its time sleeping. It wakes up either `wakeNow()` is invoked,
// or at the last time returned by the `getNextWakeTime()` callback.
//
// Whenever it wakes up, it calls getNextWakeTime() with the range of time points for the time
// that has elapsed.
class TimeKeeperThread final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    using TimeInterval = core::TimeInterval;

    struct Callbacks final {
        using OnWakeupConnector = core::AsyncFunctionStd<auto(TimeInterval)->TimePoint>;

        // Called when the time keeper thread wakes up with the time interval spent sleeping.
        // Returns the time the thread should next wake up.
        OnWakeupConnector onWakeup;
    };

    [[nodiscard]] static auto make()
            -> base::expected<std::unique_ptr<TimeKeeperThread>, std::string>;

    explicit TimeKeeperThread(Passkey passkey);

    auto editCallbacks() -> Callbacks&;

    void wakeNow();

  private:
    auto init() -> base::expected<void, std::string>;
    void stopThread();
    void threadMain();

    Callbacks mCallbacks;
    std::thread mThread;
    std::binary_semaphore mSemaphore{0};
    std::atomic_bool mStop{false};

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl1 mCleanup;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
