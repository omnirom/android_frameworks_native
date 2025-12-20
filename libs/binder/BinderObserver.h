/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <mutex>

#include "BinderObserverConfig.h"
#include "BinderStatsPusher.h"
#include "BinderStatsSpscQueue.h"
#include "BinderStatsUtils.h"

namespace android {

/**
 * Collects and manages binder transaction statistics from IPC threads.
 *
 * Gathers BinderCallData from BinderStatsSpscQueue instances associated
 * with each IPCThreadState. It periodically flushes these queues, aggregates
 * data using BinderStatsCollector, and sends statistics via BinderStatsPusher.
 *
 * How it is works:
 * - An instance is typically owned by ProcessState.
 * - IPCThreadState instances register themselves with the BinderObserver.
 * - IPCThreadState reports binder calls using onBeginTransaction/onEndTransaction
 * - this pushes BinderCallData to the stats queue of the thread via addStatMaybeFlush.
 * - flushStats (or flushIfRequired) periodically collects data from all
 *   registered queues and passes it to BinderStatsPusher.
 *
 * Threading Requirements:
 * - onBeginTransaction, onEndTransaction, registerThread, deregisterThread: thread-safe
 * - addStatMaybeFlush: thread-safe
 */
class BinderObserver {
public:
    // Initial data for tracking a binder call
    struct CallInfo {
        String16 interfaceDescriptor;
        uint32_t code;
        uid_t callingUid;
        BinderObserverConfig::TrackingInfo trackingInfo;
        int64_t startTimeNanos;
    };

    void deregisterThread(std::shared_ptr<BinderStatsSpscQueue>& queue);
    CallInfo onBeginTransaction(BBinder* binder, uint32_t code, uid_t callingUid);
    void onEndTransaction(std::shared_ptr<BinderStatsSpscQueue>& queue, const CallInfo& callInfo);

private:
    // Add stats to the local queue, flush if queue is full.
    void addStatMaybeFlush(const std::shared_ptr<BinderStatsSpscQueue>& queue,
                           const BinderCallData& stat);
    void flushStats(int64_t nowMillis);
    bool isFlushRequired(int64_t nowMillis);

    std::unique_ptr<BinderObserverConfig> mConfig = BinderObserverConfig::createConfig();
    // Time since last flush time. Used to trigger a flush if more than kSendTimeoutSec
    // has elapsed since last flush.
    std::atomic<int64_t> mLastFlushTimeSec;
    // BinderStatsCollector stores shared ptrs to queues in IPCThreadState and pulls data from them.
    BinderStatsCollector mBinderStatsCollector;
    // BinderStatsPusher aggregates Stats and pushes them to data store.
    BinderStatsPusher mPusher;
    // Lock used to ensure that only one thread is running a flush and push at a time.
    std::mutex mFlushLock;
};
} // namespace android
