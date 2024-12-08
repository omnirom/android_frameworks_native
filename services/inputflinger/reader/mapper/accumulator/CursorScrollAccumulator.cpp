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

#include "CursorScrollAccumulator.h"

#include <android_companion_virtualdevice_flags.h>
#include "EventHub.h"
#include "InputDevice.h"

namespace android {

namespace vd_flags = android::companion::virtualdevice::flags;

CursorScrollAccumulator::CursorScrollAccumulator()
      : mHaveRelWheel(false),
        mHaveRelHWheel(false),
        mHaveRelWheelHighRes(false),
        mHaveRelHWheelHighRes(false) {
    clearRelativeAxes();
}

void CursorScrollAccumulator::configure(InputDeviceContext& deviceContext) {
    mHaveRelWheel = deviceContext.hasRelativeAxis(REL_WHEEL);
    mHaveRelHWheel = deviceContext.hasRelativeAxis(REL_HWHEEL);
    if (vd_flags::high_resolution_scroll()) {
        mHaveRelWheelHighRes = deviceContext.hasRelativeAxis(REL_WHEEL_HI_RES);
        mHaveRelHWheelHighRes = deviceContext.hasRelativeAxis(REL_HWHEEL_HI_RES);
    }
}

void CursorScrollAccumulator::reset(InputDeviceContext& deviceContext) {
    clearRelativeAxes();
}

void CursorScrollAccumulator::clearRelativeAxes() {
    mRelWheel = 0;
    mRelHWheel = 0;
}

void CursorScrollAccumulator::process(const RawEvent& rawEvent) {
    if (rawEvent.type == EV_REL) {
        switch (rawEvent.code) {
            case REL_WHEEL_HI_RES:
                if (mHaveRelWheelHighRes) {
                    mRelWheel =
                            rawEvent.value / static_cast<float>(kEvdevHighResScrollUnitsPerDetent);
                }
                break;
            case REL_HWHEEL_HI_RES:
                if (mHaveRelHWheelHighRes) {
                    mRelHWheel =
                            rawEvent.value / static_cast<float>(kEvdevHighResScrollUnitsPerDetent);
                }
                break;
            case REL_WHEEL:
                // We should ignore regular scroll events, if we have already have high-res scroll
                // enabled.
                if (!mHaveRelWheelHighRes) {
                    mRelWheel = rawEvent.value;
                }
                break;
            case REL_HWHEEL:
                // We should ignore regular scroll events, if we have already have high-res scroll
                // enabled.
                if (!mHaveRelHWheelHighRes) {
                    mRelHWheel = rawEvent.value;
                }
                break;
        }
    }
}

void CursorScrollAccumulator::finishSync() {
    clearRelativeAxes();
}

} // namespace android
