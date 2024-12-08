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

#define LOG_TAG "LegacyResampler"

#include <algorithm>
#include <chrono>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <ftl/enum.h>

#include <input/Resampler.h>
#include <utils/Timers.h>

using std::chrono::nanoseconds;

namespace android {

namespace {

const bool IS_DEBUGGABLE_BUILD =
#if defined(__ANDROID__)
        android::base::GetBoolProperty("ro.debuggable", false);
#else
        true;
#endif

bool debugResampling() {
    if (!IS_DEBUGGABLE_BUILD) {
        static const bool DEBUG_TRANSPORT_RESAMPLING =
                __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling",
                                          ANDROID_LOG_INFO);
        return DEBUG_TRANSPORT_RESAMPLING;
    }
    return __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling", ANDROID_LOG_INFO);
}

constexpr std::chrono::milliseconds RESAMPLE_LATENCY{5};

constexpr std::chrono::milliseconds RESAMPLE_MIN_DELTA{2};

constexpr std::chrono::milliseconds RESAMPLE_MAX_DELTA{20};

constexpr std::chrono::milliseconds RESAMPLE_MAX_PREDICTION{8};

bool canResampleTool(ToolType toolType) {
    return toolType == ToolType::FINGER || toolType == ToolType::MOUSE ||
            toolType == ToolType::STYLUS || toolType == ToolType::UNKNOWN;
}

inline float lerp(float a, float b, float alpha) {
    return a + alpha * (b - a);
}

PointerCoords calculateResampledCoords(const PointerCoords& a, const PointerCoords& b,
                                       float alpha) {
    // We use the value of alpha to initialize resampledCoords with the latest sample information.
    PointerCoords resampledCoords = (alpha < 1.0f) ? a : b;
    resampledCoords.isResampled = true;
    resampledCoords.setAxisValue(AMOTION_EVENT_AXIS_X, lerp(a.getX(), b.getX(), alpha));
    resampledCoords.setAxisValue(AMOTION_EVENT_AXIS_Y, lerp(a.getY(), b.getY(), alpha));
    return resampledCoords;
}
} // namespace

void LegacyResampler::updateLatestSamples(const MotionEvent& motionEvent) {
    const size_t numSamples = motionEvent.getHistorySize() + 1;
    const size_t latestIndex = numSamples - 1;
    const size_t secondToLatestIndex = (latestIndex > 0) ? (latestIndex - 1) : 0;
    for (size_t sampleIndex = secondToLatestIndex; sampleIndex < numSamples; ++sampleIndex) {
        std::vector<Pointer> pointers;
        const size_t numPointers = motionEvent.getPointerCount();
        for (size_t pointerIndex = 0; pointerIndex < numPointers; ++pointerIndex) {
            // getSamplePointerCoords is the vector representation of a getHistorySize by
            // getPointerCount matrix.
            const PointerCoords& pointerCoords =
                    motionEvent.getSamplePointerCoords()[sampleIndex * numPointers + pointerIndex];
            pointers.push_back(
                    Pointer{*motionEvent.getPointerProperties(pointerIndex), pointerCoords});
        }
        mLatestSamples.pushBack(
                Sample{nanoseconds{motionEvent.getHistoricalEventTime(sampleIndex)}, pointers});
    }
}

LegacyResampler::Sample LegacyResampler::messageToSample(const InputMessage& message) {
    std::vector<Pointer> pointers;
    for (uint32_t i = 0; i < message.body.motion.pointerCount; ++i) {
        pointers.push_back(Pointer{message.body.motion.pointers[i].properties,
                                   message.body.motion.pointers[i].coords});
    }
    return Sample{nanoseconds{message.body.motion.eventTime}, pointers};
}

bool LegacyResampler::pointerPropertiesResampleable(const Sample& target, const Sample& auxiliary) {
    if (target.pointers.size() > auxiliary.pointers.size()) {
        LOG_IF(INFO, debugResampling())
                << "Not resampled. Auxiliary sample has fewer pointers than target sample.";
        return false;
    }
    for (size_t i = 0; i < target.pointers.size(); ++i) {
        if (target.pointers[i].properties.id != auxiliary.pointers[i].properties.id) {
            LOG_IF(INFO, debugResampling()) << "Not resampled. Pointer ID mismatch.";
            return false;
        }
        if (target.pointers[i].properties.toolType != auxiliary.pointers[i].properties.toolType) {
            LOG_IF(INFO, debugResampling()) << "Not resampled. Pointer ToolType mismatch.";
            return false;
        }
        if (!canResampleTool(target.pointers[i].properties.toolType)) {
            LOG_IF(INFO, debugResampling())
                    << "Not resampled. Cannot resample "
                    << ftl::enum_string(target.pointers[i].properties.toolType) << " ToolType.";
            return false;
        }
    }
    return true;
}

bool LegacyResampler::canInterpolate(const InputMessage& message) const {
    LOG_IF(FATAL, mLatestSamples.empty())
            << "Not resampled. mLatestSamples must not be empty to interpolate.";

    const Sample& pastSample = *(mLatestSamples.end() - 1);
    const Sample& futureSample = messageToSample(message);

    if (!pointerPropertiesResampleable(pastSample, futureSample)) {
        return false;
    }

    const nanoseconds delta = futureSample.eventTime - pastSample.eventTime;
    if (delta < RESAMPLE_MIN_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too small: " << delta << "ns.";
        return false;
    }
    return true;
}

