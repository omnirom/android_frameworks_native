/*
 * Copyright 2023 The Android Open Source Project
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

#include "KeyboardInputMapper.h"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include <android/input.h>
#include <android/keycodes.h>
#include <com_android_input_flags.h>
#include <flag_macros.h>
#include <ftl/flags.h>
#include <gtest/gtest.h>
#include <input/DisplayViewport.h>
#include <input/Input.h>
#include <input/InputDevice.h>
#include <ui/LogicalDisplayId.h>
#include <ui/Rotation.h>
#include <utils/Errors.h>

#include "EventHub.h"
#include "InputMapperTest.h"
#include "InterfaceMocks.h"
#include "NotifyArgs.h"
#include "TestConstants.h"
#include "TestEventMatchers.h"

#define TAG "KeyboardInputMapper_test"

namespace android {

using namespace ftl::flag_operators;
using testing::_;
using testing::AllOf;
using testing::AnyOf;
using testing::Args;
using testing::DoAll;
using testing::IsEmpty;
using testing::Return;
using testing::ReturnArg;
using testing::SaveArg;
using testing::SetArgPointee;
using testing::VariantWith;

namespace {

// Arbitrary display properties.
constexpr ui::LogicalDisplayId DISPLAY_ID = ui::LogicalDisplayId::DEFAULT;
constexpr int32_t DISPLAY_WIDTH = 480;
constexpr int32_t DISPLAY_HEIGHT = 800;

DisplayViewport createPrimaryViewport(ui::Rotation orientation) {
    const bool isRotated =
            orientation == ui::Rotation::Rotation90 || orientation == ui::Rotation::Rotation270;
    DisplayViewport v;
    v.displayId = DISPLAY_ID;
    v.orientation = orientation;
    v.logicalRight = isRotated ? DISPLAY_HEIGHT : DISPLAY_WIDTH;
    v.logicalBottom = isRotated ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    v.physicalRight = isRotated ? DISPLAY_HEIGHT : DISPLAY_WIDTH;
    v.physicalBottom = isRotated ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    v.deviceWidth = isRotated ? DISPLAY_HEIGHT : DISPLAY_WIDTH;
    v.deviceHeight = isRotated ? DISPLAY_WIDTH : DISPLAY_HEIGHT;
    v.isActive = true;
    v.uniqueId = "local:1";
    return v;
}

} // namespace

/**
 * Unit tests for KeyboardInputMapper.
 */
class KeyboardInputMapperUnitTest : public InputMapperUnitTest {
protected:
    const KeyboardLayoutInfo DEVICE_KEYBOARD_LAYOUT_INFO = KeyboardLayoutInfo("en-US", "qwerty");

    sp<FakeInputReaderPolicy> mFakePolicy;
    const std::unordered_map<int32_t, int32_t> mKeyCodeMap{{KEY_0, AKEYCODE_0},
                                                           {KEY_A, AKEYCODE_A},
                                                           {KEY_LEFTCTRL, AKEYCODE_CTRL_LEFT},
                                                           {KEY_LEFTALT, AKEYCODE_ALT_LEFT},
                                                           {KEY_RIGHTALT, AKEYCODE_ALT_RIGHT},
                                                           {KEY_LEFTSHIFT, AKEYCODE_SHIFT_LEFT},
                                                           {KEY_RIGHTSHIFT, AKEYCODE_SHIFT_RIGHT},
                                                           {KEY_FN, AKEYCODE_FUNCTION},
                                                           {KEY_LEFTCTRL, AKEYCODE_CTRL_LEFT},
                                                           {KEY_RIGHTCTRL, AKEYCODE_CTRL_RIGHT},
                                                           {KEY_LEFTMETA, AKEYCODE_META_LEFT},
                                                           {KEY_RIGHTMETA, AKEYCODE_META_RIGHT},
                                                           {KEY_CAPSLOCK, AKEYCODE_CAPS_LOCK},
                                                           {KEY_NUMLOCK, AKEYCODE_NUM_LOCK},
                                                           {KEY_SCROLLLOCK, AKEYCODE_SCROLL_LOCK}};

    void SetUp() override {
        InputMapperUnitTest::SetUp();

        // set key-codes expected in tests
        for (const auto& [evdevCode, outKeycode] : mKeyCodeMap) {
            addKeyByEvdevCode(evdevCode, outKeycode);
        }

        mFakePolicy = sp<FakeInputReaderPolicy>::make();
        EXPECT_CALL(mMockInputReaderContext, getPolicy).WillRepeatedly(Return(mFakePolicy.get()));

        ON_CALL((*mDevice), getSources).WillByDefault(Return(AINPUT_SOURCE_KEYBOARD));

        mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                         AINPUT_SOURCE_KEYBOARD);
    }

    void addKeyByEvdevCode(int32_t evdevCode, int32_t keyCode, int32_t flags = 0) {
        EXPECT_CALL(mMockEventHub, mapKey(EVENTHUB_ID, evdevCode, _, _, _, _, _))
                .WillRepeatedly([=](int32_t, int32_t, int32_t, int32_t metaState,
                                    int32_t* outKeycode, int32_t* outMetaState,
                                    uint32_t* outFlags) {
                    if (outKeycode != nullptr) {
                        *outKeycode = keyCode;
                    }
                    if (outMetaState != nullptr) {
                        *outMetaState = metaState;
                    }
                    if (outFlags != nullptr) {
                        *outFlags = flags;
                    }
                    return NO_ERROR;
                });
    }

    void addKeyByUsageCode(int32_t usageCode, int32_t keyCode, int32_t flags = 0) {
        EXPECT_CALL(mMockEventHub, mapKey(EVENTHUB_ID, _, usageCode, _, _, _, _))
                .WillRepeatedly([=](int32_t, int32_t, int32_t, int32_t metaState,
                                    int32_t* outKeycode, int32_t* outMetaState,
                                    uint32_t* outFlags) {
                    if (outKeycode != nullptr) {
                        *outKeycode = keyCode;
                    }
                    if (outMetaState != nullptr) {
                        *outMetaState = metaState;
                    }
                    if (outFlags != nullptr) {
                        *outFlags = flags;
                    }
                    return NO_ERROR;
                });
    }

    void setDisplayOrientation(ui::Rotation orientation) {
        EXPECT_CALL((*mDevice), getAssociatedViewport)
                .WillRepeatedly(Return(createPrimaryViewport(orientation)));
        std::list<NotifyArgs> args =
                mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration,
                                     InputReaderConfiguration::Change::DISPLAY_INFO);
        ASSERT_EQ(0u, args.size());
    }

    NotifyKeyArgs expectSingleKeyArg(const std::list<NotifyArgs>& args) {
        EXPECT_EQ(1u, args.size());
        return std::get<NotifyKeyArgs>(args.front());
    }

    std::list<NotifyArgs> processKeyAndSync(nsecs_t when, int32_t code, int32_t value) {
        std::list<NotifyArgs> argsList = process(when, EV_KEY, code, value);
        argsList += process(when, EV_SYN, SYN_REPORT, 0);
        return argsList;
    }

    void testDPadKeyRotation(int32_t originalEvdevCode, int32_t rotatedKeyCode,
                             ui::LogicalDisplayId displayId = DISPLAY_ID) {
        std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, originalEvdevCode, 1);
        NotifyKeyArgs args = expectSingleKeyArg(argsList);
        ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
        ASSERT_EQ(originalEvdevCode, args.scanCode);
        ASSERT_EQ(rotatedKeyCode, args.keyCode);
        ASSERT_EQ(displayId, args.displayId);

        argsList = processKeyAndSync(ARBITRARY_TIME, originalEvdevCode, 0);
        args = expectSingleKeyArg(argsList);
        ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
        ASSERT_EQ(originalEvdevCode, args.scanCode);
        ASSERT_EQ(rotatedKeyCode, args.keyCode);
        ASSERT_EQ(displayId, args.displayId);
    }
};

