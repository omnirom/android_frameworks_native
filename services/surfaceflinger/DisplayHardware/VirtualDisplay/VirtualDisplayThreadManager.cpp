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

// #define LOG_NDEBUG 0
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "VirtualDisplayThread.h"

#include "VirtualDisplayThreadManager.h"

namespace android {

VirtualDisplayThreadManager& VirtualDisplayThreadManager::getInstance() {
    [[clang::no_destroy]] static VirtualDisplayThreadManager instance;
    return instance;
}

VirtualDisplayThread::Client VirtualDisplayThreadManager::getOrCreateThread(uid_t uid) {
    std::scoped_lock _l(mMutex);

    auto it = mThreadsByUid.find(uid);
    if (it != mThreadsByUid.end()) {
        it->second.refCount++;
        return VirtualDisplayThread::Client(uid, it->second.thread);
    }

    ALOGI("Creating new VirtualDisplayThread for UID: %d", uid);
    std::shared_ptr<VirtualDisplayThread> newThread = VirtualDisplayThread::create();
    mThreadsByUid.emplace(uid, ThreadContext{newThread, 1});
    return VirtualDisplayThread::Client(uid, newThread);
}

void VirtualDisplayThreadManager::releaseThread(uid_t uid) {
    std::scoped_lock _l(mMutex);

    auto it = mThreadsByUid.find(uid);
    if (it == mThreadsByUid.end()) {
        ALOGE("Attempting to release a non-existent thread for UID: %d", uid);
        return;
    }

    it->second.refCount--;
    if (it->second.refCount == 0) {
        ALOGI("Destroying VirtualDisplayThread for UID: %d as it has no more references.", uid);
        it->second.thread->destroy();
        mThreadsByUid.erase(it);
    }
}

} // namespace android
