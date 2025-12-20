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
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    // Get a copy of the whole set of layers that are currently being tracked
    void getCurrentlyRelevantLayers(std::unordered_set<int32_t>& currentlyRelevantLayers);
    // Find out whether a given layer should be tracked
    bool isLayerRelevant(int32_t layer);
    // Get the session associations of any layers that changed since the last time we called this
    std::map<int32_t, int32_t> getLayerMappingUpdates();
    // Gets the set of items that changed
    void getUpdatedItems(std::set<int32_t>& updatedLayers, std::set<int32_t>& updatedSessions);
    // Is there any active mapping between sessions and layers currently
    bool isIdle();

private:
    struct MappedType;
    struct EntrySet {
        std::set<int32_t> updates;
        std::unordered_map<int32_t, MappedType> entries;
    };

    EntrySet mSessions;
    EntrySet mLayers;

    struct MappedType {
        MappedType(int32_t id, EntrySet& sameSet, EntrySet& otherSet)
              : mId(id), mMyEntrySet(sameSet), mOtherEntrySet(otherSet) {};
        MappedType() = delete;
        ~MappedType() { swapLinks({}); }

        // Replace the set of associated IDs for this mapped type with a different set of IDs,
        // updating only associations which have changed between the two sets
        void swapLinks(std::set<int32_t>&& incoming) {
            if (mLinks == incoming) {
                return;
            }
            auto&& oldIter = mLinks.begin();
            auto&& newIter = incoming.begin();
            bool isChanged = false;

            // Dump all outdated values and insert new ones
            while (oldIter != mLinks.end() || newIter != incoming.end()) {
                // If there is a value in the new set but not the old set
                // We should have already ensured what we're linking to exists
                if (oldIter == mLinks.end() || (newIter != incoming.end() && *newIter < *oldIter)) {
                    addRemoteAssociation(*newIter);
                    isChanged = true;
                    ++newIter;
                    continue;
                }

                // If there is a value in the old set but not the new set
                if (newIter == incoming.end() || (oldIter != mLinks.end() && *oldIter < *newIter)) {
                    dropRemoteAssociation(*oldIter);
                    isChanged = true;
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
            if (isChanged) {
                mMyEntrySet.updates.insert(mId);
                mLinks.swap(incoming);
            }
        }

        void addRemoteAssociation(int32_t other) {
            auto&& iter = mOtherEntrySet.entries.find(other);
            if (iter != mOtherEntrySet.entries.end()) {
                iter->second.mLinks.insert(mId);
                mOtherEntrySet.updates.insert(iter->first);
            } else {
                ALOGE("Existing entry in SessionLayerMap, link failed");
            }
        }

        void dropRemoteAssociation(int32_t other) {
            auto&& iter = mOtherEntrySet.entries.find(other);
            if (iter != mOtherEntrySet.entries.end()) {
                iter->second.mLinks.erase(mId);
                mOtherEntrySet.updates.insert(iter->first);
                // If the item has no links, drop them from the map
                if (iter->second.mLinks.empty()) {
                    // This does not drop them from general tracking, nor make them "dead"
                    mOtherEntrySet.entries.erase(iter);
                }
            } else {
                ALOGE("Missing entry in SessionLayerMap, unlinking failed");
            }
        }

        int32_t mId;
        std::set<int> mLinks;
        EntrySet& mMyEntrySet;
        EntrySet& mOtherEntrySet;
    };
};

} // namespace android::adpf
