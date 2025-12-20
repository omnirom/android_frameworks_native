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

// Turns the session -> layer map into a layer -> session map
std::map<size_t, std::vector<int>> invertAssociations(std::map<size_t, std::vector<int>>& sessions);
// Ensures the map associations line up with those expected
bool validateAssociations(
        std::map<size_t, std::vector<int>>& sessions,
        SessionLayerMap& map); // Get the set of unique layer IDs in a given test session mapping
std::set<int> getUniqueLayers(std::map<size_t, std::vector<int>>& sessions);
// Get the set of unique session IDs in a given test session mapping
std::set<int> getUniqueSessions(std::map<size_t, std::vector<int>>& sessions);
// Drops a set of layer IDs from the test session mapping
void dropTestLayers(std::map<size_t, std::vector<int>>& sessions, std::vector<int>& layerIds);
// Drops a set of session IDs from the test session mapping
void dropTestSessions(std::map<size_t, std::vector<int>>& sessions, std::vector<int>& sessionIds);

}; // namespace android::adpf