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

#include <ostream>

#include <input/Input.h>

namespace android {

/**
 * This file contains a copy of Matchers from .../inputflinger/tests/TestEventMatchers.h. Ideally,
 * implementations must not be duplicated.
 * TODO(b/365606513): Find a way to share TestEventMatchers.h between inputflinger and libinput.
 */

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

    bool MatchAndExplain(const MotionEvent& event, std::ostream*) const {
        bool matches = mAction == event.getAction();
        if (event.getAction() == AMOTION_EVENT_ACTION_CANCEL) {
            matches &= (event.getFlags() & AMOTION_EVENT_FLAG_CANCELED) != 0;
        }
        return matches;
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

class MotionEventIsResampledMatcher {
public:
    using is_gtest_matcher = void;

    bool MatchAndExplain(const MotionEvent& motionEvent, std::ostream*) const {
        const size_t numSamples = motionEvent.getHistorySize() + 1;
        const size_t numPointers = motionEvent.getPointerCount();
        if (numPointers <= 0 || numSamples <= 0) {
            return false;
        }
        for (size_t i = 0; i < numPointers; ++i) {
            const PointerCoords& pointerCoords =
                    motionEvent.getSamplePointerCoords()[numSamples * numPointers + i];
            if (!pointerCoords.isResampled) {
                return false;
            }
        }
        return true;
    }

    void DescribeTo(std::ostream* os) const { *os << "MotionEvent is resampled."; }

    void DescribeNegationTo(std::ostream* os) const { *os << "MotionEvent is not resampled."; }
};

inline MotionEventIsResampledMatcher MotionEventIsResampled() {
    return MotionEventIsResampledMatcher();
}
} // namespace android
