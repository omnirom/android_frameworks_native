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

#define LOG_TAG "OneEuroFilter"

#include <chrono>
#include <cmath>

#include <android-base/logging.h>
#include <input/CoordinateFilter.h>

namespace android {
namespace {

using namespace std::literals::chrono_literals;

const float kHertzPerGigahertz = 1E9f;
const float kGigahertzPerHertz = 1E-9f;

// filteredSpeed's units are position per nanosecond. beta's units are 1 / position.
inline float cutoffFreq(float minCutoffFreq, float beta, float filteredSpeed) {
    return kHertzPerGigahertz *
            ((minCutoffFreq * kGigahertzPerHertz) + beta * std::abs(filteredSpeed));
}

inline float smoothingFactor(std::chrono::nanoseconds samplingPeriod, float cutoffFreq) {
    const float constant = 2.0f * M_PI * samplingPeriod.count() * (cutoffFreq * kGigahertzPerHertz);
    return constant / (constant + 1);
}

inline float lowPassFilter(float rawValue, float prevFilteredValue, float smoothingFactor) {
    return smoothingFactor * rawValue + (1 - smoothingFactor) * prevFilteredValue;
}

} // namespace

OneEuroFilter::OneEuroFilter(float minCutoffFreq, float beta, float speedCutoffFreq)
      : mMinCutoffFreq{minCutoffFreq}, mBeta{beta}, mSpeedCutoffFreq{speedCutoffFreq} {}

float OneEuroFilter::filter(std::chrono::nanoseconds timestamp, float rawPosition) {
    LOG_IF(FATAL, mPrevTimestamp.has_value() && (*mPrevTimestamp >= timestamp))
            << "Timestamp must be greater than mPrevTimestamp. Timestamp: " << timestamp.count()
            << "ns. mPrevTimestamp: " << mPrevTimestamp->count() << "ns";

    const std::chrono::nanoseconds samplingPeriod =
            (mPrevTimestamp.has_value()) ? (timestamp - *mPrevTimestamp) : 1s;

    const float rawVelocity = (mPrevFilteredPosition.has_value())
            ? ((rawPosition - *mPrevFilteredPosition) / (samplingPeriod.count()))
            : 0.0f;

    const float speedSmoothingFactor = smoothingFactor(samplingPeriod, mSpeedCutoffFreq);

    const float filteredVelocity = (mPrevFilteredVelocity.has_value())
            ? lowPassFilter(rawVelocity, *mPrevFilteredVelocity, speedSmoothingFactor)
            : rawVelocity;

    const float positionCutoffFreq = cutoffFreq(mMinCutoffFreq, mBeta, filteredVelocity);

    const float positionSmoothingFactor = smoothingFactor(samplingPeriod, positionCutoffFreq);

    const float filteredPosition = (mPrevFilteredPosition.has_value())
            ? lowPassFilter(rawPosition, *mPrevFilteredPosition, positionSmoothingFactor)
            : rawPosition;

    mPrevTimestamp = timestamp;
    mPrevRawPosition = rawPosition;
    mPrevFilteredVelocity = filteredVelocity;
    mPrevFilteredPosition = filteredPosition;

    return filteredPosition;
}

} // namespace android
