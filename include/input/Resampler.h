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

#include <array>
#include <chrono>
#include <iterator>
#include <map>
#include <optional>
#include <vector>

#include <android-base/logging.h>
#include <ftl/mixins.h>
#include <input/CoordinateFilter.h>
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
     * `motionEvent` is unmodified. Furthermore, motionEvent is not resampled if resampleTime equals
     * the last sample eventTime of motionEvent.
     */
    void resampleMotionEvent(std::chrono::nanoseconds frameTime, MotionEvent& motionEvent,
                             const InputMessage* futureSample) override;

    std::chrono::nanoseconds getResampleLatency() const override;

private:
    struct Pointer {
        PointerProperties properties;
        PointerCoords coords;
    };

    /**
     * Container that stores pointers as an associative array, supporting O(1) lookup by pointer id,
     * as well as forward iteration in the order in which the pointer or pointers were inserted in
     * the container. PointerMap has a maximum capacity equal to MAX_POINTERS.
     */
    class PointerMap {
    public:
        struct PointerId : ftl::DefaultConstructible<PointerId, int32_t>,
                           ftl::Equatable<PointerId> {
            using DefaultConstructible::DefaultConstructible;
        };

        /**
         * Custom iterator to enable use of range-based for loops.
         */
        template <typename T>
        class iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            explicit iterator(pointer element) : mElement{element} {}

            friend bool operator==(const iterator& lhs, const iterator& rhs) {
                return lhs.mElement == rhs.mElement;
            }

            friend bool operator!=(const iterator& lhs, const iterator& rhs) {
                return !(lhs == rhs);
            }

            iterator operator++() {
                ++mElement;
                return *this;
            }

            reference operator*() const { return *mElement; }

        private:
            pointer mElement;
        };

        PointerMap() {
            idToIndex.fill(std::nullopt);
            for (Pointer& pointer : pointers) {
                pointer.properties.clear();
                pointer.coords.clear();
            }
        }

        /**
         * Forward iterators to traverse the pointers in `pointers`. The order of the pointers is
         * determined by the order in which they were inserted (not by id).
         */
        iterator<Pointer> begin() { return iterator<Pointer>{&pointers[0]}; }

        iterator<const Pointer> begin() const { return iterator<const Pointer>{&pointers[0]}; }

        iterator<Pointer> end() { return iterator<Pointer>{&pointers[nextPointerIndex]}; }

        iterator<const Pointer> end() const {
            return iterator<const Pointer>{&pointers[nextPointerIndex]};
        }

        /**
         * Inserts the given pointer into the PointerMap. Precondition: The current number of
         * contained pointers must be less than MAX_POINTERS when this function is called. It
         * fatally logs if the user tries to insert more than MAX_POINTERS, or if pointer id is out
         * of bounds.
         */
        void insert(const Pointer& pointer) {
            LOG_IF(FATAL, nextPointerIndex >= pointers.size())
                    << "Cannot insert more than " << MAX_POINTERS << " in PointerMap.";
            LOG_IF(FATAL, (pointer.properties.id < 0) || (pointer.properties.id > MAX_POINTER_ID))
                    << "Invalid pointer id.";
            idToIndex[pointer.properties.id] = std::optional<size_t>{nextPointerIndex};
            pointers[nextPointerIndex] = pointer;
            ++nextPointerIndex;
        }

        /**
         * Returns the pointer associated with the provided id if it exists.
         * Otherwise, std::nullopt is returned.
         */
        std::optional<Pointer> find(PointerId id) const {
            const int32_t idValue = ftl::to_underlying(id);
            LOG_IF(FATAL, (idValue < 0) || (idValue > MAX_POINTER_ID)) << "Invalid pointer id.";
            const std::optional<size_t> index = idToIndex[idValue];
            return index.has_value() ? std::optional{pointers[*index]} : std::nullopt;
        }

    private:
        /**
         * The index at which a pointer is inserted in `pointers`. Likewise, it represents the
         * number of pointers in PointerMap.
         */
        size_t nextPointerIndex{0};

        /**
         * Sequentially stores pointers. Each pointer's position is determined by the value of
         * nextPointerIndex at insertion time.
         */
        std::array<Pointer, MAX_POINTERS + 1> pointers;

        /**
         * Maps each pointer id to its associated index in pointers. If no pointer with the id
         * exists in pointers, the mapped value is std::nullopt.
         */
        std::array<std::optional<size_t>, MAX_POINTER_ID + 1> idToIndex;
    };

    struct Sample {
        std::chrono::nanoseconds eventTime;
        PointerMap pointerMap;

        std::vector<PointerCoords> asPointerCoords() const {
            std::vector<PointerCoords> pointersCoords;
            for (const Pointer& pointer : pointerMap) {
                pointersCoords.push_back(pointer.coords);
            }
            return pointersCoords;
        }
    };

    /**
     * Up to two latest samples from MotionEvent. Updated every time resampleMotionEvent is called.
     * Note: We store up to two samples in order to simplify the implementation. Although,
     * calculations are possible with only one previous sample.
     */
    RingBuffer<Sample> mLatestSamples{/*capacity=*/2};

    /**
     * Latest sample in mLatestSamples after resampling motion event.
     */
    std::optional<Sample> mLastRealSample;

    /**
     * Latest prediction. That is, the latest extrapolated sample.
     */
    std::optional<Sample> mPreviousPrediction;

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
     * Returns a sample interpolated between the latest sample of mLatestSamples and futureMessage,
     * if the conditions from canInterpolate are satisfied. Otherwise, returns nullopt.
     * mLatestSamples must have at least one sample when attemptInterpolation is called.
     */
    std::optional<Sample> attemptInterpolation(std::chrono::nanoseconds resampleTime,
                                               const InputMessage& futureMessage) const;

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

    /**
     * Iterates through motion event samples, and replaces real coordinates with resampled
     * coordinates to avoid jerkiness in certain conditions.
     */
    void overwriteMotionEventSamples(MotionEvent& motionEvent) const;

    /**
     * Overwrites with resampled data the pointer coordinates that did not move between motion event
     * samples, that is, both x and y values are identical to mLastRealSample.
     */
    void overwriteStillPointers(MotionEvent& motionEvent, size_t sampleIndex) const;

    /**
     * Overwrites the pointer coordinates of a sample with event time older than
     * that of mPreviousPrediction.
     */
    void overwriteOldPointers(MotionEvent& motionEvent, size_t sampleIndex) const;

    inline static void addSampleToMotionEvent(const Sample& sample, MotionEvent& motionEvent);
};

