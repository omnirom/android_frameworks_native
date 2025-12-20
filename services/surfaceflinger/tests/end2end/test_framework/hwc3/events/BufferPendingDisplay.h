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
#include <cstdint>
#include <string>

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>
#include <fmt/std.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/core/BufferId.h"
#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::events {

struct BufferPendingDisplay final {
    using AsyncConnector = core::AsyncFunctionStd<void(BufferPendingDisplay)>;

    using DisplayId = core::DisplayConfiguration::Id;
    using LayerId = int64_t;
    using TimePoint = std::chrono::steady_clock::time_point;

    DisplayId displayId{};
    LayerId layerId{};
    core::BufferId bufferId{};
    TimePoint expectedPresentTime;
    TimePoint receivedAt{std::chrono::steady_clock::now()};

    friend auto operator==(const BufferPendingDisplay&, const BufferPendingDisplay&)
            -> bool = default;
};

inline auto toString(const BufferPendingDisplay& event) -> std::string {
    const auto valueToString = [](const auto& value) { return toString(value); };
    return fmt::format(
            "hwcBufferPendingDisplay{{"
            " displayId: {},"
            " layerId: {},"
            " bufferId: {}"
            " expectedPresentTime: {}"
            " receivedAt: {}"
            " }}",
            event.displayId, event.layerId, event.bufferId,
            event.expectedPresentTime.time_since_epoch(), event.receivedAt.time_since_epoch());
}

inline auto format_as(const BufferPendingDisplay& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::events
