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

#pragma once

#include <android-base/thread_annotations.h>
#include <utils/Timers.h>

#include "InputEventTimeline.h"

namespace android::inputdispatcher {

static constexpr size_t NUM_BINS = 20;
static constexpr size_t NUM_INPUT_EVENT_TYPES = 6;

enum class LatencyStageIndex : size_t {
    EVENT_TO_READ = 0,
    READ_TO_DELIVER = 1,
    DELIVER_TO_CONSUME = 2,
    CONSUME_TO_FINISH = 3,
    CONSUME_TO_GPU_COMPLETE = 4,
    GPU_COMPLETE_TO_PRESENT = 5,
    END_TO_END = 6,
    SIZE = 7, // must be last
};

// Let's create a full timeline here:
// eventTime
// readTime
// <---- after this point, the data becomes per-connection
// deliveryTime // time at which the event was sent to the receiver
// consumeTime  // time at which the receiver read the event
// finishTime   // time at which the dispatcher reads the response from the receiver that the event
// was processed
// GraphicsTimeline::GPU_COMPLETED_TIME
// GraphicsTimeline::PRESENT_TIME

/**
 * Keep histograms with latencies of the provided events
 */
class LatencyAggregatorWithHistograms final : public InputEventTimelineProcessor {
public:
    /**
     * Record a complete event timeline
     */
    void processTimeline(const InputEventTimeline& timeline) override;

    void pushLatencyStatistics() override;

    std::string dump(const char* prefix) const override;

private:
    // ---------- Slow event handling ----------
    void processSlowEvent(const InputEventTimeline& timeline);
    nsecs_t mLastSlowEventTime = 0;
    // How many slow events have been skipped due to rate limiting
    size_t mNumSkippedSlowEvents = 0;
    // How many events have been received since the last time we reported a slow event
    size_t mNumEventsSinceLastSlowEventReport = 0;

    // ---------- Statistics handling ----------
    /**
     * Data structure to gather time samples into NUM_BINS buckets
     */
    class Histogram {
    public:
        Histogram(const std::array<int, NUM_BINS - 1>& binSizes) : mBinSizes(binSizes) {
            mBinCounts.fill(0);
        }

        // Increments binCounts of the appropriate bin when adding a new sample
        void addSample(int64_t sample) {
            size_t binIndex = getSampleBinIndex(sample);
            mBinCounts[binIndex]++;
        }

        const std::array<int32_t, NUM_BINS>& getBinCounts() const { return mBinCounts; }

    private:
        // reference to an array that represents the range of values each bin holds.
        // in bin i+1 live samples such that *mBinSizes[i] <= sample < *mBinSizes[i+1]
        const std::array<int, NUM_BINS - 1>& mBinSizes;
        std::array<int32_t, NUM_BINS>
                mBinCounts; // the number of samples that currently live in each bin

        size_t getSampleBinIndex(int64_t sample) {
            auto it = std::upper_bound(mBinSizes.begin(), mBinSizes.end(), sample);
            return std::distance(mBinSizes.begin(), it);
        }
    };

    void processStatistics(const InputEventTimeline& timeline);

    // Identifier for the an input event. If two input events have the same identifiers we
    // want to use the same histograms to count the latency samples
    using InputEventLatencyIdentifier =
            std::tuple<uint16_t /*vendorId*/, uint16_t /*productId*/,
                       const std::set<InputDeviceUsageSource> /*sources*/,
                       InputEventActionType /*inputEventActionType*/>;

    // Maps an input event identifier to an array of 7 histograms, one for each latency
    // stage. It is cleared after an atom push
    std::map<InputEventLatencyIdentifier, std::array<Histogram, 7>> mHistograms;

    void addSampleToHistogram(const InputEventLatencyIdentifier& identifier,
                              LatencyStageIndex latencyStageIndex, nsecs_t time);

    // Stores all possible arrays of bin sizes. The order in the vector does not matter, as long
    // as binSizesMappings points to the right index
    static constexpr std::array<std::array<int, NUM_BINS - 1>, 6> allBinSizes = {
            {{10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100},
             {1, 2, 3, 4, 5, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32},
             {15, 30, 45, 60, 75, 90, 105, 120, 135, 150, 165, 180, 195, 210, 225, 240, 255, 270,
              285},
             {40, 80, 120, 160, 200, 240, 280, 320, 360, 400, 440, 480, 520, 560, 600, 640, 680,
              720, 760},
             {20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300, 320, 340, 360,
              380},
             {200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500, 1600,
              1700, 1800, 1900, 2000}}};

    // Stores indexes in allBinSizes to use with each {LatencyStage, InputEventType} pair.
    // Bin sizes for a certain latencyStage and inputEventType are at:
    // *(allBinSizes[binSizesMappings[latencyStageIndex][inputEventTypeIndex]])
    // inputEventTypeIndex is the int value of InputEventActionType enum decreased by 1 since we
    // don't want to record latencies for unknown events.
    // e.g. MOTION_ACTION_DOWN is 0, MOTION_ACTION_MOVE is 1...
    static constexpr std::array<std::array<int8_t, NUM_INPUT_EVENT_TYPES>,
                                static_cast<size_t>(LatencyStageIndex::SIZE)>
            binSizesMappings = {{{0, 0, 0, 0, 0, 0},
                                 {1, 1, 1, 1, 1, 1},
                                 {1, 1, 1, 1, 1, 1},
                                 {2, 2, 2, 2, 2, 2},
                                 {3, 3, 3, 3, 3, 3},
                                 {4, 4, 4, 4, 4, 4},
                                 {5, 5, 5, 5, 5, 5}}};

    // Similar to binSizesMappings, but holds the index of the array of bin ranges to use on the
    // server. The index gets pushed with the atom within the histogram_version field.
    static constexpr std::array<std::array<int8_t, NUM_INPUT_EVENT_TYPES>,
                                static_cast<size_t>(LatencyStageIndex::SIZE)>
            histogramVersions = {{{0, 0, 0, 0, 0, 0},
                                  {1, 1, 1, 1, 1, 1},
                                  {1, 1, 1, 1, 1, 1},
                                  {2, 2, 2, 2, 2, 2},
                                  {3, 3, 3, 3, 3, 3},
                                  {4, 4, 4, 4, 4, 4},
                                  {5, 5, 5, 5, 5, 5}}};
};

} // namespace android::inputdispatcher