TEST_F(KeyboardInputMapperUnitTest, GetSources) {
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, mMapper->getSources());
}

TEST_F(KeyboardInputMapperUnitTest, KeyPressTimestampRecorded) {
    nsecs_t when = ARBITRARY_TIME;
    std::vector<int32_t> keyCodes{KEY_0, KEY_A, KEY_LEFTCTRL, KEY_RIGHTALT, KEY_LEFTSHIFT};
    EXPECT_CALL(mMockInputReaderContext, setLastKeyDownTimestamp)
            .With(Args<0>(when))
            .Times(keyCodes.size());
    for (int32_t keyCode : keyCodes) {
        processKeyAndSync(when, keyCode, 1);
        processKeyAndSync(when, keyCode, 0);
    }
}

TEST_F(KeyboardInputMapperUnitTest, RepeatEventsDiscarded) {
    std::list<NotifyArgs> args;
    args += processKeyAndSync(ARBITRARY_TIME, KEY_0, 1);
    args += processKeyAndSync(ARBITRARY_TIME, KEY_0, 2);
    args += processKeyAndSync(ARBITRARY_TIME, KEY_0, 0);

    EXPECT_THAT(args,
                ElementsAre(VariantWith<NotifyKeyArgs>(AllOf(WithKeyAction(AKEY_EVENT_ACTION_DOWN),
                                                             WithKeyCode(AKEYCODE_0),
                                                             WithScanCode(KEY_0))),
                            VariantWith<NotifyKeyArgs>(AllOf(WithKeyAction(AKEY_EVENT_ACTION_UP),
                                                             WithKeyCode(AKEYCODE_0),
                                                             WithScanCode(KEY_0)))));
}

TEST_F(KeyboardInputMapperUnitTest, Process_SimpleKeyPress) {
    const int32_t USAGE_A = 0x070004;
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME, POLICY_FLAG_WAKE);
    addKeyByUsageCode(USAGE_A, AKEYCODE_A, POLICY_FLAG_WAKE);

    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mMapper->getMetaState());

    // Key down by evdev code.
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    NotifyKeyArgs args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(AKEYCODE_HOME, args.keyCode);
    ASSERT_EQ(KEY_HOME, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(POLICY_FLAG_WAKE, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);

    // Key up by evdev code.
    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME + 1, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(AKEYCODE_HOME, args.keyCode);
    ASSERT_EQ(KEY_HOME, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(POLICY_FLAG_WAKE, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);

    // Key down by usage code.
    argsList = process(ARBITRARY_TIME, EV_MSC, MSC_SCAN, USAGE_A);
    argsList += process(ARBITRARY_TIME, EV_KEY, 0, 1);
    argsList += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(AKEYCODE_A, args.keyCode);
    ASSERT_EQ(0, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(POLICY_FLAG_WAKE, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);

    // Key up by usage code.
    argsList = process(ARBITRARY_TIME + 1, EV_MSC, MSC_SCAN, USAGE_A);
    argsList += process(ARBITRARY_TIME + 1, EV_KEY, 0, 0);
    argsList += process(ARBITRARY_TIME + 1, EV_SYN, SYN_REPORT, 0);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME + 1, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(AKEYCODE_A, args.keyCode);
    ASSERT_EQ(0, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(POLICY_FLAG_WAKE, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);
}

TEST_F(KeyboardInputMapperUnitTest, Process_UnknownKey) {
    const int32_t USAGE_UNKNOWN = 0x07ffff;
    EXPECT_CALL(mMockEventHub, mapKey(EVENTHUB_ID, KEY_UNKNOWN, USAGE_UNKNOWN, _, _, _, _))
            .WillRepeatedly(Return(NAME_NOT_FOUND));

    // Key down with unknown scan code or usage code.
    std::list<NotifyArgs> argsList = process(ARBITRARY_TIME, EV_MSC, MSC_SCAN, USAGE_UNKNOWN);
    argsList += process(ARBITRARY_TIME, EV_KEY, KEY_UNKNOWN, 1);
    argsList += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);
    NotifyKeyArgs args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(0, args.keyCode);
    ASSERT_EQ(KEY_UNKNOWN, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(0U, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);

    // Key up with unknown scan code or usage code.
    argsList = process(ARBITRARY_TIME + 1, EV_MSC, MSC_SCAN, USAGE_UNKNOWN);
    argsList += process(ARBITRARY_TIME + 1, EV_KEY, KEY_UNKNOWN, 0);
    argsList += process(ARBITRARY_TIME + 1, EV_SYN, SYN_REPORT, 0);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME + 1, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(0, args.keyCode);
    ASSERT_EQ(KEY_UNKNOWN, args.scanCode);
    ASSERT_EQ(AMETA_NONE, args.metaState);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);
    ASSERT_EQ(0U, args.policyFlags);
    ASSERT_EQ(ARBITRARY_TIME, args.downTime);
}

/**
 * Ensure that the readTime is set to the time when the EV_KEY is received.
 */
TEST_F(KeyboardInputMapperUnitTest, Process_SendsReadTime) {
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME);

    // Key down
    std::list<NotifyArgs> argsList = process(ARBITRARY_TIME, /*readTime=*/12, EV_KEY, KEY_HOME, 1);
    argsList += process(ARBITRARY_TIME, /*readTime=*/12, EV_SYN, SYN_REPORT, 0);
    ASSERT_EQ(12, expectSingleKeyArg(argsList).readTime);

    // Key up
    argsList = process(ARBITRARY_TIME, /*readTime=*/15, EV_KEY, KEY_HOME, 0);
    argsList += process(ARBITRARY_TIME, /*readTime=*/15, EV_SYN, SYN_REPORT, 0);
    ASSERT_EQ(15, expectSingleKeyArg(argsList).readTime);
}

TEST_F(KeyboardInputMapperUnitTest, Process_ShouldUpdateMetaState) {
    addKeyByEvdevCode(KEY_LEFTSHIFT, AKEYCODE_SHIFT_LEFT);
    addKeyByEvdevCode(KEY_A, AKEYCODE_A);

    EXPECT_CALL(mMockInputReaderContext, updateGlobalMetaState()).Times(2);

    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mMapper->getMetaState());

    // Metakey down.
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_LEFTSHIFT, 1);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, expectSingleKeyArg(argsList).metaState);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, mMapper->getMetaState());

    // Key down.
    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_A, 1);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, expectSingleKeyArg(argsList).metaState);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, mMapper->getMetaState());

    // Key up.
    argsList = processKeyAndSync(ARBITRARY_TIME + 2, KEY_A, 0);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, expectSingleKeyArg(argsList).metaState);
    ASSERT_EQ(AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_ON, mMapper->getMetaState());

    // Metakey up.
    argsList = processKeyAndSync(ARBITRARY_TIME + 3, KEY_LEFTSHIFT, 0);
    ASSERT_EQ(AMETA_NONE, expectSingleKeyArg(argsList).metaState);
    ASSERT_EQ(AMETA_NONE, mMapper->getMetaState());
}

