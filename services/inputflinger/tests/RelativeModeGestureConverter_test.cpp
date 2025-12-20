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

#include <memory>

#include <android/input.h>
#include <gestures/RelativeModeGestureConverter.h>
#include <gtest/gtest.h>
#include <utils/StrongPointer.h>

#include "FakeEventHub.h"
#include "FakeInputReaderPolicy.h"
#include "InstrumentedInputReader.h"
#include "NotifyArgs.h"
#include "TestConstants.h"
#include "TestEventMatchers.h"
#include "TestInputListener.h"
#include "include/gestures.h"

namespace android {

namespace {

constexpr stime_t GESTURE_TIME = 1.2;

} // namespace

using testing::AllOf;
using testing::ElementsAre;
using testing::VariantWith;

class RelativeModeGestureConverterTest : public testing::Test {
protected:
    static constexpr int32_t DEVICE_ID = END_RESERVED_ID + 1000;

    RelativeModeGestureConverterTest()
          : mFakeEventHub(std::make_unique<FakeEventHub>()),
            mFakePolicy(sp<FakeInputReaderPolicy>::make()),
            mReader(std::make_unique<InstrumentedInputReader>(mFakeEventHub, mFakePolicy,
                                                              mFakeListener)),
            mConverter(*mReader->getContext(), DEVICE_ID) {}

    std::shared_ptr<FakeEventHub> mFakeEventHub;
    sp<FakeInputReaderPolicy> mFakePolicy;
    TestInputListener mFakeListener;
    std::unique_ptr<InstrumentedInputReader> mReader;
    RelativeModeGestureConverter mConverter;
};

TEST_F(RelativeModeGestureConverterTest, Move) {
    Gesture moveGesture(kGestureMove, GESTURE_TIME, GESTURE_TIME, -5, 10);
    std::list<NotifyArgs> args =
            mConverter.handleGesture(ARBITRARY_TIME, READ_TIME, ARBITRARY_TIME, moveGesture);
    ASSERT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithMotionAction(AMOTION_EVENT_ACTION_MOVE), WithCoords(-5, 10),
                              WithRelativeMotion(-5, 10), WithButtonState(0),
                              WithSource(AINPUT_SOURCE_MOUSE_RELATIVE)))));
}

} // namespace android
