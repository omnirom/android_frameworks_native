/*
 * Copyright (C) 2020 The Android Open Source Project
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

#ifndef ANDROID_OS_VIBRATORHALWRAPPER_H
#define ANDROID_OS_VIBRATORHALWRAPPER_H

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <android-base/thread_annotations.h>
#include <android/binder_manager.h>
#include <android/hardware/vibrator/1.3/IVibrator.h>
#include <binder/IServiceManager.h>

#include <vibratorservice/VibratorCallbackScheduler.h>

namespace android {

namespace vibrator {

// -------------------------------------------------------------------------------------------------

// Base class to represent a generic result of a call to the Vibrator HAL wrapper.
class BaseHalResult {
public:
    bool isOk() const { return mStatus == SUCCESS; }
    bool isFailed() const { return mStatus == FAILED; }
    bool isUnsupported() const { return mStatus == UNSUPPORTED; }
    bool shouldRetry() const { return isFailed() && mDeadObject; }
    const char* errorMessage() const { return mErrorMessage.c_str(); }

protected:
    enum Status { SUCCESS, UNSUPPORTED, FAILED };
    Status mStatus;
    std::string mErrorMessage;
    bool mDeadObject;

    explicit BaseHalResult(Status status, const char* errorMessage = "", bool deadObject = false)
          : mStatus(status), mErrorMessage(errorMessage), mDeadObject(deadObject) {}
    virtual ~BaseHalResult() = default;
};

// Result of a call to the Vibrator HAL wrapper, holding data if successful.
template <typename T>
class HalResult : public BaseHalResult {
public:
    static HalResult<T> ok(T value) { return HalResult(value); }
    static HalResult<T> unsupported() { return HalResult(Status::UNSUPPORTED); }
    static HalResult<T> failed(const char* msg) { return HalResult(Status::FAILED, msg); }
    static HalResult<T> transactionFailed(const char* msg) {
        return HalResult(Status::FAILED, msg, /* deadObject= */ true);
    }

    // This will throw std::bad_optional_access if this result is not ok.
    const T& value() const { return mValue.value(); }
    const T valueOr(T&& defaultValue) const { return mValue.value_or(defaultValue); }

private:
    std::optional<T> mValue;

    explicit HalResult(T value)
          : BaseHalResult(Status::SUCCESS), mValue(std::make_optional(value)) {}
    explicit HalResult(Status status, const char* errorMessage = "", bool deadObject = false)
          : BaseHalResult(status, errorMessage, deadObject), mValue() {}
};

// Empty result of a call to the Vibrator HAL wrapper.
template <>
class HalResult<void> : public BaseHalResult {
public:
    static HalResult<void> ok() { return HalResult(Status::SUCCESS); }
    static HalResult<void> unsupported() { return HalResult(Status::UNSUPPORTED); }
    static HalResult<void> failed(const char* msg) { return HalResult(Status::FAILED, msg); }
    static HalResult<void> transactionFailed(const char* msg) {
        return HalResult(Status::FAILED, msg, /* deadObject= */ true);
    }

private:
    explicit HalResult(Status status, const char* errorMessage = "", bool deadObject = false)
          : BaseHalResult(status, errorMessage, deadObject) {}
};

// -------------------------------------------------------------------------------------------------

// Factory functions that convert failed HIDL/AIDL results into HalResult instances.
// Implementation of static template functions needs to be in this header file for the linker.
class HalResultFactory {
public:
    template <typename T>
    static HalResult<T> fromStatus(ndk::ScopedAStatus&& status, T data) {
        return status.isOk() ? HalResult<T>::ok(std::move(data))
                             : fromFailedStatus<T>(std::move(status));
    }

    template <typename T>
    static HalResult<T> fromStatus(hardware::vibrator::V1_0::Status&& status, T data) {
        return (status == hardware::vibrator::V1_0::Status::OK)
                ? HalResult<T>::ok(std::move(data))
                : fromFailedStatus<T>(std::move(status));
    }

    template <typename T, typename R>
    static HalResult<T> fromReturn(hardware::Return<R>&& ret, T data) {
        return ret.isOk() ? HalResult<T>::ok(std::move(data))
                          : fromFailedReturn<T, R>(std::move(ret));
    }

