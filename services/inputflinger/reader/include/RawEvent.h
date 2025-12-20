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

#pragma once

#include <stdint.h>

#include <input/Input.h>
#include <utils/Timers.h>

namespace android {

/*
 * A raw event as retrieved from the EventHub.
 */
struct RawEvent {
    // Time when the event happened
    nsecs_t when;
    // Time when the event was read by EventHub. Only populated for input events.
    // For other events (device added/removed/etc), this value is undefined and should not be read.
    nsecs_t readTime;
    RawDeviceId deviceId;
    int32_t type;
    int32_t code;
    int32_t value;
};

} // namespace android