/**
 * Resampler that first applies the LegacyResampler resampling algorithm, then independently filters
 * the X and Y coordinates with a pair of One Euro filters.
 */
class FilteredLegacyResampler final : public Resampler {
public:
    /**
     * Creates a resampler, using the given minCutoffFreq and beta to instantiate its One Euro
     * filters.
     */
    explicit FilteredLegacyResampler(float minCutoffFreq, float beta);

    void resampleMotionEvent(std::chrono::nanoseconds requestedFrameTime, MotionEvent& motionEvent,
                             const InputMessage* futureMessage) override;

    std::chrono::nanoseconds getResampleLatency() const override;

private:
    LegacyResampler mResampler;

    /**
     * Minimum cutoff frequency of the value's low pass filter. Refer to OneEuroFilter class for a
     * more detailed explanation.
     */
    const float mMinCutoffFreq;

    /**
     * Scaling factor of the adaptive cutoff frequency criterion. Refer to OneEuroFilter class for a
     * more detailed explanation.
     */
    const float mBeta;

    /*
     * Note: an associative array with constant insertion and lookup times would be more efficient.
     * When this was implemented, there was no container with these properties.
     */
    std::map<int32_t /*pointerId*/, CoordinateFilter> mFilteredPointers;
};

} // namespace android
