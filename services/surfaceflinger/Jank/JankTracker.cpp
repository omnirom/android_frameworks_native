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

#include "JankTracker.h"

#include <android/gui/IJankListener.h>
#include "BackgroundExecutor.h"

namespace android {

namespace {

constexpr size_t kJankDataBatchSize = 50;

} // anonymous namespace

std::atomic<size_t> JankTracker::sListenerCount(0);
std::atomic<bool> JankTracker::sCollectAllJankDataForTesting(false);

JankTracker::~JankTracker() {}

void JankTracker::addJankListener(int32_t layerId, sp<IBinder> listener) {
    // Increment right away, so that if an onJankData call comes in before the background thread has
    // added this listener, it will not drop the data.
    sListenerCount++;

    BackgroundExecutor::getLowPriorityInstance().sendCallbacks(
            {[layerId, listener = std::move(listener)]() {
                JankTracker& tracker = getInstance();
                const std::lock_guard<std::mutex> _l(tracker.mLock);
                tracker.addJankListenerLocked(layerId, listener);
            }});
}

void JankTracker::flushJankData(int32_t layerId) {
    BackgroundExecutor::getLowPriorityInstance().sendCallbacks(
            {[layerId]() { getInstance().doFlushJankData(layerId); }});
}

void JankTracker::removeJankListener(int32_t layerId, sp<IBinder> listener, int64_t afterVsync) {
    BackgroundExecutor::getLowPriorityInstance().sendCallbacks(
            {[layerId, listener = std::move(listener), afterVsync]() {
                JankTracker& tracker = getInstance();
                const std::lock_guard<std::mutex> _l(tracker.mLock);
                tracker.markJankListenerForRemovalLocked(layerId, listener, afterVsync);
            }});
}

void JankTracker::onJankData(int32_t layerId, gui::JankData data) {
    if (sListenerCount == 0) {
        return;
    }

    BackgroundExecutor::getLowPriorityInstance().sendCallbacks(
            {[layerId, data = std::move(data)]() {
                JankTracker& tracker = getInstance();

                tracker.mLock.lock();
                bool hasListeners = tracker.mJankListeners.count(layerId) > 0;
                tracker.mLock.unlock();

                if (!hasListeners && !sCollectAllJankDataForTesting) {
                    return;
                }

                tracker.mJankDataLock.lock();
                tracker.mJankData.emplace(layerId, data);
                size_t count = tracker.mJankData.count(layerId);
                tracker.mJankDataLock.unlock();

                if (count >= kJankDataBatchSize && !sCollectAllJankDataForTesting) {
                    tracker.doFlushJankData(layerId);
                }
            }});
}

void JankTracker::addJankListenerLocked(int32_t layerId, sp<IBinder> listener) {
    for (auto it = mJankListeners.find(layerId); it != mJankListeners.end(); it++) {
        if (it->second.mListener == listener) {
            // Undo the duplicate increment in addJankListener.
            sListenerCount--;
            return;
        }
    }

    mJankListeners.emplace(layerId, std::move(listener));
}

void JankTracker::doFlushJankData(int32_t layerId) {
    std::vector<gui::JankData> jankData;
    int64_t maxVsync = transferAvailableJankData(layerId, jankData);

    std::vector<sp<IBinder>> toSend;

    mLock.lock();
    for (auto it = mJankListeners.find(layerId); it != mJankListeners.end();) {
        if (!jankData.empty()) {
            toSend.emplace_back(it->second.mListener);
        }

        int64_t removeAfter = it->second.mRemoveAfter;
        if (removeAfter != -1 && removeAfter <= maxVsync) {
            it = mJankListeners.erase(it);
            sListenerCount--;
        } else {
            it++;
        }
    }
    mLock.unlock();

    for (const auto& listener : toSend) {
        binder::Status status = interface_cast<gui::IJankListener>(listener)->onJankData(jankData);
        if (status.exceptionCode() == binder::Status::EX_NULL_POINTER) {
            // Remove any listeners, where the App side has gone away, without
            // deregistering.
            dropJankListener(layerId, listener);
        }
    }
}

void JankTracker::markJankListenerForRemovalLocked(int32_t layerId, sp<IBinder> listener,
                                                   int64_t afterVysnc) {
    for (auto it = mJankListeners.find(layerId); it != mJankListeners.end(); it++) {
        if (it->second.mListener == listener) {
            it->second.mRemoveAfter = std::max(static_cast<int64_t>(0), afterVysnc);
            return;
        }
    }
}

int64_t JankTracker::transferAvailableJankData(int32_t layerId,
                                               std::vector<gui::JankData>& outJankData) {
    const std::lock_guard<std::mutex> _l(mJankDataLock);
    int64_t maxVsync = 0;
    auto range = mJankData.equal_range(layerId);
    for (auto it = range.first; it != range.second;) {
        maxVsync = std::max(it->second.frameVsyncId, maxVsync);
        outJankData.emplace_back(std::move(it->second));
        it = mJankData.erase(it);
    }
    return maxVsync;
}

void JankTracker::dropJankListener(int32_t layerId, sp<IBinder> listener) {
    const std::lock_guard<std::mutex> _l(mLock);
    for (auto it = mJankListeners.find(layerId); it != mJankListeners.end(); it++) {
        if (it->second.mListener == listener) {
            mJankListeners.erase(it);
            sListenerCount--;
            return;
        }
    }
}

void JankTracker::clearAndStartCollectingAllJankDataForTesting() {
    BackgroundExecutor::getLowPriorityInstance().flushQueue();

    // Clear all past tracked jank data.
    JankTracker& tracker = getInstance();
    const std::lock_guard<std::mutex> _l(tracker.mJankDataLock);
    tracker.mJankData.clear();

    // Pretend there's at least one listener.
    sListenerCount++;
    sCollectAllJankDataForTesting = true;
}

std::vector<gui::JankData> JankTracker::getCollectedJankDataForTesting(int32_t layerId) {
    JankTracker& tracker = getInstance();
    const std::lock_guard<std::mutex> _l(tracker.mJankDataLock);

    auto range = tracker.mJankData.equal_range(layerId);
    std::vector<gui::JankData> result;
    std::transform(range.first, range.second, std::back_inserter(result),
                   [](std::pair<int32_t, gui::JankData> layerIdToJankData) {
                       return layerIdToJankData.second;
                   });

    return result;
}

void JankTracker::clearAndStopCollectingAllJankDataForTesting() {
    // Undo startCollectingAllJankDataForTesting.
    sListenerCount--;
    sCollectAllJankDataForTesting = false;

    // Clear all tracked jank data.
    JankTracker& tracker = getInstance();
    const std::lock_guard<std::mutex> _l(tracker.mJankDataLock);
    tracker.mJankData.clear();
}

} // namespace android