TEST_F(KeyboardInputMapperUnitTest, Process_WhenNotOrientationAware_ShouldNotRotateDPad) {
    addKeyByEvdevCode(KEY_UP, AKEYCODE_DPAD_UP);
    addKeyByEvdevCode(KEY_RIGHT, AKEYCODE_DPAD_RIGHT);
    addKeyByEvdevCode(KEY_DOWN, AKEYCODE_DPAD_DOWN);
    addKeyByEvdevCode(KEY_LEFT, AKEYCODE_DPAD_LEFT);

    setDisplayOrientation(ui::Rotation::Rotation90);
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_UP, AKEYCODE_DPAD_UP));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_RIGHT, AKEYCODE_DPAD_RIGHT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_DOWN, AKEYCODE_DPAD_DOWN));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_LEFT, AKEYCODE_DPAD_LEFT));
}

TEST_F(KeyboardInputMapperUnitTest, Process_WhenOrientationAware_ShouldRotateDPad) {
    addKeyByEvdevCode(KEY_UP, AKEYCODE_DPAD_UP);
    addKeyByEvdevCode(KEY_RIGHT, AKEYCODE_DPAD_RIGHT);
    addKeyByEvdevCode(KEY_DOWN, AKEYCODE_DPAD_DOWN);
    addKeyByEvdevCode(KEY_LEFT, AKEYCODE_DPAD_LEFT);

    mPropertyMap.addProperty("keyboard.orientationAware", "1");
    mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                     AINPUT_SOURCE_KEYBOARD);
    setDisplayOrientation(ui::ROTATION_0);

    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_UP, AKEYCODE_DPAD_UP));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_RIGHT, AKEYCODE_DPAD_RIGHT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_DOWN, AKEYCODE_DPAD_DOWN));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_LEFT, AKEYCODE_DPAD_LEFT));

    setDisplayOrientation(ui::ROTATION_90);
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_UP, AKEYCODE_DPAD_LEFT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_RIGHT, AKEYCODE_DPAD_UP));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_DOWN, AKEYCODE_DPAD_RIGHT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_LEFT, AKEYCODE_DPAD_DOWN));

    setDisplayOrientation(ui::ROTATION_180);
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_UP, AKEYCODE_DPAD_DOWN));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_RIGHT, AKEYCODE_DPAD_LEFT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_DOWN, AKEYCODE_DPAD_UP));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_LEFT, AKEYCODE_DPAD_RIGHT));

    setDisplayOrientation(ui::ROTATION_270);
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_UP, AKEYCODE_DPAD_RIGHT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_RIGHT, AKEYCODE_DPAD_DOWN));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_DOWN, AKEYCODE_DPAD_LEFT));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(KEY_LEFT, AKEYCODE_DPAD_UP));

    // Special case: if orientation changes while key is down, we still emit the same keycode
    // in the key up as we did in the key down.
    setDisplayOrientation(ui::ROTATION_270);
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 1);
    NotifyKeyArgs args = expectSingleKeyArg(argsList);
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(KEY_UP, args.scanCode);
    ASSERT_EQ(AKEYCODE_DPAD_RIGHT, args.keyCode);

    setDisplayOrientation(ui::ROTATION_180);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 0);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(KEY_UP, args.scanCode);
    ASSERT_EQ(AKEYCODE_DPAD_RIGHT, args.keyCode);
}

TEST_F(KeyboardInputMapperUnitTest, DisplayIdConfigurationChange_NotOrientationAware) {
    // If the keyboard is not orientation aware,
    // key events should not be associated with a specific display id
    addKeyByEvdevCode(KEY_UP, AKEYCODE_DPAD_UP);

    // Display id should be LogicalDisplayId::INVALID without any display configuration.
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 1);
    ASSERT_GT(argsList.size(), 0u);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 0);
    ASSERT_GT(argsList.size(), 0u);
    ASSERT_EQ(ui::LogicalDisplayId::INVALID, std::get<NotifyKeyArgs>(argsList.front()).displayId);
}

TEST_F(KeyboardInputMapperUnitTest, DisplayIdConfigurationChange_OrientationAware) {
    // If the keyboard is orientation aware,
    // key events should be associated with the internal viewport
    addKeyByEvdevCode(KEY_UP, AKEYCODE_DPAD_UP);

    mPropertyMap.addProperty("keyboard.orientationAware", "1");
    mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                     AINPUT_SOURCE_KEYBOARD);

    // Display id should be LogicalDisplayId::INVALID without any display configuration.
    // ^--- already checked by the previous test

    setDisplayOrientation(ui::ROTATION_0);
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 1);
    ASSERT_GT(argsList.size(), 0u);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 0);
    ASSERT_GT(argsList.size(), 0u);
    ASSERT_EQ(DISPLAY_ID, std::get<NotifyKeyArgs>(argsList.front()).displayId);

    constexpr ui::LogicalDisplayId newDisplayId = ui::LogicalDisplayId{2};
    DisplayViewport newViewport = createPrimaryViewport(ui::ROTATION_0);
    newViewport.displayId = newDisplayId;
    EXPECT_CALL((*mDevice), getAssociatedViewport).WillRepeatedly(Return(newViewport));
    argsList = mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration,
                                    InputReaderConfiguration::Change::DISPLAY_INFO);
    ASSERT_EQ(0u, argsList.size());
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 1);
    ASSERT_GT(argsList.size(), 0u);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_UP, 0);
    ASSERT_GT(argsList.size(), 0u);
    ASSERT_EQ(newDisplayId, std::get<NotifyKeyArgs>(argsList.front()).displayId);
}

TEST_F(KeyboardInputMapperUnitTest, GetKeyCodeState) {
    EXPECT_CALL(mMockEventHub, getKeyCodeState(EVENTHUB_ID, AKEYCODE_A))
            .WillRepeatedly(Return(AKEY_STATE_DOWN));
    ASSERT_EQ(AKEY_STATE_DOWN, mMapper->getKeyCodeState(AINPUT_SOURCE_ANY, AKEYCODE_A));

    EXPECT_CALL(mMockEventHub, getKeyCodeState(EVENTHUB_ID, AKEYCODE_A))
            .WillRepeatedly(Return(AKEY_STATE_UP));
    ASSERT_EQ(AKEY_STATE_UP, mMapper->getKeyCodeState(AINPUT_SOURCE_ANY, AKEYCODE_A));
}

TEST_F(KeyboardInputMapperUnitTest, GetKeyCodeForKeyLocation) {
    EXPECT_CALL(mMockEventHub, getKeyCodeForKeyLocation(EVENTHUB_ID, _))
            .WillRepeatedly(ReturnArg<1>());
    EXPECT_CALL(mMockEventHub, getKeyCodeForKeyLocation(EVENTHUB_ID, AKEYCODE_Y))
            .WillRepeatedly(Return(AKEYCODE_Z));
    ASSERT_EQ(AKEYCODE_Z, mMapper->getKeyCodeForKeyLocation(AKEYCODE_Y))
            << "If a mapping is available, the result is equal to the mapping";

    ASSERT_EQ(AKEYCODE_A, mMapper->getKeyCodeForKeyLocation(AKEYCODE_A))
            << "If no mapping is available, the result is the key location";
}

