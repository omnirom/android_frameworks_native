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

#include <bit>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

#include <android-base/logging.h>
#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>

#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

namespace {

using TimePoint = std::chrono::steady_clock::time_point;

// std::chrono::steady_clock uses signed 64 bit values for time points, but that complicates
// this computation. We offset those by this amount so TimePoint::min() becomes zero, and
// TimePoint::max() becomes UINT64_MAX.
constexpr auto kSignedToUnsignedOffset = std::numeric_limits<uint64_t>::max() / 2 + 1;

// Converts a TimePoint to a bare uint64_t count.
[[nodiscard]] constexpr auto timePointToUnsigned(TimePoint value) -> uint64_t {
    return std::bit_cast<uint64_t>(value.time_since_epoch().count()) + kSignedToUnsignedOffset;
};

// Converts a bare uint64_t count to a TimePoint.
[[nodiscard]] constexpr auto timePointFromUnsigned(uint64_t value) -> TimePoint {
    return TimePoint{TimePoint::duration(std::bit_cast<int64_t>(value - kSignedToUnsignedOffset))};
};

// Computes the next event time after the input (forTimeNs), given the period and phase offset for
// the event schedule.
[[nodiscard]] constexpr auto computeNextEventTime(uint64_t periodNs, uint64_t phaseOffsetNs,
                                                  uint64_t forTimeNs) -> uint64_t {
    // By adding or not adding periodNs, we can eliminate underflow and overflow in the modulus
    // portion of the computation, as doing so does not change the post-modulus result.
    const uint64_t avoidUnderAndOverflow = (forTimeNs < periodNs) ? periodNs : 0;

    // Note that adding periodNs here overflows if the next event time is greater than UINT64_MAX.
    return forTimeNs + periodNs - ((forTimeNs + avoidUnderAndOverflow - phaseOffsetNs) % periodNs);
}

// NOLINTBEGIN(*-magic-numbers)

// These compile time checks ensure the computations are free of undefined behavior.

// Ensure timePointToUnsigned maps TimePoint::min() to zero
static_assert(timePointToUnsigned(TimePoint::min()) == 0);

// Ensure timePointToUnsigned maps TimePoint::max() to UINT64_MAX
static_assert(timePointToUnsigned(TimePoint::max()) == std::numeric_limits<uint64_t>::max());

// Ensure timePointFromUnsigned maps zero to TimePoint::min()
static_assert(timePointFromUnsigned(0) == TimePoint::min());

// Ensure timePointFromUnsigned maps UINT64_MAX to TimePoint::max()
static_assert(timePointFromUnsigned(std::numeric_limits<uint64_t>::max()) == TimePoint::max());

// Events with a period of 5ms and a phase offset of 0ms should happen at (5ms, 10ms, 15ms...)
static_assert(computeNextEventTime(5'000'000, 0'000'000, 0'000'000U) == 5'000'000U);
static_assert(computeNextEventTime(5'000'000, 0'000'000, 4'999'999U) == 5'000'000U);
static_assert(computeNextEventTime(5'000'000, 0'000'000, 5'000'000U) == 10'000'000U);
static_assert(computeNextEventTime(5'000'000, 0'000'000, 9'999'999U) == 10'000'000U);
static_assert(computeNextEventTime(5'000'000, 0'000'000, 10'000'000U) == 15'000'000U);

// Events with a period of 5ms and a phase offset of 1ms should happen at (1ms, 6ms, 11ms, ...)
static_assert(computeNextEventTime(5'000'000, 1'000'000, 0'000'000U) == 1'000'000U);
static_assert(computeNextEventTime(5'000'000, 1'000'000, 999'999U) == 1'000'000U);
static_assert(computeNextEventTime(5'000'000, 1'000'000, 1'000'000U) == 6'000'000U);
static_assert(computeNextEventTime(5'000'000, 1'000'000, 5'999'999U) == 6'000'000U);
static_assert(computeNextEventTime(5'000'000, 1'000'000, 6'000'000U) == 11'000'000U);

// Events with a period of 4ms and a phase offset of 1ms should happen at (1ms, 5ms, 9ms, ...)
static_assert(computeNextEventTime(4'000'000, 1'000'000, 0'000'000U) == 1'000'000U);
static_assert(computeNextEventTime(4'000'000, 1'000'000, 999'999U) == 1'000'000U);
static_assert(computeNextEventTime(4'000'000, 1'000'000, 1'000'000U) == 5'000'000U);
static_assert(computeNextEventTime(4'000'000, 1'000'000, 4'999'999U) == 5'000'000U);
static_assert(computeNextEventTime(4'000'000, 1'000'000, 5'000'000U) == 9'000'000U);

// Events with a period of 5ms and a phase offset of 2ms should happen at (2ms, 7ms, 12ms, ...)
static_assert(computeNextEventTime(5'000'000, 2'000'000, 0'000'000U) == 2'000'000U);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 1'999'999U) == 2'000'000U);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 2'000'000U) == 7'000'000U);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 6'999'999U) == 7'000'000U);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 7'000'000U) == 12'000'000U);

// Test the upper bound of the computation with forTimes near the uint64_t maximum of
// UINT64_MAX (18'446'544'073'709'551'614).

// Input values between the next to last representable event time and up the last representable
// event time  should compute the last representable event time.
static_assert(computeNextEventTime(5'000'000, 2'000'000, 18'446'744'073'702'000'000U) ==
              18'446'744'073'707'000'000U);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 18'446'744'073'706'999'999U) ==
              18'446'744'073'707'000'000U);

// Values greater from the last representable event time up to UINT64_MAX will unavoidably overflow
// as the next event time is beyond UINT64_MAX. Instead a small truncated value will be returned.
static_assert(computeNextEventTime(5'000'000, 2'000'000, 18'446'744'073'707'000'000U) == 2'448'384);
static_assert(computeNextEventTime(5'000'000, 2'000'000, 18'446'744'073'709'551'615U) == 2'448'384);

// If the next event time happens to be representable as UINT64_MAX, that value is returned.
static_assert(computeNextEventTime(1'00'000, 551'615, 18'446'744'073'709'551'614U) ==
              18'446'744'073'709'551'615U);

// NOLINTEND(*-magic-numbers)

}  // namespace

[[nodiscard]] auto SingleDisplayRefreshSchedule::nextEvent(TimePoint forTime) const -> TimePoint {
    // The period must be positive (and not zero).
    CHECK_GT(period, TimePoint::duration(0));
    const auto periodNs = static_cast<uint64_t>(period.count());

    // Convert and reduce the base timestamp to a phase offset value in the interval [0, period) by
    // computing the remainder. This minimizes the overflow range.
    const auto phaseOffsetNs = timePointToUnsigned(base) % periodNs;

    // Convert the input timestamp.
    const auto forTimeNs = timePointToUnsigned(forTime);

    // Compute the next event time after the input time.
    const auto nextTimeNs = computeNextEventTime(periodNs, phaseOffsetNs, forTimeNs);

    // Ensure that the next time point is actually after the input time point. The only time this
    // check should trigger is for times near TimePoint::max() (317 years since the epoch), where
    // the computation of nextTimeNs exceeds TimePoint::max().
    CHECK_GT(nextTimeNs, forTimeNs);

    // Convert back to a signed TimePoint value.
    return timePointFromUnsigned(nextTimeNs);
}

auto toString(const SingleDisplayRefreshSchedule& schedule) -> std::string {
    return fmt::format("SingleDisplayRefreshSchedule{{base: {}, period: {}}}",
                       schedule.base.time_since_epoch(), schedule.period);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
