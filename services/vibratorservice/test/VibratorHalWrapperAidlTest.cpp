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

#define LOG_TAG "VibratorHalWrapperAidlTest"

#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <android/persistable_bundle_aidl.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utils/Log.h>
#include <thread>

#include <vibratorservice/VibratorCallbackScheduler.h>
#include <vibratorservice/VibratorHalWrapper.h>

#include "test_mocks.h"
#include "test_utils.h"

using aidl::android::hardware::vibrator::Braking;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorCallback;
using aidl::android::hardware::vibrator::PrimitivePwle;
using aidl::android::hardware::vibrator::PwleV2Primitive;
using aidl::android::hardware::vibrator::VendorEffect;
using aidl::android::os::PersistableBundle;

using namespace android;
using namespace std::chrono_literals;
using namespace testing;

// -------------------------------------------------------------------------------------------------

class VibratorHalWrapperAidlTest : public Test {
public:
    void SetUp() override {
        mMockHal = ndk::SharedRefBase::make<StrictMock<vibrator::MockIVibrator>>();
        mMockScheduler = std::make_shared<StrictMock<vibrator::MockCallbackScheduler>>();
        mWrapper = std::make_unique<vibrator::AidlHalWrapper>(mMockScheduler, mMockHal);
        ASSERT_NE(mWrapper, nullptr);
    }

protected:
    std::shared_ptr<StrictMock<vibrator::MockCallbackScheduler>> mMockScheduler = nullptr;
    std::unique_ptr<vibrator::HalWrapper> mWrapper = nullptr;
    std::shared_ptr<StrictMock<vibrator::MockIVibrator>> mMockHal = nullptr;
};

// -------------------------------------------------------------------------------------------------