TEST_F(KeyboardInputMapperUnitTest, GetScanCodeState) {
    EXPECT_CALL(mMockEventHub, getScanCodeState(EVENTHUB_ID, KEY_A))
            .WillRepeatedly(Return(AKEY_STATE_DOWN));
    ASSERT_EQ(AKEY_STATE_DOWN, mMapper->getScanCodeState(AINPUT_SOURCE_ANY, KEY_A));

    EXPECT_CALL(mMockEventHub, getScanCodeState(EVENTHUB_ID, KEY_A))
            .WillRepeatedly(Return(AKEY_STATE_UP));
    ASSERT_EQ(AKEY_STATE_UP, mMapper->getScanCodeState(AINPUT_SOURCE_ANY, KEY_A));
}

TEST_F(KeyboardInputMapperUnitTest, Process_LockedKeysShouldToggleMetaStateAndLeds) {
    EXPECT_CALL(mMockEventHub,
                hasLed(EVENTHUB_ID, AnyOf(LED_CAPSL, LED_NUML, LED_SCROLLL /*NOTYPO*/)))
            .WillRepeatedly(Return(true));
    bool capsLockLed = true;    // Initially on
    bool numLockLed = false;    // Initially off
    bool scrollLockLed = false; // Initially off
    EXPECT_CALL(mMockEventHub, setLedState(EVENTHUB_ID, LED_CAPSL, _))
            .WillRepeatedly(SaveArg<2>(&capsLockLed));
    EXPECT_CALL(mMockEventHub, setLedState(EVENTHUB_ID, LED_NUML, _))
            .WillRepeatedly(SaveArg<2>(&numLockLed));
    EXPECT_CALL(mMockEventHub, setLedState(EVENTHUB_ID, LED_SCROLLL /*NOTYPO*/, _))
            .WillRepeatedly(SaveArg<2>(&scrollLockLed));
    addKeyByEvdevCode(KEY_CAPSLOCK, AKEYCODE_CAPS_LOCK);
    addKeyByEvdevCode(KEY_NUMLOCK, AKEYCODE_NUM_LOCK);
    addKeyByEvdevCode(KEY_SCROLLLOCK, AKEYCODE_SCROLL_LOCK);

    // In real operation, mappers pass new LED states to InputReader (via the context), which then
    // calls back to the mappers to apply that state. Mimic the same thing here with mocks.
    int32_t ledMetaState;
    EXPECT_CALL(mMockInputReaderContext, updateLedMetaState(_))
            .WillRepeatedly([&](int32_t newState) {
                ledMetaState = newState;
                mMapper->updateLedState(false);
            });
    EXPECT_CALL(mMockInputReaderContext, getLedMetaState())
            .WillRepeatedly(testing::ReturnPointee(&ledMetaState));

    ASSERT_THAT(mMapper->reset(ARBITRARY_TIME), IsEmpty());

    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mMapper->getMetaState());

    // Initialization should have turned all of the lights off.
    ASSERT_FALSE(capsLockLed);
    ASSERT_FALSE(numLockLed);
    ASSERT_FALSE(scrollLockLed);

    // Toggle caps lock on.
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_CAPSLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_CAPSLOCK, 0);
    ASSERT_TRUE(capsLockLed);
    ASSERT_FALSE(numLockLed);
    ASSERT_FALSE(scrollLockLed);
    ASSERT_EQ(AMETA_CAPS_LOCK_ON, mMapper->getMetaState());

    // Toggle num lock on.
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_NUMLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_NUMLOCK, 0);
    ASSERT_TRUE(capsLockLed);
    ASSERT_TRUE(numLockLed);
    ASSERT_FALSE(scrollLockLed);
    ASSERT_EQ(AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON, mMapper->getMetaState());

    // Toggle caps lock off.
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_CAPSLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_CAPSLOCK, 0);
    ASSERT_FALSE(capsLockLed);
    ASSERT_TRUE(numLockLed);
    ASSERT_FALSE(scrollLockLed);
    ASSERT_EQ(AMETA_NUM_LOCK_ON, mMapper->getMetaState());

    // Toggle scroll lock on.
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_SCROLLLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_FALSE(capsLockLed);
    ASSERT_TRUE(numLockLed);
    ASSERT_TRUE(scrollLockLed);
    ASSERT_EQ(AMETA_NUM_LOCK_ON | AMETA_SCROLL_LOCK_ON, mMapper->getMetaState());

    // Toggle num lock off.
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_NUMLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_NUMLOCK, 0);
    ASSERT_FALSE(capsLockLed);
    ASSERT_FALSE(numLockLed);
    ASSERT_TRUE(scrollLockLed);
    ASSERT_EQ(AMETA_SCROLL_LOCK_ON, mMapper->getMetaState());

    // Toggle scroll lock off.
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_SCROLLLOCK, 1);
    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_FALSE(capsLockLed);
    ASSERT_FALSE(numLockLed);
    ASSERT_FALSE(scrollLockLed);
    ASSERT_EQ(AMETA_NONE, mMapper->getMetaState());
}

TEST_F(KeyboardInputMapperUnitTest, DisablingDeviceResetsPressedKeys) {
    const int32_t USAGE_A = 0x070004;
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME, POLICY_FLAG_WAKE);
    addKeyByUsageCode(USAGE_A, AKEYCODE_A, POLICY_FLAG_WAKE);

    // Key down by evdev code.
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    NotifyKeyArgs args = expectSingleKeyArg(argsList);
    ASSERT_EQ(DEVICE_ID, args.deviceId);
    ASSERT_EQ(AINPUT_SOURCE_KEYBOARD, args.source);
    ASSERT_EQ(ARBITRARY_TIME, args.eventTime);
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(AKEYCODE_HOME, args.keyCode);
    ASSERT_EQ(KEY_HOME, args.scanCode);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM, args.flags);

    // Disable device, it should synthesize cancellation events for down events.
    mReaderConfiguration.disabledDevices.insert(DEVICE_ID);
    argsList = mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration,
                                    InputReaderConfiguration::Change::ENABLED_STATE);
    argsList += mMapper->reset(ARBITRARY_TIME);
    args = expectSingleKeyArg(argsList);
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(AKEYCODE_HOME, args.keyCode);
    ASSERT_EQ(KEY_HOME, args.scanCode);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM | AKEY_EVENT_FLAG_CANCELED, args.flags);
}

TEST_F(KeyboardInputMapperUnitTest, Configure_AssignKeyboardLayoutInfo) {
    std::list<NotifyArgs> unused =
            mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration, /*changes=*/{});

    int32_t generation = mDevice->getGeneration();
    mReaderConfiguration.keyboardLayoutAssociations.insert(
            {mIdentifier.location, DEVICE_KEYBOARD_LAYOUT_INFO});

    unused += mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration,
                                   InputReaderConfiguration::Change::KEYBOARD_LAYOUT_ASSOCIATION);

    InputDeviceInfo deviceInfo;
    mMapper->populateDeviceInfo(deviceInfo);
    ASSERT_EQ(DEVICE_KEYBOARD_LAYOUT_INFO.languageTag,
              deviceInfo.getKeyboardLayoutInfo()->languageTag);
    ASSERT_EQ(DEVICE_KEYBOARD_LAYOUT_INFO.layoutType,
              deviceInfo.getKeyboardLayoutInfo()->layoutType);
    ASSERT_GT(mDevice->getGeneration(), generation);

    // Call change layout association with the same values: Generation shouldn't change
    generation = mDevice->getGeneration();
    unused += mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration,
                                   InputReaderConfiguration::Change::KEYBOARD_LAYOUT_ASSOCIATION);
    ASSERT_EQ(mDevice->getGeneration(), generation);
}

