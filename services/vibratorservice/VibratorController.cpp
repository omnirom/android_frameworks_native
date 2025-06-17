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

#define LOG_TAG "VibratorController"

#ifndef qDoWithRetries
#define qDoWithRetries(op) doWithRetries(op, __FUNCTION__)
#endif

#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>

#include <utils/Log.h>

#include <vibratorservice/VibratorController.h>

using ::aidl::android::hardware::vibrator::Effect;
using ::aidl::android::hardware::vibrator::EffectStrength;
using ::aidl::android::hardware::vibrator::IVibrator;

using Status = ::ndk::ScopedAStatus;

using namespace std::placeholders;

namespace android {

namespace vibrator {

// -------------------------------------------------------------------------------------------------

inline bool isStatusUnsupported(const Status& status) {
    // STATUS_UNKNOWN_TRANSACTION means the HAL is an older version, so operation is unsupported.
    return status.getStatus() == STATUS_UNKNOWN_TRANSACTION ||
            status.getExceptionCode() == EX_UNSUPPORTED_OPERATION;
}

inline bool isStatusTransactionFailed(const Status& status) {
    // STATUS_UNKNOWN_TRANSACTION means the HAL is an older version, so operation is unsupported.
    return status.getStatus() != STATUS_UNKNOWN_TRANSACTION &&
            status.getExceptionCode() == EX_TRANSACTION_FAILED;
}

// -------------------------------------------------------------------------------------------------

bool VibratorProvider::isDeclared() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mIsDeclared.has_value()) {
        return *mIsDeclared;
    }

    bool isDeclared = AServiceManager_isDeclared(mServiceName.c_str());
    if (!isDeclared) {
        ALOGV("Vibrator HAL service not declared.");
    }

    mIsDeclared.emplace(isDeclared);
    return isDeclared;
}

std::shared_ptr<IVibrator> VibratorProvider::waitForVibrator() {
    if (!isDeclared()) {
        return nullptr;
    }

    auto vibrator = IVibrator::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(mServiceName.c_str())));
    if (vibrator) {
        ALOGV("Successfully connected to Vibrator HAL service.");
    } else {
        ALOGE("Error connecting to declared Vibrator HAL service.");
    }

    return vibrator;
}

std::shared_ptr<IVibrator> VibratorProvider::checkForVibrator() {
    if (!isDeclared()) {
        return nullptr;
    }

    auto vibrator = IVibrator::fromBinder(
            ndk::SpAIBinder(AServiceManager_checkService(mServiceName.c_str())));
    if (vibrator) {
        ALOGV("Successfully reconnected to Vibrator HAL service.");
    } else {
        ALOGE("Error reconnecting to declared Vibrator HAL service.");
    }

    return vibrator;
}

// -------------------------------------------------------------------------------------------------

bool VibratorController::init() {
    if (!mVibratorProvider->isDeclared()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mMutex);
    if (mVibrator == nullptr) {
        mVibrator = mVibratorProvider->waitForVibrator();
    }
    return mVibratorProvider->isDeclared();
}

Status VibratorController::off() {
    return qDoWithRetries(std::bind(&IVibrator::off, _1));
}

Status VibratorController::setAmplitude(float amplitude) {
    return qDoWithRetries(std::bind(&IVibrator::setAmplitude, _1, amplitude));
}

Status VibratorController::setExternalControl(bool enabled) {
    return qDoWithRetries(std::bind(&IVibrator::setExternalControl, _1, enabled));
}

Status VibratorController::alwaysOnEnable(int32_t id, const Effect& effect,
                                          const EffectStrength& strength) {
    return qDoWithRetries(std::bind(&IVibrator::alwaysOnEnable, _1, id, effect, strength));
}

Status VibratorController::alwaysOnDisable(int32_t id) {
    return qDoWithRetries(std::bind(&IVibrator::alwaysOnDisable, _1, id));
}

// -------------------------------------------------------------------------------------------------

std::shared_ptr<IVibrator> VibratorController::reconnectToVibrator() {
    std::lock_guard<std::mutex> lock(mMutex);
    mVibrator = mVibratorProvider->checkForVibrator();
    return mVibrator;
}

Status VibratorController::doWithRetries(const VibratorController::VibratorOp& op,
                                         const char* logLabel) {
    if (!init()) {
        ALOGV("Skipped %s because Vibrator HAL is not declared", logLabel);
        return Status::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE, "IVibrator not declared");
    }
    std::shared_ptr<IVibrator> vibrator;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        vibrator = mVibrator;
    }

    if (!vibrator) {
        ALOGE("Skipped %s because Vibrator HAL is declared but failed to load", logLabel);
        return Status::fromExceptionCodeWithMessage(EX_ILLEGAL_STATE,
                                                    "IVibrator declared but failed to load");
    }

    auto status = doOnce(vibrator.get(), op, logLabel);
    for (int i = 1; i < MAX_ATTEMPTS && isStatusTransactionFailed(status); i++) {
        vibrator = reconnectToVibrator();
        if (!vibrator) {
            // Failed to reconnect to vibrator HAL after a transaction failed, skip retries.
            break;
        }
        status = doOnce(vibrator.get(), op, logLabel);
    }

    return status;
}

Status VibratorController::doOnce(IVibrator* vibrator, const VibratorController::VibratorOp& op,
                                  const char* logLabel) {
    auto status = op(vibrator);
    if (!status.isOk()) {
        if (isStatusUnsupported(status)) {
            ALOGV("Vibrator HAL %s is unsupported: %s", logLabel, status.getMessage());
        } else {
            ALOGE("Vibrator HAL %s failed: %s", logLabel, status.getMessage());
        }
    }
    return status;
}

// -------------------------------------------------------------------------------------------------

}; // namespace vibrator

}; // namespace android
