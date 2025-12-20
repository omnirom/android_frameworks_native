/*
 * Copyright (C) 2025 The Android Open Source Project
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
#include <android/os/BnStatsBootstrapAtomService.h> // For mock implementation
#include <android/os/StatsBootstrapAtom.h>
#include <android/os/StatsBootstrapAtomValue.h>
#include <dlfcn.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/SystemClock.h>

#include <../BinderStatsPusher.h>
#include <../BinderStatsUtils.h>
#include <../JvmUtils.h>
#include "fakeservicemanager/FakeServiceManager.h"

#include <jni.h>

using android::FakeServiceManager;
using namespace android;
using namespace testing;

// --- Mocks ---
constexpr int64_t kSpamAggregationWindowSec = 5;
constexpr int64_t kLatencyAggregationWindowSec = 5; // Same as spam for now
// Mock for IStatsBootstrapAtomService
class MockStatsBootstrapAtomService : public os::BnStatsBootstrapAtomService {
public:
    MOCK_METHOD(binder::Status, reportBootstrapAtom, (const os::StatsBootstrapAtom& atom),
                (override));
    MOCK_METHOD(BBinder*, localBinder, (), (override));
};

// Mock for IServiceManager to control service lookup
class MockServiceManager : public FakeServiceManager {
public:
    MOCK_METHOD(sp<IBinder>, checkService, (const String16& name), (const, override));
};

// --- Test Fixture ---
void initServiceManagerOnce() {
    static std::once_flag gSmOnce;
    std::call_once(gSmOnce, [] {
        sp<NiceMock<MockServiceManager>> mockServiceManager =
                sp<NiceMock<MockServiceManager>>::make();
        setDefaultServiceManager(mockServiceManager);
    });
}

// Helper function to create BinderCallData for tests
BinderCallData createStatsData(uid_t uid, uint32_t code, const char* desc, int64_t startNanos) {
    return {.interfaceDescriptor = String16(desc),
            .transactionCode = code,
            .startTimeNanos = startNanos,
            .endTimeNanos = 0,
            .senderUid = uid};
}

BinderCallData createStatsData(uid_t uid, const char* desc, uint32_t code, int64_t startNanos,
                               int64_t endNanos) {
    return BinderCallData{
            .interfaceDescriptor = String16(desc),
            .transactionCode = code,
            .startTimeNanos = startNanos,
            .endTimeNanos = endNanos,
            .senderUid = uid,
    };
}

// Helper function to create a StatsBootstrapAtom for spam comparison
os::StatsBootstrapAtom createExpectedAtom(const BinderCallData& datum, int count125, int count250) {
    auto atom = os::StatsBootstrapAtom();
    // TODO: use directly from stats/atoms/framework/framework_extension_atoms.proto
    atom.atomId = 1064; // kBinderSpamAtomId
    atom.values.push_back(createPrimitiveValue((int64_t)datum.senderUid));
    atom.values.push_back(createPrimitiveValue((int64_t)getuid())); // host uid
    atom.values.push_back(createPrimitiveValue(datum.interfaceDescriptor));
    atom.values.push_back(createPrimitiveValue(std::to_string(datum.transactionCode)));
    atom.values.push_back(createPrimitiveValue((int32_t)count125));
    atom.values.push_back(createPrimitiveValue((int32_t)count250));
    return atom;
}

// Helper function to create a StatsBootstrapAtom for latency comparison
os::StatsBootstrapAtom createExpectedLatencyAtom(const BinderCallData& datum, int64_t callCount,
                                                 int64_t durationSumMicros,
                                                 int32_t secondsWithAtLeast10Calls,
                                                 int32_t secondsWithAtLeast50Calls) {
    auto atom = os::StatsBootstrapAtom();
    // TODO: use directly from stats/atoms/framework/framework_extension_atoms.proto
    atom.atomId = 1090; // kBinderLatencyAtomId
    atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(datum.senderUid)));
    atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(getuid()))); // host uid
    atom.values.push_back(createPrimitiveValue(datum.interfaceDescriptor));
    atom.values.push_back(createPrimitiveValue(std::to_string(datum.transactionCode)));
    atom.values.push_back(createPrimitiveValue(callCount));
    atom.values.push_back(createPrimitiveValue(durationSumMicros));
    atom.values.push_back(createPrimitiveValue(secondsWithAtLeast10Calls));
    atom.values.push_back(createPrimitiveValue(secondsWithAtLeast50Calls));
    return atom;
}

// Matcher for StatsBootstrapAtom
MATCHER_P(StatsAtomEq, expectedAtom, "") {
    if (arg.atomId != expectedAtom.atomId) return false;
    if (arg.values.size() != expectedAtom.values.size()) return false;
    for (size_t i = 0; i < arg.values.size(); ++i) {
        if (arg.values[i].value.getTag() != expectedAtom.values[i].value.getTag()) return false;
    }
    return true;
}

class BinderStatsPusherTest : public Test {
protected:
    sp<StrictMock<MockStatsBootstrapAtomService>> mockStatsService;
    sp<NiceMock<MockServiceManager>> mockServiceManager;
    BinderStatsPusher pusher;

    void SetUp() override {
        mockStatsService = sp<StrictMock<MockStatsBootstrapAtomService>>::make();
        initServiceManagerOnce();
        mockServiceManager = sp<NiceMock<MockServiceManager>>::cast(defaultServiceManager());
        ASSERT_NE(mockServiceManager, nullptr)
                << "Default service manager is not the expected mock type";
        // Default behavior: Service Manager returns the mock Stats Service
        ON_CALL(*mockServiceManager.get(), checkService(String16("statsbootstrap")))
                .WillByDefault(Return(IInterface::asBinder(mockStatsService)));
        ON_CALL(*mockStatsService.get(), localBinder()).WillByDefault(Return(nullptr));
    }

    void TearDown() override { testing::Mock::VerifyAndClear(defaultServiceManager().get()); }
};

// --- Test Cases ---

TEST_F(BinderStatsPusherTest, GetBootstrapService) {
    EXPECT_CALL(*mockServiceManager, checkService(String16("statsbootstrap")))
            .Times(1)
            .WillOnce(Return(IInterface::asBinder(mockStatsService)));

    auto service = pusher.getBootstrapAtomServiceLocked(15);
    ASSERT_EQ(service, mockStatsService);
}

TEST_F(BinderStatsPusherTest, AggregateSpamNoSpamBelowThreshold) {
    std::vector<BinderCallData> data;
    int64_t currentTimeSec = 14;
    // Create data within the delay window (kDelaySeconds = 2)
    for (int i = 0; i < 50; ++i) { // Less than kMinSpamCount (125)
        data.push_back(createStatsData(1001, 1, "IFoo", (currentTimeSec - 5) * 1000000000LL));
    }

    // Expect no calls to reportBootstrapAtom
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec); //  pushLocked calls
                                             //  aggregateBinderSpamLocked
}

TEST_F(BinderStatsPusherTest, AggregateSpamOneSecondSpam) {
    std::vector<BinderCallData> data;

    int64_t currentTimeNanos = 9'100'000'000;
    // Create enough data in the *same second* to trigger spam, far enough in the past
    for (int i = 0; i < 150; ++i) { // More than kMinSpamCount (125)
        data.push_back(createStatsData(1001, 1, "IFoo", currentTimeNanos - 8000'000'000));
    }

    auto expectedAtom = createExpectedAtom(data[0], 1, 0); // 1 second with >= 125 calls
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeNanos / 1000'000'000);
}

TEST_F(BinderStatsPusherTest, AggregateSpamDelayedSpam) {
    std::vector<BinderCallData> data;
    int64_t currentTimeNanos = 9'100'000'000;

    // Create spam data within the delay window (kDelaySeconds = 2)
    for (int i = 0; i < 150; ++i) {
        data.push_back(createStatsData(1002, 2, "IBar", currentTimeNanos - 1000'000'000));
    }

    // Expect no calls, data is delayed
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);
    pusher.pushLocked(data, currentTimeNanos / 1000'000'000);
}

TEST_F(BinderStatsPusherTest, AggregateSpamMixedOlderAndDelayed) {
    std::vector<BinderCallData> data;
    int64_t currentTimeNanos = 9'100'000'000;

    for (int i = 0; i < 130; ++i) {
        data.push_back(createStatsData(1003, 3, "IBaz", currentTimeNanos - 8000'000'000));
    }
    // Delayed spam data (within kDelaySeconds)
    for (int i = 0; i < 140; ++i) {
        data.push_back(createStatsData(1004, 4, "IQux", currentTimeNanos - 1000'000'000));
    }

    // Expect immediate spam to be reported now
    auto expectedImmediateAtom = createExpectedAtom(data[0], 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedImmediateAtom)))
            .Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);
    pusher.pushLocked(data, currentTimeNanos / 1000'000'000);
}

TEST_F(BinderStatsPusherTest, AggregateSpamSecondWatermark) {
    std::vector<BinderCallData> data;
    int64_t spamTimeNanos = 2'000'000'000LL;
    int64_t currentTimeSec = (spamTimeNanos / 1000'000'000LL) + kSpamAggregationWindowSec + 1;

    // Create data exceeding the second watermark (250 calls/sec)
    for (int i = 0; i < 300; ++i) {
        data.push_back(createStatsData(1005, 5, "IHighVolume", spamTimeNanos));
    }

    auto expectedAtom = createExpectedAtom(data[0], 1, 1);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, AggregateSpamAcrossMultipleSeconds) {
    std::vector<BinderCallData> data;
    int64_t firstSpamSecondNanos = 2'000'000'000LL;  // 2s
    int64_t secondSpamSecondNanos = 3'000'000'000LL; // 3s
    int64_t currentTimeSec = (secondSpamSecondNanos / 1000'000'000LL) + kSpamAggregationWindowSec +
            1; // 3 + 5 + 1 = 9s

    // Spam for the first second
    for (int i = 0; i < 150; ++i) {
        data.push_back(createStatsData(1006, 6, "IMultiSecond", firstSpamSecondNanos));
    }
    // Spam for the second second
    for (int i = 0; i < 160; ++i) {
        // Use the same UID, code, desc for aggregation
        data.push_back(createStatsData(1006, 6, "IMultiSecond", secondSpamSecondNanos));
    }

    // Expect one atom representing spam across 2 seconds
    auto expectedAtom = createExpectedAtom(data[0], 2, 0); // 2 seconds with >= 125 calls
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, AggregateSpamProcessesDelayedDataOnSubsequentCall) {
    std::vector<BinderCallData> callData1;
    int64_t callTimeSec1 = 10;
    int64_t spamDataTimeNanos = (callTimeSec1 - 2) * 1000'000'000LL; // 8s, will be delayed

    for (int i = 0; i < 150; ++i) {
        callData1.push_back(createStatsData(1007, 7, "IDelayed", spamDataTimeNanos));
    }

    // First push: data should be buffered as it's too recent
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1); // For the first push
    pusher.pushLocked(callData1, callTimeSec1);

    // Second push: advance time so the previous data is now outside the aggregation window
    std::vector<BinderCallData> callData2; // Can be empty or contain new data
    int64_t call2_time_sec = callTimeSec1 + kSpamAggregationWindowSec + 1; // 10 + 5 + 1 = 16s

    auto expectedAtom = createExpectedAtom(callData1[0], 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1); // For the second push

    pusher.pushLocked(callData2, call2_time_sec);
}

TEST_F(BinderStatsPusherTest, AggregateSpamForDifferentMethodsSimultaneously) {
    std::vector<BinderCallData> data;
    int64_t spamTimeNanos = 4'000'000'000LL; // 4s
    int64_t currentTimeSec =
            (spamTimeNanos / 1000'000'000LL) + kSpamAggregationWindowSec + 1; // 4 + 5 + 1 = 10s

    // Spam for method 1
    BinderCallData method1Spam = createStatsData(1008, 8, "IMultiMethod", spamTimeNanos);
    for (int i = 0; i < 200; ++i) {
        data.push_back(method1Spam);
    }

    // Spam for method 2 (different transaction code)
    BinderCallData method2Spam = createStatsData(1008, 9, "IMultiMethod", spamTimeNanos);
    for (int i = 0; i < 200; ++i) {
        data.push_back(method2Spam);
    }

    auto expectedAtom1 = createExpectedAtom(method1Spam, 1, 0);
    auto expectedAtom2 = createExpectedAtom(method2Spam, 1, 0);

    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(2);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, SkipPushForLocalBinderWithoutJvm) {
    // Simulate a local binder service
    sp<BBinder> localBinderInstance = sp<BBinder>::make();
    ON_CALL(*mockStatsService.get(), localBinder())
            .WillByDefault(Return(localBinderInstance.get()));
    // getJavaVM() is expected to return nullptr in the test environment (JvmUtils.h)

    std::vector<BinderCallData> data;
    int64_t spamTimeNanos = 2'000'000'000LL;                                                   // 2s
    int64_t currentTimeSec = (spamTimeNanos / 1000'000'000LL) + kSpamAggregationWindowSec + 1; // 8s

    for (int i = 0; i < 150; ++i) {
        data.push_back(createStatsData(1009, 10, "ILocalSkipped", spamTimeNanos));
    }

    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0); // Should be skipped
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, DataNotDroppedWhenPushIsSkippedThenSucceeds) {
    std::vector<BinderCallData> spamCallData1;
    int64_t timeSec1 = 20;
    // Data old enough to be processed immediately
    int64_t spamDataTimeNanos = (timeSec1 - kSpamAggregationWindowSec) * 1000'000'000LL;

    for (int i = 0; i < 150; ++i) { // Enough to trigger kSpamFirstWatermark
        spamCallData1.push_back(createStatsData(1010, 11, "IServiceSkipped", spamDataTimeNanos));
    }

    // First push: Service is unavailable
    EXPECT_CALL(*mockServiceManager, checkService(String16("statsbootstrap")))
            .Times(1)
            .WillOnce(Return(nullptr));
    // localBinder() shouldn't be called if service is null in aggregateBinderSpamLocked
    EXPECT_CALL(*mockStatsService, localBinder()).Times(0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);

    pusher.pushLocked(spamCallData1, timeSec1);
    Mock::VerifyAndClearExpectations(mockServiceManager.get());
    Mock::VerifyAndClearExpectations(mockStatsService.get());

    // Second push: Service becomes available. Advance time beyond service check timeout.
    std::vector<BinderCallData> spamCallData2; // Empty data for the second call
    int64_t timeSec2 = timeSec1 + 6;

    EXPECT_CALL(*mockServiceManager, checkService(String16("statsbootstrap")))
            .Times(1)
            .WillOnce(Return(IInterface::asBinder(mockStatsService)));
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1); // Called when service is not null
    auto expectedAtom = createExpectedAtom(spamCallData1[0], 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);

    pusher.pushLocked(spamCallData2, timeSec2);
}

// --- Latency Tests ---

TEST_F(BinderStatsPusherTest, AggregateLatencyNoLatencyBelowThreshold) {
    std::vector<BinderCallData> data;
    int64_t currentTimeSec = 14;
    // Create data within the delay window
    for (int i = 0; i < 5; ++i) { // Less than kLatencyCountFirstWatermark (10)
        data.push_back(
                createStatsData(2001, "ILatencyFoo", 1,
                                (currentTimeSec - kLatencyAggregationWindowSec + 1) * 1000000000LL,
                                (currentTimeSec - kLatencyAggregationWindowSec + 1) * 1000000000LL +
                                        1000000 /* 1ms */));
    }

    // Expect no calls to reportBootstrapAtom
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, AggregateLatencyOneSecondLatency) {
    std::vector<BinderCallData> data;
    int64_t callTimeNanos = 2'000'000'000LL; // 2s
    int64_t currentTimeSec =
            (callTimeNanos / 1000'000'000LL) + kLatencyAggregationWindowSec + 1; // 2 + 5 + 1 = 8s
    int64_t totalDurationMicros = 0;

    // Create enough data in the *same second* to trigger latency reporting
    for (int i = 0; i < 15; ++i) {              // More than kLatencyCountFirstWatermark (10)
        int64_t durationMicros = (i + 1) * 100; // e.g. 100us, 200us ..
        data.push_back(createStatsData(2002, "ILatencyBar", 2, callTimeNanos,
                                       callTimeNanos + durationMicros * 1000));
        totalDurationMicros += durationMicros;
    }

    auto expectedAtom = createExpectedLatencyAtom(data[0], 15, totalDurationMicros, 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}

TEST_F(BinderStatsPusherTest, AggregateLatencyDelayedLatency) {
    std::vector<BinderCallData> data;
    int64_t currentTimeNanos = 9'100'000'000;

    // Create latency data within the aggregation window
    for (int i = 0; i < 15; ++i) {
        data.push_back(createStatsData(2003, "ILatencyBaz", 3, currentTimeNanos - 1000'000'000,
                                       currentTimeNanos - 1000'000'000 + 500000 /* 0.5ms */));
    }

    // Expect no calls, data is delayed
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);
    pusher.pushLocked(data, currentTimeNanos / 1000'000'000);
}

