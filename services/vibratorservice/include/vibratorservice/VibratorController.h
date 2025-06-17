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

#ifndef ANDROID_OS_VIBRATOR_CONTROLLER_H
#define ANDROID_OS_VIBRATOR_CONTROLLER_H

#include <aidl/android/hardware/vibrator/IVibrator.h>

#include <android-base/thread_annotations.h>

namespace android {

namespace vibrator {

// -------------------------------------------------------------------------------------------------

/* Provider for IVibrator HAL service instances. */
class VibratorProvider {
public:
    using IVibrator = ::aidl::android::hardware::vibrator::IVibrator;

    VibratorProvider() : mServiceName(std::string(IVibrator::descriptor) + "/default") {}
    virtual ~VibratorProvider() = default;

    /* Returns true if vibrator HAL service is declared in the device, false otherwise. */
    virtual bool isDeclared();

    /* Connects to vibrator HAL, possibly waiting for the declared service to become available. */
    virtual std::shared_ptr<IVibrator> waitForVibrator();

    /* Connects to vibrator HAL if declared and available, without waiting. */
    virtual std::shared_ptr<IVibrator> checkForVibrator();

private:
    std::mutex mMutex;
    const std::string mServiceName;
    std::optional<bool> mIsDeclared GUARDED_BY(mMutex);
};

// -------------------------------------------------------------------------------------------------

/* Controller for Vibrator HAL handle.
 * This relies on VibratorProvider to connect to the underlying Vibrator HAL service and reconnects
 * after each transaction failed call. This also ensures connecting to the service is thread-safe.
 */
class VibratorController {
public:
    using Effect = ::aidl::android::hardware::vibrator::Effect;
    using EffectStrength = ::aidl::android::hardware::vibrator::EffectStrength;
    using IVibrator = ::aidl::android::hardware::vibrator::IVibrator;
    using Status = ::ndk::ScopedAStatus;
    using VibratorOp = std::function<Status(IVibrator*)>;

    VibratorController() : VibratorController(std::make_shared<VibratorProvider>()) {}
    VibratorController(std::shared_ptr<VibratorProvider> vibratorProvider)
          : mVibratorProvider(std::move(vibratorProvider)), mVibrator(nullptr) {}
    virtual ~VibratorController() = default;

    /* Connects HAL service, possibly waiting for the declared service to become available.
     * This will automatically be called at the first API usage if it was not manually called
     * beforehand. Call this manually during the setup phase to avoid slowing the first API call.
     * Returns true if HAL service is declared, false otherwise.
     */
    bool init();

    /* Turn vibrator off. */
    Status off();

    /* Set vibration amplitude in [0,1]. */
    Status setAmplitude(float amplitude);

    /* Enable/disable external control. */
    Status setExternalControl(bool enabled);

    /* Enable always-on for given id, with given effect and strength. */
    Status alwaysOnEnable(int32_t id, const Effect& effect, const EffectStrength& strength);

    /* Disable always-on for given id. */
    Status alwaysOnDisable(int32_t id);

private:
    /* Max number of attempts to perform an operation when it fails with transaction error. */
    static constexpr int MAX_ATTEMPTS = 2;

    std::mutex mMutex;
    std::shared_ptr<VibratorProvider> mVibratorProvider;
    std::shared_ptr<IVibrator> mVibrator GUARDED_BY(mMutex);

    /* Reconnects HAL service without waiting for the service to become available. */
    std::shared_ptr<IVibrator> reconnectToVibrator();

    /* Perform given operation on HAL with retries on transaction failures. */
    Status doWithRetries(const VibratorOp& op, const char* logLabel);

    /* Perform given operation on HAL with logs for error/unsupported results. */
    static Status doOnce(IVibrator* vibrator, const VibratorOp& op, const char* logLabel);
};

// -------------------------------------------------------------------------------------------------

}; // namespace vibrator

}; // namespace android

#endif // ANDROID_OS_VIBRATOR_CONTROLLER_H
