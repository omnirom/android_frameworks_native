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
#include <string>

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

// A simple fixed schedule for refresh (VSync) events for a single display
// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct SingleDisplayRefreshSchedule final {
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;

    // The base time for the refresh schedule. Each refresh occurs at (base + period * N) for some
    // integer value N. Note: This time can be in the past or in the future, either should work
    // fine. Refresh events will be generated according to the current time.
    TimePoint base;

    // The period for the refresh schedule. Each refresh occurs this amount of time after the prior
    // refresh time point.
    Duration period{};

    [[nodiscard]] auto nextEvent(TimePoint forTime) const -> TimePoint;

    friend auto operator==(const SingleDisplayRefreshSchedule&,
                           const SingleDisplayRefreshSchedule&) -> bool = default;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

auto toString(const SingleDisplayRefreshSchedule& schedule) -> std::string;

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