    template <typename T, typename R>
    static HalResult<T> fromReturn(hardware::Return<R>&& ret,
                                   hardware::vibrator::V1_0::Status status, T data) {
        return ret.isOk() ? fromStatus<T>(std::move(status), std::move(data))
                          : fromFailedReturn<T, R>(std::move(ret));
    }

    static HalResult<void> fromStatus(status_t status) {
        return (status == android::OK) ? HalResult<void>::ok()
                                       : fromFailedStatus<void>(std::move(status));
    }

    static HalResult<void> fromStatus(ndk::ScopedAStatus&& status) {
        return status.isOk() ? HalResult<void>::ok() : fromFailedStatus<void>(std::move(status));
    }

    static HalResult<void> fromStatus(hardware::vibrator::V1_0::Status&& status) {
        return (status == hardware::vibrator::V1_0::Status::OK)
                ? HalResult<void>::ok()
                : fromFailedStatus<void>(std::move(status));
    }

    template <typename R>
    static HalResult<void> fromReturn(hardware::Return<R>&& ret) {
        return ret.isOk() ? HalResult<void>::ok() : fromFailedReturn<void, R>(std::move(ret));
    }

private:
    template <typename T>
    static HalResult<T> fromFailedStatus(status_t status) {
        auto msg = "status_t = " + statusToString(status);
        return (status == android::DEAD_OBJECT) ? HalResult<T>::transactionFailed(msg.c_str())
                                                : HalResult<T>::failed(msg.c_str());
    }

    template <typename T>
    static HalResult<T> fromFailedStatus(ndk::ScopedAStatus&& status) {
        if (status.getExceptionCode() == EX_UNSUPPORTED_OPERATION ||
            status.getStatus() == STATUS_UNKNOWN_TRANSACTION) {
            // STATUS_UNKNOWN_TRANSACTION means the HAL implementation is an older version, so this
            // is the same as the operation being unsupported by this HAL. Should not retry.
            return HalResult<T>::unsupported();
        }
        if (status.getExceptionCode() == EX_TRANSACTION_FAILED) {
            return HalResult<T>::transactionFailed(status.getMessage());
        }
        return HalResult<T>::failed(status.getMessage());
    }

    template <typename T>
    static HalResult<T> fromFailedStatus(hardware::vibrator::V1_0::Status&& status) {
        switch (status) {
            case hardware::vibrator::V1_0::Status::UNSUPPORTED_OPERATION:
                return HalResult<T>::unsupported();
            default:
                auto msg = "android::hardware::vibrator::V1_0::Status = " + toString(status);
                return HalResult<T>::failed(msg.c_str());
        }
    }

    template <typename T, typename R>
    static HalResult<T> fromFailedReturn(hardware::Return<R>&& ret) {
        return ret.isDeadObject() ? HalResult<T>::transactionFailed(ret.description().c_str())
                                  : HalResult<T>::failed(ret.description().c_str());
    }
};

// -------------------------------------------------------------------------------------------------

class HalCallbackWrapper : public aidl::android::hardware::vibrator::BnVibratorCallback {
public:
    HalCallbackWrapper(std::function<void()> completionCallback)
          : mCompletionCallback(completionCallback) {}

    ndk::ScopedAStatus onComplete() override {
        mCompletionCallback();
        return ndk::ScopedAStatus::ok();
    }

private:
    const std::function<void()> mCompletionCallback;
};

// -------------------------------------------------------------------------------------------------

// Vibrator HAL capabilities.
enum class Capabilities : int32_t {
    NONE = 0,
    ON_CALLBACK = aidl::android::hardware::vibrator::IVibrator::CAP_ON_CALLBACK,
    PERFORM_CALLBACK = aidl::android::hardware::vibrator::IVibrator::CAP_PERFORM_CALLBACK,
    AMPLITUDE_CONTROL = aidl::android::hardware::vibrator::IVibrator::CAP_AMPLITUDE_CONTROL,
    EXTERNAL_CONTROL = aidl::android::hardware::vibrator::IVibrator::CAP_EXTERNAL_CONTROL,
    EXTERNAL_AMPLITUDE_CONTROL =
            aidl::android::hardware::vibrator::IVibrator::CAP_EXTERNAL_AMPLITUDE_CONTROL,
    COMPOSE_EFFECTS = aidl::android::hardware::vibrator::IVibrator::CAP_COMPOSE_EFFECTS,
    COMPOSE_PWLE_EFFECTS = aidl::android::hardware::vibrator::IVibrator::CAP_COMPOSE_PWLE_EFFECTS,
    ALWAYS_ON_CONTROL = aidl::android::hardware::vibrator::IVibrator::CAP_ALWAYS_ON_CONTROL,
};

