/*
 * Copyright (C) 2019 The Android Open Source Project
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

// clang-format off
#include "../Macros.h"
// clang-format on

#include "RotaryEncoderInputMapper.h"

#include <Counter.h>
#include <com_android_input_flags.h>
#include <utils/Timers.h>
#include <optional>

#include "CursorScrollAccumulator.h"

namespace android {

using android::expresslog::Counter;

constexpr float kDefaultResolution = 0;
constexpr float kDefaultScaleFactor = 1.0f;
constexpr int32_t kDefaultMinRotationsToLog = 3;

RotaryEncoderInputMapper::RotaryEncoderInputMapper(InputDeviceContext& deviceContext,
                                                   const InputReaderConfiguration& readerConfig)
      : RotaryEncoderInputMapper(deviceContext, readerConfig,
                                 Counter::logIncrement /* telemetryLogCounter */) {}

RotaryEncoderInputMapper::RotaryEncoderInputMapper(
        InputDeviceContext& deviceContext, const InputReaderConfiguration& readerConfig,
        std::function<void(const char*, int64_t)> telemetryLogCounter)
      : InputMapper(deviceContext, readerConfig),
        mSource(AINPUT_SOURCE_ROTARY_ENCODER),
        mScalingFactor(kDefaultScaleFactor),
        mResolution(kDefaultResolution),
        mOrientation(ui::ROTATION_0),
        mTelemetryLogCounter(telemetryLogCounter) {}

RotaryEncoderInputMapper::~RotaryEncoderInputMapper() {}

uint32_t RotaryEncoderInputMapper::getSources() const {
    return mSource;
}

void RotaryEncoderInputMapper::populateDeviceInfo(InputDeviceInfo& info) {
    InputMapper::populateDeviceInfo(info);

    if (mRotaryEncoderScrollAccumulator.haveRelativeVWheel()) {
        const PropertyMap& config = getDeviceContext().getConfiguration();
        std::optional<float> res = config.getFloat("device.res");
        if (!res.has_value()) {
            ALOGW("Rotary Encoder device configuration file didn't specify resolution!\n");
        }
        mResolution = res.value_or(kDefaultResolution);
        std::optional<float> scalingFactor = config.getFloat("device.scalingFactor");
        if (!scalingFactor.has_value()) {
            ALOGW("Rotary Encoder device configuration file didn't specify scaling factor,"
                  "default to %f!\n",
                  kDefaultScaleFactor);
        }
        mScalingFactor = scalingFactor.value_or(kDefaultScaleFactor);
        info.addMotionRange(AMOTION_EVENT_AXIS_SCROLL, mSource, -1.0f, 1.0f, 0.0f, 0.0f,
                            mResolution * mScalingFactor);

        if (com::android::input::flags::rotary_input_telemetry()) {
            mMinRotationsToLog = config.getInt("rotary_encoder.min_rotations_to_log");
            if (!mMinRotationsToLog.has_value()) {
                ALOGI("Rotary Encoder device configuration file didn't specify min log rotation.");
            } else if (*mMinRotationsToLog <= 0) {
                ALOGE("Rotary Encoder device configuration specified non-positive min log rotation "
                      ": %d. Telemetry logging of rotations disabled.",
                      *mMinRotationsToLog);
                mMinRotationsToLog = {};
            } else {
                ALOGD("Rotary Encoder telemetry enabled. mMinRotationsToLog=%d",
                      *mMinRotationsToLog);
            }
        }
    }
}

void RotaryEncoderInputMapper::dump(std::string& dump) {
    dump += INDENT2 "Rotary Encoder Input Mapper:\n";
    dump += StringPrintf(INDENT3 "HaveWheel: %s\n",
                         toString(mRotaryEncoderScrollAccumulator.haveRelativeVWheel()));
    dump += StringPrintf(INDENT3 "HaveSlopController: %s\n", toString(mSlopController != nullptr));
}

