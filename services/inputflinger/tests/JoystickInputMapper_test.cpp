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

#include "JoystickInputMapper.h"

#include <list>
#include <optional>

#include <EventHub.h>
#include <NotifyArgs.h>
#include <ftl/flags.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <input/DisplayViewport.h>
#include <linux/input-event-codes.h>
#include <ui/LogicalDisplayId.h>

#include "InputMapperTest.h"
#include "TestConstants.h"
#include "TestEventMatchers.h"

namespace android {

using namespace ftl::flag_operators;
using testing::ElementsAre;
using testing::IsEmpty;
using testing::Return;
using testing::VariantWith;

class JoystickInputMapperTest : public InputMapperUnitTest {
protected:
    void SetUp() override {
        InputMapperUnitTest::SetUp();
        EXPECT_CALL(mMockEventHub, getDeviceClasses(EVENTHUB_ID))
                .WillRepeatedly(Return(InputDeviceClass::JOYSTICK | InputDeviceClass::EXTERNAL));

        // The mapper requests info on all ABS axis IDs, including ones which aren't actually used
        // (e.g. in the range from 0x0b (ABS_BRAKE) to 0x0f (ABS_HAT0X)), so just return nullopt for
        // all axes we don't explicitly set up below.
        EXPECT_CALL(mMockEventHub, getAbsoluteAxisInfo(EVENTHUB_ID, testing::_))
                .WillRepeatedly(Return(std::nullopt));

        setupAxis(ABS_X, /*valid=*/true, /*min=*/-32767, /*max=*/32767, /*resolution=*/0);
        setupAxis(ABS_Y, /*valid=*/true, /*min=*/-32767, /*max=*/32767, /*resolution=*/0);
    }
};

TEST_F(JoystickInputMapperTest, Configure_AssignsDisplayUniqueId) {
    DisplayViewport viewport;
    viewport.displayId = ui::LogicalDisplayId{1};
    EXPECT_CALL((*mDevice), getAssociatedViewport).WillRepeatedly(Return(viewport));
    mMapper = createInputMapper<JoystickInputMapper>(*mDeviceContext,
                                                     mFakePolicy->getReaderConfiguration());

    std::list<NotifyArgs> out;

    // Send an axis event
    out = process(EV_ABS, ABS_X, 100);
    ASSERT_THAT(out, IsEmpty());
    out = process(EV_SYN, SYN_REPORT, 0);
    ASSERT_THAT(out, ElementsAre(VariantWith<NotifyMotionArgs>(WithDisplayId(viewport.displayId))));

    // Send another axis event
    out = process(EV_ABS, ABS_Y, 100);
    ASSERT_THAT(out, IsEmpty());
    out = process(EV_SYN, SYN_REPORT, 0);
    ASSERT_THAT(out, ElementsAre(VariantWith<NotifyMotionArgs>(WithDisplayId(viewport.displayId))));
}

} // namespace android
