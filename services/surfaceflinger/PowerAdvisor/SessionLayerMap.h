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

#include <log/log.h>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace android::adpf {

class SessionLayerMap {
public:
    // Inform the SessionLayerMap about dead sessions
    void notifySessionsDied(std::vector<int32_t>& sessionIds);
    // Inform the SessionLayerMap about dead layers
    void notifyLayersDied(std::vector<int32_t>& layers);
    // Associate a session with a specific set of layer ids
    bool bindSessionIDToLayers(int sessionId, const std::vector<int32_t>& layerIds);
    // Get the set of sessions that are mapped to a specific layer id
    void getAssociatedSessions(int32_t layerId, std::vector<int32_t>& sessionIdsOut);
    // Get the set of layers that are currently being tracked
    void getCurrentlyRelevantLayers(std::unordered_set<int32_t>& currentlyRelevantLayers);

private:
    struct MappedType {
        MappedType(int32_t id, std::unordered_map<int32_t, MappedType>& otherList)
              : mId(id), mOtherList(otherList) {};
        MappedType() = delete;
        ~MappedType() { swapLinks({}); }

        // Replace the set of associated IDs for this mapped type with a different set of IDs,
        // updating only associations which have changed between the two sets
        void swapLinks(std::set<int32_t>&& incoming) {
            auto&& oldIter = mLinks.begin();
            auto&& newIter = incoming.begin();

            // Dump all outdated values and insert new ones
            while (oldIter != mLinks.end() || newIter != incoming.end()) {
                // If there is a value in the new set but not the old set
                // We should have already ensured what we're linking to exists
                if (oldIter == mLinks.end() || (newIter != incoming.end() && *newIter < *oldIter)) {
                    addRemoteAssociation(*newIter);
                    ++newIter;
                    continue;
                }

                // If there is a value in the old set but not the new set
                if (newIter == incoming.end() || (oldIter != mLinks.end() && *oldIter < *newIter)) {
                    dropRemoteAssociation(*oldIter);
                    ++oldIter;
                    continue;
                }

                // If they're the same, skip
                if (*oldIter == *newIter) {
                    ++oldIter;
                    ++newIter;
                    continue;
                }
            }

            mLinks.swap(incoming);
        }

        void addRemoteAssociation(int32_t other) {
            auto&& iter = mOtherList.find(other);
            if (iter != mOtherList.end()) {
                iter->second.mLinks.insert(mId);
            } else {
                ALOGE("Existing entry in SessionLayerMap, link failed");
            }
        }

        void dropRemoteAssociation(int32_t other) {
            auto&& iter = mOtherList.find(other);
            if (iter != mOtherList.end()) {
                iter->second.mLinks.erase(mId);
                if (iter->second.mLinks.empty()) {
                    // This only erases them from the map, not from general tracking
                    mOtherList.erase(iter);
                }
            } else {
                ALOGE("Missing entry in SessionLayerMap, unlinking failed");
            }
        }

        int32_t mId;
        std::set<int> mLinks;
        std::unordered_map<int32_t, MappedType>& mOtherList;
    };

    std::unordered_map<int32_t, MappedType> mSessions;
    std::unordered_map<int32_t, MappedType> mLayers;
};

} // namespace android::adpf
