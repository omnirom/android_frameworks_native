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

struct DisplayPresented final {
    using AsyncConnector = core::AsyncFunctionStd<void(DisplayPresented)>;

    using DisplayId = core::DisplayConfiguration::Id;
    using TimePoint = std::chrono::steady_clock::time_point;

    DisplayId displayId{};
    TimePoint expectedPresentTime;
    TimePoint receivedAt{std::chrono::steady_clock::now()};

    friend auto operator==(const DisplayPresented&, const DisplayPresented&) -> bool = default;
};

inline auto toString(const DisplayPresented& event) -> std::string {
    return fmt::format(
            "hwcDisplayPresented{{"
            " displayId: {},"
            " expectedPresentTime: {}"
            " receivedAt: {}"
            " }}",
            event.displayId, event.expectedPresentTime.time_since_epoch(),
            event.receivedAt.time_since_epoch());
}

inline auto format_as(const DisplayPresented& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::events
