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
#include <iomanip>
#include <ostream>

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <ftl/enum.h>

#include <input/Resampler.h>
#include <utils/Timers.h>

namespace android {
namespace {

const bool IS_DEBUGGABLE_BUILD =
#if defined(__ANDROID__)
        android::base::GetBoolProperty("ro.debuggable", false);
#else
        true;
#endif

/**
 * Log debug messages about timestamp and coordinates of event resampling.
 * Enable this via "adb shell setprop log.tag.LegacyResamplerResampling DEBUG"
 * (requires restart)
 */
bool debugResampling() {
    if (!IS_DEBUGGABLE_BUILD) {
        static const bool DEBUG_TRANSPORT_RESAMPLING =
                __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling",
                                          ANDROID_LOG_INFO);
        return DEBUG_TRANSPORT_RESAMPLING;
    }
    return __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG "Resampling", ANDROID_LOG_INFO);
}

using std::chrono::nanoseconds;

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

bool equalXY(const PointerCoords& a, const PointerCoords& b) {
    return (a.getX() == b.getX()) && (a.getY() == b.getY());
}

void setMotionEventPointerCoords(MotionEvent& motionEvent, size_t sampleIndex, size_t pointerIndex,
                                 const PointerCoords& pointerCoords) {
    // Ideally, we should not cast away const. In this particular case, it's safe to cast away const
    // and dereference getHistoricalRawPointerCoords returned pointer because motionEvent is a
    // nonconst reference to a MotionEvent object, so mutating the object should not be undefined
    // behavior; moreover, the invoked method guarantees to return a valid pointer. Otherwise, it
    // fatally logs. Alternatively, we could've created a new MotionEvent from scratch, but this
    // approach is simpler and more efficient.
    PointerCoords& motionEventCoords = const_cast<PointerCoords&>(
            *(motionEvent.getHistoricalRawPointerCoords(pointerIndex, sampleIndex)));
    motionEventCoords.setAxisValue(AMOTION_EVENT_AXIS_X, pointerCoords.getX());
    motionEventCoords.setAxisValue(AMOTION_EVENT_AXIS_Y, pointerCoords.getY());
    motionEventCoords.isResampled = pointerCoords.isResampled;
}

std::ostream& operator<<(std::ostream& os, const PointerCoords& pointerCoords) {
    os << "(" << pointerCoords.getX() << ", " << pointerCoords.getY() << ")";
    return os;
}

} // namespace

void LegacyResampler::updateLatestSamples(const MotionEvent& motionEvent) {
    const size_t numSamples = motionEvent.getHistorySize() + 1;
    const size_t latestIndex = numSamples - 1;
    const size_t secondToLatestIndex = (latestIndex > 0) ? (latestIndex - 1) : 0;
    for (size_t sampleIndex = secondToLatestIndex; sampleIndex < numSamples; ++sampleIndex) {
        PointerMap pointerMap;
        for (size_t pointerIndex = 0; pointerIndex < motionEvent.getPointerCount();
             ++pointerIndex) {
            pointerMap.insert(Pointer{*(motionEvent.getPointerProperties(pointerIndex)),
                                      *(motionEvent.getHistoricalRawPointerCoords(pointerIndex,
                                                                                  sampleIndex))});
        }
        mLatestSamples.pushBack(
                Sample{nanoseconds{motionEvent.getHistoricalEventTime(sampleIndex)}, pointerMap});
    }
}

LegacyResampler::Sample LegacyResampler::messageToSample(const InputMessage& message) {
    PointerMap pointerMap;
    for (uint32_t i = 0; i < message.body.motion.pointerCount; ++i) {
        pointerMap.insert(Pointer{message.body.motion.pointers[i].properties,
                                  message.body.motion.pointers[i].coords});
    }
    return Sample{nanoseconds{message.body.motion.eventTime}, pointerMap};
}

