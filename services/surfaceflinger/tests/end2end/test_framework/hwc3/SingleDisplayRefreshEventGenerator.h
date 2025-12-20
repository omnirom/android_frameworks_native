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
#include <vector>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/core/TimeInterval.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct SingleDisplayRefreshEventGenerator final {
    using DisplayId = core::DisplayConfiguration::Id;
    using TimeInterval = core::TimeInterval;
    using TimePoint = std::chrono::steady_clock::time_point;

    // The identifier for this display.
    DisplayId id{};

    // The current schedule for refresh events for this display.
    SingleDisplayRefreshSchedule schedule;

    // The result from generateEventsFor().
    struct GenerateResult {
        // The clock time at which the next refresh events should be generated
        TimePoint nextRefreshAt{TimePoint::max()};

        // The refresh events that were generated.
        std::vector<events::VSync> events;

        friend auto operator==(const GenerateResult&, const GenerateResult&) -> bool = default;
    };

    [[nodiscard]] auto generateEventsFor(TimeInterval elapsed) const -> GenerateResult;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3