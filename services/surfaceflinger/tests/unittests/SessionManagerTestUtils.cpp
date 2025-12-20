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

#include "PowerAdvisor/SessionManager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android::adpf {

std::map<size_t, std::vector<int>> invertAssociations(std::map<size_t, std::vector<int>>& input) {
    std::map<size_t, std::vector<int>> inverted;
    for (auto&& association : input) {
        for (auto&& layerId : association.second) {
            inverted[(size_t)layerId].push_back((int)association.first);
        }
    }
    for (auto&& association : inverted) {
        std::sort(association.second.begin(), association.second.end());
    }
    return inverted;
}

std::set<int> getUniqueLayers(std::map<size_t, std::vector<int>>& sessions) {
    std::set<int> uniqueLayers;
    for (auto&& association : sessions) {
        uniqueLayers.insert(association.second.begin(), association.second.end());
    }
    return uniqueLayers;
}

std::set<int> getUniqueSessions(std::map<size_t, std::vector<int>>& sessions) {
    std::set<int> uniqueSessions;
    for (auto&& association : sessions) {
        uniqueSessions.insert(static_cast<int32_t>(association.first));
    }
    return uniqueSessions;
}

void dropTestSessions(std::map<size_t, std::vector<int>>& sessions, std::vector<int>& sessionIds) {
    for (auto&& sessionId : sessionIds) {
        sessions.erase((size_t)sessionId);
    }
}

void dropTestLayers(std::map<size_t, std::vector<int>>& sessions, std::vector<int>& layerIds) {
    for (auto&& association : sessions) {
        std::erase_if(association.second, [&](int elem) {
            return std::find(layerIds.begin(), layerIds.end(), elem) != layerIds.end();
        });
    }
}

} // namespace android::adpf