inline Capabilities operator|(Capabilities lhs, Capabilities rhs) {
    using underlying = typename std::underlying_type<Capabilities>::type;
    return static_cast<Capabilities>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
}

inline Capabilities& operator|=(Capabilities& lhs, Capabilities rhs) {
    return lhs = lhs | rhs;
}

inline Capabilities operator&(Capabilities lhs, Capabilities rhs) {
    using underlying = typename std::underlying_type<Capabilities>::type;
    return static_cast<Capabilities>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
}

inline Capabilities& operator&=(Capabilities& lhs, Capabilities rhs) {
    return lhs = lhs & rhs;
}

// -------------------------------------------------------------------------------------------------

class Info {
public:
    using Effect = aidl::android::hardware::vibrator::Effect;
    using EffectStrength = aidl::android::hardware::vibrator::EffectStrength;
    using CompositePrimitive = aidl::android::hardware::vibrator::CompositePrimitive;
    using Braking = aidl::android::hardware::vibrator::Braking;

    const HalResult<Capabilities> capabilities;
    const HalResult<std::vector<Effect>> supportedEffects;
    const HalResult<std::vector<Braking>> supportedBraking;
    const HalResult<std::vector<CompositePrimitive>> supportedPrimitives;
    const HalResult<std::vector<std::chrono::milliseconds>> primitiveDurations;
    const HalResult<std::chrono::milliseconds> primitiveDelayMax;
    const HalResult<std::chrono::milliseconds> pwlePrimitiveDurationMax;
    const HalResult<int32_t> compositionSizeMax;
    const HalResult<int32_t> pwleSizeMax;
    const HalResult<float> minFrequency;
    const HalResult<float> resonantFrequency;
    const HalResult<float> frequencyResolution;
    const HalResult<float> qFactor;
    const HalResult<std::vector<float>> maxAmplitudes;
    const HalResult<int32_t> maxEnvelopeEffectSize;
    const HalResult<std::chrono::milliseconds> minEnvelopeEffectControlPointDuration;
    const HalResult<std::chrono::milliseconds> maxEnvelopeEffectControlPointDuration;

    void logFailures() const {
        logFailure<Capabilities>(capabilities, "getCapabilities");
        logFailure<std::vector<Effect>>(supportedEffects, "getSupportedEffects");
        logFailure<std::vector<Braking>>(supportedBraking, "getSupportedBraking");
        logFailure<std::vector<CompositePrimitive>>(supportedPrimitives, "getSupportedPrimitives");
        logFailure<std::vector<std::chrono::milliseconds>>(primitiveDurations,
                                                           "getPrimitiveDuration");
        logFailure<std::chrono::milliseconds>(primitiveDelayMax, "getPrimitiveDelayMax");
        logFailure<std::chrono::milliseconds>(pwlePrimitiveDurationMax,
                                              "getPwlePrimitiveDurationMax");
        logFailure<int32_t>(compositionSizeMax, "getCompositionSizeMax");
        logFailure<int32_t>(pwleSizeMax, "getPwleSizeMax");
        logFailure<float>(minFrequency, "getMinFrequency");
        logFailure<float>(resonantFrequency, "getResonantFrequency");
        logFailure<float>(frequencyResolution, "getFrequencyResolution");
        logFailure<float>(qFactor, "getQFactor");
        logFailure<std::vector<float>>(maxAmplitudes, "getMaxAmplitudes");
        logFailure<int32_t>(maxEnvelopeEffectSize, "getMaxEnvelopeEffectSize");
        logFailure<std::chrono::milliseconds>(minEnvelopeEffectControlPointDuration,
                                              "getMinEnvelopeEffectControlPointDuration");
        logFailure<std::chrono::milliseconds>(maxEnvelopeEffectControlPointDuration,
                                              "getMaxEnvelopeEffectControlPointDuration");
    }

