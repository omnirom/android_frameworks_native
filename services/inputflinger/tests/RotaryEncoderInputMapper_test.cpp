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

#include "RotaryEncoderInputMapper.h"

#include <list>
#include <string>
#include <tuple>
#include <variant>

#include <android-base/logging.h>
#include <android_companion_virtualdevice_flags.h>
#include <gtest/gtest.h>
#include <input/DisplayViewport.h>
#include <linux/input-event-codes.h>
#include <linux/input.h>
#include <utils/Timers.h>

#include "InputMapperTest.h"
#include "InputReaderBase.h"
#include "InterfaceMocks.h"
#include "NotifyArgs.h"
#include "TestEventMatchers.h"
#include "ui/Rotation.h"

#define TAG "RotaryEncoderInputMapper_test"

namespace android {

using testing::AllOf;
using testing::Return;
using testing::VariantWith;
constexpr ui::LogicalDisplayId DISPLAY_ID = ui::LogicalDisplayId::DEFAULT;
constexpr ui::LogicalDisplayId SECONDARY_DISPLAY_ID = ui::LogicalDisplayId{DISPLAY_ID.val() + 1};
constexpr int32_t DISPLAY_WIDTH = 480;
constexpr int32_t DISPLAY_HEIGHT = 800;

namespace {

DisplayViewport createViewport() {
    DisplayViewport v;
    v.orientation = ui::Rotation::Rotation0;
    v.logicalRight = DISPLAY_HEIGHT;
    v.logicalBottom = DISPLAY_WIDTH;
    v.physicalRight = DISPLAY_HEIGHT;
    v.physicalBottom = DISPLAY_WIDTH;
    v.deviceWidth = DISPLAY_HEIGHT;
    v.deviceHeight = DISPLAY_WIDTH;
    v.isActive = true;
    return v;
}

DisplayViewport createPrimaryViewport() {
    DisplayViewport v = createViewport();
    v.displayId = DISPLAY_ID;
    v.uniqueId = "local:1";
    return v;
}

DisplayViewport createSecondaryViewport() {
    DisplayViewport v = createViewport();
    v.displayId = SECONDARY_DISPLAY_ID;
    v.uniqueId = "local:2";
    v.type = ViewportType::EXTERNAL;
    return v;
}

} // namespace

namespace vd_flags = android::companion::virtualdevice::flags;

/**
 * Unit tests for RotaryEncoderInputMapper.
 */
class RotaryEncoderInputMapperTest : public InputMapperUnitTest {
protected:
    void SetUp() override { SetUpWithBus(BUS_USB); }
    void SetUpWithBus(int bus) override {
        InputMapperUnitTest::SetUpWithBus(bus);

        EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_WHEEL))
                .WillRepeatedly(Return(true));
        EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_HWHEEL))
                .WillRepeatedly(Return(false));
        EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_WHEEL_HI_RES))
                .WillRepeatedly(Return(false));
        EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_HWHEEL_HI_RES))
                .WillRepeatedly(Return(false));
    }
};

TEST_F(RotaryEncoderInputMapperTest, ConfigureDisplayIdWithAssociatedViewport) {
    DisplayViewport primaryViewport = createPrimaryViewport();
    DisplayViewport secondaryViewport = createSecondaryViewport();
    mReaderConfiguration.setDisplayViewports({primaryViewport, secondaryViewport});

    // Set up the secondary display as the associated viewport of the mapper.
    EXPECT_CALL((*mDevice), getAssociatedViewport).WillRepeatedly(Return(secondaryViewport));
    mMapper = createInputMapper<RotaryEncoderInputMapper>(*mDeviceContext, mReaderConfiguration);

    std::list<NotifyArgs> args;
    // Ensure input events are generated for the secondary display.
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL, 1);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);
    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithMotionAction(AMOTION_EVENT_ACTION_SCROLL),
                              WithSource(AINPUT_SOURCE_ROTARY_ENCODER),
                              WithDisplayId(SECONDARY_DISPLAY_ID)))));
}

TEST_F(RotaryEncoderInputMapperTest, ConfigureDisplayIdNoAssociatedViewport) {
    // Set up the default display.
    mFakePolicy->clearViewports();
    mFakePolicy->addDisplayViewport(createPrimaryViewport());

    // Set up the mapper with no associated viewport.
    mMapper = createInputMapper<RotaryEncoderInputMapper>(*mDeviceContext, mReaderConfiguration);

    // Ensure input events are generated without display ID
    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL, 1);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);
    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithMotionAction(AMOTION_EVENT_ACTION_SCROLL),
                              WithSource(AINPUT_SOURCE_ROTARY_ENCODER),
                              WithDisplayId(ui::LogicalDisplayId::INVALID)))));
}

TEST_F(RotaryEncoderInputMapperTest, ProcessRegularScroll) {
    mMapper = createInputMapper<RotaryEncoderInputMapper>(*mDeviceContext, mReaderConfiguration);

    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL, 1);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);

    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithSource(AINPUT_SOURCE_ROTARY_ENCODER),
                              WithMotionAction(AMOTION_EVENT_ACTION_SCROLL), WithScroll(1.0f)))));
}

TEST_F(RotaryEncoderInputMapperTest, ProcessHighResScroll) {
    vd_flags::high_resolution_scroll(true);
    EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_WHEEL_HI_RES))
            .WillRepeatedly(Return(true));
    mMapper = createInputMapper<RotaryEncoderInputMapper>(*mDeviceContext, mReaderConfiguration);

    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL_HI_RES, 60);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);

    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithSource(AINPUT_SOURCE_ROTARY_ENCODER),
                              WithMotionAction(AMOTION_EVENT_ACTION_SCROLL), WithScroll(0.5f)))));
}

TEST_F(RotaryEncoderInputMapperTest, HighResScrollIgnoresRegularScroll) {
    vd_flags::high_resolution_scroll(true);
    EXPECT_CALL(mMockEventHub, hasRelativeAxis(EVENTHUB_ID, REL_WHEEL_HI_RES))
            .WillRepeatedly(Return(true));
    mMapper = createInputMapper<RotaryEncoderInputMapper>(*mDeviceContext, mReaderConfiguration);

    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL_HI_RES, 60);
    args += process(ARBITRARY_TIME, EV_REL, REL_WHEEL, 1);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);

    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyMotionArgs>(
                        AllOf(WithSource(AINPUT_SOURCE_ROTARY_ENCODER),
                              WithMotionAction(AMOTION_EVENT_ACTION_SCROLL), WithScroll(0.5f)))));
}

} // namespace android