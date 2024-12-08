/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_EXTERNAL_VIBRATION_UTILS_H
#define ANDROID_EXTERNAL_VIBRATION_UTILS_H

#include <cstring>
#include <sstream>
#include <string>

namespace android::os {

enum class HapticLevel : int32_t {
    MUTE = -100,
    VERY_LOW = -2,
    LOW = -1,
    NONE = 0,
    HIGH = 1,
    VERY_HIGH = 2,
};

class HapticScale {
private:
HapticLevel mLevel = HapticLevel::NONE;
float mScaleFactor = -1.0f; // undefined, use haptic level to define scale factor
float mAdaptiveScaleFactor = 1.0f;

public:
    explicit HapticScale(HapticLevel level, float scaleFactor, float adaptiveScaleFactor)
          : mLevel(level), mScaleFactor(scaleFactor), mAdaptiveScaleFactor(adaptiveScaleFactor) {}
    explicit HapticScale(HapticLevel level) : mLevel(level) {}
    constexpr HapticScale() {}

    HapticLevel getLevel() const { return mLevel; }
    float getScaleFactor() const { return mScaleFactor; }
    float getAdaptiveScaleFactor() const { return mAdaptiveScaleFactor; }

    bool operator==(const HapticScale& other) const {
        return mLevel == other.mLevel && mScaleFactor == other.mScaleFactor &&
                mAdaptiveScaleFactor == other.mAdaptiveScaleFactor;
    }

bool isScaleNone() const {
    return (mLevel == HapticLevel::NONE || mScaleFactor == 1.0f) && mAdaptiveScaleFactor == 1.0f;
}

bool isScaleMute() const {
    return mLevel == HapticLevel::MUTE || mScaleFactor == 0 || mAdaptiveScaleFactor == 0;
}

std::string toString() const {
    std::ostringstream os;
    os << "HapticScale { level: " << static_cast<int>(mLevel);
    os << ", scaleFactor: " << mScaleFactor;
    os << ", adaptiveScaleFactor: " << mAdaptiveScaleFactor;
    os << "}";
    return os.str();
}

static HapticScale mute() { return os::HapticScale(os::HapticLevel::MUTE); }

static HapticScale none() { return os::HapticScale(os::HapticLevel::NONE); }
};

bool isValidHapticScale(HapticScale scale);

/* Scales the haptic data in given buffer using the selected HapticScaleLevel and ensuring no
 * absolute value will be larger than the absolute of given limit.
 * The limit will be ignored if it is NaN or zero.
 */
void scaleHapticData(float* buffer, size_t length, HapticScale scale, float limit);

} // namespace android::os

#endif // ANDROID_EXTERNAL_VIBRATION_UTILS_H