TEST_F(KeyboardInputMapperUnitTest, LayoutInfoCorrectlyMapped) {
    EXPECT_CALL(mMockEventHub, getRawLayoutInfo(EVENTHUB_ID))
            .WillRepeatedly(Return(RawLayoutInfo{.languageTag = "en", .layoutType = "extended"}));

    // Configuration
    std::list<NotifyArgs> unused =
            mMapper->reconfigure(ARBITRARY_TIME, mReaderConfiguration, /*changes=*/{});

    InputDeviceInfo deviceInfo;
    mMapper->populateDeviceInfo(deviceInfo);
    ASSERT_EQ("en", deviceInfo.getKeyboardLayoutInfo()->languageTag);
    ASSERT_EQ("extended", deviceInfo.getKeyboardLayoutInfo()->layoutType);
}

TEST_F(KeyboardInputMapperUnitTest, Process_GestureEventToSetFlagKeepTouchMode) {
    addKeyByEvdevCode(KEY_LEFT, AKEYCODE_DPAD_LEFT, POLICY_FLAG_GESTURE);

    // Key down
    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_LEFT, 1);
    ASSERT_EQ(AKEY_EVENT_FLAG_FROM_SYSTEM | AKEY_EVENT_FLAG_KEEP_TOUCH_MODE,
              expectSingleKeyArg(argsList).flags);
}

// --- KeyboardInputMapperUnitTest_WakeFlagOverride ---

class KeyboardInputMapperUnitTest_WakeFlagOverride : public KeyboardInputMapperUnitTest {
protected:
    virtual void SetUp() override {
        SetUp(/*wakeFlag=*/com::android::input::flags::enable_alphabetic_keyboard_wake());
    }

    void SetUp(bool wakeFlag) {
        mWakeFlagInitialValue = com::android::input::flags::enable_alphabetic_keyboard_wake();
        com::android::input::flags::enable_alphabetic_keyboard_wake(wakeFlag);
        KeyboardInputMapperUnitTest::SetUp();
    }

    void TearDown() override {
        com::android::input::flags::enable_alphabetic_keyboard_wake(mWakeFlagInitialValue);
        KeyboardInputMapperUnitTest::TearDown();
    }

    bool mWakeFlagInitialValue;
};

// --- KeyboardInputMapperUnitTest_NonAlphabeticKeyboard_WakeFlagEnabled ---

class KeyboardInputMapperUnitTest_NonAlphabeticKeyboard_WakeFlagEnabled
      : public KeyboardInputMapperUnitTest_WakeFlagOverride {
protected:
    void SetUp() override {
        KeyboardInputMapperUnitTest_WakeFlagOverride::SetUp(/*wakeFlag=*/true);
    }
};

TEST_F(KeyboardInputMapperUnitTest_NonAlphabeticKeyboard_WakeFlagEnabled,
       NonAlphabeticDevice_WakeBehavior) {
    // For internal non-alphabetic devices keys will not trigger wake.

    addKeyByEvdevCode(KEY_A, AKEYCODE_A);
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME);
    addKeyByEvdevCode(KEY_PLAYPAUSE, AKEYCODE_MEDIA_PLAY_PAUSE);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_A, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_A, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAYPAUSE, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAYPAUSE, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));
}

// --- KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagEnabled ---

class KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagEnabled
      : public KeyboardInputMapperUnitTest_WakeFlagOverride {
protected:
    void SetUp() override {
        KeyboardInputMapperUnitTest_WakeFlagOverride::SetUp(/*wakeFlag=*/true);

        ON_CALL((*mDevice), getKeyboardType).WillByDefault(Return(KeyboardType::ALPHABETIC));
    }
};

TEST_F(KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagEnabled, WakeBehavior) {
    // For internal alphabetic devices, keys will trigger wake on key down when
    // flag is enabled.
    addKeyByEvdevCode(KEY_A, AKEYCODE_A);
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME);
    addKeyByEvdevCode(KEY_PLAYPAUSE, AKEYCODE_MEDIA_PLAY_PAUSE);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_A, 1);
    EXPECT_THAT(argsList,
                ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(POLICY_FLAG_WAKE))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_A, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    EXPECT_THAT(argsList,
                ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(POLICY_FLAG_WAKE))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAYPAUSE, 1);
    EXPECT_THAT(argsList,
                ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(POLICY_FLAG_WAKE))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAYPAUSE, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));
}

TEST_F(KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagEnabled, WakeBehavior_UnknownKey) {
    // For internal alphabetic devices, unknown keys will trigger wake on key down when
    // flag is enabled.

    const int32_t USAGE_UNKNOWN = 0x07ffff;
    EXPECT_CALL(mMockEventHub, mapKey(EVENTHUB_ID, KEY_UNKNOWN, USAGE_UNKNOWN, _, _, _, _))
            .WillRepeatedly(Return(NAME_NOT_FOUND));

    // Key down with unknown scan code or usage code.
    std::list<NotifyArgs> argsList = process(ARBITRARY_TIME, EV_MSC, MSC_SCAN, USAGE_UNKNOWN);
    argsList += process(ARBITRARY_TIME, EV_KEY, KEY_UNKNOWN, 1);
    EXPECT_THAT(argsList,
                ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(POLICY_FLAG_WAKE))));

    // Key up with unknown scan code or usage code.
    argsList = process(ARBITRARY_TIME, EV_MSC, MSC_SCAN, USAGE_UNKNOWN);
    argsList += process(ARBITRARY_TIME + 1, EV_KEY, KEY_UNKNOWN, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));
}

// --- KeyboardInputMapperUnitTest_AlphabeticDevice_AlphabeticKeyboardWakeDisabled ---

class KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagDisabled
      : public KeyboardInputMapperUnitTest_WakeFlagOverride {
protected:
    void SetUp() override {
        KeyboardInputMapperUnitTest_WakeFlagOverride::SetUp(/*wakeFlag=*/false);

        ON_CALL((*mDevice), getKeyboardType).WillByDefault(Return(KeyboardType::ALPHABETIC));
    }
};

TEST_F(KeyboardInputMapperUnitTest_AlphabeticKeyboard_WakeFlagDisabled, WakeBehavior) {
    // For internal alphabetic devices, keys will not trigger wake when flag is
    // disabled.

    addKeyByEvdevCode(KEY_A, AKEYCODE_A);
    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME);
    addKeyByEvdevCode(KEY_PLAYPAUSE, AKEYCODE_MEDIA_PLAY_PAUSE);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_A, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_A, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAYPAUSE, 1);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAYPAUSE, 0);
    EXPECT_THAT(argsList, ElementsAre(VariantWith<NotifyKeyArgs>(WithPolicyFlags(0U))));
}

