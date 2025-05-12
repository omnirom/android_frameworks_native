/*
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

#define LOG_TAG "LatencyAggregatorWithHistograms"
#include "../InputDeviceMetricsSource.h"
#include "InputDispatcher.h"

#include <inttypes.h>
#include <log/log_event_list.h>
#include <statslog.h>

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <input/Input.h>
#include <log/log.h>
#include <server_configurable_flags/get_flags.h>

using android::base::StringPrintf;
using std::chrono_literals::operator""ms;

namespace {

// Convert the provided nanoseconds into hundreds of microseconds.
// Use hundreds of microseconds (as opposed to microseconds) to preserve space.
static inline int64_t ns2hus(nsecs_t nanos) {
    return ns2us(nanos) / 100;
}

// Category (=namespace) name for the input settings that are applied at boot time
static const char* INPUT_NATIVE_BOOT = "input_native_boot";
// Feature flag name for the threshold of end-to-end touch latency that would trigger
// SlowEventReported atom to be pushed
static const char* SLOW_EVENT_MIN_REPORTING_LATENCY_MILLIS =
        "slow_event_min_reporting_latency_millis";
// Feature flag name for the minimum delay before reporting a slow event after having just reported
// a slow event. This helps limit the amount of data sent to the server
static const char* SLOW_EVENT_MIN_REPORTING_INTERVAL_MILLIS =
        "slow_event_min_reporting_interval_millis";

// If an event has end-to-end latency > 200 ms, it will get reported as a slow event.
std::chrono::milliseconds DEFAULT_SLOW_EVENT_MIN_REPORTING_LATENCY = 200ms;
// If we receive two slow events less than 1 min apart, we will only report 1 of them.
std::chrono::milliseconds DEFAULT_SLOW_EVENT_MIN_REPORTING_INTERVAL = 60000ms;

static std::chrono::milliseconds getSlowEventMinReportingLatency() {
    std::string millis = server_configurable_flags::
            GetServerConfigurableFlag(INPUT_NATIVE_BOOT, SLOW_EVENT_MIN_REPORTING_LATENCY_MILLIS,
                                      std::to_string(
                                              DEFAULT_SLOW_EVENT_MIN_REPORTING_LATENCY.count()));
    return std::chrono::milliseconds(std::stoi(millis));
}

static std::chrono::milliseconds getSlowEventMinReportingInterval() {
    std::string millis = server_configurable_flags::
            GetServerConfigurableFlag(INPUT_NATIVE_BOOT, SLOW_EVENT_MIN_REPORTING_INTERVAL_MILLIS,
                                      std::to_string(
                                              DEFAULT_SLOW_EVENT_MIN_REPORTING_INTERVAL.count()));
    return std::chrono::milliseconds(std::stoi(millis));
}

} // namespace

namespace android::inputdispatcher {

int32_t LatencyStageIndexToAtomEnum(LatencyStageIndex latencyStageIndex) {
    switch (latencyStageIndex) {
        case LatencyStageIndex::EVENT_TO_READ:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__EVENT_TO_READ;
        case LatencyStageIndex::READ_TO_DELIVER:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__READ_TO_DELIVER;
        case LatencyStageIndex::DELIVER_TO_CONSUME:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__DELIVER_TO_CONSUME;
        case LatencyStageIndex::CONSUME_TO_FINISH:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__CONSUME_TO_FINISH;
        case LatencyStageIndex::CONSUME_TO_GPU_COMPLETE:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__CONSUME_TO_GPU_COMPLETE;
        case LatencyStageIndex::GPU_COMPLETE_TO_PRESENT:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__GPU_COMPLETE_TO_PRESENT;
        case LatencyStageIndex::END_TO_END:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__END_TO_END;
        default:
            return util::INPUT_EVENT_LATENCY_REPORTED__LATENCY_STAGE__UNKNOWN_LATENCY_STAGE;
    }
}

int32_t InputEventTypeEnumToAtomEnum(InputEventActionType inputEventActionType) {
    switch (inputEventActionType) {
        case InputEventActionType::UNKNOWN_INPUT_EVENT:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__UNKNOWN_INPUT_EVENT;
        case InputEventActionType::MOTION_ACTION_DOWN:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__MOTION_ACTION_DOWN;
        case InputEventActionType::MOTION_ACTION_MOVE:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__MOTION_ACTION_MOVE;
        case InputEventActionType::MOTION_ACTION_UP:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__MOTION_ACTION_UP;
        case InputEventActionType::MOTION_ACTION_HOVER_MOVE:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__MOTION_ACTION_HOVER_MOVE;
        case InputEventActionType::MOTION_ACTION_SCROLL:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__MOTION_ACTION_SCROLL;
        case InputEventActionType::KEY:
            return util::INPUT_EVENT_LATENCY_REPORTED__INPUT_EVENT_TYPE__KEY;
    }
}

void LatencyAggregatorWithHistograms::processTimeline(const InputEventTimeline& timeline) {
    processStatistics(timeline);
    processSlowEvent(timeline);
}

void LatencyAggregatorWithHistograms::addSampleToHistogram(
        const InputEventLatencyIdentifier& identifier, LatencyStageIndex latencyStageIndex,
        nsecs_t latency) {
    // Only record positive values for the statistics
    if (latency > 0) {
        auto it = mHistograms.find(identifier);
        if (it != mHistograms.end()) {
            it->second[static_cast<size_t>(latencyStageIndex)].addSample(ns2hus(latency));
        }
    }
}

void LatencyAggregatorWithHistograms::processStatistics(const InputEventTimeline& timeline) {
    // Only gather data for Down, Move and Up motion events and Key events
    if (!(timeline.inputEventActionType == InputEventActionType::MOTION_ACTION_DOWN ||
          timeline.inputEventActionType == InputEventActionType::MOTION_ACTION_MOVE ||
          timeline.inputEventActionType == InputEventActionType::MOTION_ACTION_UP ||
          timeline.inputEventActionType == InputEventActionType::KEY))
        return;

    // Don't collect data for unidentified devices. This situation can occur for the first few input
    // events produced when an input device is first connected
    if (timeline.vendorId == 0xFFFF && timeline.productId == 0xFFFF) return;

    InputEventLatencyIdentifier identifier = {timeline.vendorId, timeline.productId,
                                              timeline.sources, timeline.inputEventActionType};
    // Check if there's a value in mHistograms map associated to identifier.
    // If not, add an array with 7 empty histograms as an entry
    if (mHistograms.count(identifier) == 0) {
        if (static_cast<int32_t>(timeline.inputEventActionType) - 1 < 0) {
            LOG(FATAL) << "Action index is smaller than 0. Action type: "
                       << ftl::enum_string(timeline.inputEventActionType);
            return;
        }
        size_t actionIndex =
                static_cast<size_t>(static_cast<int32_t>(timeline.inputEventActionType) - 1);
        if (actionIndex >= NUM_INPUT_EVENT_TYPES) {
            LOG(FATAL) << "Action index greater than the number of input event types. Action Type: "
                       << ftl::enum_string(timeline.inputEventActionType)
                       << "; Action Type Index: " << actionIndex;
            return;
        }

        std::array<Histogram, 7> histograms =
                {Histogram(allBinSizes[binSizesMappings[0][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[1][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[2][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[3][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[4][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[5][actionIndex]]),
                 Histogram(allBinSizes[binSizesMappings[6][actionIndex]])};
        mHistograms.insert({identifier, histograms});
    }

    // Process common ones first
    const nsecs_t eventToRead = timeline.readTime - timeline.eventTime;
    addSampleToHistogram(identifier, LatencyStageIndex::EVENT_TO_READ, eventToRead);

    // Now process per-connection ones
    for (const auto& [connectionToken, connectionTimeline] : timeline.connectionTimelines) {
        if (!connectionTimeline.isComplete()) {
            continue;
        }
        const nsecs_t readToDeliver = connectionTimeline.deliveryTime - timeline.readTime;
        const nsecs_t deliverToConsume =
                connectionTimeline.consumeTime - connectionTimeline.deliveryTime;
        const nsecs_t consumeToFinish =
                connectionTimeline.finishTime - connectionTimeline.consumeTime;
        const nsecs_t gpuCompletedTime =
                connectionTimeline.graphicsTimeline[GraphicsTimeline::GPU_COMPLETED_TIME];
        const nsecs_t presentTime =
                connectionTimeline.graphicsTimeline[GraphicsTimeline::PRESENT_TIME];
        const nsecs_t consumeToGpuComplete = gpuCompletedTime - connectionTimeline.consumeTime;
        const nsecs_t gpuCompleteToPresent = presentTime - gpuCompletedTime;
        const nsecs_t endToEnd = presentTime - timeline.eventTime;

        addSampleToHistogram(identifier, LatencyStageIndex::READ_TO_DELIVER, readToDeliver);
        addSampleToHistogram(identifier, LatencyStageIndex::DELIVER_TO_CONSUME, deliverToConsume);
        addSampleToHistogram(identifier, LatencyStageIndex::CONSUME_TO_FINISH, consumeToFinish);
        addSampleToHistogram(identifier, LatencyStageIndex::CONSUME_TO_GPU_COMPLETE,
                             consumeToGpuComplete);
        addSampleToHistogram(identifier, LatencyStageIndex::GPU_COMPLETE_TO_PRESENT,
                             gpuCompleteToPresent);
        addSampleToHistogram(identifier, LatencyStageIndex::END_TO_END, endToEnd);
    }
}

void LatencyAggregatorWithHistograms::pushLatencyStatistics() {
    for (auto& [id, histograms] : mHistograms) {
        auto [vendorId, productId, sources, action] = id;
        for (size_t latencyStageIndex = static_cast<size_t>(LatencyStageIndex::EVENT_TO_READ);
             latencyStageIndex < static_cast<size_t>(LatencyStageIndex::SIZE);
             ++latencyStageIndex) {
            // Convert sources set to vector for atom logging:
            std::vector<int32_t> sourcesVector = {};
            for (auto& elem : sources) {
                sourcesVector.push_back(static_cast<int32_t>(elem));
            }

            // convert histogram bin counts array to vector for atom logging:
            std::array arr = histograms[latencyStageIndex].getBinCounts();
            std::vector<int32_t> binCountsVector(arr.begin(), arr.end());

            if (static_cast<int32_t>(action) - 1 < 0) {
                ALOGW("Action index is smaller than 0. Action type: %s",
                      ftl::enum_string(action).c_str());
                continue;
            }
            size_t actionIndex = static_cast<size_t>(static_cast<int32_t>(action) - 1);
            if (actionIndex >= NUM_INPUT_EVENT_TYPES) {
                ALOGW("Action index greater than the number of input event types. Action Type: %s; "
                      "Action Type Index: %zu",
                      ftl::enum_string(action).c_str(), actionIndex);
                continue;
            }

            stats_write(android::util::INPUT_EVENT_LATENCY_REPORTED, vendorId, productId,
                        sourcesVector, InputEventTypeEnumToAtomEnum(action),
                        LatencyStageIndexToAtomEnum(
                                static_cast<LatencyStageIndex>(latencyStageIndex)),
                        histogramVersions[latencyStageIndex][actionIndex], binCountsVector);
        }
    }
    mHistograms.clear();
}

// TODO (b/270049345): For now, we just copied the code from LatencyAggregator to populate the old
// atom, but eventually we should migrate this to use the new SlowEventReported atom
void LatencyAggregatorWithHistograms::processSlowEvent(const InputEventTimeline& timeline) {
    static const std::chrono::duration sSlowEventThreshold = getSlowEventMinReportingLatency();
    static const std::chrono::duration sSlowEventReportingInterval =
            getSlowEventMinReportingInterval();
    for (const auto& [token, connectionTimeline] : timeline.connectionTimelines) {
        if (!connectionTimeline.isComplete()) {
            continue;
        }
        mNumEventsSinceLastSlowEventReport++;
        const nsecs_t presentTime =
                connectionTimeline.graphicsTimeline[GraphicsTimeline::PRESENT_TIME];
        const std::chrono::nanoseconds endToEndLatency =
                std::chrono::nanoseconds(presentTime - timeline.eventTime);
        if (endToEndLatency < sSlowEventThreshold) {
            continue;
        }
        // This is a slow event. Before we report it, check if we are reporting too often
        const std::chrono::duration elapsedSinceLastReport =
                std::chrono::nanoseconds(timeline.eventTime - mLastSlowEventTime);
        if (elapsedSinceLastReport < sSlowEventReportingInterval) {
            mNumSkippedSlowEvents++;
            continue;
        }

        const nsecs_t eventToRead = timeline.readTime - timeline.eventTime;
        const nsecs_t readToDeliver = connectionTimeline.deliveryTime - timeline.readTime;
        const nsecs_t deliverToConsume =
                connectionTimeline.consumeTime - connectionTimeline.deliveryTime;
        const nsecs_t consumeToFinish =
                connectionTimeline.finishTime - connectionTimeline.consumeTime;
        const nsecs_t gpuCompletedTime =
                connectionTimeline.graphicsTimeline[GraphicsTimeline::GPU_COMPLETED_TIME];
        const nsecs_t consumeToGpuComplete = gpuCompletedTime - connectionTimeline.consumeTime;
        const nsecs_t gpuCompleteToPresent = presentTime - gpuCompletedTime;

        android::util::stats_write(android::util::SLOW_INPUT_EVENT_REPORTED,
                                   timeline.inputEventActionType ==
                                           InputEventActionType::MOTION_ACTION_DOWN,
                                   static_cast<int32_t>(ns2us(eventToRead)),
                                   static_cast<int32_t>(ns2us(readToDeliver)),
                                   static_cast<int32_t>(ns2us(deliverToConsume)),
                                   static_cast<int32_t>(ns2us(consumeToFinish)),
                                   static_cast<int32_t>(ns2us(consumeToGpuComplete)),
                                   static_cast<int32_t>(ns2us(gpuCompleteToPresent)),
                                   static_cast<int32_t>(ns2us(endToEndLatency.count())),
                                   static_cast<int32_t>(mNumEventsSinceLastSlowEventReport),
                                   static_cast<int32_t>(mNumSkippedSlowEvents));
        mNumEventsSinceLastSlowEventReport = 0;
        mNumSkippedSlowEvents = 0;
        mLastSlowEventTime = timeline.readTime;
    }
}

std::string LatencyAggregatorWithHistograms::dump(const char* prefix) const {
    std::string statisticsStr = StringPrintf("%s Histograms:\n", prefix);
    for (const auto& [id, histograms] : mHistograms) {
        auto [vendorId, productId, sources, action] = id;

        std::string identifierStr =
                StringPrintf("%s  Identifier: vendor %d, product %d, sources: {", prefix, vendorId,
                             productId);
        bool firstSource = true;
        for (const auto& source : sources) {
            if (!firstSource) {
                identifierStr += ", ";
            }
            identifierStr += StringPrintf("%d", static_cast<int32_t>(source));
            firstSource = false;
        }
        identifierStr += StringPrintf("}, action: %d\n", static_cast<int32_t>(action));

        std::string histogramsStr;
        for (size_t stageIndex = 0; stageIndex < static_cast<size_t>(LatencyStageIndex::SIZE);
             stageIndex++) {
            const auto& histogram = histograms[stageIndex];
            const std::array<int, NUM_BINS>& binCounts = histogram.getBinCounts();

            histogramsStr += StringPrintf("%s   %zu: ", prefix, stageIndex);
            histogramsStr += StringPrintf("%d", binCounts[0]);
            for (size_t bin = 1; bin < NUM_BINS; bin++) {
                histogramsStr += StringPrintf(", %d", binCounts[bin]);
            }
            histogramsStr += StringPrintf("\n");
        }
        statisticsStr += identifierStr + histogramsStr;
    }

    return StringPrintf("%sLatencyAggregatorWithHistograms:\n", prefix) + statisticsStr +
            StringPrintf("%s  mLastSlowEventTime=%" PRId64 "\n", prefix, mLastSlowEventTime) +
            StringPrintf("%s  mNumEventsSinceLastSlowEventReport = %zu\n", prefix,
                         mNumEventsSinceLastSlowEventReport) +
            StringPrintf("%s  mNumSkippedSlowEvents = %zu\n", prefix, mNumSkippedSlowEvents);
}

} // namespace android::inputdispatcher
