/*
 * Copyright (C) 2012 The Android Open Source Project
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

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include <inttypes.h>

#include <android-base/stringprintf.h>
#include <android/log.h>

#include <ui/FrameStats.h>

#include "FrameTracker.h"

namespace android {

FrameTracker::FrameTracker() : mOffset(0), mNumFences(0), mDisplayPeriod(0) {}

void FrameTracker::setDesiredPresentTime(nsecs_t presentTime) {
    Mutex::Autolock lock(mMutex);
    mFrameRecords[mOffset].desiredPresentTime = presentTime;
}

void FrameTracker::setFrameReadyTime(nsecs_t readyTime) {
    Mutex::Autolock lock(mMutex);
    mFrameRecords[mOffset].frameReadyTime = readyTime;
}

void FrameTracker::setFrameReadyFence(
        std::shared_ptr<FenceTime>&& readyFence) {
    Mutex::Autolock lock(mMutex);
    mFrameRecords[mOffset].frameReadyFence = std::move(readyFence);
    mNumFences++;
}

void FrameTracker::setActualPresentTime(nsecs_t presentTime) {
    Mutex::Autolock lock(mMutex);
    mFrameRecords[mOffset].actualPresentTime = presentTime;
}

void FrameTracker::setActualPresentFence(const std::shared_ptr<FenceTime>& readyFence) {
    Mutex::Autolock lock(mMutex);
    mFrameRecords[mOffset].actualPresentFence = readyFence;
    mNumFences++;
}

void FrameTracker::setDisplayRefreshPeriod(nsecs_t displayPeriod) {
    Mutex::Autolock lock(mMutex);
    mDisplayPeriod = displayPeriod;
}

void FrameTracker::advanceFrame() {
    Mutex::Autolock lock(mMutex);

    // Advance to the next frame.
    mOffset = (mOffset+1) % NUM_FRAME_RECORDS;
    mFrameRecords[mOffset].desiredPresentTime = INT64_MAX;
    mFrameRecords[mOffset].frameReadyTime = INT64_MAX;
    mFrameRecords[mOffset].actualPresentTime = INT64_MAX;

    if (mFrameRecords[mOffset].frameReadyFence != nullptr) {
        // We're clobbering an unsignaled fence, so we need to decrement the
        // fence count.
        mFrameRecords[mOffset].frameReadyFence = nullptr;
        mNumFences--;
    }

    if (mFrameRecords[mOffset].actualPresentFence != nullptr) {
        // We're clobbering an unsignaled fence, so we need to decrement the
        // fence count.
        mFrameRecords[mOffset].actualPresentFence = nullptr;
        mNumFences--;
    }
}

void FrameTracker::clearStats() {
    Mutex::Autolock lock(mMutex);
    for (size_t i = 0; i < NUM_FRAME_RECORDS; i++) {
        mFrameRecords[i].desiredPresentTime = 0;
        mFrameRecords[i].frameReadyTime = 0;
        mFrameRecords[i].actualPresentTime = 0;
        mFrameRecords[i].frameReadyFence.reset();
        mFrameRecords[i].actualPresentFence.reset();
    }
    mNumFences = 0;
    mFrameRecords[mOffset].desiredPresentTime = INT64_MAX;
    mFrameRecords[mOffset].frameReadyTime = INT64_MAX;
    mFrameRecords[mOffset].actualPresentTime = INT64_MAX;
}

void FrameTracker::getStats(FrameStats* outStats) const {
    Mutex::Autolock lock(mMutex);
    processFencesLocked();

    outStats->refreshPeriodNano = mDisplayPeriod;

    const size_t offset = mOffset;
    for (size_t i = 1; i < NUM_FRAME_RECORDS; i++) {
        const size_t index = (offset + i) % NUM_FRAME_RECORDS;

        // Skip frame records with no data (if buffer not yet full).
        if (mFrameRecords[index].desiredPresentTime == 0) {
            continue;
        }

        nsecs_t desiredPresentTimeNano = mFrameRecords[index].desiredPresentTime;
        outStats->desiredPresentTimesNano.push_back(desiredPresentTimeNano);

        nsecs_t actualPresentTimeNano = mFrameRecords[index].actualPresentTime;
        outStats->actualPresentTimesNano.push_back(actualPresentTimeNano);

        nsecs_t frameReadyTimeNano = mFrameRecords[index].frameReadyTime;
        outStats->frameReadyTimesNano.push_back(frameReadyTimeNano);
    }
}

void FrameTracker::processFencesLocked() const {
    FrameRecord* records = const_cast<FrameRecord*>(mFrameRecords);
    int& numFences = const_cast<int&>(mNumFences);

    for (int i = 1; i < NUM_FRAME_RECORDS && numFences > 0; i++) {
        size_t idx = (mOffset + NUM_FRAME_RECORDS - i) % NUM_FRAME_RECORDS;

        const std::shared_ptr<FenceTime>& rfence = records[idx].frameReadyFence;
        if (rfence != nullptr) {
            records[idx].frameReadyTime = rfence->getSignalTime();
            if (records[idx].frameReadyTime < INT64_MAX) {
                records[idx].frameReadyFence = nullptr;
                numFences--;
            }
        }

        const std::shared_ptr<FenceTime>& pfence =
                records[idx].actualPresentFence;
        if (pfence != nullptr) {
            records[idx].actualPresentTime = pfence->getSignalTime();
            if (records[idx].actualPresentTime < INT64_MAX) {
                records[idx].actualPresentFence = nullptr;
                numFences--;
            }
        }
    }
}

bool FrameTracker::isFrameValidLocked(size_t idx) const {
    return mFrameRecords[idx].actualPresentTime > 0 &&
            mFrameRecords[idx].actualPresentTime < INT64_MAX;
}

void FrameTracker::dumpStats(std::string& result) const {
    Mutex::Autolock lock(mMutex);
    processFencesLocked();

    const size_t o = mOffset;
    for (size_t i = 1; i < NUM_FRAME_RECORDS; i++) {
        const size_t index = (o+i) % NUM_FRAME_RECORDS;
        base::StringAppendF(&result, "%" PRId64 "\t%" PRId64 "\t%" PRId64 "\n",
                            mFrameRecords[index].desiredPresentTime,
                            mFrameRecords[index].actualPresentTime,
                            mFrameRecords[index].frameReadyTime);
    }
    result.append("\n");
}

} // namespace android

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"