    bool shouldRetry() const {
        return capabilities.shouldRetry() || supportedEffects.shouldRetry() ||
                supportedBraking.shouldRetry() || supportedPrimitives.shouldRetry() ||
                primitiveDurations.shouldRetry() || primitiveDelayMax.shouldRetry() ||
                pwlePrimitiveDurationMax.shouldRetry() || compositionSizeMax.shouldRetry() ||
                pwleSizeMax.shouldRetry() || minFrequency.shouldRetry() ||
                resonantFrequency.shouldRetry() || frequencyResolution.shouldRetry() ||
                qFactor.shouldRetry() || maxAmplitudes.shouldRetry() ||
                maxEnvelopeEffectSize.shouldRetry() ||
                minEnvelopeEffectControlPointDuration.shouldRetry() ||
                maxEnvelopeEffectControlPointDuration.shouldRetry();
    }

private:
    template <typename T>
    void logFailure(HalResult<T> result, const char* functionName) const {
        if (result.isFailed()) {
            ALOGE("Vibrator HAL %s failed: %s", functionName, result.errorMessage());
        }
    }
};

class InfoCache {
public:
    Info get() {
        return {mCapabilities,
                mSupportedEffects,
                mSupportedBraking,
                mSupportedPrimitives,
                mPrimitiveDurations,
                mPrimitiveDelayMax,
                mPwlePrimitiveDurationMax,
                mCompositionSizeMax,
                mPwleSizeMax,
                mMinFrequency,
                mResonantFrequency,
                mFrequencyResolution,
                mQFactor,
                mMaxAmplitudes,
                mMaxEnvelopeEffectSize,
                mMinEnvelopeEffectControlPointDuration,
                mMaxEnvelopeEffectControlPointDuration};
    }

private:
    // Create a transaction failed results as default so we can retry on the first time we get them.
    static const constexpr char* MSG = "never loaded";
    HalResult<Capabilities> mCapabilities = HalResult<Capabilities>::transactionFailed(MSG);
    HalResult<std::vector<Info::Effect>> mSupportedEffects =
            HalResult<std::vector<Info::Effect>>::transactionFailed(MSG);
    HalResult<std::vector<Info::Braking>> mSupportedBraking =
            HalResult<std::vector<Info::Braking>>::transactionFailed(MSG);
    HalResult<std::vector<Info::CompositePrimitive>> mSupportedPrimitives =
            HalResult<std::vector<Info::CompositePrimitive>>::transactionFailed(MSG);
    HalResult<std::vector<std::chrono::milliseconds>> mPrimitiveDurations =
            HalResult<std::vector<std::chrono::milliseconds>>::transactionFailed(MSG);
    HalResult<std::chrono::milliseconds> mPrimitiveDelayMax =
            HalResult<std::chrono::milliseconds>::transactionFailed(MSG);
    HalResult<std::chrono::milliseconds> mPwlePrimitiveDurationMax =
            HalResult<std::chrono::milliseconds>::transactionFailed(MSG);
    HalResult<int32_t> mCompositionSizeMax = HalResult<int>::transactionFailed(MSG);
    HalResult<int32_t> mPwleSizeMax = HalResult<int>::transactionFailed(MSG);
    HalResult<float> mMinFrequency = HalResult<float>::transactionFailed(MSG);
    HalResult<float> mResonantFrequency = HalResult<float>::transactionFailed(MSG);
    HalResult<float> mFrequencyResolution = HalResult<float>::transactionFailed(MSG);
    HalResult<float> mQFactor = HalResult<float>::transactionFailed(MSG);
    HalResult<std::vector<float>> mMaxAmplitudes =
            HalResult<std::vector<float>>::transactionFailed(MSG);
    HalResult<int32_t> mMaxEnvelopeEffectSize = HalResult<int>::transactionFailed(MSG);
    HalResult<std::chrono::milliseconds> mMinEnvelopeEffectControlPointDuration =
            HalResult<std::chrono::milliseconds>::transactionFailed(MSG);
    HalResult<std::chrono::milliseconds> mMaxEnvelopeEffectControlPointDuration =
            HalResult<std::chrono::milliseconds>::transactionFailed(MSG);

    friend class HalWrapper;
};

// Wrapper for Vibrator HAL handlers.
class HalWrapper {
public:
    using Effect = aidl::android::hardware::vibrator::Effect;
    using EffectStrength = aidl::android::hardware::vibrator::EffectStrength;
    using VendorEffect = aidl::android::hardware::vibrator::VendorEffect;
    using CompositePrimitive = aidl::android::hardware::vibrator::CompositePrimitive;
    using CompositeEffect = aidl::android::hardware::vibrator::CompositeEffect;
    using Braking = aidl::android::hardware::vibrator::Braking;
    using PrimitivePwle = aidl::android::hardware::vibrator::PrimitivePwle;
    using PwleV2Primitive = aidl::android::hardware::vibrator::PwleV2Primitive;
    using PwleV2OutputMapEntry = aidl::android::hardware::vibrator::PwleV2OutputMapEntry;