TEST_F(VibratorHalWrapperAidlTest, TestOnWithCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_ON_CALLBACK),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), on(Eq(10), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), on(Eq(100), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
        EXPECT_CALL(*mMockHal.get(), on(Eq(1000), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    ASSERT_TRUE(mWrapper->on(10ms, callback).isOk());
    ASSERT_EQ(1, *callbackCounter.get());

    ASSERT_TRUE(mWrapper->on(100ms, callback).isUnsupported());
    // Callback not triggered for unsupported
    ASSERT_EQ(1, *callbackCounter.get());

    ASSERT_TRUE(mWrapper->on(1000ms, callback).isFailed());
    // Callback not triggered on failure
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestOnWithoutCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_COMPOSE_EFFECTS),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), on(Eq(10), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
        EXPECT_CALL(*mMockScheduler.get(), schedule(_, Eq(10ms)))
                .Times(Exactly(1))
                .WillOnce(vibrator::TriggerSchedulerCallback());
        EXPECT_CALL(*mMockHal.get(), on(Eq(11), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
        EXPECT_CALL(*mMockHal.get(), on(Eq(12), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    ASSERT_TRUE(mWrapper->on(10ms, callback).isOk());
    ASSERT_EQ(1, *callbackCounter.get());

    ASSERT_TRUE(mWrapper->on(11ms, callback).isUnsupported());
    ASSERT_TRUE(mWrapper->on(12ms, callback).isFailed());

    // Callback not triggered for unsupported and on failure
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestOff) {
    EXPECT_CALL(*mMockHal.get(), off())
            .Times(Exactly(3))
            .WillOnce(Return(ndk::ScopedAStatus::ok()))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));

    ASSERT_TRUE(mWrapper->off().isOk());
    ASSERT_TRUE(mWrapper->off().isUnsupported());
    ASSERT_TRUE(mWrapper->off().isFailed());
}

TEST_F(VibratorHalWrapperAidlTest, TestSetAmplitude) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), setAmplitude(Eq(0.1f)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
        EXPECT_CALL(*mMockHal.get(), setAmplitude(Eq(0.2f)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
        EXPECT_CALL(*mMockHal.get(), setAmplitude(Eq(0.5f)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    ASSERT_TRUE(mWrapper->setAmplitude(0.1f).isOk());
    ASSERT_TRUE(mWrapper->setAmplitude(0.2f).isUnsupported());
    ASSERT_TRUE(mWrapper->setAmplitude(0.5f).isFailed());
}

TEST_F(VibratorHalWrapperAidlTest, TestSetExternalControl) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), setExternalControl(Eq(true)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
        EXPECT_CALL(*mMockHal.get(), setExternalControl(Eq(false)))
                .Times(Exactly(2))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    ASSERT_TRUE(mWrapper->setExternalControl(true).isOk());
    ASSERT_TRUE(mWrapper->setExternalControl(false).isUnsupported());
    ASSERT_TRUE(mWrapper->setExternalControl(false).isFailed());
}

TEST_F(VibratorHalWrapperAidlTest, TestAlwaysOnEnable) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(),
                    alwaysOnEnable(Eq(1), Eq(Effect::CLICK), Eq(EffectStrength::LIGHT)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
        EXPECT_CALL(*mMockHal.get(),
                    alwaysOnEnable(Eq(2), Eq(Effect::TICK), Eq(EffectStrength::MEDIUM)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
        EXPECT_CALL(*mMockHal.get(),
                    alwaysOnEnable(Eq(3), Eq(Effect::POP), Eq(EffectStrength::STRONG)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    auto result = mWrapper->alwaysOnEnable(1, Effect::CLICK, EffectStrength::LIGHT);
    ASSERT_TRUE(result.isOk());
    result = mWrapper->alwaysOnEnable(2, Effect::TICK, EffectStrength::MEDIUM);
    ASSERT_TRUE(result.isUnsupported());
    result = mWrapper->alwaysOnEnable(3, Effect::POP, EffectStrength::STRONG);
    ASSERT_TRUE(result.isFailed());
}

TEST_F(VibratorHalWrapperAidlTest, TestAlwaysOnDisable) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), alwaysOnDisable(Eq(1)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::ok()));
        EXPECT_CALL(*mMockHal.get(), alwaysOnDisable(Eq(2)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
        EXPECT_CALL(*mMockHal.get(), alwaysOnDisable(Eq(3)))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    ASSERT_TRUE(mWrapper->alwaysOnDisable(1).isOk());
    ASSERT_TRUE(mWrapper->alwaysOnDisable(2).isUnsupported());
    ASSERT_TRUE(mWrapper->alwaysOnDisable(3).isFailed());
}

TEST_F(VibratorHalWrapperAidlTest, TestGetInfoDoesNotCacheFailedResult) {
    constexpr float F_MIN = 100.f;
    constexpr float F0 = 123.f;
    constexpr float F_RESOLUTION = 0.5f;
    constexpr float Q_FACTOR = 123.f;
    constexpr int32_t COMPOSITION_SIZE_MAX = 10;
    constexpr int32_t PWLE_SIZE_MAX = 20;
    constexpr int32_t PRIMITIVE_DELAY_MAX = 100;
    constexpr int32_t PWLE_DURATION_MAX = 200;
    constexpr int32_t PWLE_V2_COMPOSITION_SIZE_MAX = 16;
    constexpr int32_t PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS = 20;
    constexpr int32_t PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS = 1000;
    std::vector<Effect> supportedEffects = {Effect::CLICK, Effect::TICK};
    std::vector<CompositePrimitive> supportedPrimitives = {CompositePrimitive::CLICK};
    std::vector<Braking> supportedBraking = {Braking::CLAB};
    std::vector<float> amplitudes = {0.f, 1.f, 0.f};

    std::vector<std::chrono::milliseconds> primitiveDurations;
    constexpr auto primitiveRange = ndk::enum_range<CompositePrimitive>();
    constexpr auto primitiveCount = std::distance(primitiveRange.begin(), primitiveRange.end());
    primitiveDurations.resize(primitiveCount);
    primitiveDurations[static_cast<size_t>(CompositePrimitive::CLICK)] = 10ms;

    EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_ON_CALLBACK),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getSupportedEffects(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(supportedEffects), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getSupportedBraking(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(supportedBraking), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getSupportedPrimitives(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(
                    DoAll(SetArgPointee<0>(supportedPrimitives), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::CLICK), _))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<1>(10), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getCompositionSizeMax(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(COMPOSITION_SIZE_MAX),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getCompositionDelayMax(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(
                    DoAll(SetArgPointee<0>(PRIMITIVE_DELAY_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwlePrimitiveDurationMax(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_DURATION_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleCompositionSizeMax(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_SIZE_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getFrequencyMinimum(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(F_MIN), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getResonantFrequency(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(F0), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getFrequencyResolution(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(F_RESOLUTION), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getQFactor(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(Q_FACTOR), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getBandwidthAmplitudeMap(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(amplitudes), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleV2CompositionSizeMax(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_COMPOSITION_SIZE_MAX),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleV2PrimitiveDurationMinMillis(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleV2PrimitiveDurationMaxMillis(_))
            .Times(Exactly(2))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS),
                            Return(ndk::ScopedAStatus::ok())));

    vibrator::Info failed = mWrapper->getInfo();
    ASSERT_TRUE(failed.capabilities.isFailed());
    ASSERT_TRUE(failed.supportedEffects.isFailed());
    ASSERT_TRUE(failed.supportedBraking.isFailed());
    ASSERT_TRUE(failed.supportedPrimitives.isFailed());
    ASSERT_TRUE(failed.primitiveDurations.isFailed());
    ASSERT_TRUE(failed.primitiveDelayMax.isFailed());
    ASSERT_TRUE(failed.pwlePrimitiveDurationMax.isFailed());
    ASSERT_TRUE(failed.compositionSizeMax.isFailed());
    ASSERT_TRUE(failed.pwleSizeMax.isFailed());
    ASSERT_TRUE(failed.minFrequency.isFailed());
    ASSERT_TRUE(failed.resonantFrequency.isFailed());
    ASSERT_TRUE(failed.frequencyResolution.isFailed());
    ASSERT_TRUE(failed.qFactor.isFailed());
    ASSERT_TRUE(failed.maxAmplitudes.isFailed());
    ASSERT_TRUE(failed.maxEnvelopeEffectSize.isFailed());
    ASSERT_TRUE(failed.minEnvelopeEffectControlPointDuration.isFailed());
    ASSERT_TRUE(failed.maxEnvelopeEffectControlPointDuration.isFailed());

    vibrator::Info successful = mWrapper->getInfo();
    ASSERT_EQ(vibrator::Capabilities::ON_CALLBACK, successful.capabilities.value());
    ASSERT_EQ(supportedEffects, successful.supportedEffects.value());
    ASSERT_EQ(supportedBraking, successful.supportedBraking.value());
    ASSERT_EQ(supportedPrimitives, successful.supportedPrimitives.value());
    ASSERT_EQ(primitiveDurations, successful.primitiveDurations.value());
    ASSERT_EQ(std::chrono::milliseconds(PRIMITIVE_DELAY_MAX), successful.primitiveDelayMax.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_DURATION_MAX),
              successful.pwlePrimitiveDurationMax.value());
    ASSERT_EQ(COMPOSITION_SIZE_MAX, successful.compositionSizeMax.value());
    ASSERT_EQ(PWLE_SIZE_MAX, successful.pwleSizeMax.value());
    ASSERT_EQ(F_MIN, successful.minFrequency.value());
    ASSERT_EQ(F0, successful.resonantFrequency.value());
    ASSERT_EQ(F_RESOLUTION, successful.frequencyResolution.value());
    ASSERT_EQ(Q_FACTOR, successful.qFactor.value());
    ASSERT_EQ(amplitudes, successful.maxAmplitudes.value());
    ASSERT_EQ(PWLE_V2_COMPOSITION_SIZE_MAX, successful.maxEnvelopeEffectSize.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS),
              successful.minEnvelopeEffectControlPointDuration.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS),
              successful.maxEnvelopeEffectControlPointDuration.value());
}

TEST_F(VibratorHalWrapperAidlTest, TestGetInfoCachesResult) {
    constexpr float F_MIN = 100.f;
    constexpr float F0 = 123.f;
    constexpr int32_t COMPOSITION_SIZE_MAX = 10;
    constexpr int32_t PWLE_SIZE_MAX = 20;
    constexpr int32_t PRIMITIVE_DELAY_MAX = 100;
    constexpr int32_t PWLE_DURATION_MAX = 200;
    constexpr int32_t PWLE_V2_COMPOSITION_SIZE_MAX = 16;
    constexpr int32_t PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS = 20;
    constexpr int32_t PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS = 1000;
    std::vector<Effect> supportedEffects = {Effect::CLICK, Effect::TICK};

    EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_ON_CALLBACK),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getSupportedEffects(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(supportedEffects), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getQFactor(_))
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
    EXPECT_CALL(*mMockHal.get(), getSupportedPrimitives(_))
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
    EXPECT_CALL(*mMockHal.get(), getCompositionSizeMax(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(COMPOSITION_SIZE_MAX),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getCompositionDelayMax(_))
            .Times(Exactly(1))
            .WillOnce(
                    DoAll(SetArgPointee<0>(PRIMITIVE_DELAY_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwlePrimitiveDurationMax(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_DURATION_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleCompositionSizeMax(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_SIZE_MAX), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getFrequencyMinimum(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(F_MIN), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getResonantFrequency(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(F0), Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getFrequencyResolution(_))
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
    EXPECT_CALL(*mMockHal.get(), getBandwidthAmplitudeMap(_))
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
    EXPECT_CALL(*mMockHal.get(), getSupportedBraking(_))
            .Times(Exactly(1))
            .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
    EXPECT_CALL(*mMockHal.get(), getPwleV2CompositionSizeMax(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_COMPOSITION_SIZE_MAX),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleV2PrimitiveDurationMinMillis(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS),
                            Return(ndk::ScopedAStatus::ok())));
    EXPECT_CALL(*mMockHal.get(), getPwleV2PrimitiveDurationMaxMillis(_))
            .Times(Exactly(1))
            .WillOnce(DoAll(SetArgPointee<0>(PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS),
                            Return(ndk::ScopedAStatus::ok())));

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.push_back(
                std::thread([&]() { ASSERT_TRUE(mWrapper->getInfo().capabilities.isOk()); }));
    }
    std::for_each(threads.begin(), threads.end(), [](std::thread& t) { t.join(); });

    vibrator::Info info = mWrapper->getInfo();
    ASSERT_EQ(vibrator::Capabilities::ON_CALLBACK, info.capabilities.value());
    ASSERT_EQ(supportedEffects, info.supportedEffects.value());
    ASSERT_TRUE(info.supportedBraking.isUnsupported());
    ASSERT_TRUE(info.supportedPrimitives.isUnsupported());
    ASSERT_TRUE(info.primitiveDurations.isUnsupported());
    ASSERT_EQ(std::chrono::milliseconds(PRIMITIVE_DELAY_MAX), info.primitiveDelayMax.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_DURATION_MAX), info.pwlePrimitiveDurationMax.value());
    ASSERT_EQ(COMPOSITION_SIZE_MAX, info.compositionSizeMax.value());
    ASSERT_EQ(PWLE_SIZE_MAX, info.pwleSizeMax.value());
    ASSERT_EQ(F_MIN, info.minFrequency.value());
    ASSERT_EQ(F0, info.resonantFrequency.value());
    ASSERT_TRUE(info.frequencyResolution.isUnsupported());
    ASSERT_TRUE(info.qFactor.isUnsupported());
    ASSERT_TRUE(info.maxAmplitudes.isUnsupported());
    ASSERT_EQ(PWLE_V2_COMPOSITION_SIZE_MAX, info.maxEnvelopeEffectSize.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_V2_MAX_ALLOWED_PRIMITIVE_MIN_DURATION_MS),
              info.minEnvelopeEffectControlPointDuration.value());
    ASSERT_EQ(std::chrono::milliseconds(PWLE_V2_MIN_REQUIRED_PRIMITIVE_MAX_DURATION_MS),
              info.maxEnvelopeEffectControlPointDuration.value());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformEffectWithCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_PERFORM_CALLBACK),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::CLICK), Eq(EffectStrength::LIGHT), _, _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<3>(1000), WithArg<2>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::POP), Eq(EffectStrength::MEDIUM), _, _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::THUD), Eq(EffectStrength::STRONG), _, _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performEffect(Effect::CLICK, EffectStrength::LIGHT, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(1000ms, result.value());
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performEffect(Effect::POP, EffectStrength::MEDIUM, callback);
    ASSERT_TRUE(result.isUnsupported());
    // Callback not triggered for unsupported
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performEffect(Effect::THUD, EffectStrength::STRONG, callback);
    ASSERT_TRUE(result.isFailed());
    // Callback not triggered on failure
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformEffectWithoutCallbackSupport) {
    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getCapabilities(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(IVibrator::CAP_ON_CALLBACK),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::CLICK), Eq(EffectStrength::LIGHT), _, _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<3>(10), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockScheduler.get(), schedule(_, Eq(10ms)))
                .Times(Exactly(1))
                .WillOnce(vibrator::TriggerSchedulerCallback());
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::POP), Eq(EffectStrength::MEDIUM), _, _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
        EXPECT_CALL(*mMockHal.get(), perform(Eq(Effect::THUD), Eq(EffectStrength::STRONG), _, _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performEffect(Effect::CLICK, EffectStrength::LIGHT, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(10ms, result.value());
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performEffect(Effect::POP, EffectStrength::MEDIUM, callback);
    ASSERT_TRUE(result.isUnsupported());

    result = mWrapper->performEffect(Effect::THUD, EffectStrength::STRONG, callback);
    ASSERT_TRUE(result.isFailed());

    // Callback not triggered for unsupported and on failure
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformVendorEffect) {
    PersistableBundle vendorData;
    vendorData.putInt("key", 1);
    VendorEffect vendorEffect;
    vendorEffect.vendorData = vendorData;
    vendorEffect.strength = EffectStrength::MEDIUM;
    vendorEffect.scale = 0.5f;

    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), performVendorEffect(_, _))
                .Times(Exactly(3))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performVendorEffect(vendorEffect, callback);
    ASSERT_TRUE(result.isUnsupported());
    // Callback not triggered on failure
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->performVendorEffect(vendorEffect, callback);
    ASSERT_TRUE(result.isFailed());
    // Callback not triggered for unsupported
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->performVendorEffect(vendorEffect, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformComposedEffect) {
    std::vector<CompositePrimitive> supportedPrimitives = {CompositePrimitive::CLICK,
                                                           CompositePrimitive::SPIN,
                                                           CompositePrimitive::THUD};
    std::vector<CompositeEffect> emptyEffects, singleEffect, multipleEffects;
    singleEffect.push_back(
            vibrator::TestFactory::createCompositeEffect(CompositePrimitive::CLICK, 10ms, 0.0f));
    multipleEffects.push_back(
            vibrator::TestFactory::createCompositeEffect(CompositePrimitive::SPIN, 100ms, 0.5f));
    multipleEffects.push_back(
            vibrator::TestFactory::createCompositeEffect(CompositePrimitive::THUD, 1000ms, 1.0f));

    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getSupportedPrimitives(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(supportedPrimitives),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::CLICK), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(1), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::SPIN), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(2), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::THUD), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(3), Return(ndk::ScopedAStatus::ok())));

        EXPECT_CALL(*mMockHal.get(), compose(Eq(emptyEffects), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), compose(Eq(singleEffect), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromStatus(STATUS_UNKNOWN_TRANSACTION)));
        EXPECT_CALL(*mMockHal.get(), compose(Eq(multipleEffects), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performComposedEffect(emptyEffects, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(0ms, result.value());
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performComposedEffect(singleEffect, callback);
    ASSERT_TRUE(result.isUnsupported());
    // Callback not triggered for unsupported
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performComposedEffect(multipleEffects, callback);
    ASSERT_TRUE(result.isFailed());
    // Callback not triggered on failure
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformComposedCachesPrimitiveDurationsAndIgnoresFailures) {
    std::vector<CompositePrimitive> supportedPrimitives = {CompositePrimitive::SPIN,
                                                           CompositePrimitive::THUD};
    std::vector<CompositeEffect> multipleEffects;
    multipleEffects.push_back(
            vibrator::TestFactory::createCompositeEffect(CompositePrimitive::SPIN, 10ms, 0.5f));
    multipleEffects.push_back(
            vibrator::TestFactory::createCompositeEffect(CompositePrimitive::THUD, 100ms, 1.0f));

    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), getSupportedPrimitives(_))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<0>(supportedPrimitives),
                                Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::SPIN), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(2), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::THUD), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)));
        EXPECT_CALL(*mMockHal.get(), compose(Eq(multipleEffects), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));

        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::SPIN), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(2), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), getPrimitiveDuration(Eq(CompositePrimitive::THUD), _))
                .Times(Exactly(1))
                .WillOnce(DoAll(SetArgPointee<1>(2), Return(ndk::ScopedAStatus::ok())));
        EXPECT_CALL(*mMockHal.get(), compose(Eq(multipleEffects), _))
                .Times(Exactly(2))
                // ndk::ScopedAStatus::ok() cannot be copy-constructed so can't use WillRepeatedly
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performComposedEffect(multipleEffects, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(112ms, result.value()); // Failed primitive durations counted as 1.
    ASSERT_EQ(1, *callbackCounter.get());

    result = mWrapper->performComposedEffect(multipleEffects, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(114ms, result.value()); // Second fetch succeeds and returns primitive duration.
    ASSERT_EQ(2, *callbackCounter.get());

    result = mWrapper->performComposedEffect(multipleEffects, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(114ms, result.value()); // Cached durations not fetched again, same duration returned.
    ASSERT_EQ(3, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestPerformPwleEffect) {
    std::vector<PrimitivePwle> emptyPrimitives, multiplePrimitives;
    multiplePrimitives.push_back(vibrator::TestFactory::createActivePwle(0, 1, 0, 1, 10ms));
    multiplePrimitives.push_back(vibrator::TestFactory::createBrakingPwle(Braking::NONE, 100ms));

    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), composePwle(Eq(emptyPrimitives), _))
                .Times(Exactly(1))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)));
        EXPECT_CALL(*mMockHal.get(), composePwle(Eq(multiplePrimitives), _))
                .Times(Exactly(2))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->performPwleEffect(emptyPrimitives, callback);
    ASSERT_TRUE(result.isUnsupported());
    // Callback not triggered on failure
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->performPwleEffect(multiplePrimitives, callback);
    ASSERT_TRUE(result.isFailed());
    // Callback not triggered for unsupported
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->performPwleEffect(multiplePrimitives, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(1, *callbackCounter.get());
}

TEST_F(VibratorHalWrapperAidlTest, TestComposePwleV2) {
    auto pwleEffect = {
            PwleV2Primitive(/*amplitude=*/0.2, /*frequency=*/50, /*time=*/100),
            PwleV2Primitive(/*amplitude=*/0.5, /*frequency=*/150, /*time=*/100),
            PwleV2Primitive(/*amplitude=*/0.8, /*frequency=*/250, /*time=*/100),
    };

    {
        InSequence seq;
        EXPECT_CALL(*mMockHal.get(), composePwleV2(_, _))
                .Times(Exactly(3))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION)))
                .WillOnce(Return(ndk::ScopedAStatus::fromExceptionCode(EX_SECURITY)))
                .WillOnce(DoAll(WithArg<1>(vibrator::TriggerCallback()),
                                Return(ndk::ScopedAStatus::ok())));
    }

    std::unique_ptr<int32_t> callbackCounter = std::make_unique<int32_t>();
    auto callback = vibrator::TestFactory::createCountingCallback(callbackCounter.get());

    auto result = mWrapper->composePwleV2(pwleEffect, callback);
    ASSERT_TRUE(result.isUnsupported());
    // Callback not triggered on failure
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->composePwleV2(pwleEffect, callback);
    ASSERT_TRUE(result.isFailed());
    // Callback not triggered for unsupported
    ASSERT_EQ(0, *callbackCounter.get());

    result = mWrapper->composePwleV2(pwleEffect, callback);
    ASSERT_TRUE(result.isOk());
    ASSERT_EQ(1, *callbackCounter.get());
}
