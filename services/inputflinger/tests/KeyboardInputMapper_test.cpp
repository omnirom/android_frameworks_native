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

#include <gtest/gtest.h>

#include "InputMapperTest.h"
#include "InterfaceMocks.h"

#define TAG "KeyboardInputMapper_test"

namespace android {

using testing::_;
using testing::Args;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;

/**
 * Unit tests for KeyboardInputMapper.
 */
class KeyboardInputMapperUnitTest : public InputMapperUnitTest {
protected:
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
        for (const auto& [scanCode, outKeycode] : mKeyCodeMap) {
            EXPECT_CALL(mMockEventHub, mapKey(EVENTHUB_ID, scanCode, _, _, _, _, _))
                    .WillRepeatedly(DoAll(SetArgPointee<4>(outKeycode), Return(NO_ERROR)));
        }

        mFakePolicy = sp<FakeInputReaderPolicy>::make();
        EXPECT_CALL(mMockInputReaderContext, getPolicy).WillRepeatedly(Return(mFakePolicy.get()));

        ON_CALL((*mDevice), getSources).WillByDefault(Return(AINPUT_SOURCE_KEYBOARD));

        mMapper = createInputMapper<KeyboardInputMapper>(*mDeviceContext, mReaderConfiguration,
                                                         AINPUT_SOURCE_KEYBOARD);
    }
};

TEST_F(KeyboardInputMapperUnitTest, KeyPressTimestampRecorded) {
    nsecs_t when = ARBITRARY_TIME;
    std::vector<int32_t> keyCodes{KEY_0, KEY_A, KEY_LEFTCTRL, KEY_RIGHTALT, KEY_LEFTSHIFT};
    EXPECT_CALL(mMockInputReaderContext, setLastKeyDownTimestamp)
            .With(Args<0>(when))
            .Times(keyCodes.size());
    for (int32_t keyCode : keyCodes) {
        process(when, EV_KEY, keyCode, 1);
        process(when, EV_SYN, SYN_REPORT, 0);
        process(when, EV_KEY, keyCode, 0);
        process(when, EV_SYN, SYN_REPORT, 0);
    }
}

} // namespace android
