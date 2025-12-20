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

namespace android::adpf {

void SessionLayerMap::notifySessionsDied(std::vector<int32_t>& sessions) {
    for (int session : sessions) {
        mSessions.entries.erase(session);
        // We don't create update entries for dying elements
        mSessions.updates.erase(session);
    }
}

void SessionLayerMap::notifyLayersDied(std::vector<int32_t>& layers) {
    for (auto&& layer : layers) {
        mLayers.entries.erase(layer);
        // We don't create update entries for dying elements
        mLayers.updates.erase(layer);
    }
}

bool SessionLayerMap::bindSessionIDToLayers(int sessionId, const std::vector<int32_t>& layerIds) {
    // If there is no association, just drop from map
    if (layerIds.empty()) {
        mSessions.entries.erase(sessionId);
        return false;
    }

    // Ensure session exists
    if (!mSessions.entries.contains(sessionId)) {
        mSessions.entries.emplace(sessionId, MappedType(sessionId, mSessions, mLayers));
    }

    MappedType& session = mSessions.entries.at(sessionId);
    std::set<int32_t> newLinks;

    // For each incoming link
    for (auto&& layerId : layerIds) {
        auto&& iter = mLayers.entries.find(layerId);

        // If it's not in the map, add it
        if (iter == mLayers.entries.end()) {
            mLayers.entries.emplace(layerId, MappedType(layerId, mLayers, mSessions));
        }

        // Make a ref to it in the session's new association map
        newLinks.insert(layerId);
    }

    session.swapLinks(std::move(newLinks));
    return true;
}

void SessionLayerMap::getAssociatedSessions(int32_t layerId, std::vector<int32_t>& sessionIdsOut) {
    sessionIdsOut.clear();
    auto&& iter = mLayers.entries.find(layerId);

    if (iter == mLayers.entries.end()) {
        return;
    }

    // Dump the internal association set into this vector
    sessionIdsOut.insert(sessionIdsOut.begin(), iter->second.mLinks.begin(),
                         iter->second.mLinks.end());
}

bool SessionLayerMap::isLayerRelevant(int32_t layer) {
    return mLayers.entries.contains(layer);
}

void SessionLayerMap::getCurrentlyRelevantLayers(
        std::unordered_set<int32_t>& currentlyRelevantLayers) {
    currentlyRelevantLayers.clear();
    for (auto&& layer : mLayers.entries) {
        currentlyRelevantLayers.insert(layer.first);
    }
}

void SessionLayerMap::getUpdatedItems(std::set<int32_t>& updatedLayers,
                                      std::set<int32_t>& updatedSessions) {
    updatedLayers.swap(mLayers.updates);
    updatedSessions.swap(mSessions.updates);
    mLayers.updates.clear();
    mSessions.updates.clear();
};

bool SessionLayerMap::isIdle() {
    return mLayers.entries.empty() && mLayers.updates.empty() && mSessions.entries.empty() &&
            mSessions.updates.empty();
}

} // namespace android::adpf