bool LegacyResampler::pointerPropertiesResampleable(const Sample& target, const Sample& auxiliary) {
    for (const Pointer& pointer : target.pointerMap) {
        const std::optional<Pointer> auxiliaryPointer =
                auxiliary.pointerMap.find(PointerMap::PointerId{pointer.properties.id});
        if (!auxiliaryPointer.has_value()) {
            LOG_IF(INFO, debugResampling())
                    << "Not resampled. Auxiliary sample does not contain all pointers from target.";
            return false;
        }
        if (pointer.properties.toolType != auxiliaryPointer->properties.toolType) {
            LOG_IF(INFO, debugResampling()) << "Not resampled. Pointer ToolType mismatch.";
            return false;
        }
        if (!canResampleTool(pointer.properties.toolType)) {
            LOG_IF(INFO, debugResampling())
                    << "Not resampled. Cannot resample "
                    << ftl::enum_string(pointer.properties.toolType) << " ToolType.";
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
        LOG_IF(INFO, debugResampling())
                << "Not resampled. Delta is too small: " << std::setprecision(3)
                << std::chrono::duration<double, std::milli>{delta}.count() << "ms";
        return false;
    }
    return true;
}

std::optional<LegacyResampler::Sample> LegacyResampler::attemptInterpolation(
        nanoseconds resampleTime, const InputMessage& futureMessage) const {
    if (!canInterpolate(futureMessage)) {
        return std::nullopt;
    }
    LOG_IF(FATAL, mLatestSamples.empty())
            << "Not resampled. mLatestSamples must not be empty to interpolate.";

    const Sample& pastSample = *(mLatestSamples.end() - 1);
    const Sample& futureSample = messageToSample(futureMessage);

    const nanoseconds delta = nanoseconds{futureSample.eventTime} - pastSample.eventTime;
    const float alpha =
            std::chrono::duration<float, std::nano>(resampleTime - pastSample.eventTime) / delta;

    PointerMap resampledPointerMap;
    for (const Pointer& pointer : pastSample.pointerMap) {
        if (std::optional<Pointer> futureSamplePointer =
                    futureSample.pointerMap.find(PointerMap::PointerId{pointer.properties.id});
            futureSamplePointer.has_value()) {
            const PointerCoords& resampledCoords =
                    calculateResampledCoords(pointer.coords, futureSamplePointer->coords, alpha);
            resampledPointerMap.insert(Pointer{pointer.properties, resampledCoords});
        }
    }
    return Sample{resampleTime, resampledPointerMap};
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
        LOG_IF(INFO, debugResampling())
                << "Not resampled. Delta is too small: " << std::setprecision(3)
                << std::chrono::duration<double, std::milli>{delta}.count() << "ms";
        return false;
    } else if (delta > RESAMPLE_MAX_DELTA) {
        LOG_IF(INFO, debugResampling())
                << "Not resampled. Delta is too large: " << std::setprecision(3)
                << std::chrono::duration<double, std::milli>{delta}.count() << "ms";
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
            << std::setprecision(3)
            << std::chrono::duration<double, std::milli>{resampleTime - presentSample.eventTime}
                       .count()
            << "ms to "
            << std::chrono::duration<double, std::milli>{farthestPrediction -
                                                         presentSample.eventTime}
                       .count()
            << "ms";
    const float alpha =
            std::chrono::duration<float, std::nano>(newResampleTime - pastSample.eventTime) / delta;

    PointerMap resampledPointerMap;
    for (const Pointer& pointer : presentSample.pointerMap) {
        if (std::optional<Pointer> pastSamplePointer =
                    pastSample.pointerMap.find(PointerMap::PointerId{pointer.properties.id});
            pastSamplePointer.has_value()) {
            const PointerCoords& resampledCoords =
                    calculateResampledCoords(pastSamplePointer->coords, pointer.coords, alpha);
            resampledPointerMap.insert(Pointer{pointer.properties, resampledCoords});
        }
    }
    return Sample{newResampleTime, resampledPointerMap};
}

inline void LegacyResampler::addSampleToMotionEvent(const Sample& sample,
                                                    MotionEvent& motionEvent) {
    motionEvent.addSample(sample.eventTime.count(), sample.asPointerCoords().data(),
                          motionEvent.getId());
}

nanoseconds LegacyResampler::getResampleLatency() const {
    return RESAMPLE_LATENCY;
}

/**
 * The resampler is unaware of ACTION_DOWN. Thus, it needs to constantly check for pointer IDs
 * occurrences. This problem could be fixed if the resampler has access to the entire stream of
 * MotionEvent actions. That way, both ACTION_DOWN and ACTION_UP will be visible; therefore,
 * facilitating pointer tracking between samples.
 */
void LegacyResampler::overwriteMotionEventSamples(MotionEvent& motionEvent) const {
    const size_t numSamples = motionEvent.getHistorySize() + 1;
    for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        overwriteStillPointers(motionEvent, sampleIndex);
        overwriteOldPointers(motionEvent, sampleIndex);
    }
}

void LegacyResampler::overwriteStillPointers(MotionEvent& motionEvent, size_t sampleIndex) const {
    if (!mLastRealSample.has_value() || !mPreviousPrediction.has_value()) {
        LOG_IF(INFO, debugResampling()) << "Still pointers not overwritten. Not enough data.";
        return;
    }
    for (size_t pointerIndex = 0; pointerIndex < motionEvent.getPointerCount(); ++pointerIndex) {
        const std::optional<Pointer> lastRealPointer = mLastRealSample->pointerMap.find(
                PointerMap::PointerId{motionEvent.getPointerId(pointerIndex)});
        const std::optional<Pointer> previousPointer = mPreviousPrediction->pointerMap.find(
                PointerMap::PointerId{motionEvent.getPointerId(pointerIndex)});
        // This could happen because resampler only receives ACTION_MOVE events.
        if (!lastRealPointer.has_value() || !previousPointer.has_value()) {
            continue;
        }
        const PointerCoords& pointerCoords =
                *(motionEvent.getHistoricalRawPointerCoords(pointerIndex, sampleIndex));
        if (equalXY(pointerCoords, lastRealPointer->coords)) {
            LOG_IF(INFO, debugResampling())
                    << "Pointer ID: " << motionEvent.getPointerId(pointerIndex)
                    << " did not move. Overwriting its coordinates from " << pointerCoords << " to "
                    << previousPointer->coords;
            setMotionEventPointerCoords(motionEvent, sampleIndex, pointerIndex,
                                        previousPointer->coords);
        }
    }
}

