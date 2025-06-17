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

#define LOG_TAG "VibratorHalController"

#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <android/binder_manager.h>

#include <utils/Log.h>

#include <vibratorservice/VibratorCallbackScheduler.h>
#include <vibratorservice/VibratorHalController.h>
#include <vibratorservice/VibratorHalWrapper.h>

using aidl::android::hardware::vibrator::CompositeEffect;
using aidl::android::hardware::vibrator::CompositePrimitive;
using aidl::android::hardware::vibrator::Effect;
using aidl::android::hardware::vibrator::EffectStrength;
using aidl::android::hardware::vibrator::IVibrator;

using std::chrono::milliseconds;

namespace android {

namespace vibrator {

// -------------------------------------------------------------------------------------------------

std::shared_ptr<HalWrapper> connectHal(std::shared_ptr<CallbackScheduler> scheduler) {
    static bool gHalExists = true;
    if (!gHalExists) {
        // We already tried to connect to all of the vibrator HAL versions and none was available.
        return nullptr;
    }

    auto serviceName = std::string(IVibrator::descriptor) + "/default";
    if (AServiceManager_isDeclared(serviceName.c_str())) {
        std::shared_ptr<IVibrator> hal = IVibrator::fromBinder(
                ndk::SpAIBinder(AServiceManager_waitForService(serviceName.c_str())));
        if (hal) {
            ALOGV("Successfully connected to Vibrator HAL AIDL service.");
            return std::make_shared<AidlHalWrapper>(std::move(scheduler), std::move(hal));
        }
    }

    ALOGV("Vibrator HAL service not available.");
    gHalExists = false;
    return nullptr;
}

// -------------------------------------------------------------------------------------------------

bool HalController::init() {
    std::lock_guard<std::mutex> lock(mConnectedHalMutex);
    if (mConnectedHal == nullptr) {
        mConnectedHal = mConnector(mCallbackScheduler);
    }
    return mConnectedHal != nullptr;
}

void HalController::tryReconnect() {
    std::lock_guard<std::mutex> lock(mConnectedHalMutex);
    if (mConnectedHal == nullptr) {
        mConnectedHal = mConnector(mCallbackScheduler);
    } else {
        mConnectedHal->tryReconnect();
    }
}

}; // namespace vibrator

}; // namespace android