std::optional<LegacyResampler::Sample> LegacyResampler::attemptInterpolation(
        nanoseconds resampleTime, const InputMessage& futureSample) const {
    if (!canInterpolate(futureSample)) {
        return std::nullopt;
    }
    LOG_IF(FATAL, mLatestSamples.empty())
            << "Not resampled. mLatestSamples must not be empty to interpolate.";

    const Sample& pastSample = *(mLatestSamples.end() - 1);

    const nanoseconds delta =
            nanoseconds{futureSample.body.motion.eventTime} - pastSample.eventTime;
    const float alpha =
            std::chrono::duration<float, std::milli>(resampleTime - pastSample.eventTime) / delta;

    std::vector<Pointer> resampledPointers;
    for (size_t i = 0; i < pastSample.pointers.size(); ++i) {
        const PointerCoords& resampledCoords =
                calculateResampledCoords(pastSample.pointers[i].coords,
                                         futureSample.body.motion.pointers[i].coords, alpha);
        resampledPointers.push_back(Pointer{pastSample.pointers[i].properties, resampledCoords});
    }
    return Sample{resampleTime, resampledPointers};
}

bool LegacyResampler::canExtrapolate() const {
    if (mLatestSamples.size() < 2) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Not enough data.";
        return false;
    }

    const Sample& pastSample = *(mLatestSamples.end() - 2);
    const Sample& presentSample = *(mLatestSamples.end() - 1);

    if (!pointerPropertiesResampleable(presentSample, pastSample)) {
        return false;
    }

    const nanoseconds delta = presentSample.eventTime - pastSample.eventTime;
    if (delta < RESAMPLE_MIN_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too small: " << delta << "ns.";
        return false;
    } else if (delta > RESAMPLE_MAX_DELTA) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Delta is too large: " << delta << "ns.";
        return false;
    }
    return true;
}

std::optional<LegacyResampler::Sample> LegacyResampler::attemptExtrapolation(
        nanoseconds resampleTime) const {
    if (!canExtrapolate()) {
        return std::nullopt;
    }
    LOG_IF(FATAL, mLatestSamples.size() < 2)
            << "Not resampled. mLatestSamples must have at least two samples to extrapolate.";

    const Sample& pastSample = *(mLatestSamples.end() - 2);
    const Sample& presentSample = *(mLatestSamples.end() - 1);

    const nanoseconds delta = presentSample.eventTime - pastSample.eventTime;
    // The farthest future time to which we can extrapolate. If the given resampleTime exceeds this,
    // we use this value as the resample time target.
    const nanoseconds farthestPrediction =
            presentSample.eventTime + std::min<nanoseconds>(delta / 2, RESAMPLE_MAX_PREDICTION);
    const nanoseconds newResampleTime =
            (resampleTime > farthestPrediction) ? (farthestPrediction) : (resampleTime);
    LOG_IF(INFO, debugResampling() && newResampleTime == farthestPrediction)
            << "Resample time is too far in the future. Adjusting prediction from "
            << (resampleTime - presentSample.eventTime) << " to "
            << (farthestPrediction - presentSample.eventTime) << "ns.";
    const float alpha =
            std::chrono::duration<float, std::milli>(newResampleTime - pastSample.eventTime) /
            delta;

    std::vector<Pointer> resampledPointers;
    for (size_t i = 0; i < presentSample.pointers.size(); ++i) {
        const PointerCoords& resampledCoords =
                calculateResampledCoords(pastSample.pointers[i].coords,
                                         presentSample.pointers[i].coords, alpha);
        resampledPointers.push_back(Pointer{presentSample.pointers[i].properties, resampledCoords});
    }
    return Sample{newResampleTime, resampledPointers};
}

inline void LegacyResampler::addSampleToMotionEvent(const Sample& sample,
                                                    MotionEvent& motionEvent) {
    motionEvent.addSample(sample.eventTime.count(), sample.asPointerCoords().data(),
                          motionEvent.getId());
}

nanoseconds LegacyResampler::getResampleLatency() const {
    return RESAMPLE_LATENCY;
}

void LegacyResampler::resampleMotionEvent(nanoseconds frameTime, MotionEvent& motionEvent,
                                          const InputMessage* futureSample) {
    if (mPreviousDeviceId && *mPreviousDeviceId != motionEvent.getDeviceId()) {
        mLatestSamples.clear();
    }
    mPreviousDeviceId = motionEvent.getDeviceId();

    const nanoseconds resampleTime = frameTime - RESAMPLE_LATENCY;

    updateLatestSamples(motionEvent);

    const std::optional<Sample> sample = (futureSample != nullptr)
            ? (attemptInterpolation(resampleTime, *futureSample))
            : (attemptExtrapolation(resampleTime));
    if (sample.has_value()) {
        addSampleToMotionEvent(*sample, motionEvent);
    }
}
} // namespace android
