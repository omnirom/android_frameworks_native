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
#define LOG_TAG "ExternalVibrationUtils"

#include <cstring>

#include <android_os_vibrator.h>

#include <algorithm>
#include <math.h>

#include <log/log.h>
#include <vibrator/ExternalVibrationUtils.h>

namespace android::os {

namespace {
static constexpr float SCALE_GAMMA = 0.65f; // Same as VibrationEffect.SCALE_GAMMA
static constexpr float SCALE_LEVEL_GAIN = 1.4f; // Same as VibrationConfig.DEFAULT_SCALE_LEVEL_GAIN

/* Same as VibrationScaler.getScaleFactor */
float getHapticScaleFactor(HapticScale scale) {
    if (android_os_vibrator_haptics_scale_v2_enabled()) {
        if (scale.getScaleFactor() >= 0) {
            // ExternalVibratorService provided the scale factor, use it.
            return scale.getScaleFactor();
        }

        HapticLevel level = scale.getLevel();
        switch (level) {
            case HapticLevel::MUTE:
                return 0.0f;
            case HapticLevel::NONE:
                return 1.0f;
            default:
                float scaleFactor = powf(SCALE_LEVEL_GAIN, static_cast<int32_t>(level));
                if (scaleFactor <= 0) {
                    ALOGE("Invalid scale factor %.2f for level %d, using fallback to 1.0",
                          scaleFactor, static_cast<int32_t>(level));
                    scaleFactor = 1.0f;
                }
                return scaleFactor;
        }
    }
    // Same as VibrationScaler.SCALE_FACTOR_*
    switch (scale.getLevel()) {
        case HapticLevel::MUTE:
            return 0.0f;
        case HapticLevel::VERY_LOW:
            return 0.6f;
        case HapticLevel::LOW:
            return 0.8f;
        case HapticLevel::HIGH:
            return 1.2f;
        case HapticLevel::VERY_HIGH:
            return 1.4f;
        default:
            return 1.0f;
    }
}

float applyHapticScale(float value, float scaleFactor) {
    if (android_os_vibrator_haptics_scale_v2_enabled()) {
        // Using S * x / (1 + (S - 1) * x^2) as the scale up function to converge to 1.0.
        float scaledValue = (scaleFactor <= 1 || value == 0)
                ? (value * scaleFactor)
                : (value * scaleFactor) / (1 + (scaleFactor - 1) * value * value);
        if (android_os_vibrator_vibration_scale_bounds_fix_enabled()) {
            return std::clamp(scaledValue, -1.0f, 1.0f);
        } else {
            return scaledValue;
        }
    }
    float scale = powf(scaleFactor, 1.0f / SCALE_GAMMA);
    if (scaleFactor <= 1) {
        // Scale down is simply a gamma corrected application of scaleFactor to the intensity.
        // Scale up requires a different curve to ensure the intensity will not become > 1.
        return value * scale;
    }

    float sign = value >= 0 ? 1.0f : -1.0f;
    float extraScale = powf(scaleFactor, 4.0f - scaleFactor);
    float x = fabsf(value) * scale * extraScale;
    float maxX = scale * extraScale; // scaled x for intensity == 1

    float expX = expf(x);
    float expMaxX = expf(maxX);

    // Using f = tanh as the scale up function so the max value will converge.
    // a = 1/f(maxX), used to scale f so that a*f(maxX) = 1 (the value will converge to 1).
    float a = (expMaxX + 1.0f) / (expMaxX - 1.0f);
    float fx = (expX - 1.0f) / (expX + 1.0f);

    return sign * std::clamp(a * fx, 0.0f, 1.0f);
}

// TODO(b/345186129): remove this once flag android_os_vibrator_haptics_scale_v2_enabled removed
float applyHapticScale(float value, HapticScale scale, float scaleFactor) {
    if (android_os_vibrator_vibration_scale_device_config_enabled()) {
        if (scale.getScaleFactor() >= 0) {
            // Has device configured scale factor, use scale v2 for it.
            scaleFactor = scale.getScaleFactor();
            float scaledValue = (scaleFactor <= 1 || value == 0)
                    ? (value * scaleFactor)
                    : (value * scaleFactor) / (1 + (scaleFactor - 1) * value * value);
            return std::clamp(scaledValue, -1.0f, 1.0f);
        } // else apply regular scaling
    }
    if (scale.getLevel() == HapticLevel::NONE) {
        return value;
    }
    return applyHapticScale(value, scaleFactor);
}

void applyHapticScale(float* buffer, size_t length, HapticScale scale) {
    if (scale.isScaleMute()) {
        memset(buffer, 0, length * sizeof(float));
        return;
    }
    if (scale.isScaleNone()) {
        return;
    }
    float scaleFactor = getHapticScaleFactor(scale);
    float adaptiveScaleFactor = scale.getAdaptiveScaleFactor();

    for (size_t i = 0; i < length; i++) {
        buffer[i] = applyHapticScale(buffer[i], scale, scaleFactor);
        if (adaptiveScaleFactor >= 0 && adaptiveScaleFactor != 1.0f) {
            if (android_os_vibrator_vibration_scale_bounds_fix_enabled()) {
                buffer[i] = std::clamp(buffer[i] * adaptiveScaleFactor, -1.0f, 1.0f);
            } else {
                buffer[i] *= adaptiveScaleFactor;
            }
        }
    }
}

void clipHapticData(float* buffer, size_t length, float limit) {
    if (android_os_vibrator_vibration_scale_bounds_fix_enabled()) {
        if (isnan(limit) || limit <= 0 || limit >= 1) {
            return;
        }
        for (size_t i = 0; i < length; i++) {
            buffer[i] = std::clamp(buffer[i], -limit, limit);
        }
        return;
    }
    if (isnan(limit) || limit == 0) {
        return;
    }
    limit = fabsf(limit);
    for (size_t i = 0; i < length; i++) {
        float sign = buffer[i] >= 0 ? 1.0 : -1.0;
        if (fabsf(buffer[i]) > limit) {
            buffer[i] = limit * sign;
        }
    }
}

} // namespace

bool isValidHapticScale(HapticScale scale) {
    switch (scale.getLevel()) {
    case HapticLevel::MUTE:
    case HapticLevel::VERY_LOW:
    case HapticLevel::LOW:
    case HapticLevel::NONE:
    case HapticLevel::HIGH:
    case HapticLevel::VERY_HIGH:
        return true;
    }
    return false;
}

void scaleHapticData(float* buffer, size_t length, HapticScale scale, float limit) {
    if (isValidHapticScale(scale)) {
        applyHapticScale(buffer, length, scale);
    }
    clipHapticData(buffer, length, limit);
}

} // namespace android::os