TEST_F(BinderStatsPusherTest, AggregateLatencyProcessesDelayedDataOnSubsequentCall) {
    std::vector<BinderCallData> callData1;
    int64_t callTimeSec1 = 10;
    int64_t latencyDataTimeNanos = (callTimeSec1 - 2) * 1000'000'000LL; // 8s, will be delayed
    int64_t totalDurationMicros1 = 0;

    for (int i = 0; i < 15; ++i) {
        int64_t durationMicros = (i + 1) * 150;
        callData1.push_back(createStatsData(2004, "ILatencyDelayed", 4, latencyDataTimeNanos,
                                            latencyDataTimeNanos + durationMicros * 1000));
        totalDurationMicros1 += durationMicros;
    }

    // First push: data should be buffered as it's too recent
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(_)).Times(0);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);
    pusher.pushLocked(callData1, callTimeSec1);

    // Second push: advance time so the previous data is now outside the aggregation window
    std::vector<BinderCallData> callData2;                                    // Can be empty
    int64_t call2_time_sec = callTimeSec1 + kLatencyAggregationWindowSec + 1; // 10 + 5 + 1 = 16s

    auto expectedAtom = createExpectedLatencyAtom(callData1[0], 15, totalDurationMicros1, 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(callData2, call2_time_sec);
}

