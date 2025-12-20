/*
 * Copyright 2024 The Android Open Source Project
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

#define LOG_TAG "KeyboardClassifier"

#include <android-base/logging.h>
#include <ftl/flags.h>
#include <input/KeyboardClassifier.h>

#include "input_cxx_bridge.rs.h"

using android::input::RustInputDeviceIdentifier;

namespace android {

KeyboardClassifier::KeyboardClassifier() {
    mRustClassifier = android::input::keyboardClassifier::create();
}

KeyboardType KeyboardClassifier::getKeyboardType(DeviceId deviceId) {
    return static_cast<KeyboardType>(
            android::input::keyboardClassifier::getKeyboardType(**mRustClassifier, deviceId));
}

void KeyboardClassifier::notifyKeyboardChanged(DeviceId deviceId,
                                               const InputDeviceIdentifier& identifier,
                                               uint32_t deviceClasses) {
    RustInputDeviceIdentifier rustIdentifier;
    rustIdentifier.name = rust::String::lossy(identifier.name);
    rustIdentifier.location = rust::String::lossy(identifier.location);
    rustIdentifier.unique_id = rust::String::lossy(identifier.uniqueId);
    rustIdentifier.bus = identifier.bus;
    rustIdentifier.vendor = identifier.vendor;
    rustIdentifier.product = identifier.product;
    rustIdentifier.version = identifier.version;
    rustIdentifier.descriptor = rust::String::lossy(identifier.descriptor);
    android::input::keyboardClassifier::notifyKeyboardChanged(**mRustClassifier, deviceId,
                                                                rustIdentifier, deviceClasses);
}

void KeyboardClassifier::processKey(DeviceId deviceId, int32_t evdevCode, uint32_t metaState) {
    if (!android::input::keyboardClassifier::isFinalized(**mRustClassifier, deviceId)) {
        android::input::keyboardClassifier::processKey(**mRustClassifier, deviceId, evdevCode,
                                                       metaState);
    }
}

} // namespace android
