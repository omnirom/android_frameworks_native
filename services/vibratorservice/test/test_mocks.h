/*
 * Copyright (C) 2024 The Android Open Source Project
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

#ifndef VIBRATORSERVICE_UNITTEST_MOCKS_H_
#define VIBRATORSERVICE_UNITTEST_MOCKS_H_

#include <gmock/gmock.h>

#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <vibratorservice/VibratorCallbackScheduler.h>
#include <vibratorservice/VibratorHalController.h>
#include <vibratorservice/VibratorHalWrapper.h>

namespace android {

namespace vibrator {

using std::chrono::milliseconds;

using namespace testing;

using aidl::android::hardware::vibrator::Braking;
using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;
using aidl::android::hardware::vibrator::IVibratorCallback;
using aidl::android::hardware::vibrator::PrimitivePwle;
using aidl::android::hardware::vibrator::PwleV2OutputMapEntry;
using aidl::android::hardware::vibrator::PwleV2Primitive;
using aidl::android::hardware::vibrator::VendorEffect;

// -------------------------------------------------------------------------------------------------

class MockIVibrator : public IVibrator {
public:
    MockIVibrator() = default;

    MOCK_METHOD(ndk::ScopedAStatus, getCapabilities, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, off, (), (override));
    MOCK_METHOD(ndk::ScopedAStatus, on,
                (int32_t timeout, const std::shared_ptr<IVibratorCallback>& cb), (override));
    MOCK_METHOD(ndk::ScopedAStatus, perform,
                (Effect e, EffectStrength s, const std::shared_ptr<IVibratorCallback>& cb,
                 int32_t* ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, performVendorEffect,
                (const VendorEffect& e, const std::shared_ptr<IVibratorCallback>& cb), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedEffects, (std::vector<Effect> * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, setAmplitude, (float amplitude), (override));
    MOCK_METHOD(ndk::ScopedAStatus, setExternalControl, (bool enabled), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getCompositionDelayMax, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getCompositionSizeMax, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedPrimitives, (std::vector<CompositePrimitive> * ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPrimitiveDuration, (CompositePrimitive p, int32_t* ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, compose,
                (const std::vector<CompositeEffect>& e,
                 const std::shared_ptr<IVibratorCallback>& cb),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, composePwle,
                (const std::vector<PrimitivePwle>& e, const std::shared_ptr<IVibratorCallback>& cb),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedAlwaysOnEffects, (std::vector<Effect> * ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, alwaysOnEnable, (int32_t id, Effect e, EffectStrength s),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, alwaysOnDisable, (int32_t id), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getQFactor, (float* ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getResonantFrequency, (float* ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getFrequencyResolution, (float* ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getFrequencyMinimum, (float* ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getBandwidthAmplitudeMap, (std::vector<float> * ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwlePrimitiveDurationMax, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwleCompositionSizeMax, (int32_t * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getSupportedBraking, (std::vector<Braking> * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwleV2FrequencyToOutputAccelerationMap,
                (std::vector<PwleV2OutputMapEntry> * ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwleV2PrimitiveDurationMaxMillis, (int32_t* ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwleV2PrimitiveDurationMinMillis, (int32_t* ret),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getPwleV2CompositionSizeMax, (int32_t* ret), (override));
    MOCK_METHOD(ndk::ScopedAStatus, composePwleV2,
                (const std::vector<PwleV2Primitive>& e,
                 const std::shared_ptr<IVibratorCallback>& cb),
                (override));
    MOCK_METHOD(ndk::ScopedAStatus, getInterfaceVersion, (int32_t*), (override));
    MOCK_METHOD(ndk::ScopedAStatus, getInterfaceHash, (std::string*), (override));
    MOCK_METHOD(ndk::SpAIBinder, asBinder, (), (override));
    MOCK_METHOD(bool, isRemote, (), (override));
};

// gmock requirement to provide a WithArg<0>(TriggerCallback()) matcher
typedef void TriggerCallbackFunction(const std::shared_ptr<IVibratorCallback>&);

class TriggerCallbackAction : public ActionInterface<TriggerCallbackFunction> {
public:
    explicit TriggerCallbackAction() {}

    virtual Result Perform(const ArgumentTuple& args) {
        const std::shared_ptr<IVibratorCallback>& callback = get<0>(args);
        if (callback) {
            callback->onComplete();
        }
    }
};

inline Action<TriggerCallbackFunction> TriggerCallback() {
    return MakeAction(new TriggerCallbackAction());
}

// -------------------------------------------------------------------------------------------------

class MockCallbackScheduler : public CallbackScheduler {
public:
    MOCK_METHOD(void, schedule, (std::function<void()> callback, std::chrono::milliseconds delay),
                (override));
};

ACTION(TriggerSchedulerCallback) {
    arg0();
}

// -------------------------------------------------------------------------------------------------

class MockHalWrapper : public HalWrapper {
public:
    MockHalWrapper(std::shared_ptr<CallbackScheduler> scheduler) : HalWrapper(scheduler) {}
    virtual ~MockHalWrapper() = default;

    MOCK_METHOD(vibrator::HalResult<void>, ping, (), (override));
    MOCK_METHOD(void, tryReconnect, (), (override));
    MOCK_METHOD(vibrator::HalResult<void>, on,
                (milliseconds timeout, const std::function<void()>& completionCallback),
                (override));
    MOCK_METHOD(vibrator::HalResult<void>, off, (), (override));
    MOCK_METHOD(vibrator::HalResult<void>, setAmplitude, (float amplitude), (override));
    MOCK_METHOD(vibrator::HalResult<void>, setExternalControl, (bool enabled), (override));
    MOCK_METHOD(vibrator::HalResult<void>, alwaysOnEnable,
                (int32_t id, Effect effect, EffectStrength strength), (override));
    MOCK_METHOD(vibrator::HalResult<void>, alwaysOnDisable, (int32_t id), (override));
    MOCK_METHOD(vibrator::HalResult<milliseconds>, performEffect,
                (Effect effect, EffectStrength strength,
                 const std::function<void()>& completionCallback),
                (override));
    MOCK_METHOD(vibrator::HalResult<vibrator::Capabilities>, getCapabilitiesInternal, (),
                (override));

    CallbackScheduler* getCallbackScheduler() { return mCallbackScheduler.get(); }
};

class MockHalController : public vibrator::HalController {
public:
    MockHalController() = default;
    virtual ~MockHalController() = default;

    MOCK_METHOD(bool, init, (), (override));
    MOCK_METHOD(void, tryReconnect, (), (override));
};

// -------------------------------------------------------------------------------------------------

} // namespace vibrator

} // namespace android

#endif // VIBRATORSERVICE_UNITTEST_MOCKS_H_
