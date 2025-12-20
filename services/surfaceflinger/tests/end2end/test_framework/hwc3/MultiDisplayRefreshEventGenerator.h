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
#include <map>
#include <vector>

#include <android-base/logging.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/core/TimeInterval.h"
#include "test_framework/hwc3/SingleDisplayRefreshEventGenerator.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

class MultiDisplayRefreshEventGenerator final {
  public:
    using DisplayId = core::DisplayConfiguration::Id;
    using TimeInterval = core::TimeInterval;
    using TimePoint = std::chrono::steady_clock::time_point;

    void addDisplay(DisplayId displayId, SingleDisplayRefreshSchedule schedule);
    void removeDisplay(DisplayId displayId);

    // The result from generateEventsFor().
    struct GenerateResult {
        // The clock time at which the next refresh events should be generated
        TimePoint nextRefreshAt{TimePoint::max()};

        // The refresh events that were generated.
        std::vector<events::VSync> events;

        friend auto operator==(const GenerateResult&, const GenerateResult&) -> bool = default;
    };

    [[nodiscard]] auto generateEventsFor(TimeInterval elapsed) const -> GenerateResult;

  private:
    // Note: map instead of unordered_map so traversal is consistent.
    std::map<DisplayId, SingleDisplayRefreshEventGenerator> generators;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3