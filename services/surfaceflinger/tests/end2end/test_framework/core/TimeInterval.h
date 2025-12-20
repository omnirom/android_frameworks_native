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

namespace android::surfaceflinger::tests::end2end::test_framework::core {

// Represents a span of time between two absolute points in time.
// The interval is half-open, including the beginning point, but not the end point.
// Note: "interval" is used to be consistent with the mathematical definition of the term.
// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct TimeInterval final {
    using TimePoint = std::chrono::steady_clock::time_point;

    // The half-open interval [begin, end), where the end time is not included.
    TimePoint begin;
    TimePoint end;

    [[nodiscard]] auto empty() const -> bool { return begin == end; }
    friend auto operator==(const TimeInterval&, const TimeInterval&) -> bool = default;
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

inline auto toString(const TimeInterval& value) -> std::string {
    return fmt::format("[{}, {})", value.begin.time_since_epoch(), value.end.time_since_epoch());
}

inline auto format_as(const TimeInterval& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
