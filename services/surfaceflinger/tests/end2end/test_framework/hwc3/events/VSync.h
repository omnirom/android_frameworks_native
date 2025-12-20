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

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::events {

struct VSync final {
    using AsyncConnector = core::AsyncFunctionStd<void(VSync)>;

    using DisplayId = core::DisplayConfiguration::Id;
    using TimePoint = std::chrono::steady_clock::time_point;
    using Duration = std::chrono::steady_clock::duration;

    DisplayId displayId{};
    TimePoint expectedAt;
    Duration expectedPeriod{};
    TimePoint receivedAt{std::chrono::steady_clock::now()};

    friend auto operator==(const VSync&, const VSync&) -> bool = default;
};

inline auto toString(const VSync& event) -> std::string {
    return fmt::format(
            "hwcVSync{{"
            " displayId: {},"
            " expectedAt: {},"
            " expectedPeriod: {},"
            " receivedAt: {}"
            " }}",
            event.displayId, event.expectedAt.time_since_epoch(), event.expectedPeriod,
            event.receivedAt.time_since_epoch());
}

inline auto format_as(const VSync& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::events
