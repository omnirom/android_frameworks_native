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

#include <aidl/android/adpf/BnSessionManager.h>
#include <sys/types.h>

#include <utils/Thread.h>
#include "Common.h"
#include "SessionLayerMap.h"

#include <string>

namespace android {

class Layer;

namespace adpf {
namespace impl {

class PowerAdvisor;

}

// Talks to HMS to manage sessions for PowerHAL
class SessionManager : public BnSessionManager {
public:
    SessionManager(uid_t uid);

    // ISessionManager binder methods
    ndk::ScopedAStatus trackedSessionsDied(const std::vector<int32_t>& in_sessionId) override;
    ndk::ScopedAStatus associateSessionToLayers(
            int32_t sessionId, int32_t ownerUid,
            const std::vector<::ndk::SpAIBinder>& layers) override;

    // Update the lifecycles of any tracked sessions or layers. This is intended to accepts the
    // "destroyedHandles" object from updateLayerSnapshots in SF, and should reflect that type
    void updateTrackingState(const std::vector<std::pair<uint32_t, std::string>>& handles);

private:
    // Session metadata tracked by the mTrackedSessionData map
    struct SessionData {
        int32_t sessionId;
        int uid;
    };

    // Layer metadata tracked by the mTrackedSessionData map
    struct LayerData {
        int32_t layerId;
    };

    // Checks if the layer is currently associated with a specific session in the SessionLayerMap
    // This helps us know which layers might be included in an update for the HAL
    bool isLayerRelevant(int32_t layerId);

    // The UID of whoever created our ISessionManager connection
    // FIXME: This is set but is not used anywhere.
    [[maybe_unused]] const uid_t mUid;

    // State owned by the main thread

    // Set of layers that are currently being tracked in the SessionLayerMap. This is used to
    // filter out which layers we actually care about during the latching process
    std::unordered_set<int32_t> mCurrentlyRelevantLayers;

    // Tracks active associations between sessions and layers. Items in this map can be thought of
    // as "active" connections, and any session or layer not in this map will not receive updates or
    // be collected in SurfaceFlinger
    SessionLayerMap mMap;

    // The list of currently-living layers which have ever been tracked, this is used to persist any
    // data we want to track across potential mapping disconnects, and to determine when to send
    // death updates
    std::unordered_map<int32_t, LayerData> mTrackedLayerData;

    // The list of currently-living sessions which have ever been tracked, this is used to persist
    // any data we want to track across mapping disconnects
    std::unordered_map<int32_t, SessionData> mTrackedSessionData;

    // State owned by mSessionManagerMutex

    std::mutex mSessionManagerMutex;

    // The list of sessions that have died since we last called updateTrackingState
    std::vector<int32_t> mDeadSessions GUARDED_BY(mSessionManagerMutex);
};

} // namespace adpf
} // namespace android
