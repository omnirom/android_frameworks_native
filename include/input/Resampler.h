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
#include <vector>

#include <input/Input.h>
#include <input/InputTransport.h>
#include <input/RingBuffer.h>
#include <utils/Timers.h>

namespace android {

/**
 * Resampler is an interface for resampling MotionEvents. Every resampling implementation
 * must use this interface to enable resampling inside InputConsumer's logic.
 */
struct Resampler {
    virtual ~Resampler() = default;

    /**
     * Tries to resample motionEvent at frameTime. The provided frameTime must be greater than
     * the latest sample time of motionEvent. It is not guaranteed that resampling occurs at
     * frameTime. Interpolation may occur is futureSample is available. Otherwise, motionEvent
     * may be resampled by another method, or not resampled at all. Furthermore, it is the
     * implementer's responsibility to guarantee the following:
     * - If resampling occurs, a single additional sample should be added to motionEvent. That is,
     * if motionEvent had N samples before being passed to Resampler, then it will have N + 1
     * samples by the end of the resampling. No other field of motionEvent should be modified.
     * - If resampling does not occur, then motionEvent must not be modified in any way.
     */
    virtual void resampleMotionEvent(std::chrono::nanoseconds frameTime, MotionEvent& motionEvent,
                                     const InputMessage* futureSample) = 0;

    /**
     * Returns resample latency. Resample latency is the time difference between frame time and
     * resample time. More precisely, let frameTime and resampleTime be two timestamps, and
     * frameTime > resampleTime. Resample latency is defined as frameTime - resampleTime.
     */
    virtual std::chrono::nanoseconds getResampleLatency() const = 0;
};

class LegacyResampler final : public Resampler {
public:
    /**
     * Tries to resample `motionEvent` at `frameTime` by adding a resampled sample at the end of
     * `motionEvent` with eventTime equal to `resampleTime` and pointer coordinates determined by
     * linear interpolation or linear extrapolation. An earlier `resampleTime` will be used if
     * extrapolation takes place and `resampleTime` is too far in the future. If `futureSample` is
     * not null, interpolation will occur. If `futureSample` is null and there is enough historical
     * data, LegacyResampler will extrapolate. Otherwise, no resampling takes place and
     * `motionEvent` is unmodified.
     */
    void resampleMotionEvent(std::chrono::nanoseconds frameTime, MotionEvent& motionEvent,
                             const InputMessage* futureSample) override;

    std::chrono::nanoseconds getResampleLatency() const override;

private:
    struct Pointer {
        PointerProperties properties;
        PointerCoords coords;
    };

    struct Sample {
        std::chrono::nanoseconds eventTime;
        std::vector<Pointer> pointers;

        std::vector<PointerCoords> asPointerCoords() const {
            std::vector<PointerCoords> pointersCoords;
            for (const Pointer& pointer : pointers) {
                pointersCoords.push_back(pointer.coords);
            }
            return pointersCoords;
        }
    };

    /**
     * Keeps track of the previous MotionEvent deviceId to enable comparison between the previous
     * and the current deviceId.
     */
    std::optional<DeviceId> mPreviousDeviceId;

    /**
     * Up to two latest samples from MotionEvent. Updated every time resampleMotionEvent is called.
     * Note: We store up to two samples in order to simplify the implementation. Although,
     * calculations are possible with only one previous sample.
     */
    RingBuffer<Sample> mLatestSamples{/*capacity=*/2};

    /**
     * Adds up to mLatestSamples.capacity() of motionEvent's latest samples to mLatestSamples. If
     * motionEvent has fewer samples than mLatestSamples.capacity(), then the available samples are
     * added to mLatestSamples.
     */
    void updateLatestSamples(const MotionEvent& motionEvent);

    static Sample messageToSample(const InputMessage& message);

    /**
     * Checks if auxiliary sample has the same pointer properties of target sample. That is,
     * auxiliary pointer IDs must appear in the same order as target pointer IDs, their toolType
     * must match and be resampleable.
     */
    static bool pointerPropertiesResampleable(const Sample& target, const Sample& auxiliary);

    /**
     * Checks if there are necessary conditions to interpolate. For example, interpolation cannot
     * take place if samples are too far apart in time. mLatestSamples must have at least one sample
     * when canInterpolate is invoked.
     */
    bool canInterpolate(const InputMessage& futureSample) const;

    /**
     * Returns a sample interpolated between the latest sample of mLatestSamples and futureSample,
     * if the conditions from canInterpolate are satisfied. Otherwise, returns nullopt.
     * mLatestSamples must have at least one sample when attemptInterpolation is called.
     */
    std::optional<Sample> attemptInterpolation(std::chrono::nanoseconds resampleTime,
                                               const InputMessage& futureSample) const;

    /**
     * Checks if there are necessary conditions to extrapolate. That is, there are at least two
     * samples in mLatestSamples, and delta is bounded within a time interval.
     */
    bool canExtrapolate() const;

    /**
     * Returns a sample extrapolated from the two samples of mLatestSamples, if the conditions from
     * canExtrapolate are satisfied. The returned sample either has eventTime equal to resampleTime,
     * or an earlier time if resampleTime is too far in the future. If canExtrapolate returns false,
     * this function returns nullopt.
     */
    std::optional<Sample> attemptExtrapolation(std::chrono::nanoseconds resampleTime) const;

    inline static void addSampleToMotionEvent(const Sample& sample, MotionEvent& motionEvent);
};
} // namespace android