// --- KeyboardInputMapperTest ---

// TODO(b/283812079): convert the tests for this class, which use multiple mappers each, to use
//  InputMapperUnitTest.
class KeyboardInputMapperTest : public InputMapperTest {
protected:
    void SetUp() override {
        InputMapperTest::SetUp(DEVICE_CLASSES | InputDeviceClass::KEYBOARD |
                               InputDeviceClass::ALPHAKEY);
    }
    const std::string UNIQUE_ID = "local:0";

    void testDPadKeyRotation(KeyboardInputMapper& mapper, int32_t originalEvdevCode,
                             int32_t rotatedKeyCode,
                             ui::LogicalDisplayId displayId = ui::LogicalDisplayId::INVALID);

    void processKeyAndSync(InputMapper& mapper, nsecs_t when, nsecs_t readTime, int32_t code,
                           int32_t value) {
        process(mapper, when, readTime, EV_KEY, code, value);
        process(mapper, when, readTime, EV_SYN, SYN_REPORT, 0);
    }
};

void KeyboardInputMapperTest::testDPadKeyRotation(KeyboardInputMapper& mapper,
                                                  int32_t originalEvdevCode, int32_t rotatedKeyCode,
                                                  ui::LogicalDisplayId displayId) {
    NotifyKeyArgs args;

    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, originalEvdevCode, 1);
    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(&args));
    ASSERT_EQ(AKEY_EVENT_ACTION_DOWN, args.action);
    ASSERT_EQ(originalEvdevCode, args.scanCode);
    ASSERT_EQ(rotatedKeyCode, args.keyCode);
    ASSERT_EQ(displayId, args.displayId);

    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, originalEvdevCode, 0);
    ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(&args));
    ASSERT_EQ(AKEY_EVENT_ACTION_UP, args.action);
    ASSERT_EQ(originalEvdevCode, args.scanCode);
    ASSERT_EQ(rotatedKeyCode, args.keyCode);
    ASSERT_EQ(displayId, args.displayId);
}

TEST_F(KeyboardInputMapperTest, Configure_AssignsDisplayPort) {
    // keyboard 1.
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_UP, 0, AKEYCODE_DPAD_UP, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_RIGHT, 0, AKEYCODE_DPAD_RIGHT, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_DOWN, 0, AKEYCODE_DPAD_DOWN, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_LEFT, 0, AKEYCODE_DPAD_LEFT, 0);

    // keyboard 2.
    const std::string USB2 = "USB2";
    const std::string DEVICE_NAME2 = "KEYBOARD2";
    constexpr int32_t SECOND_DEVICE_ID = DEVICE_ID + 1;
    constexpr int32_t SECOND_EVENTHUB_ID = EVENTHUB_ID + 1;
    std::shared_ptr<InputDevice> device2 =
            newDevice(SECOND_DEVICE_ID, DEVICE_NAME2, USB2, SECOND_EVENTHUB_ID,
                      ftl::Flags<InputDeviceClass>(0));

    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_UP, 0, AKEYCODE_DPAD_UP, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_RIGHT, 0, AKEYCODE_DPAD_RIGHT, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_DOWN, 0, AKEYCODE_DPAD_DOWN, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_LEFT, 0, AKEYCODE_DPAD_LEFT, 0);

    KeyboardInputMapper& mapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);

    device2->addEmptyEventHubDevice(SECOND_EVENTHUB_ID);
    KeyboardInputMapper& mapper2 =
            device2->constructAndAddMapper<KeyboardInputMapper>(SECOND_EVENTHUB_ID,
                                                                mFakePolicy
                                                                        ->getReaderConfiguration(),
                                                                AINPUT_SOURCE_KEYBOARD);
    std::list<NotifyArgs> unused =
            device2->configure(ARBITRARY_TIME, mFakePolicy->getReaderConfiguration(),
                    /*changes=*/{});
    unused += device2->reset(ARBITRARY_TIME);

    // Prepared displays and associated info.
    constexpr uint8_t hdmi1 = 0;
    constexpr uint8_t hdmi2 = 1;
    const std::string SECONDARY_UNIQUE_ID = "local:1";

    mFakePolicy->addInputPortAssociation(DEVICE_LOCATION, hdmi1);
    mFakePolicy->addInputPortAssociation(USB2, hdmi2);

    // No associated display viewport found, should disable the device.
    unused += device2->configure(ARBITRARY_TIME, mFakePolicy->getReaderConfiguration(),
                                 InputReaderConfiguration::Change::DISPLAY_INFO);
    ASSERT_FALSE(device2->isEnabled());

    // Prepare second display.
    constexpr ui::LogicalDisplayId newDisplayId = ui::LogicalDisplayId{2};
    setDisplayInfoAndReconfigure(DISPLAY_ID, DISPLAY_WIDTH, DISPLAY_HEIGHT, ui::ROTATION_0,
                                 UNIQUE_ID, hdmi1, ViewportType::INTERNAL);
    setDisplayInfoAndReconfigure(newDisplayId, DISPLAY_WIDTH, DISPLAY_HEIGHT, ui::ROTATION_0,
                                 SECONDARY_UNIQUE_ID, hdmi2, ViewportType::EXTERNAL);
    // Default device will reconfigure above, need additional reconfiguration for another device.
    unused += device2->configure(ARBITRARY_TIME, mFakePolicy->getReaderConfiguration(),
                                 InputReaderConfiguration::Change::DISPLAY_INFO);

    // Device should be enabled after the associated display is found.
    ASSERT_TRUE(mDevice->isEnabled());
    ASSERT_TRUE(device2->isEnabled());

    // Test pad key events
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(mapper, KEY_UP, AKEYCODE_DPAD_UP, DISPLAY_ID));
    ASSERT_NO_FATAL_FAILURE(
            testDPadKeyRotation(mapper, KEY_RIGHT, AKEYCODE_DPAD_RIGHT, DISPLAY_ID));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(mapper, KEY_DOWN, AKEYCODE_DPAD_DOWN, DISPLAY_ID));
    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(mapper, KEY_LEFT, AKEYCODE_DPAD_LEFT, DISPLAY_ID));

    ASSERT_NO_FATAL_FAILURE(testDPadKeyRotation(mapper2, KEY_UP, AKEYCODE_DPAD_UP, newDisplayId));
    ASSERT_NO_FATAL_FAILURE(
            testDPadKeyRotation(mapper2, KEY_RIGHT, AKEYCODE_DPAD_RIGHT, newDisplayId));
    ASSERT_NO_FATAL_FAILURE(
            testDPadKeyRotation(mapper2, KEY_DOWN, AKEYCODE_DPAD_DOWN, newDisplayId));
    ASSERT_NO_FATAL_FAILURE(
            testDPadKeyRotation(mapper2, KEY_LEFT, AKEYCODE_DPAD_LEFT, newDisplayId));
}

