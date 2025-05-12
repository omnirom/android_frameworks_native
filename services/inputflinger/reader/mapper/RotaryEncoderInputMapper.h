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

#pragma once

#include <ui/Rotation.h>

#include "CursorScrollAccumulator.h"
#include "InputMapper.h"
#include "SlopController.h"

namespace android {

class RotaryEncoderInputMapper : public InputMapper {
public:
    template <class T, class... Args>
    friend std::unique_ptr<T> createInputMapper(InputDeviceContext& deviceContext,
                                                const InputReaderConfiguration& readerConfig,
                                                Args... args);
    virtual ~RotaryEncoderInputMapper();

    virtual uint32_t getSources() const override;
    virtual void populateDeviceInfo(InputDeviceInfo& deviceInfo) override;
    virtual void dump(std::string& dump) override;
    [[nodiscard]] std::list<NotifyArgs> reconfigure(nsecs_t when,
                                                    const InputReaderConfiguration& config,
                                                    ConfigurationChanges changes) override;
    [[nodiscard]] std::list<NotifyArgs> reset(nsecs_t when) override;
    [[nodiscard]] std::list<NotifyArgs> process(const RawEvent& rawEvent) override;

private:
    CursorScrollAccumulator mRotaryEncoderScrollAccumulator;

    int32_t mSource;
    float mScalingFactor;
    /** Units per rotation, provided via the `device.res` IDC property. */
    float mResolution;
    ui::Rotation mOrientation;
    /**
     * The minimum number of rotations to log for telemetry.
     * Provided via `rotary_encoder.min_rotations_to_log` IDC property. If no value is provided in
     * the IDC file, or if a non-positive value is provided, the telemetry will be disabled, and
     * this value is set to the empty optional.
     */
    std::optional<int32_t> mMinRotationsToLog;
    /**
     * A function to log count for telemetry.
     * The char* is the logging key, and the int64_t is the value to log.
     * Abstracting the actual logging APIs via this function is helpful for simple unit testing.
     */
    std::function<void(const char*, int64_t)> mTelemetryLogCounter;
    ui::LogicalDisplayId mDisplayId = ui::LogicalDisplayId::INVALID;
    std::unique_ptr<SlopController> mSlopController;

    /** Amount of raw scrolls (pre-slop) not yet logged for telemetry. */
    float mUnloggedScrolls = 0;

    explicit RotaryEncoderInputMapper(InputDeviceContext& deviceContext,
                                      const InputReaderConfiguration& readerConfig);

    /** This is a test constructor that allows injecting the expresslog Counter logic. */
    RotaryEncoderInputMapper(InputDeviceContext& deviceContext,
                             const InputReaderConfiguration& readerConfig,
                             std::function<void(const char*, int64_t)> expressLogCounter);
    [[nodiscard]] std::list<NotifyArgs> sync(nsecs_t when, nsecs_t readTime);

    /** Logs a given amount of scroll for telemetry. */
    void logScroll(float scroll);
};

} // namespace android
