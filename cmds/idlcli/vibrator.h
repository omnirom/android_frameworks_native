/*
 * Copyright (C) 2019 The Android Open Source Project
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
#pragma once

#include <future>

#include <aidl/android/hardware/vibrator/BnVibratorCallback.h>
#include <aidl/android/hardware/vibrator/IVibrator.h>
#include <aidl/android/hardware/vibrator/IVibratorManager.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include "IdlCli.h"
#include "utils.h"

namespace android {

using ::aidl::android::hardware::vibrator::IVibrator;
using idlcli::IdlCli;

inline auto getService(std::string name) {
    const auto instance = std::string() + IVibrator::descriptor + "/" + name;
    auto vibBinder = ndk::SpAIBinder(AServiceManager_checkService(instance.c_str()));
    return IVibrator::fromBinder(vibBinder);
}

static auto getHal() {
    // Assume that if getService returns a nullptr, HAL is not available on the device.
    const auto name = IdlCli::Get().getName();
    return getService(name.empty() ? "default" : name);
}

namespace idlcli {
namespace vibrator {

namespace aidl = ::aidl::android::hardware::vibrator;

class VibratorCallback : public aidl::BnVibratorCallback {
public:
    ndk::ScopedAStatus onComplete() override {
        mPromise.set_value();
        return ndk::ScopedAStatus::ok();
    }
    void waitForComplete() { mPromise.get_future().wait(); }

private:
    std::promise<void> mPromise;
};

} // namespace vibrator
} // namespace idlcli

} // namespace android