TEST_F(BinderStatsPusherTest, AggregateLatencyMultipleSeconds) {
    std::vector<BinderCallData> data;
    int64_t firstCallSecondNanos = 2'000'000'000LL;  // 2s
    int64_t secondCallSecondNanos = 3'000'000'000LL; // 3s
    int64_t currentTimeSec = (secondCallSecondNanos / 1000'000'000LL) +
            kLatencyAggregationWindowSec + 1; // 3 + 5 + 1 = 9s
    int64_t totalDurationMicros = 0;

    // Data for the first second
    for (int i = 0; i < 12; ++i) { // 12 calls
        int64_t durationMicros = (i + 1) * 100;
        data.push_back(createStatsData(2005, "ILatencyMultiSec", 5, firstCallSecondNanos,
                                       firstCallSecondNanos + durationMicros * 1000));
        totalDurationMicros += durationMicros;
    }
    // Data for the second second
    for (int i = 0; i < 8; ++i) { // 8 calls
        int64_t durationMicros = (i + 1) * 120;
        // Use the same UID, code, desc for aggregation
        data.push_back(createStatsData(2005, "ILatencyMultiSec", 5, secondCallSecondNanos,
                                       secondCallSecondNanos + durationMicros * 1000));
        totalDurationMicros += durationMicros;
    }

    // Expect one atom representing aggregated data across 2 seconds
    // Total calls = 12 + 8 = 20
    // secondsWithAtLeast10Calls = 1 (only the first second has >= 10 calls)
    // secondsWithAtLeast50Calls = 0
    auto expectedAtom = createExpectedLatencyAtom(data[0], 20, totalDurationMicros, 1, 0);
    EXPECT_CALL(*mockStatsService, reportBootstrapAtom(StatsAtomEq(expectedAtom))).Times(1);
    EXPECT_CALL(*mockStatsService, localBinder()).Times(1);

    pusher.pushLocked(data, currentTimeSec);
}
