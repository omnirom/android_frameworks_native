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

#undef LOG_TAG
#define LOG_TAG "SessionLayerMapTest"

#include "PowerAdvisor/SessionLayerMap.h"
#include "SessionManagerTestUtils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android::adpf {

class SessionLayerMapTest : public testing::Test {
public:
    void SetUp() override;
    void checkAssociations();

protected:
    std::map<size_t, std::vector<int>> mSessionAssociations{{0, {0, 1, 2, 3}}, {1, {1, 2}},
                                                            {2, {2, 3}},       {3, {3}},
                                                            {4, {4, 5}},       {5, {4}},
                                                            {6, {5}}};
    std::unique_ptr<SessionLayerMap> mSessionLayerMap;
};

void SessionLayerMapTest::SetUp() {
    mSessionLayerMap = std::make_unique<SessionLayerMap>();
    for (auto&& association : mSessionAssociations) {
        mSessionLayerMap->bindSessionIDToLayers(static_cast<int32_t>(association.first),
                                                association.second);
    }
}

void SessionLayerMapTest::checkAssociations() {
    auto remapped = invertAssociations(mSessionAssociations);
    for (auto&& layerEntry : remapped) {
        std::vector<int32_t> out;
        mSessionLayerMap->getAssociatedSessions(static_cast<int32_t>(layerEntry.first), out);
        EXPECT_EQ(layerEntry.second, out);
    }
}

TEST_F(SessionLayerMapTest, testElementsExist) {
    checkAssociations();
}

TEST_F(SessionLayerMapTest, testLayersRemoved) {
    std::vector<int> layersToRemove = {3, 4};
    mSessionLayerMap->notifyLayersDied(layersToRemove);
    dropTestLayers(mSessionAssociations, layersToRemove);
    checkAssociations();
}

TEST_F(SessionLayerMapTest, testSessionsRemoved) {
    std::vector<int> sessionsToRemove = {0, 2, 4};
    mSessionLayerMap->notifySessionsDied(sessionsToRemove);
    dropTestSessions(mSessionAssociations, sessionsToRemove);
    checkAssociations();
}

TEST_F(SessionLayerMapTest, testRelevancyMakesSense) {
    std::map<size_t, std::vector<int>> toInsert{{80, {20, 21, 22}},
                                                {81, {20}},
                                                {82, {21, 24}},
                                                {83, {24, 25}},
                                                {84, {25, 26}}};
    for (auto& items : toInsert) {
        mSessionLayerMap->bindSessionIDToLayers((int32_t)items.first, items.second);
        mSessionAssociations[items.first] = items.second;
    }

    std::unordered_set<int32_t> startingRelevance;
    mSessionLayerMap->getCurrentlyRelevantLayers(startingRelevance);
    std::set<int> uniqueLayerSet = getUniqueLayers(mSessionAssociations);
    std::set<int> relevantLayerSet;
    relevantLayerSet.insert(startingRelevance.begin(), startingRelevance.end());

    // All unique layers should appear exactly once in the relevance set
    EXPECT_EQ(uniqueLayerSet, relevantLayerSet);

    std::vector<int32_t> sessionsToDrop{83, 84};
    std::vector<int32_t> layersToDrop{1, 2};

    // This should dump 4 layers: 1, 2, 25 and 26
    dropTestSessions(mSessionAssociations, sessionsToDrop);
    dropTestLayers(mSessionAssociations, layersToDrop);
    mSessionLayerMap->notifyLayersDied(layersToDrop);
    mSessionLayerMap->notifySessionsDied(sessionsToDrop);

    std::unordered_set<int32_t> updatedRelevance;
    mSessionLayerMap->getCurrentlyRelevantLayers(updatedRelevance);
    std::set<int> updatedUniqueLayerSet = getUniqueLayers(mSessionAssociations);
    std::set<int> updatedRelevantLayerSet;
    updatedRelevantLayerSet.insert(updatedRelevance.begin(), updatedRelevance.end());

    EXPECT_EQ(updatedUniqueLayerSet, updatedRelevantLayerSet);

    // Verify that we actually dumped four layers
    EXPECT_EQ(updatedRelevantLayerSet.size(), relevantLayerSet.size() - 4);
}

TEST_F(SessionLayerMapTest, testDiffsMakeSense) {
    std::set<int32_t> gotLayers, gotSessions;

    std::set<int32_t> expectedSessions{0, 1, 2, 3, 4, 5, 6};
    std::set<int32_t> expectedLayers{0, 1, 2, 3, 4, 5};

    mSessionLayerMap->getUpdatedItems(gotLayers, gotSessions);

    EXPECT_EQ(expectedLayers, gotLayers);
    EXPECT_EQ(expectedSessions, gotSessions);

    expectedSessions = std::set<int32_t>{2};
    expectedLayers = std::set<int32_t>{4};

    // Add an arbitrary binding
    mSessionLayerMap->bindSessionIDToLayers(2, {2, 3, 4});
    mSessionLayerMap->getUpdatedItems(gotLayers, gotSessions);

    EXPECT_EQ(expectedLayers, gotLayers);
    EXPECT_EQ(expectedSessions, gotSessions);

    std::vector<int32_t> sessionsToDrop{4, 6};
    std::vector<int32_t> layersToDrop{3};

    dropTestSessions(mSessionAssociations, sessionsToDrop);
    dropTestLayers(mSessionAssociations, layersToDrop);
    mSessionLayerMap->notifyLayersDied(layersToDrop);
    mSessionLayerMap->notifySessionsDied(sessionsToDrop);

    // This should only contain updates for things associated with layers or sessions that dropped
    // Dropped layers are not included, because we don't create updates for dead objects
    expectedSessions = std::set<int32_t>{0, 2, 3};
    expectedLayers = std::set<int32_t>{4, 5};

    mSessionLayerMap->getUpdatedItems(gotLayers, gotSessions);

    EXPECT_EQ(expectedLayers, gotLayers);
    EXPECT_EQ(expectedSessions, gotSessions);
}

} // namespace android::adpf
