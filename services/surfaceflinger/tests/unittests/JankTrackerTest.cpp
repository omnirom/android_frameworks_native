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

#include <android/gui/BnJankListener.h>
#include <binder/IInterface.h>
#include "BackgroundExecutor.h"
#include "Jank/JankTracker.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android {

namespace {

using namespace testing;

class MockJankListener : public gui::BnJankListener {
public:
    MockJankListener() = default;
    ~MockJankListener() override = default;

    MOCK_METHOD(binder::Status, onJankData, (const std::vector<gui::JankData>& jankData),
                (override));
};

} // anonymous namespace

class JankTrackerTest : public Test {
public:
    JankTrackerTest() {}

    void SetUp() override { mListener = sp<StrictMock<MockJankListener>>::make(); }

    void addJankListener(int32_t layerId) {
        JankTracker::addJankListener(layerId, IInterface::asBinder(mListener));
    }

    void removeJankListener(int32_t layerId, int64_t after) {
        JankTracker::removeJankListener(layerId, IInterface::asBinder(mListener), after);
    }

    void addJankData(int32_t layerId, int jankType) {
        gui::JankData data;
        data.frameVsyncId = mVsyncId++;
        data.jankType = jankType;
        data.frameIntervalNs = 8333333;
        JankTracker::onJankData(layerId, data);
    }

    void flushBackgroundThread() { BackgroundExecutor::getLowPriorityInstance().flushQueue(); }

    size_t listenerCount() { return JankTracker::sListenerCount; }

    std::vector<gui::JankData> getCollectedJankData(int32_t layerId) {
        return JankTracker::getCollectedJankDataForTesting(layerId);
    }

    sp<StrictMock<MockJankListener>> mListener = nullptr;
    int64_t mVsyncId = 1000;
};

TEST_F(JankTrackerTest, jankDataIsTrackedAndPropagated) {
    ASSERT_EQ(listenerCount(), 0u);

    EXPECT_CALL(*mListener.get(), onJankData(SizeIs(3)))
            .WillOnce([](const std::vector<gui::JankData>& jankData) {
                EXPECT_EQ(jankData[0].frameVsyncId, 1000);
                EXPECT_EQ(jankData[0].jankType, 1);
                EXPECT_EQ(jankData[0].frameIntervalNs, 8333333);

                EXPECT_EQ(jankData[1].frameVsyncId, 1001);
                EXPECT_EQ(jankData[1].jankType, 2);
                EXPECT_EQ(jankData[1].frameIntervalNs, 8333333);

                EXPECT_EQ(jankData[2].frameVsyncId, 1002);
                EXPECT_EQ(jankData[2].jankType, 3);
                EXPECT_EQ(jankData[2].frameIntervalNs, 8333333);
                return binder::Status::ok();
            });
    EXPECT_CALL(*mListener.get(), onJankData(SizeIs(2)))
            .WillOnce([](const std::vector<gui::JankData>& jankData) {
                EXPECT_EQ(jankData[0].frameVsyncId, 1003);
                EXPECT_EQ(jankData[0].jankType, 4);
                EXPECT_EQ(jankData[0].frameIntervalNs, 8333333);

                EXPECT_EQ(jankData[1].frameVsyncId, 1004);
                EXPECT_EQ(jankData[1].jankType, 5);
                EXPECT_EQ(jankData[1].frameIntervalNs, 8333333);

                return binder::Status::ok();
            });

    addJankListener(123);
    addJankData(123, 1);
    addJankData(123, 2);
    addJankData(123, 3);
    JankTracker::flushJankData(123);
    addJankData(123, 4);
    removeJankListener(123, mVsyncId);
    addJankData(123, 5);
    JankTracker::flushJankData(123);
    addJankData(123, 6);
    JankTracker::flushJankData(123);
    removeJankListener(123, 0);

    flushBackgroundThread();
}

TEST_F(JankTrackerTest, jankDataIsAutomaticallyFlushedInBatches) {
    ASSERT_EQ(listenerCount(), 0u);

    // needs to be larger than kJankDataBatchSize in JankTracker.cpp.
    constexpr size_t kNumberOfJankDataToSend = 234;

    size_t jankDataReceived = 0;
    size_t numBatchesReceived = 0;

    EXPECT_CALL(*mListener.get(), onJankData(_))
            .WillRepeatedly([&](const std::vector<gui::JankData>& jankData) {
                jankDataReceived += jankData.size();
                numBatchesReceived++;
                return binder::Status::ok();
            });

    addJankListener(123);
    for (size_t i = 0; i < kNumberOfJankDataToSend; i++) {
        addJankData(123, 0);
    }

    flushBackgroundThread();
    // Check that we got some data, without explicitly flushing.
    EXPECT_GT(jankDataReceived, 0u);
    EXPECT_GT(numBatchesReceived, 0u);
    EXPECT_LT(numBatchesReceived, jankDataReceived); // batches should be > size 1.

    removeJankListener(123, 0);
    JankTracker::flushJankData(123);
    flushBackgroundThread();
    EXPECT_EQ(jankDataReceived, kNumberOfJankDataToSend);
}

TEST_F(JankTrackerTest, jankListenerIsRemovedWhenReturningNullError) {
    ASSERT_EQ(listenerCount(), 0u);

    EXPECT_CALL(*mListener.get(), onJankData(SizeIs(3)))
            .WillOnce(Return(binder::Status::fromExceptionCode(binder::Status::EX_NULL_POINTER)));

    addJankListener(123);
    addJankData(123, 1);
    addJankData(123, 2);
    addJankData(123, 3);
    JankTracker::flushJankData(123);
    addJankData(123, 4);
    addJankData(123, 5);
    JankTracker::flushJankData(123);
    flushBackgroundThread();

    EXPECT_EQ(listenerCount(), 0u);
}

TEST_F(JankTrackerTest, jankDataIsDroppedIfNobodyIsListening) {
    ASSERT_EQ(listenerCount(), 0u);

    addJankData(123, 1);
    addJankData(123, 2);
    addJankData(123, 3);
    flushBackgroundThread();

    EXPECT_EQ(getCollectedJankData(123).size(), 0u);
}

TEST_F(JankTrackerTest, listenerCountTracksRegistrations) {
    ASSERT_EQ(listenerCount(), 0u);

    addJankListener(123);
    addJankListener(456);
    flushBackgroundThread();
    EXPECT_EQ(listenerCount(), 2u);

    removeJankListener(123, 0);
    JankTracker::flushJankData(123);
    removeJankListener(456, 0);
    JankTracker::flushJankData(456);
    flushBackgroundThread();
    EXPECT_EQ(listenerCount(), 0u);
}

TEST_F(JankTrackerTest, listenerCountIsAccurateOnDuplicateRegistration) {
    ASSERT_EQ(listenerCount(), 0u);

    addJankListener(123);
    addJankListener(123);
    flushBackgroundThread();
    EXPECT_EQ(listenerCount(), 1u);

    removeJankListener(123, 0);
    JankTracker::flushJankData(123);
    flushBackgroundThread();
    EXPECT_EQ(listenerCount(), 0u);
}

} // namespace android