TEST_F(KeyboardInputMapperTest, Process_LockedKeysShouldToggleAfterReattach) {
    mFakeEventHub->addLed(EVENTHUB_ID, LED_CAPSL, true /*initially on*/);
    mFakeEventHub->addLed(EVENTHUB_ID, LED_NUML, false /*initially off*/);
    mFakeEventHub->addLed(EVENTHUB_ID, LED_SCROLLL, false /*initially off*/);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    KeyboardInputMapper& mapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);
    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mapper.getMetaState());

    // Initialization should have turned all of the lights off.
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_CAPSL));
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_NUML));
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_SCROLLL));

    // Toggle caps lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_CAPSL));
    ASSERT_EQ(AMETA_CAPS_LOCK_ON, mapper.getMetaState());

    // Toggle num lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_NUML));
    ASSERT_EQ(AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON, mapper.getMetaState());

    // Toggle scroll lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_SCROLLL));
    ASSERT_EQ(AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON | AMETA_SCROLL_LOCK_ON, mapper.getMetaState());

    mFakeEventHub->removeDevice(EVENTHUB_ID);
    mReader->loopOnce();

    // keyboard 2 should default toggle keys.
    const std::string USB2 = "USB2";
    const std::string DEVICE_NAME2 = "KEYBOARD2";
    constexpr int32_t SECOND_DEVICE_ID = DEVICE_ID + 1;
    constexpr int32_t SECOND_EVENTHUB_ID = EVENTHUB_ID + 1;
    std::shared_ptr<InputDevice> device2 =
            newDevice(SECOND_DEVICE_ID, DEVICE_NAME2, USB2, SECOND_EVENTHUB_ID,
                      ftl::Flags<InputDeviceClass>(0));
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_CAPSL, true /*initially on*/);
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_NUML, false /*initially off*/);
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_SCROLLL, false /*initially off*/);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    device2->addEmptyEventHubDevice(SECOND_EVENTHUB_ID);
    KeyboardInputMapper& mapper2 =
            device2->constructAndAddMapper<KeyboardInputMapper>(SECOND_EVENTHUB_ID,
                                                                mFakePolicy
                                                                        ->getReaderConfiguration(),
                                                                AINPUT_SOURCE_KEYBOARD);
    std::list<NotifyArgs> unused =
            device2->configure(ARBITRARY_TIME, mFakePolicy->getReaderConfiguration(),
                    /*changes=*/{});
    unused += device2->reset(ARBITRARY_TIME);

    ASSERT_TRUE(mFakeEventHub->getLedState(SECOND_EVENTHUB_ID, LED_CAPSL));
    ASSERT_TRUE(mFakeEventHub->getLedState(SECOND_EVENTHUB_ID, LED_NUML));
    ASSERT_TRUE(mFakeEventHub->getLedState(SECOND_EVENTHUB_ID, LED_SCROLLL));
    ASSERT_EQ(AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON | AMETA_SCROLL_LOCK_ON,
              mapper2.getMetaState());
}

TEST_F(KeyboardInputMapperTest, Process_toggleCapsLockState) {
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    // Suppose we have two mappers. (DPAD + KEYBOARD)
    constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_DPAD);
    KeyboardInputMapper& mapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);
    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mapper.getMetaState());

    mReader->toggleCapsLockState(DEVICE_ID);
    ASSERT_EQ(AMETA_CAPS_LOCK_ON, mapper.getMetaState());
}

TEST_F(KeyboardInputMapperTest, Process_ResetLockedModifierState) {
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    KeyboardInputMapper& mapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);
    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mapper.getMetaState());

    // Toggle caps lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 0);

    // Toggle num lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 0);

    // Toggle scroll lock on.
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 1);
    processKeyAndSync(mapper, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_EQ(AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON | AMETA_SCROLL_LOCK_ON, mapper.getMetaState());

    mReader->resetLockedModifierState();
    ASSERT_EQ(AMETA_NONE, mapper.getMetaState());
}

TEST_F(KeyboardInputMapperTest, Process_LockedKeysShouldToggleInMultiDevices) {
    // keyboard 1.
    mFakeEventHub->addLed(EVENTHUB_ID, LED_CAPSL, true /*initially on*/);
    mFakeEventHub->addLed(EVENTHUB_ID, LED_NUML, false /*initially off*/);
    mFakeEventHub->addLed(EVENTHUB_ID, LED_SCROLLL, false /*initially off*/);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    KeyboardInputMapper& mapper1 =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);

    // keyboard 2.
    const std::string USB2 = "USB2";
    const std::string DEVICE_NAME2 = "KEYBOARD2";
    constexpr int32_t SECOND_DEVICE_ID = DEVICE_ID + 1;
    constexpr int32_t SECOND_EVENTHUB_ID = EVENTHUB_ID + 1;
    std::shared_ptr<InputDevice> device2 =
            newDevice(SECOND_DEVICE_ID, DEVICE_NAME2, USB2, SECOND_EVENTHUB_ID,
                      ftl::Flags<InputDeviceClass>(0));
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_CAPSL, true /*initially on*/);
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_NUML, false /*initially off*/);
    mFakeEventHub->addLed(SECOND_EVENTHUB_ID, LED_SCROLLL, false /*initially off*/);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_CAPSLOCK, 0, AKEYCODE_CAPS_LOCK, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_NUMLOCK, 0, AKEYCODE_NUM_LOCK, 0);
    mFakeEventHub->addKey(SECOND_EVENTHUB_ID, KEY_SCROLLLOCK, 0, AKEYCODE_SCROLL_LOCK, 0);

    device2->addEmptyEventHubDevice(SECOND_EVENTHUB_ID);
    KeyboardInputMapper& mapper2 =
            device2->constructAndAddMapper<KeyboardInputMapper>(SECOND_EVENTHUB_ID,
                                                                mFakePolicy
                                                                        ->getReaderConfiguration(),
                                                                AINPUT_SOURCE_KEYBOARD);
    std::list<NotifyArgs> unused =
            device2->configure(ARBITRARY_TIME, mFakePolicy->getReaderConfiguration(),
                    /*changes=*/{});
    unused += device2->reset(ARBITRARY_TIME);

    // Initial metastate is AMETA_NONE.
    ASSERT_EQ(AMETA_NONE, mapper1.getMetaState());
    ASSERT_EQ(AMETA_NONE, mapper2.getMetaState());

    // Toggle num lock on and off.
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_NUML));
    ASSERT_EQ(AMETA_NUM_LOCK_ON, mapper1.getMetaState());
    ASSERT_EQ(AMETA_NUM_LOCK_ON, mapper2.getMetaState());

    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_NUMLOCK, 0);
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_NUML));
    ASSERT_EQ(AMETA_NONE, mapper1.getMetaState());
    ASSERT_EQ(AMETA_NONE, mapper2.getMetaState());

    // Toggle caps lock on and off.
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_CAPSL));
    ASSERT_EQ(AMETA_CAPS_LOCK_ON, mapper1.getMetaState());
    ASSERT_EQ(AMETA_CAPS_LOCK_ON, mapper2.getMetaState());

    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_CAPSLOCK, 0);
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_CAPSL));
    ASSERT_EQ(AMETA_NONE, mapper1.getMetaState());
    ASSERT_EQ(AMETA_NONE, mapper2.getMetaState());

    // Toggle scroll lock on and off.
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_TRUE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_SCROLLL));
    ASSERT_EQ(AMETA_SCROLL_LOCK_ON, mapper1.getMetaState());
    ASSERT_EQ(AMETA_SCROLL_LOCK_ON, mapper2.getMetaState());

    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 1);
    processKeyAndSync(mapper1, ARBITRARY_TIME, READ_TIME, KEY_SCROLLLOCK, 0);
    ASSERT_FALSE(mFakeEventHub->getLedState(EVENTHUB_ID, LED_SCROLLL));
    ASSERT_EQ(AMETA_NONE, mapper1.getMetaState());
    ASSERT_EQ(AMETA_NONE, mapper2.getMetaState());
}

