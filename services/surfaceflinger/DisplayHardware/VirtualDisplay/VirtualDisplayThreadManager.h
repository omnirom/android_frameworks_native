/*
 * Copyright 2025 The Android Open Source Project
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

#include <memory>
#include <mutex>
#include <unordered_map>

#include "VirtualDisplayThread.h"

namespace android {

/**
 * Manages the lifecycle of VirtualDisplayThread instances.
 *
 * Each UID that creates a virtual display will have its own dedicated thread for handling sink
 * surface operations. This manager ensures that only one thread exists per UID and that threads are
 * properly released when no longer needed.
 */
class VirtualDisplayThreadManager {
public:
    static VirtualDisplayThreadManager& getInstance();

    VirtualDisplayThread::Client getOrCreateThread(uid_t uid);

private:
    friend VirtualDisplayThread::Client;

    struct ThreadContext {
        std::shared_ptr<VirtualDisplayThread> thread;
        uint32_t refCount = 0;
    };

    VirtualDisplayThreadManager() = default;
    ~VirtualDisplayThreadManager() = default;

    // Disable copy and move
    VirtualDisplayThreadManager(const VirtualDisplayThreadManager&) = delete;
    VirtualDisplayThreadManager& operator=(const VirtualDisplayThreadManager&) = delete;
    VirtualDisplayThreadManager(VirtualDisplayThreadManager&&) = delete;
    VirtualDisplayThreadManager& operator=(VirtualDisplayThreadManager&&) = delete;

    void releaseThread(uid_t uid);

    std::mutex mMutex;
    std::unordered_map<uid_t, ThreadContext> mThreadsByUid GUARDED_BY(mMutex);
};

} // namespace android
