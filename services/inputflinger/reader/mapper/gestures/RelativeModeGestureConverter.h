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

#include <list>

#include <android/input.h>

#include "InputReaderContext.h"
#include "NotifyArgs.h"
#include "input/Input.h"

#include "include/gestures.h"

namespace android {

// Converts Gesture structs from the gestures library into NotifyArgs when a touchpad is captured in
// relative mode.
class RelativeModeGestureConverter {
public:
    RelativeModeGestureConverter(InputReaderContext& readerContext, DeviceId deviceId);

    [[nodiscard]] std::list<NotifyArgs> handleGesture(nsecs_t when, nsecs_t readTime,
                                                      nsecs_t gestureStartTime,
                                                      const Gesture& gesture);

private:
    [[nodiscard]] std::list<NotifyArgs> handleMove(nsecs_t when, nsecs_t readTime,
                                                   nsecs_t gestureStartTime,
                                                   const Gesture& gesture);

    NotifyMotionArgs makeMotionArgs(nsecs_t when, nsecs_t readTime, int32_t action,
                                    const PointerCoords* pointerCoords);

    const DeviceId mDeviceId;
    InputReaderContext& mReaderContext;

    static constexpr uint32_t SOURCE = AINPUT_SOURCE_MOUSE_RELATIVE;
};

} // namespace android