std::list<NotifyArgs> RotaryEncoderInputMapper::reconfigure(nsecs_t when,
                                                            const InputReaderConfiguration& config,
                                                            ConfigurationChanges changes) {
    std::list<NotifyArgs> out = InputMapper::reconfigure(when, config, changes);
    if (!changes.any()) {
        mRotaryEncoderScrollAccumulator.configure(getDeviceContext());

        const PropertyMap& propertyMap = getDeviceContext().getConfiguration();
        float slopThreshold = propertyMap.getInt("rotary_encoder.slop_threshold").value_or(0);
        int32_t slopDurationNs = milliseconds_to_nanoseconds(
                propertyMap.getInt("rotary_encoder.slop_duration_ms").value_or(0));
        if (slopThreshold > 0 && slopDurationNs > 0) {
            mSlopController = std::make_unique<SlopController>(slopThreshold, slopDurationNs);
        } else {
            mSlopController = nullptr;
        }
    }
    if (!changes.any() || changes.test(InputReaderConfiguration::Change::DISPLAY_INFO)) {
        if (getDeviceContext().getAssociatedViewport()) {
            mDisplayId = getDeviceContext().getAssociatedViewport()->displayId;
            mOrientation = getDeviceContext().getAssociatedViewport()->orientation;
        } else {
            mDisplayId = ui::LogicalDisplayId::INVALID;
            std::optional<DisplayViewport> internalViewport =
                    config.getDisplayViewportByType(ViewportType::INTERNAL);
            if (internalViewport) {
                mOrientation = internalViewport->orientation;
            } else {
                mOrientation = ui::ROTATION_0;
            }
        }
    }
    return out;
}

std::list<NotifyArgs> RotaryEncoderInputMapper::reset(nsecs_t when) {
    mRotaryEncoderScrollAccumulator.reset(getDeviceContext());

    return InputMapper::reset(when);
}

std::list<NotifyArgs> RotaryEncoderInputMapper::process(const RawEvent& rawEvent) {
    std::list<NotifyArgs> out;
    mRotaryEncoderScrollAccumulator.process(rawEvent);

    if (rawEvent.type == EV_SYN && rawEvent.code == SYN_REPORT) {
        out += sync(rawEvent.when, rawEvent.readTime);
    }
    return out;
}

void RotaryEncoderInputMapper::logScroll(float scroll) {
    if (mResolution <= 0 || !mMinRotationsToLog) return;

    mUnloggedScrolls += fabs(scroll);

    // unitsPerRotation = (2 * PI * radians) * (units per radian (i.e. resolution))
    const float unitsPerRotation = 2 * M_PI * mResolution;
    const float scrollsPerMinRotationsToLog = *mMinRotationsToLog * unitsPerRotation;
    const int32_t numMinRotationsToLog =
            static_cast<int32_t>(mUnloggedScrolls / scrollsPerMinRotationsToLog);
    mUnloggedScrolls = std::fmod(mUnloggedScrolls, scrollsPerMinRotationsToLog);
    if (numMinRotationsToLog) {
        mTelemetryLogCounter("input.value_rotary_input_device_full_rotation_count",
                             numMinRotationsToLog * (*mMinRotationsToLog));
    }
}

std::list<NotifyArgs> RotaryEncoderInputMapper::sync(nsecs_t when, nsecs_t readTime) {
    std::list<NotifyArgs> out;

    float scroll = mRotaryEncoderScrollAccumulator.getRelativeVWheel();
    logScroll(scroll);

    if (mSlopController) {
        scroll = mSlopController->consumeEvent(when, scroll);
    }

    bool scrolled = scroll != 0;

    // Send motion event.
    if (scrolled) {
        int32_t metaState = getContext()->getGlobalMetaState();

        if (mOrientation == ui::ROTATION_180) {
            scroll = -scroll;
        }

        PointerCoords pointerCoords;
        pointerCoords.clear();
        pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_SCROLL, scroll * mScalingFactor);

        PointerProperties pointerProperties;
        pointerProperties.clear();
        pointerProperties.id = 0;
        pointerProperties.toolType = ToolType::UNKNOWN;

        uint32_t policyFlags = 0;
        if (getDeviceContext().isExternal()) {
            policyFlags |= POLICY_FLAG_WAKE;
        }

        out.push_back(
                NotifyMotionArgs(getContext()->getNextId(), when, readTime, getDeviceId(), mSource,
                                 mDisplayId, policyFlags, AMOTION_EVENT_ACTION_SCROLL, 0, 0,
                                 metaState, /*buttonState=*/0, MotionClassification::NONE,
                                 AMOTION_EVENT_EDGE_FLAG_NONE, 1, &pointerProperties,
                                 &pointerCoords, 0, 0, AMOTION_EVENT_INVALID_CURSOR_POSITION,
                                 AMOTION_EVENT_INVALID_CURSOR_POSITION, 0, /*videoFrames=*/{}));
    }

    mRotaryEncoderScrollAccumulator.finishSync();
    return out;
}

} // namespace android
