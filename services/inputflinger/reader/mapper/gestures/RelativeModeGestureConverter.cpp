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

#include "gestures/RelativeModeGestureConverter.h"

#include <list>

#include "NotifyArgs.h"
#include "include/gestures.h"
#include "input/Input.h"
#include "ui/LogicalDisplayId.h"

namespace android {

RelativeModeGestureConverter::RelativeModeGestureConverter(InputReaderContext& readerContext,
                                                           DeviceId deviceId)
      : mDeviceId(deviceId), mReaderContext(readerContext) {}

std::list<NotifyArgs> RelativeModeGestureConverter::handleGesture(nsecs_t when, nsecs_t readTime,
                                                                  nsecs_t gestureStartTime,
                                                                  const Gesture& gesture) {
    switch (gesture.type) {
        case kGestureTypeMove:
            return handleMove(when, readTime, gestureStartTime, gesture);
        default:
            // TODO(b/403531245): handle other types of gesture.
            return {};
    }
}

std::list<NotifyArgs> RelativeModeGestureConverter::handleMove(nsecs_t when, nsecs_t readTime,
                                                               nsecs_t gestureStartTime,
                                                               const Gesture& gesture) {
    float deltaX = gesture.details.move.dx;
    float deltaY = gesture.details.move.dy;
    // TODO(b/403531245): scale the deltas to be similar to those from a captured mouse.

    std::list<NotifyArgs> out;
    PointerCoords coords;
    coords.clear();
    coords.setAxisValue(AMOTION_EVENT_AXIS_X, deltaX);
    coords.setAxisValue(AMOTION_EVENT_AXIS_Y, deltaY);
    coords.setAxisValue(AMOTION_EVENT_AXIS_RELATIVE_X, deltaX);
    coords.setAxisValue(AMOTION_EVENT_AXIS_RELATIVE_Y, deltaY);

    out.push_back(makeMotionArgs(when, readTime, AMOTION_EVENT_ACTION_MOVE, &coords));
    return out;
}

NotifyMotionArgs RelativeModeGestureConverter::makeMotionArgs(nsecs_t when, nsecs_t readTime,
                                                              int32_t action,
                                                              const PointerCoords* pointerCoords) {
    int32_t flags = 0;
    if (action == AMOTION_EVENT_ACTION_CANCEL) {
        flags |= AMOTION_EVENT_FLAG_CANCELED;
    }

    PointerProperties pointerProperties;
    pointerProperties.clear();
    pointerProperties.id = 0;
    pointerProperties.toolType = ToolType::MOUSE;

    // TODO(b/403531245): set the down time once we handle button gestures.
    return NotifyMotionArgs(mReaderContext.getNextId(), when, readTime, mDeviceId, SOURCE,
                            ui::LogicalDisplayId::INVALID, POLICY_FLAG_WAKE, action,
                            /*actionButton=*/0, flags, mReaderContext.getGlobalMetaState(),
                            /*buttonState=*/0, MotionClassification::NONE, /*pointerCount=*/1,
                            &pointerProperties, pointerCoords, /*xPrecision=*/1.0f,
                            /*yPrecision=*/1.0f, AMOTION_EVENT_INVALID_CURSOR_POSITION,
                            AMOTION_EVENT_INVALID_CURSOR_POSITION, /*downTime=*/0,
                            /*videoFrames=*/{});
}

} // namespace android