    explicit HalWrapper(std::shared_ptr<CallbackScheduler> scheduler)
          : mCallbackScheduler(std::move(scheduler)) {}
    virtual ~HalWrapper() = default;

    /* reloads wrapped HAL service instance without waiting. This can be used to reconnect when the
     * service restarts, to rapidly retry after a failure.
     */
    virtual void tryReconnect() = 0;

    Info getInfo();

    virtual HalResult<void> ping() = 0;
    virtual HalResult<void> on(std::chrono::milliseconds timeout,
                               const std::function<void()>& completionCallback) = 0;
    virtual HalResult<void> off() = 0;

    virtual HalResult<void> setAmplitude(float amplitude) = 0;
    virtual HalResult<void> setExternalControl(bool enabled) = 0;

    virtual HalResult<void> alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) = 0;
    virtual HalResult<void> alwaysOnDisable(int32_t id) = 0;

    virtual HalResult<std::chrono::milliseconds> performEffect(
            Effect effect, EffectStrength strength,
            const std::function<void()>& completionCallback) = 0;

    virtual HalResult<void> performVendorEffect(const VendorEffect& effect,
                                                const std::function<void()>& completionCallback);

    virtual HalResult<std::chrono::milliseconds> performComposedEffect(
            const std::vector<CompositeEffect>& primitives,
            const std::function<void()>& completionCallback);

    virtual HalResult<void> performPwleEffect(const std::vector<PrimitivePwle>& primitives,
                                              const std::function<void()>& completionCallback);

    virtual HalResult<void> composePwleV2(const std::vector<PwleV2Primitive>& composite,
                                          const std::function<void()>& completionCallback);

protected:
    // Shared pointer to allow CallbackScheduler to outlive this wrapper.
    const std::shared_ptr<CallbackScheduler> mCallbackScheduler;

    // Load and cache vibrator info, returning cached result is present.
    HalResult<Capabilities> getCapabilities();
    HalResult<std::vector<std::chrono::milliseconds>> getPrimitiveDurations();

    // Request vibrator info to HAL skipping cache.
    virtual HalResult<Capabilities> getCapabilitiesInternal() = 0;
    virtual HalResult<std::vector<Effect>> getSupportedEffectsInternal();
    virtual HalResult<std::vector<Braking>> getSupportedBrakingInternal();
    virtual HalResult<std::vector<CompositePrimitive>> getSupportedPrimitivesInternal();
    virtual HalResult<std::vector<std::chrono::milliseconds>> getPrimitiveDurationsInternal(
            const std::vector<CompositePrimitive>& supportedPrimitives);
    virtual HalResult<std::chrono::milliseconds> getPrimitiveDelayMaxInternal();
    virtual HalResult<std::chrono::milliseconds> getPrimitiveDurationMaxInternal();
    virtual HalResult<int32_t> getCompositionSizeMaxInternal();
    virtual HalResult<int32_t> getPwleSizeMaxInternal();
    virtual HalResult<float> getMinFrequencyInternal();
    virtual HalResult<float> getResonantFrequencyInternal();
    virtual HalResult<float> getFrequencyResolutionInternal();
    virtual HalResult<float> getQFactorInternal();
    virtual HalResult<std::vector<float>> getMaxAmplitudesInternal();
    virtual HalResult<int32_t> getMaxEnvelopeEffectSizeInternal();
    virtual HalResult<std::chrono::milliseconds> getMinEnvelopeEffectControlPointDurationInternal();
    virtual HalResult<std::chrono::milliseconds> getMaxEnvelopeEffectControlPointDurationInternal();

private:
    std::mutex mInfoMutex;
    InfoCache mInfoCache GUARDED_BY(mInfoMutex);
};

// Wrapper for the AIDL Vibrator HAL.
class AidlHalWrapper : public HalWrapper {
public:
    using IVibrator = aidl::android::hardware::vibrator::IVibrator;
    using reconnect_fn = std::function<HalResult<std::shared_ptr<IVibrator>>()>;

