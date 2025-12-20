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

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

#include <android-base/logging.h>

#include "test_framework/core/TimeInterval.h"
#include "test_framework/hwc3/SingleDisplayRefreshEventGenerator.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

namespace {

[[nodiscard]] auto getEventInterval(const SingleDisplayRefreshSchedule& schedule,
                                    const core::TimeInterval& input) -> core::TimeInterval {
    // Transform the input interval into an interval for the next event times for each endpoint in
    // the input interval.
    auto interval = core::TimeInterval{
            .begin = schedule.nextEvent(input.begin),
            .end = schedule.nextEvent(input.end),
    };

    // Get the timepoint for the refresh event just before the last one in the interval.
    const auto oneBeforeEnd = interval.end - schedule.period;

    // We try to catch up on events since the start of the elapsed time. However if there are
    // too many we limit the past events to just the single most recent one.
    const auto catchUpLimit = schedule.period * 4;
    const auto catchUpBegin =
            (interval.begin + catchUpLimit >= oneBeforeEnd) ? interval.begin : oneBeforeEnd;

    // Ensure lastReturnedEndPoint is not in the returned interval, so we only emit events
    // after that time.
    interval.begin = std::max(interval.begin, catchUpBegin);

    // Verify that the two end points exactly correspond to refresh event times, which also
    // means they are an integer multiple of the period apart.
    CHECK(schedule.nextEvent(interval.begin) == interval.begin + schedule.period);
    CHECK(schedule.nextEvent(interval.end) == interval.end + schedule.period);
    CHECK(interval.begin <= interval.end);  // Check an invariant.

    return interval;
}

}  // namespace

auto SingleDisplayRefreshEventGenerator::generateEventsFor(TimeInterval elapsed) const
        -> GenerateResult {
    auto generation = getEventInterval(schedule, elapsed);

    LOG(VERBOSE) << "unemitted events for " << id << " are  " << toString(generation);

    std::vector<events::VSync> events;
    for (auto eventTime = generation.begin; eventTime < generation.end;
         eventTime += schedule.period) {
        events.push_back({id, eventTime, schedule.period, elapsed.end});
    }

    return {.nextRefreshAt = generation.end, .events = std::move(events)};
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3