/*
 * Copyright (C) 2025 The Android Open Source Project
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

#define LOG_TAG "VibratorControllerTest"

#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utils/Log.h>
#include <thread>

#include <vibratorservice/VibratorController.h>

#include "test_mocks.h"
#include "test_utils.h"

using ::aidl::android::hardware::vibrator::Effect;
using ::aidl::android::hardware::vibrator::EffectStrength;
using ::aidl::android::hardware::vibrator::IVibrator;

using namespace android;
using namespace testing;

const auto kReturnOk = []() { return ndk::ScopedAStatus::ok(); };
const auto kReturnUnsupported = []() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
};
const auto kReturnTransactionFailed = []() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_TRANSACTION_FAILED);
};
const auto kReturnUnknownTransaction = []() {
    return ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION);
};
const auto kReturnIllegalArgument = []() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
};

// -------------------------------------------------------------------------------------------------

/* Provides mock IVibrator instance for testing. */
class FakeVibratorProvider : public vibrator::VibratorProvider {
public:
    FakeVibratorProvider()
          : mIsDeclared(true),
            mMockVibrator(ndk::SharedRefBase::make<StrictMock<vibrator::MockIVibrator>>()),
            mConnectCount(0),
            mReconnectCount(0) {}
    virtual ~FakeVibratorProvider() = default;

    bool isDeclared() override { return mIsDeclared; }

    std::shared_ptr<IVibrator> waitForVibrator() override {
        mConnectCount++;
        return mIsDeclared ? mMockVibrator : nullptr;
    }

    std::shared_ptr<IVibrator> checkForVibrator() override {
        mReconnectCount++;
        return mIsDeclared ? mMockVibrator : nullptr;
    }

    void setDeclared(bool isDeclared) { mIsDeclared = isDeclared; }

    int32_t getConnectCount() { return mConnectCount; }

    int32_t getReconnectCount() { return mReconnectCount; }

    std::shared_ptr<StrictMock<vibrator::MockIVibrator>> getMockVibrator() { return mMockVibrator; }

private:
    bool mIsDeclared;
    std::shared_ptr<StrictMock<vibrator::MockIVibrator>> mMockVibrator;
    int32_t mConnectCount;
    int32_t mReconnectCount;
};

// -------------------------------------------------------------------------------------------------

class VibratorControllerTest : public Test {
public:
    void SetUp() override {
        mProvider = std::make_shared<FakeVibratorProvider>();
        mController = std::make_unique<vibrator::VibratorController>(mProvider);
        ASSERT_NE(mController, nullptr);
    }

protected:
    std::shared_ptr<FakeVibratorProvider> mProvider = nullptr;
    std::unique_ptr<vibrator::VibratorController> mController = nullptr;
};

// -------------------------------------------------------------------------------------------------

TEST_F(VibratorControllerTest, TestInitServiceDeclared) {
    ASSERT_TRUE(mController->init());
    ASSERT_EQ(1, mProvider->getConnectCount());
    ASSERT_EQ(0, mProvider->getReconnectCount());

    // Noop when wrapper was already initialized.
    ASSERT_TRUE(mController->init());
    ASSERT_EQ(1, mProvider->getConnectCount());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestInitServiceNotDeclared) {
    mProvider->setDeclared(false);

    ASSERT_FALSE(mController->init());
    ASSERT_EQ(0, mProvider->getConnectCount());
    ASSERT_EQ(0, mProvider->getReconnectCount());

    ASSERT_FALSE(mController->init());
    ASSERT_EQ(0, mProvider->getConnectCount());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestFirstCallTriggersInit) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);

    auto status = mController->off();
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(1, mProvider->getConnectCount());
}

TEST_F(VibratorControllerTest, TestSuccessfulResultDoesNotRetry) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);

    auto status = mController->off();
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestUnsupportedOperationResultDoesNotRetry) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnUnsupported);

    auto status = mController->off();
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestUnknownTransactionResultDoesNotRetry) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnUnknownTransaction);

    auto status = mController->off();
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestOperationFailedDoesNotRetry) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnIllegalArgument);

    auto status = mController->off();
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(0, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestTransactionFailedRetriesOnlyOnce) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(2))
            .WillRepeatedly(kReturnTransactionFailed);

    auto status = mController->off();
    ASSERT_FALSE(status.isOk());
    ASSERT_EQ(1, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestTransactionFailedThenSucceedsReturnsSuccessAfterRetries) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(2))
            .WillOnce(kReturnTransactionFailed)
            .WillRepeatedly(kReturnOk);

    auto status = mController->off();
    ASSERT_TRUE(status.isOk());
    ASSERT_EQ(1, mProvider->getReconnectCount());
}

TEST_F(VibratorControllerTest, TestOff) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), off())
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);

    auto status = mController->off();
    ASSERT_TRUE(status.isOk());
}

TEST_F(VibratorControllerTest, TestSetAmplitude) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), setAmplitude(Eq(0.1f)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);
    EXPECT_CALL(*mProvider->getMockVibrator().get(), setAmplitude(Eq(0.2f)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnIllegalArgument);

    ASSERT_TRUE(mController->setAmplitude(0.1f).isOk());
    ASSERT_FALSE(mController->setAmplitude(0.2f).isOk());
}

TEST_F(VibratorControllerTest, TestSetExternalControl) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(), setExternalControl(Eq(false)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);
    EXPECT_CALL(*mProvider->getMockVibrator().get(), setExternalControl(Eq(true)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnIllegalArgument);

    ASSERT_TRUE(mController->setExternalControl(false).isOk());
    ASSERT_FALSE(mController->setExternalControl(true).isOk());
}

TEST_F(VibratorControllerTest, TestAlwaysOnEnable) {
    EXPECT_CALL(*mProvider->getMockVibrator().get(),
                alwaysOnEnable(Eq(1), Eq(Effect::CLICK), Eq(EffectStrength::LIGHT)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnOk);
    EXPECT_CALL(*mProvider->getMockVibrator().get(),
                alwaysOnEnable(Eq(2), Eq(Effect::TICK), Eq(EffectStrength::MEDIUM)))
            .Times(Exactly(1))
            .WillRepeatedly(kReturnIllegalArgument);

    ASSERT_TRUE(mController->alwaysOnEnable(1, Effect::CLICK, EffectStrength::LIGHT).isOk());
    ASSERT_FALSE(mController->alwaysOnEnable(2, Effect::TICK, EffectStrength::MEDIUM).isOk());
}
