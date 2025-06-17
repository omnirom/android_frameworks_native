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

#include "SessionLayerMap.h"
#include <android/binder_libbinder.h>

namespace android::adpf {

void SessionLayerMap::notifySessionsDied(std::vector<int32_t>& sessionIds) {
    for (int id : sessionIds) {
        auto&& iter = mSessions.find(id);
        if (iter != mSessions.end()) {
            mSessions.erase(iter);
        }
    }
}

void SessionLayerMap::notifyLayersDied(std::vector<int32_t>& layers) {
    for (auto&& layer : layers) {
        auto&& iter = mLayers.find(layer);
        if (iter != mLayers.end()) {
            mLayers.erase(iter);
        }
    }
}

bool SessionLayerMap::bindSessionIDToLayers(int sessionId, const std::vector<int32_t>& layerIds) {
    // If there is no association, just drop from map
    if (layerIds.empty()) {
        mSessions.erase(sessionId);
        return false;
    }

    // Ensure session exists
    if (!mSessions.contains(sessionId)) {
        mSessions.emplace(sessionId, MappedType(sessionId, mLayers));
    }

    MappedType& session = mSessions.at(sessionId);
    std::set<int32_t> newLinks;

    // For each incoming link
    for (auto&& layerId : layerIds) {
        auto&& iter = mLayers.find(layerId);

        // If it's not in the map, add it
        if (iter == mLayers.end()) {
            mLayers.emplace(layerId, MappedType(layerId, mSessions));
        }

        // Make a ref to it in the session's new association map
        newLinks.insert(layerId);
    }

    session.swapLinks(std::move(newLinks));
    return true;
}

void SessionLayerMap::getAssociatedSessions(int32_t layerId, std::vector<int32_t>& sessionIdsOut) {
    sessionIdsOut.clear();
    auto&& iter = mLayers.find(layerId);

    if (iter == mLayers.end()) {
        return;
    }

    // Dump the internal association set into this vector
    sessionIdsOut.insert(sessionIdsOut.begin(), iter->second.mLinks.begin(),
                         iter->second.mLinks.end());
}

void SessionLayerMap::getCurrentlyRelevantLayers(
        std::unordered_set<int32_t>& currentlyRelevantLayers) {
    currentlyRelevantLayers.clear();
    for (auto&& layer : mLayers) {
        currentlyRelevantLayers.insert(layer.first);
    }
}

} // namespace android::adpf