    AidlHalWrapper(
            std::shared_ptr<CallbackScheduler> scheduler, std::shared_ptr<IVibrator> handle,
            reconnect_fn reconnectFn =
                    []() {
                        auto serviceName = std::string(IVibrator::descriptor) + "/default";
                        auto hal = IVibrator::fromBinder(
                                ndk::SpAIBinder(AServiceManager_checkService(serviceName.c_str())));
                        return HalResult<std::shared_ptr<IVibrator>>::ok(std::move(hal));
                    })
          : HalWrapper(std::move(scheduler)),
            mReconnectFn(reconnectFn),
            mHandle(std::move(handle)) {}
    virtual ~AidlHalWrapper() = default;

    HalResult<void> ping() override final;
    void tryReconnect() override final;

    HalResult<void> on(std::chrono::milliseconds timeout,
                       const std::function<void()>& completionCallback) override final;
    HalResult<void> off() override final;

    HalResult<void> setAmplitude(float amplitude) override final;
    HalResult<void> setExternalControl(bool enabled) override final;

    HalResult<void> alwaysOnEnable(int32_t id, Effect effect,
                                   EffectStrength strength) override final;
    HalResult<void> alwaysOnDisable(int32_t id) override final;

    HalResult<std::chrono::milliseconds> performEffect(
            Effect effect, EffectStrength strength,
            const std::function<void()>& completionCallback) override final;

    HalResult<void> performVendorEffect(
            const VendorEffect& effect,
            const std::function<void()>& completionCallback) override final;

    HalResult<std::chrono::milliseconds> performComposedEffect(
            const std::vector<CompositeEffect>& primitives,
            const std::function<void()>& completionCallback) override final;

    HalResult<void> performPwleEffect(
            const std::vector<PrimitivePwle>& primitives,
            const std::function<void()>& completionCallback) override final;

    HalResult<void> composePwleV2(const std::vector<PwleV2Primitive>& composite,
                                  const std::function<void()>& completionCallback) override final;

protected:
    HalResult<Capabilities> getCapabilitiesInternal() override final;
    HalResult<std::vector<Effect>> getSupportedEffectsInternal() override final;
    HalResult<std::vector<Braking>> getSupportedBrakingInternal() override final;
    HalResult<std::vector<CompositePrimitive>> getSupportedPrimitivesInternal() override final;
    HalResult<std::vector<std::chrono::milliseconds>> getPrimitiveDurationsInternal(
            const std::vector<CompositePrimitive>& supportedPrimitives) override final;
    HalResult<std::chrono::milliseconds> getPrimitiveDelayMaxInternal() override final;
    HalResult<std::chrono::milliseconds> getPrimitiveDurationMaxInternal() override final;
    HalResult<int32_t> getCompositionSizeMaxInternal() override final;
    HalResult<int32_t> getPwleSizeMaxInternal() override final;
    HalResult<float> getMinFrequencyInternal() override final;
    HalResult<float> getResonantFrequencyInternal() override final;
    HalResult<float> getFrequencyResolutionInternal() override final;
    HalResult<float> getQFactorInternal() override final;
    HalResult<std::vector<float>> getMaxAmplitudesInternal() override final;
    HalResult<int32_t> getMaxEnvelopeEffectSizeInternal() override final;

    HalResult<std::chrono::milliseconds> getMinEnvelopeEffectControlPointDurationInternal()
            override final;

    HalResult<std::chrono::milliseconds> getMaxEnvelopeEffectControlPointDurationInternal()
            override final;

private:
    const reconnect_fn mReconnectFn;
    std::mutex mHandleMutex;
    std::shared_ptr<IVibrator> mHandle GUARDED_BY(mHandleMutex);

    std::shared_ptr<IVibrator> getHal();
};

// Wrapper for the HDIL Vibrator HALs.
template <typename I>
class HidlHalWrapper : public HalWrapper {
public:
    HidlHalWrapper(std::shared_ptr<CallbackScheduler> scheduler, sp<I> handle)
          : HalWrapper(std::move(scheduler)), mHandle(std::move(handle)) {}
    virtual ~HidlHalWrapper() = default;

    HalResult<void> ping() override final;
    void tryReconnect() override final;

    HalResult<void> on(std::chrono::milliseconds timeout,
                       const std::function<void()>& completionCallback) override final;
    HalResult<void> off() override final;