/**
 * When there is more than one KeyboardInputMapper for an InputDevice, each mapper should produce
 * events that use the shared keyboard source across all mappers. This is to ensure that each
 * input device generates key events in a consistent manner, regardless of which mapper produces
 * the event.
 */
TEST_F(KeyboardInputMapperTest, UsesSharedKeyboardSource) {
    mFakeEventHub->addKey(EVENTHUB_ID, KEY_HOME, 0, AKEYCODE_HOME, POLICY_FLAG_WAKE);

    // Add a mapper with SOURCE_KEYBOARD
    KeyboardInputMapper& keyboardMapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_KEYBOARD);

    processKeyAndSync(keyboardMapper, ARBITRARY_TIME, 0, KEY_HOME, 1);
    ASSERT_NO_FATAL_FAILURE(
            mFakeListener->assertNotifyKeyWasCalled(WithSource(AINPUT_SOURCE_KEYBOARD)));
    processKeyAndSync(keyboardMapper, ARBITRARY_TIME, 0, KEY_HOME, 0);
    ASSERT_NO_FATAL_FAILURE(
            mFakeListener->assertNotifyKeyWasCalled(WithSource(AINPUT_SOURCE_KEYBOARD)));

    // Add a mapper with SOURCE_DPAD
    KeyboardInputMapper& dpadMapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_DPAD);
    for (auto* mapper : {&keyboardMapper, &dpadMapper}) {
        processKeyAndSync(*mapper, ARBITRARY_TIME, 0, KEY_HOME, 1);
        ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(
                WithSource(AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_DPAD)));
        processKeyAndSync(*mapper, ARBITRARY_TIME, 0, KEY_HOME, 0);
        ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(
                WithSource(AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_DPAD)));
    }

    // Add a mapper with SOURCE_GAMEPAD
    KeyboardInputMapper& gamepadMapper =
            constructAndAddMapper<KeyboardInputMapper>(AINPUT_SOURCE_GAMEPAD);
    for (auto* mapper : {&keyboardMapper, &dpadMapper, &gamepadMapper}) {
        processKeyAndSync(*mapper, ARBITRARY_TIME, 0, KEY_HOME, 1);
        ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(
                WithSource(AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_DPAD | AINPUT_SOURCE_GAMEPAD)));
        processKeyAndSync(*mapper, ARBITRARY_TIME, 0, KEY_HOME, 0);
        ASSERT_NO_FATAL_FAILURE(mFakeListener->assertNotifyKeyWasCalled(
                WithSource(AINPUT_SOURCE_KEYBOARD | AINPUT_SOURCE_DPAD | AINPUT_SOURCE_GAMEPAD)));
    }
}

// --- KeyboardInputMapperTest_ExternalAlphabeticDevice ---

class KeyboardInputMapperTest_ExternalAlphabeticDevice : public KeyboardInputMapperUnitTest {
protected:
    void SetUp() override {
        InputMapperUnitTest::SetUp(/*bus=*/0, /*isExternal=*/true);
        ON_CALL((*mDevice), getSources).WillByDefault(Return(AINPUT_SOURCE_KEYBOARD));
        ON_CALL((*mDevice), getKeyboardType).WillByDefault(Return(KeyboardType::ALPHABETIC));
        EXPECT_CALL(mMockEventHub, getDeviceClasses(EVENTHUB_ID))
                .WillRepeatedly(Return(InputDeviceClass::KEYBOARD | InputDeviceClass::ALPHAKEY |
                                       InputDeviceClass::EXTERNAL));
        mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                         AINPUT_SOURCE_KEYBOARD);
    }
};

// --- KeyboardInputMapperTest_ExternalNonAlphabeticDevice ---

class KeyboardInputMapperTest_ExternalNonAlphabeticDevice : public KeyboardInputMapperUnitTest {
protected:
    void SetUp() override {
        InputMapperUnitTest::SetUp(/*bus=*/0, /*isExternal=*/true);
        ON_CALL((*mDevice), getSources).WillByDefault(Return(AINPUT_SOURCE_KEYBOARD));
        ON_CALL((*mDevice), getKeyboardType).WillByDefault(Return(KeyboardType::NON_ALPHABETIC));
        EXPECT_CALL(mMockEventHub, getDeviceClasses(EVENTHUB_ID))
                .WillRepeatedly(Return(InputDeviceClass::KEYBOARD | InputDeviceClass::EXTERNAL));
        mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                         AINPUT_SOURCE_KEYBOARD);
    }
};

TEST_F(KeyboardInputMapperTest_ExternalAlphabeticDevice, WakeBehavior_AlphabeticKeyboard) {
    // For external devices, keys will trigger wake on key down. Media keys should also trigger
    // wake if triggered from external devices.

    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME);
    addKeyByEvdevCode(KEY_PLAY, AKEYCODE_MEDIA_PLAY);
    addKeyByEvdevCode(KEY_PLAYPAUSE, AKEYCODE_MEDIA_PLAY_PAUSE, POLICY_FLAG_WAKE);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAY, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAY, 0);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAYPAUSE, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAYPAUSE, 0);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);
}

TEST_F(KeyboardInputMapperTest_ExternalNonAlphabeticDevice, WakeBehavior_NonAlphabeticKeyboard) {
    // For external devices, keys will trigger wake on key down. Media keys should not trigger
    // wake if triggered from external non-alphaebtic keyboard (e.g. headsets).

    addKeyByEvdevCode(KEY_PLAY, AKEYCODE_MEDIA_PLAY);
    addKeyByEvdevCode(KEY_PLAYPAUSE, AKEYCODE_MEDIA_PLAY_PAUSE, POLICY_FLAG_WAKE);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAY, 1);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAY, 0);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAYPAUSE, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAYPAUSE, 0);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);
}

TEST_F(KeyboardInputMapperTest_ExternalAlphabeticDevice, DoNotWakeByDefaultBehavior) {
    // Tv Remote key's wake behavior is prescribed by the keylayout file.

    addKeyByEvdevCode(KEY_HOME, AKEYCODE_HOME, POLICY_FLAG_WAKE);
    addKeyByEvdevCode(KEY_DOWN, AKEYCODE_DPAD_DOWN);
    addKeyByEvdevCode(KEY_PLAY, AKEYCODE_MEDIA_PLAY, POLICY_FLAG_WAKE);

    mPropertyMap.addProperty("keyboard.doNotWakeByDefault", "1");
    mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                     AINPUT_SOURCE_KEYBOARD);

    std::list<NotifyArgs> argsList = processKeyAndSync(ARBITRARY_TIME, KEY_HOME, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_HOME, 0);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_DOWN, 1);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_DOWN, 0);
    ASSERT_EQ(uint32_t(0), expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME, KEY_PLAY, 1);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);

    argsList = processKeyAndSync(ARBITRARY_TIME + 1, KEY_PLAY, 0);
    ASSERT_EQ(POLICY_FLAG_WAKE, expectSingleKeyArg(argsList).policyFlags);
}

} // namespace android
