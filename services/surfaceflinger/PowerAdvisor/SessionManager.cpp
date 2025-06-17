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

#include "PowerAdvisor/SessionManager.h"
#include <android/binder_libbinder.h>
#include <android/binder_status.h>
#include <binder/IPCThreadState.h>
#include "FrontEnd/LayerHandle.h"
#include "Layer.h"
#include "SurfaceFlinger.h"

namespace android::adpf {

SessionManager::SessionManager(uid_t uid) : mUid(uid) {}

ndk::ScopedAStatus SessionManager::associateSessionToLayers(
        int32_t sessionId, int32_t ownerUid, const std::vector<::ndk::SpAIBinder>& layerTokens) {
    std::scoped_lock lock{mSessionManagerMutex};

    std::vector<int32_t> layerIds;

    for (auto&& token : layerTokens) {
        auto platformToken = AIBinder_toPlatformBinder(token.get());

        // Get the layer id for it
        int32_t layerId =
                static_cast<int32_t>(surfaceflinger::LayerHandle::getLayerId(platformToken));
        auto&& iter = mTrackedLayerData.find(layerId);

        // Ensure it is being tracked
        if (iter == mTrackedLayerData.end()) {
            mTrackedLayerData.emplace(layerId, LayerData{.layerId = layerId});
        }
        layerIds.push_back(layerId);
    }

    // Register the session then track it
    if (mMap.bindSessionIDToLayers(sessionId, layerIds) &&
        !mTrackedSessionData.contains(sessionId)) {
        mTrackedSessionData.emplace(sessionId,
                                    SessionData{.sessionId = sessionId, .uid = ownerUid});
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus SessionManager::trackedSessionsDied(const std::vector<int32_t>& sessionIds) {
    std::scoped_lock lock{mSessionManagerMutex};
    for (int sessionId : sessionIds) {
        mDeadSessions.push_back(sessionId);
        mTrackedSessionData.erase(sessionId);
    }

    return ndk::ScopedAStatus::ok();
}

void SessionManager::updateTrackingState(
        const std::vector<std::pair<uint32_t, std::string>>& handles) {
    std::scoped_lock lock{mSessionManagerMutex};
    std::vector<int32_t> deadLayers;
    for (auto&& handle : handles) {
        int32_t handleId = static_cast<int32_t>(handle.first);
        auto it = mTrackedLayerData.find(handleId);
        if (it != mTrackedLayerData.end()) {
            // Track any dead layers to remove from the mapping
            mTrackedLayerData.erase(it);
            deadLayers.push_back(it->first);
        }
    }
    mMap.notifyLayersDied(deadLayers);
    mMap.notifySessionsDied(mDeadSessions);

    mDeadSessions.clear();
    mMap.getCurrentlyRelevantLayers(mCurrentlyRelevantLayers);
}

bool SessionManager::isLayerRelevant(int32_t layerId) {
    return mCurrentlyRelevantLayers.contains(layerId);
}

} // namespace android::adpf