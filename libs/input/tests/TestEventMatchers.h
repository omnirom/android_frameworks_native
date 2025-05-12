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
#include <cmath>
#include <ostream>
#include <vector>

#include <android-base/logging.h>
#include <gtest/gtest.h>
#include <input/Input.h>

namespace android {

namespace {

using ::testing::Matcher;

} // namespace

/**
 * This file contains a copy of Matchers from .../inputflinger/tests/TestEventMatchers.h. Ideally,
 * implementations must not be duplicated.
 * TODO(b/365606513): Find a way to share TestEventMatchers.h between inputflinger and libinput.
 */

struct PointerArgs {
    float x{0.0f};
    float y{0.0f};
    bool isResampled{false};
};

struct Sample {
    std::chrono::nanoseconds eventTime{0};
    std::vector<PointerArgs> pointers{};
};

class WithDeviceIdMatcher {
public:
    using is_gtest_matcher = void;
    explicit WithDeviceIdMatcher(DeviceId deviceId) : mDeviceId(deviceId) {}

    bool MatchAndExplain(const InputEvent& event, std::ostream*) const {
        return mDeviceId == event.getDeviceId();
    }

    void DescribeTo(std::ostream* os) const { *os << "with device id " << mDeviceId; }

    void DescribeNegationTo(std::ostream* os) const { *os << "wrong device id"; }

private:
    const DeviceId mDeviceId;
};

inline WithDeviceIdMatcher WithDeviceId(int32_t deviceId) {
    return WithDeviceIdMatcher(deviceId);
}

class WithMotionActionMatcher {
public:
    using is_gtest_matcher = void;
    explicit WithMotionActionMatcher(int32_t action) : mAction(action) {}

    bool MatchAndExplain(const MotionEvent& event, testing::MatchResultListener* listener) const {
        if (mAction != event.getAction()) {
            *listener << "expected " << MotionEvent::actionToString(mAction) << ", but got "
                      << MotionEvent::actionToString(event.getAction());
            return false;
        }
        if (event.getAction() == AMOTION_EVENT_ACTION_CANCEL &&
            (event.getFlags() & AMOTION_EVENT_FLAG_CANCELED) == 0) {
            *listener << "event with CANCEL action is missing FLAG_CANCELED";
            return false;
        }
        return true;
    }

    void DescribeTo(std::ostream* os) const {
        *os << "with motion action " << MotionEvent::actionToString(mAction);
        if (mAction == AMOTION_EVENT_ACTION_CANCEL) {
            *os << " and FLAG_CANCELED";
        }
    }

    void DescribeNegationTo(std::ostream* os) const { *os << "wrong action"; }

private:
    const int32_t mAction;
};

inline WithMotionActionMatcher WithMotionAction(int32_t action) {
    return WithMotionActionMatcher(action);
}

class WithSampleCountMatcher {
public:
    using is_gtest_matcher = void;
    explicit WithSampleCountMatcher(size_t sampleCount) : mExpectedSampleCount{sampleCount} {}

    bool MatchAndExplain(const MotionEvent& motionEvent, std::ostream*) const {
        return (motionEvent.getHistorySize() + 1) == mExpectedSampleCount;
    }

    void DescribeTo(std::ostream* os) const { *os << "sample count " << mExpectedSampleCount; }

    void DescribeNegationTo(std::ostream* os) const { *os << "different sample count"; }

private:
    const size_t mExpectedSampleCount;
};

inline WithSampleCountMatcher WithSampleCount(size_t sampleCount) {
    return WithSampleCountMatcher(sampleCount);
}

class WithSampleMatcher {
public:
    using is_gtest_matcher = void;
    explicit WithSampleMatcher(size_t sampleIndex, const Sample& sample)
          : mSampleIndex{sampleIndex}, mSample{sample} {}

    bool MatchAndExplain(const MotionEvent& motionEvent, std::ostream* os) const {
        if (motionEvent.getHistorySize() < mSampleIndex) {
            *os << "sample index out of bounds";
            return false;
        }

        if (motionEvent.getHistoricalEventTime(mSampleIndex) != mSample.eventTime.count()) {
            *os << "event time mismatch. sample: "
                << motionEvent.getHistoricalEventTime(mSampleIndex)
                << " expected: " << mSample.eventTime.count();
            return false;
        }

        if (motionEvent.getPointerCount() != mSample.pointers.size()) {
            *os << "pointer count mismatch. sample: " << motionEvent.getPointerCount()
                << " expected: " << mSample.pointers.size();
            return false;
        }

        for (size_t pointerIndex = 0; pointerIndex < motionEvent.getPointerCount();
             ++pointerIndex) {
            const PointerCoords& pointerCoords =
                    *(motionEvent.getHistoricalRawPointerCoords(pointerIndex, mSampleIndex));

            if ((std::abs(pointerCoords.getX() - mSample.pointers[pointerIndex].x) >
                 MotionEvent::ROUNDING_PRECISION) ||
                (std::abs(pointerCoords.getY() - mSample.pointers[pointerIndex].y) >
                 MotionEvent::ROUNDING_PRECISION)) {
                *os << "sample coordinates mismatch at pointer index " << pointerIndex
                    << ". sample: (" << pointerCoords.getX() << ", " << pointerCoords.getY()
                    << ") expected: (" << mSample.pointers[pointerIndex].x << ", "
                    << mSample.pointers[pointerIndex].y << ")";
                return false;
            }

            if (motionEvent.isResampled(pointerIndex, mSampleIndex) !=
                mSample.pointers[pointerIndex].isResampled) {
                *os << "resampling flag mismatch. sample: "
                    << motionEvent.isResampled(pointerIndex, mSampleIndex)
                    << " expected: " << mSample.pointers[pointerIndex].isResampled;
                return false;
            }
        }
        return true;
    }

    void DescribeTo(std::ostream* os) const { *os << "motion event sample properties match."; }

    void DescribeNegationTo(std::ostream* os) const {
        *os << "motion event sample properties do not match expected properties.";
    }

private:
    const size_t mSampleIndex;
    const Sample mSample;
};

inline WithSampleMatcher WithSample(size_t sampleIndex, const Sample& sample) {
    return WithSampleMatcher(sampleIndex, sample);
}

} // namespace android
