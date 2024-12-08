/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "VibratorManagerHalWrapperAidlTest"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utils/Log.h>

#include <vibratorservice/VibratorManagerHalWrapper.h>

#include "test_mocks.h"
#include "test_utils.h"

using aidl::android::hardware::vibrator::Braking;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorCallback;
using aidl::android::hardware::vibrator::IVibratorManager;
using aidl::android::hardware::vibrator::PrimitivePwle;

using namespace android;
using namespace testing;

static const auto OFF_FN = [](vibrator::HalWrapper* hal) { return hal->off(); };

// -------------------------------------------------------------------------------------------------

class MockIVibratorManager : public IVibratorManager {
public:
    MockIVibratorManager() = default;

    MOCK_METHOD(ndk::ScopedAStatus, getCapabilities, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getVibratorIds, (std::vector<int32_t> * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getVibrator, (int32_t id, std::shared_ptr<IVibrator>* ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, prepareSynced, (const std::vector<int32_t>& ids), (override));
    MOCK_METHOD(ndk::ScopedAStatus, triggerSynced, (const std::shared_ptr<IVibratorCallback>& cb),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, cancelSynced, (), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getInterfaceVersion, (int32_t*), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getInterfaceHash, (std::string*), (override));
    MOCK_METHOD(ndk::SpAIBinder, asBinder, (), (override));
    MOCK_METHOD(bool, isRemote, (), (override));
};

// -------------------------------------------------------------------------------------------------

class VibratorManagerHalWrapperAidlTest : public Test {
public:
    void SetUp() override {
        mMockVibrator = ndk::SharedRefBase::make<StrictMock<vibrator::MockIVibrator>>();
        mMockHal = ndk::SharedRefBase::make<StrictMock<MockIVibratorManager>>();
        mMockScheduler = std::make_shared<StrictMock<vibrator::MockCallbackScheduler>>();
        mWrapper = std::make_unique<vibrator::AidlManagerHalWrapper>(mMockScheduler, mMockHal);
        ASSERT_NE(mWrapper, nullptr);
    }

protected:
    std::shared_ptr<StrictMock<vibrator::MockCallbackScheduler>> mMockScheduler = nullptr;
    std::unique_ptr<vibrator::ManagerHalWrapper> mWrapper = nullptr;
    std::shared_ptr<StrictMock<MockIVibratorManager>> mMockHal = nullptr;
    std::shared_ptr<StrictMock<vibrator::MockIVibrator>> mMockVibrator = nullptr;
};

// -------------------------------------------------------------------------------------------------

static const std::vector<int32_t> kVibratorIds = {1, 2};
static constexpr int kVibratorId = 1;

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetCapabilitiesDoesNotCacheFailedResult) {
    EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(IVibratorManager::CAP_SYNC),
                            Return(ndk::ScopedAStatus::ok())));

    ASSERT_TRUE(mWrapper->getCapabilities().isUnsupported());
    ASSERT_TRUE(mWrapper->getCapabilities().isFailed());

    auto result = mWrapper->getCapabilities();
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(vibrator::ManagerCapabilities::SYNC, result.value());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetCapabilitiesCachesResult) {
    EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(IVibratorManager::CAP_SYNC),
                            Return(ndk::ScopedAStatus::ok())));

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([&]() {
            auto result = mWrapper->getCapabilities();
            ASSERT_TRUE(result.isOk());
            ASSERT_EQ(vibrator::ManagerCapabilities::SYNC, result.value());
        }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });

    auto result = mWrapper->getCapabilities();
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(vibrator::ManagerCapabilities::SYNC, result.value());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetVibratorIdsDoesNotCacheFailedResult) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    ASSERT_TRUE(mWrapper->getVibratorIds().isUnsupported());
    ASSERT_TRUE(mWrapper->getVibratorIds().isFailed());

    auto result = mWrapper->getVibratorIds();
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(kVibratorIds, result.value());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetVibratorIdsCachesResult) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.push_back(std::thread([&]() {
            auto result = mWrapper->getVibratorIds();
            ASSERT_TRUE(result.isOk());
            ASSERT_EQ(kVibratorIds, result.value());
        }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });

    auto result = mWrapper->getVibratorIds();
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(kVibratorIds, result.value());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetVibratorWithValidIdReturnsController) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

        EXPECT_CALL(*mMockHal.get(), getVibrator(Eq(kVibratorId), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())));
    }

    auto result = mWrapper->getVibrator(kVibratorId);
    ASSERT_TRUE(result.isOk());
    ASSERT_NE(nullptr, result.value().get());
    ASSERT_TRUE(result.value().get()->init());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetVibratorWithInvalidIdFails) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    ASSERT_TRUE(mWrapper->getVibrator(0).isFailed());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestGetVibratorRecoversVibratorPointer) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockHal.get(), getVibrator(Eq(kVibratorId), _))
            .Times(Exactly(3))
            .WillOnce(DoAll(SetArgPointee<1>(nullptr),
                            Return(ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED))))
            // ndk::ScopedAStatus::ok() cannot be copy-constructed so can't use WillRepeatedly
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())))
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockVibrator.get(), off())
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED)))
            .WillOnce(Return(ndk::ScopedAStatus::ok()));

    // Get vibrator controller is successful even if first getVibrator.
    auto result = mWrapper->getVibrator(kVibratorId);
    ASSERT_TRUE(result.isOk());
    ASSERT_NE(nullptr, result.value().get());

    auto vibrator = result.value();
    // First getVibrator call fails.
    ASSERT_FALSE(vibrator->init());
    // First and second off() calls fail, reload IVibrator with getVibrator.
    ASSERT_TRUE(vibrator->doWithRetry<void>(OFF_FN, "off").isFailed());
    // Third call to off() worked after IVibrator reloaded.
    ASSERT_TRUE(vibrator->doWithRetry<void>(OFF_FN, "off").isOk());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestPrepareSynced) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockHal.get(), getVibrator(_, _))
            .Times(Exactly(2))
            // ndk::ScopedAStatus::ok() cannot be copy-constructed so can't use WillRepeatedly
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())))
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockHal.get(), prepareSynced(Eq(kVibratorIds)))
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(Return(ndk::ScopedAStatus::ok()));

    ASSERT_TRUE(mWrapper->getVibratorIds().isOk());
    ASSERT_TRUE(mWrapper->prepareSynced(kVibratorIds).isUnsupported());
    ASSERT_TRUE(mWrapper->prepareSynced(kVibratorIds).isFailed());
    ASSERT_TRUE(mWrapper->prepareSynced(kVibratorIds).isOk());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestTriggerSyncedWithCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibratorManager::CAP_TRIGGER_CALLBACK),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), triggerSynced(_))
                .Times(Exactly(3))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
                .WillOnce(DoAll(vibrator::TriggerCallback(), Return(ndk::ScopedAStatus::ok())));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    ASSERT_TRUE(mWrapper->triggerSynced(callback).isUnsupported());
    ASSERT_TRUE(mWrapper->triggerSynced(callback).isFailed());
    ASSERT_TRUE(mWrapper->triggerSynced(callback).isOk());
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestTriggerSyncedWithoutCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibratorManager::CAP_SYNC),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), triggerSynced(Eq(nullptr)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    ASSERT_TRUE(mWrapper->triggerSynced(callback).isOk());
    ASSERT_EQ(0, *callbackCounter.get());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestCancelSynced) {
    EXPECT_CALL(*mMockHal.get(), cancelSynced())
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(Return(ndk::ScopedAStatus::ok()));

    ASSERT_TRUE(mWrapper->cancelSynced().isUnsupported());
    ASSERT_TRUE(mWrapper->cancelSynced().isFailed());
    ASSERT_TRUE(mWrapper->cancelSynced().isOk());
}

TEST_F(VibratorManagerHalWrapperAidlTest, TestCancelSyncedReloadsAllControllers) {
    EXPECT_CALL(*mMockHal.get(), getVibratorIds(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(kVibratorIds), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockHal.get(), getVibrator(_, _))
            .Times(Exactly(2))
            // ndk::ScopedAStatus::ok() cannot be copy-constructed so can't use WillRepeatedly
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())))
            .WillOnce(DoAll(SetArgPointee<1>(mMockVibrator), Return(ndk::ScopedAStatus::ok())));

    EXPECT_CALL(*mMockHal.get(), cancelSynced())
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::ok()));

    ASSERT_TRUE(mWrapper->getVibratorIds().isOk());
    ASSERT_TRUE(mWrapper->cancelSynced().isOk());
}
