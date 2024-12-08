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

#include <cstdint>
#include <mutex>
#include <unordered_map>

#include <android/gui/JankData.h>
#include <binder/IBinder.h>
#include <utils/Mutex.h>

namespace android {
namespace frametimeline {
class FrameTimelineTest;
}

/**
 * JankTracker maintains a backlog of frame jank classification and manages and notififies any
 * registered jank data listeners.
 */
class JankTracker {
public:
    ~JankTracker();

    static void addJankListener(int32_t layerId, sp<IBinder> listener);
    static void flushJankData(int32_t layerId);
    static void removeJankListener(int32_t layerId, sp<IBinder> listener, int64_t afterVysnc);

    static void onJankData(int32_t layerId, gui::JankData data);

protected:
    // The following methods can be used to force the tracker to collect all jank data and not
    // flush it for a short time period and should *only* be used for testing. Every call to
    // clearAndStartCollectingAllJankDataForTesting needs to be followed by a call to
    // clearAndStopCollectingAllJankDataForTesting.
    static void clearAndStartCollectingAllJankDataForTesting();
    static std::vector<gui::JankData> getCollectedJankDataForTesting(int32_t layerId);
    static void clearAndStopCollectingAllJankDataForTesting();

    friend class frametimeline::FrameTimelineTest;

private:
    JankTracker() {}
    JankTracker(const JankTracker&) = delete;
    JankTracker(JankTracker&&) = delete;

    JankTracker& operator=(const JankTracker&) = delete;
    JankTracker& operator=(JankTracker&&) = delete;

    static JankTracker& getInstance() {
        static JankTracker instance;
        return instance;
    }

    void addJankListenerLocked(int32_t layerId, sp<IBinder> listener) REQUIRES(mLock);
    void doFlushJankData(int32_t layerId);
    void markJankListenerForRemovalLocked(int32_t layerId, sp<IBinder> listener, int64_t afterVysnc)
            REQUIRES(mLock);

    int64_t transferAvailableJankData(int32_t layerId, std::vector<gui::JankData>& jankData);
    void dropJankListener(int32_t layerId, sp<IBinder> listener);

    struct Listener {
        sp<IBinder> mListener;
        int64_t mRemoveAfter;

        Listener(sp<IBinder>&& listener) : mListener(listener), mRemoveAfter(-1) {}
    };

    // We keep track of the current listener count, so that the onJankData call, which is on the
    // main thread, can short-curcuit the scheduling on the background thread (which involves
    // locking) if there are no listeners registered, which is the most common case.
    static std::atomic<size_t> sListenerCount;
    static std::atomic<bool> sCollectAllJankDataForTesting;

    std::mutex mLock;
    std::unordered_multimap<int32_t, Listener> mJankListeners GUARDED_BY(mLock);
    std::mutex mJankDataLock;
    std::unordered_multimap<int32_t, gui::JankData> mJankData GUARDED_BY(mJankDataLock);

    friend class JankTrackerTest;
};

} // namespace android