void LegacyResampler::overwriteOldPointers(MotionEvent& motionEvent, size_t sampleIndex) const {
    if (!mPreviousPrediction.has_value()) {
        LOG_IF(INFO, debugResampling()) << "Old sample not overwritten. Not enough data.";
        return;
    }
    if (nanoseconds{motionEvent.getHistoricalEventTime(sampleIndex)} <
        mPreviousPrediction->eventTime) {
        LOG_IF(INFO, debugResampling())
                << "Motion event sample older than predicted sample. Overwriting event time from "
                << std::setprecision(3)
                << std::chrono::duration<double,
                                         std::milli>{nanoseconds{motionEvent.getHistoricalEventTime(
                                                             sampleIndex)}}
                           .count()
                << "ms to "
                << std::chrono::duration<double, std::milli>{mPreviousPrediction->eventTime}.count()
                << "ms";
        for (size_t pointerIndex = 0; pointerIndex < motionEvent.getPointerCount();
             ++pointerIndex) {
            const std::optional<Pointer> previousPointer = mPreviousPrediction->pointerMap.find(
                    PointerMap::PointerId{motionEvent.getPointerId(pointerIndex)});
            // This could happen because resampler only receives ACTION_MOVE events.
            if (!previousPointer.has_value()) {
                continue;
            }
            setMotionEventPointerCoords(motionEvent, sampleIndex, pointerIndex,
                                        previousPointer->coords);
        }
    }
}

void LegacyResampler::resampleMotionEvent(nanoseconds frameTime, MotionEvent& motionEvent,
                                          const InputMessage* futureSample) {
    const nanoseconds resampleTime = frameTime - RESAMPLE_LATENCY;

    if (resampleTime.count() == motionEvent.getEventTime()) {
        LOG_IF(INFO, debugResampling()) << "Not resampled. Resample time equals motion event time.";
        return;
    }

    updateLatestSamples(motionEvent);

    const std::optional<Sample> sample = (futureSample != nullptr)
            ? (attemptInterpolation(resampleTime, *futureSample))
            : (attemptExtrapolation(resampleTime));
    if (sample.has_value()) {
        addSampleToMotionEvent(*sample, motionEvent);
        if (mPreviousPrediction.has_value()) {
            overwriteMotionEventSamples(motionEvent);
        }
        // mPreviousPrediction is only updated whenever extrapolation occurs because extrapolation
        // is about predicting upcoming scenarios.
        if (futureSample == nullptr) {
            mPreviousPrediction = sample;
        }
    }
    LOG_IF(FATAL, mLatestSamples.empty()) << "mLatestSamples must contain at least one sample.";
    mLastRealSample = *(mLatestSamples.end() - 1);
}

// --- FilteredLegacyResampler ---

FilteredLegacyResampler::FilteredLegacyResampler(float minCutoffFreq, float beta)
      : mResampler{}, mMinCutoffFreq{minCutoffFreq}, mBeta{beta} {}

void FilteredLegacyResampler::resampleMotionEvent(std::chrono::nanoseconds requestedFrameTime,
                                                  MotionEvent& motionEvent,
                                                  const InputMessage* futureSample) {
    mResampler.resampleMotionEvent(requestedFrameTime, motionEvent, futureSample);
    const size_t numSamples = motionEvent.getHistorySize() + 1;
    for (size_t sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex) {
        for (size_t pointerIndex = 0; pointerIndex < motionEvent.getPointerCount();
             ++pointerIndex) {
            const int32_t pointerId = motionEvent.getPointerProperties(pointerIndex)->id;
            const nanoseconds eventTime =
                    nanoseconds{motionEvent.getHistoricalEventTime(sampleIndex)};
            // Refer to the static function `setMotionEventPointerCoords` for a justification of
            // casting away const.
            PointerCoords& pointerCoords = const_cast<PointerCoords&>(
                    *(motionEvent.getHistoricalRawPointerCoords(pointerIndex, sampleIndex)));
            const auto& [iter, _] = mFilteredPointers.try_emplace(pointerId, mMinCutoffFreq, mBeta);
            iter->second.filter(eventTime, pointerCoords);
        }
    }
}

std::chrono::nanoseconds FilteredLegacyResampler::getResampleLatency() const {
    return mResampler.getResampleLatency();
}

} // namespace android
