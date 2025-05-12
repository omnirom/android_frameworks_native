/**
 * Copyright 2024 The Android Open Source Project
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
#include <optional>

#include <input/Input.h>

namespace android {

/**
 * Low pass filter with adaptive low pass frequency based on the signal's speed. The signal's cutoff
 * frequency is determined by f_c = f_c_min + β|̇x_filtered|. Refer to
 * https://dl.acm.org/doi/10.1145/2207676.2208639 for details on how the filter works and how to
 * tune it.
 */
class OneEuroFilter {
public:
    /**
     * Default cutoff frequency of the filtered signal's speed. 1.0 Hz is the value in the filter's
     * paper.
     */
    static constexpr float kDefaultSpeedCutoffFreq = 1.0;

    OneEuroFilter() = delete;

    explicit OneEuroFilter(float minCutoffFreq, float beta,
                           float speedCutoffFreq = kDefaultSpeedCutoffFreq);

    OneEuroFilter(const OneEuroFilter&) = delete;
    OneEuroFilter& operator=(const OneEuroFilter&) = delete;
    OneEuroFilter(OneEuroFilter&&) = delete;
    OneEuroFilter& operator=(OneEuroFilter&&) = delete;

    /**
     * Returns the filtered value of rawPosition. Each call to filter must provide a timestamp
     * strictly greater than the timestamp of the previous call. The first time the method is
     * called, it returns the value of rawPosition. Any subsequent calls provide a filtered value.
     *
     * @param timestamp The timestamp at which to filter. It must be strictly greater than the one
     * provided in the previous call.
     * @param rawPosition Position to be filtered.
     */
    float filter(std::chrono::nanoseconds timestamp, float rawPosition);

private:
    /**
     * Minimum cutoff frequency. This is the constant term in the adaptive cutoff frequency
     * criterion. Units are Hertz.
     */
    const float mMinCutoffFreq;

    /**
     * Slope of the cutoff frequency criterion. This is the term scaling the absolute value of the
     * filtered signal's speed. Units are 1 / position.
     */
    const float mBeta;

    /**
     * Cutoff frequency of the signal's speed. This is the cutoff frequency applied to the filtering
     * of the signal's speed. Units are Hertz.
     */
    const float mSpeedCutoffFreq;

    /**
     * The timestamp from the previous call.
     */
    std::optional<std::chrono::nanoseconds> mPrevTimestamp;

    /**
     * The raw position from the previous call.
     */
    std::optional<float> mPrevRawPosition;

    /**
     * The filtered velocity from the previous call. Units are position per nanosecond.
     */
    std::optional<float> mPrevFilteredVelocity;

    /**
     * The filtered position from the previous call.
     */
    std::optional<float> mPrevFilteredPosition;
};

} // namespace android
