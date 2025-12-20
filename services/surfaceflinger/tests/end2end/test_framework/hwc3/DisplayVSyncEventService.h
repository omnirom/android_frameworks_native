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

#include <chrono>
#include <memory>
#include <string>

#include <android-base/expected.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/core/GuardedSharedState.h"
#include "test_framework/core/TimeInterval.h"
#include "test_framework/hwc3/MultiDisplayRefreshEventGenerator.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

class TimeKeeperThread;

class DisplayVSyncEventService final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    struct Callbacks final {
        events::VSync::AsyncConnector onVSync;
    };

    using DisplayId = core::DisplayConfiguration::Id;

    [[nodiscard]] static auto make()
            -> base::expected<std::shared_ptr<DisplayVSyncEventService>, std::string>;

    explicit DisplayVSyncEventService(Passkey passkey);

    [[nodiscard]] auto editCallbacks() -> Callbacks&;

    void addDisplay(DisplayId displayId, SingleDisplayRefreshSchedule schedule);

    void removeDisplay(DisplayId displayId);

  private:
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;
    using TimeInterval = core::TimeInterval;

    auto init() -> base::expected<void, std::string>;
    auto onWakeup(TimeInterval elapsed) -> TimePoint;

    core::GuardedSharedState<MultiDisplayRefreshEventGenerator> mGenerator;

    // No mutex needed.
    Callbacks mCallbacks;

    // Note: The time keeper thread should be destroyed first, so is declared last.
    std::unique_ptr<TimeKeeperThread> mTimeKeeper;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