    HalResult<void> setAmplitude(float amplitude) override final;
    virtual HalResult<void> setExternalControl(bool enabled) override;

    HalResult<void> alwaysOnEnable(int32_t id, HalWrapper::Effect effect,
                                   HalWrapper::EffectStrength strength) override final;
    HalResult<void> alwaysOnDisable(int32_t id) override final;

protected:
    std::mutex mHandleMutex;
    sp<I> mHandle GUARDED_BY(mHandleMutex);

    virtual HalResult<Capabilities> getCapabilitiesInternal() override;

    template <class T>
    using perform_fn =
            hardware::Return<void> (I::*)(T, hardware::vibrator::V1_0::EffectStrength,
                                          hardware::vibrator::V1_0::IVibrator::perform_cb);

    template <class T>
    HalResult<std::chrono::milliseconds> performInternal(
            perform_fn<T> performFn, sp<I> handle, T effect, HalWrapper::EffectStrength strength,
            const std::function<void()>& completionCallback);

    sp<I> getHal();
};

// Wrapper for the HDIL Vibrator HAL v1.0.
class HidlHalWrapperV1_0 : public HidlHalWrapper<hardware::vibrator::V1_0::IVibrator> {
public:
    HidlHalWrapperV1_0(std::shared_ptr<CallbackScheduler> scheduler,
                       sp<hardware::vibrator::V1_0::IVibrator> handle)
          : HidlHalWrapper<hardware::vibrator::V1_0::IVibrator>(std::move(scheduler),
                                                                std::move(handle)) {}
    virtual ~HidlHalWrapperV1_0() = default;

    HalResult<std::chrono::milliseconds> performEffect(
            HalWrapper::Effect effect, HalWrapper::EffectStrength strength,
            const std::function<void()>& completionCallback) override final;
};

// Wrapper for the HDIL Vibrator HAL v1.1.
class HidlHalWrapperV1_1 : public HidlHalWrapper<hardware::vibrator::V1_1::IVibrator> {
public:
    HidlHalWrapperV1_1(std::shared_ptr<CallbackScheduler> scheduler,
                       sp<hardware::vibrator::V1_1::IVibrator> handle)
          : HidlHalWrapper<hardware::vibrator::V1_1::IVibrator>(std::move(scheduler),
                                                                std::move(handle)) {}
    virtual ~HidlHalWrapperV1_1() = default;

    HalResult<std::chrono::milliseconds> performEffect(
            HalWrapper::Effect effect, HalWrapper::EffectStrength strength,
            const std::function<void()>& completionCallback) override final;
};

// Wrapper for the HDIL Vibrator HAL v1.2.
class HidlHalWrapperV1_2 : public HidlHalWrapper<hardware::vibrator::V1_2::IVibrator> {
public:
    HidlHalWrapperV1_2(std::shared_ptr<CallbackScheduler> scheduler,
                       sp<hardware::vibrator::V1_2::IVibrator> handle)
          : HidlHalWrapper<hardware::vibrator::V1_2::IVibrator>(std::move(scheduler),
                                                                std::move(handle)) {}
    virtual ~HidlHalWrapperV1_2() = default;

    HalResult<std::chrono::milliseconds> performEffect(
            HalWrapper::Effect effect, HalWrapper::EffectStrength strength,
            const std::function<void()>& completionCallback) override final;
};

// Wrapper for the HDIL Vibrator HAL v1.3.
class HidlHalWrapperV1_3 : public HidlHalWrapper<hardware::vibrator::V1_3::IVibrator> {
public:
    HidlHalWrapperV1_3(std::shared_ptr<CallbackScheduler> scheduler,
                       sp<hardware::vibrator::V1_3::IVibrator> handle)
          : HidlHalWrapper<hardware::vibrator::V1_3::IVibrator>(std::move(scheduler),
                                                                std::move(handle)) {}
    virtual ~HidlHalWrapperV1_3() = default;

    HalResult<void> setExternalControl(bool enabled) override final;

    HalResult<std::chrono::milliseconds> performEffect(
            HalWrapper::Effect effect, HalWrapper::EffectStrength strength,
            const std::function<void()>& completionCallback) override final;

protected:
    HalResult<Capabilities> getCapabilitiesInternal() override final;
};

// -------------------------------------------------------------------------------------------------

}; // namespace vibrator

}; // namespace android

#endif // ANDROID_OS_VIBRATORHALWRAPPER_H
