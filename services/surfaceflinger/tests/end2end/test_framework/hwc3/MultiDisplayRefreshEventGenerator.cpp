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
#include <utility>

#include "test_framework/hwc3/MultiDisplayRefreshEventGenerator.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

void MultiDisplayRefreshEventGenerator::addDisplay(DisplayId displayId,
                                                   SingleDisplayRefreshSchedule schedule) {
    generators[displayId] = {.id = displayId, .schedule = schedule};
}

void MultiDisplayRefreshEventGenerator::removeDisplay(DisplayId displayId) {
    generators.erase(displayId);
}

auto MultiDisplayRefreshEventGenerator::generateEventsFor(TimeInterval elapsed) const
        -> GenerateResult {
    GenerateResult multiDisplayResult{};

    for (const auto& [_, generator] : generators) {
        auto singleDisplayResult = generator.generateEventsFor(elapsed);
        multiDisplayResult.nextRefreshAt =
                std::min(multiDisplayResult.nextRefreshAt, singleDisplayResult.nextRefreshAt);
        multiDisplayResult.events.append_range(std::move(singleDisplayResult.events));
    }

    return multiDisplayResult;
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3