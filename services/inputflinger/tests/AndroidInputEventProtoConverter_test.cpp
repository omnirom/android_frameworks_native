/*
 * Copyright 2025 The Android Open Source Project
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

#include "../dispatcher/trace/AndroidInputEventProtoConverter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace android::inputdispatcher::trace {

namespace {

using testing::Return, testing::_;

class MockProtoAxisValue {
public:
    MOCK_METHOD(void, set_axis, (int32_t));
    MOCK_METHOD(void, set_value, (float));
};

class MockProtoPointer {
public:
    MOCK_METHOD(void, set_pointer_id, (uint32_t));
    MOCK_METHOD(void, set_tool_type, (int32_t));
    MOCK_METHOD(MockProtoAxisValue*, add_axis_value, ());
};

class MockProtoMotion {
public:
    MOCK_METHOD(void, set_event_id, (uint32_t));
    MOCK_METHOD(void, set_event_time_nanos, (int64_t));
    MOCK_METHOD(void, set_down_time_nanos, (int64_t));
    MOCK_METHOD(void, set_source, (uint32_t));
    MOCK_METHOD(void, set_action, (int32_t));
    MOCK_METHOD(void, set_device_id, (uint32_t));
    MOCK_METHOD(void, set_display_id, (uint32_t));
    MOCK_METHOD(void, set_classification, (int32_t));
    MOCK_METHOD(void, set_flags, (uint32_t));
    MOCK_METHOD(void, set_policy_flags, (uint32_t));
    MOCK_METHOD(void, set_button_state, (uint32_t));
    MOCK_METHOD(void, set_action_button, (uint32_t));
    MOCK_METHOD(void, set_cursor_position_x, (float));
    MOCK_METHOD(void, set_cursor_position_y, (float));
    MOCK_METHOD(void, set_meta_state, (uint32_t));
    MOCK_METHOD(void, set_precision_x, (float));
    MOCK_METHOD(void, set_precision_y, (float));
    MOCK_METHOD(MockProtoPointer*, add_pointer, ());
};

class MockProtoKey {
public:
    MOCK_METHOD(void, set_event_id, (uint32_t));
    MOCK_METHOD(void, set_event_time_nanos, (int64_t));
    MOCK_METHOD(void, set_down_time_nanos, (int64_t));
    MOCK_METHOD(void, set_source, (uint32_t));
    MOCK_METHOD(void, set_action, (int32_t));
    MOCK_METHOD(void, set_device_id, (uint32_t));
    MOCK_METHOD(void, set_display_id, (uint32_t));
    MOCK_METHOD(void, set_repeat_count, (uint32_t));
    MOCK_METHOD(void, set_flags, (uint32_t));
    MOCK_METHOD(void, set_policy_flags, (uint32_t));
    MOCK_METHOD(void, set_key_code, (uint32_t));
    MOCK_METHOD(void, set_scan_code, (uint32_t));
    MOCK_METHOD(void, set_meta_state, (uint32_t));
};

class MockProtoDispatchPointer {
public:
    MOCK_METHOD(void, set_pointer_id, (uint32_t));
    MOCK_METHOD(void, set_x_in_display, (float));
    MOCK_METHOD(void, set_y_in_display, (float));
    MOCK_METHOD(MockProtoAxisValue*, add_axis_value_in_window, ());
};

class MockProtoDispatch {
public:
    MOCK_METHOD(void, set_event_id, (uint32_t));
    MOCK_METHOD(void, set_vsync_id, (uint32_t));
    MOCK_METHOD(void, set_window_id, (uint32_t));
    MOCK_METHOD(void, set_resolved_flags, (uint32_t));
    MOCK_METHOD(MockProtoDispatchPointer*, add_dispatched_pointer, ());
};

using TestProtoConverter =
        AndroidInputEventProtoConverter<MockProtoMotion, MockProtoKey, MockProtoDispatch,
                                        proto::AndroidInputEventConfig::Decoder>;

TEST(AndroidInputEventProtoConverterTest, ToProtoMotionEvent) {
    TracedMotionEvent event{};
    event.id = 1;
    event.eventTime = 2;
    event.downTime = 3;
    event.source = AINPUT_SOURCE_MOUSE;
    event.action = AMOTION_EVENT_ACTION_BUTTON_PRESS;
    event.deviceId = 4;
    event.displayId = ui::LogicalDisplayId(5);
    event.classification = MotionClassification::PINCH;
    event.flags = 6;
    event.policyFlags = 7;
    event.buttonState = 8;
    event.actionButton = 9;
    event.xCursorPosition = 10.0f;
    event.yCursorPosition = 11.0f;
    event.metaState = 12;
    event.xPrecision = 13.0f;
    event.yPrecision = 14.0f;
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 15,
            .toolType = ToolType::MOUSE,
    });
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 16,
            .toolType = ToolType::FINGER,
    });
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 17.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 18.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 19.0f);
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 20.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 21.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 22.0f);

    testing::StrictMock<MockProtoMotion> proto;
    testing::StrictMock<MockProtoPointer> pointer1;
    testing::StrictMock<MockProtoPointer> pointer2;
    testing::StrictMock<MockProtoAxisValue> axisValue1;
    testing::StrictMock<MockProtoAxisValue> axisValue2;
    testing::StrictMock<MockProtoAxisValue> axisValue3;
    testing::StrictMock<MockProtoAxisValue> axisValue4;
    testing::StrictMock<MockProtoAxisValue> axisValue5;
    testing::StrictMock<MockProtoAxisValue> axisValue6;

    EXPECT_CALL(proto, set_event_id(1));
    EXPECT_CALL(proto, set_event_time_nanos(2));
    EXPECT_CALL(proto, set_down_time_nanos(3));
    EXPECT_CALL(proto, set_source(AINPUT_SOURCE_MOUSE));
    EXPECT_CALL(proto, set_action(AMOTION_EVENT_ACTION_BUTTON_PRESS));
    EXPECT_CALL(proto, set_device_id(4));
    EXPECT_CALL(proto, set_display_id(5));
    EXPECT_CALL(proto, set_classification(AMOTION_EVENT_CLASSIFICATION_PINCH));
    EXPECT_CALL(proto, set_flags(6));
    EXPECT_CALL(proto, set_policy_flags(7));
    EXPECT_CALL(proto, set_button_state(8));
    EXPECT_CALL(proto, set_action_button(9));
    EXPECT_CALL(proto, set_cursor_position_x(10.0f));
    EXPECT_CALL(proto, set_cursor_position_y(11.0f));
    EXPECT_CALL(proto, set_meta_state(12));
    EXPECT_CALL(proto, set_precision_x(13.0f));
    EXPECT_CALL(proto, set_precision_y(14.0f));

    EXPECT_CALL(proto, add_pointer()).WillOnce(Return(&pointer1)).WillOnce(Return(&pointer2));

    EXPECT_CALL(pointer1, set_pointer_id(15));
    EXPECT_CALL(pointer1, set_tool_type(AMOTION_EVENT_TOOL_TYPE_MOUSE));
    EXPECT_CALL(pointer1, add_axis_value())
            .WillOnce(Return(&axisValue1))
            .WillOnce(Return(&axisValue2))
            .WillOnce(Return(&axisValue3));
    EXPECT_CALL(axisValue1, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue1, set_value(17.0f));
    EXPECT_CALL(axisValue2, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue2, set_value(18.0f));
    EXPECT_CALL(axisValue3, set_axis(AMOTION_EVENT_AXIS_PRESSURE));
    EXPECT_CALL(axisValue3, set_value(19.0f));

    EXPECT_CALL(pointer2, set_pointer_id(16));
    EXPECT_CALL(pointer2, set_tool_type(AMOTION_EVENT_TOOL_TYPE_FINGER));
    EXPECT_CALL(pointer2, add_axis_value())
            .WillOnce(Return(&axisValue4))
            .WillOnce(Return(&axisValue5))
            .WillOnce(Return(&axisValue6));
    EXPECT_CALL(axisValue4, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue4, set_value(20.0f));
    EXPECT_CALL(axisValue5, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue5, set_value(21.0f));
    EXPECT_CALL(axisValue6, set_axis(AMOTION_EVENT_AXIS_PRESSURE));
    EXPECT_CALL(axisValue6, set_value(22.0f));

    TestProtoConverter::toProtoMotionEvent(event, proto, /*isRedacted=*/false);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoMotionEvent_Redacted) {
    TracedMotionEvent event{};
    event.id = 1;
    event.eventTime = 2;
    event.downTime = 3;
    event.source = AINPUT_SOURCE_MOUSE;
    event.action = AMOTION_EVENT_ACTION_BUTTON_PRESS;
    event.deviceId = 4;
    event.displayId = ui::LogicalDisplayId(5);
    event.classification = MotionClassification::PINCH;
    event.flags = 6;
    event.policyFlags = 7;
    event.buttonState = 8;
    event.actionButton = 9;
    event.xCursorPosition = 10.0f;
    event.yCursorPosition = 11.0f;
    event.metaState = 12;
    event.xPrecision = 13.0f;
    event.yPrecision = 14.0f;
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 15,
            .toolType = ToolType::MOUSE,
    });
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 16,
            .toolType = ToolType::FINGER,
    });
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 17.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 18.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 19.0f);
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 20.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 21.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 22.0f);

    testing::StrictMock<MockProtoMotion> proto;
    testing::StrictMock<MockProtoPointer> pointer1;
    testing::StrictMock<MockProtoPointer> pointer2;
    testing::StrictMock<MockProtoAxisValue> axisValue1;
    testing::StrictMock<MockProtoAxisValue> axisValue2;
    testing::StrictMock<MockProtoAxisValue> axisValue3;
    testing::StrictMock<MockProtoAxisValue> axisValue4;
    testing::StrictMock<MockProtoAxisValue> axisValue5;
    testing::StrictMock<MockProtoAxisValue> axisValue6;

    EXPECT_CALL(proto, set_event_id(1));
    EXPECT_CALL(proto, set_event_time_nanos(2));
    EXPECT_CALL(proto, set_down_time_nanos(3));
    EXPECT_CALL(proto, set_source(AINPUT_SOURCE_MOUSE));
    EXPECT_CALL(proto, set_action(AMOTION_EVENT_ACTION_BUTTON_PRESS));
    EXPECT_CALL(proto, set_device_id(4));
    EXPECT_CALL(proto, set_display_id(5));
    EXPECT_CALL(proto, set_classification(AMOTION_EVENT_CLASSIFICATION_PINCH));
    EXPECT_CALL(proto, set_flags(6));
    EXPECT_CALL(proto, set_policy_flags(7));
    EXPECT_CALL(proto, set_button_state(8));
    EXPECT_CALL(proto, set_action_button(9));

    EXPECT_CALL(proto, add_pointer()).WillOnce(Return(&pointer1)).WillOnce(Return(&pointer2));

    EXPECT_CALL(pointer1, set_pointer_id(15));
    EXPECT_CALL(pointer1, set_tool_type(AMOTION_EVENT_TOOL_TYPE_MOUSE));
    EXPECT_CALL(pointer1, add_axis_value())
            .WillOnce(Return(&axisValue1))
            .WillOnce(Return(&axisValue2))
            .WillOnce(Return(&axisValue3));
    EXPECT_CALL(axisValue1, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue2, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue3, set_axis(AMOTION_EVENT_AXIS_PRESSURE));

    EXPECT_CALL(pointer2, set_pointer_id(16));
    EXPECT_CALL(pointer2, set_tool_type(AMOTION_EVENT_TOOL_TYPE_FINGER));
    EXPECT_CALL(pointer2, add_axis_value())
            .WillOnce(Return(&axisValue4))
            .WillOnce(Return(&axisValue5))
            .WillOnce(Return(&axisValue6));
    EXPECT_CALL(axisValue4, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue5, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue6, set_axis(AMOTION_EVENT_AXIS_PRESSURE));

    // Redacted fields
    EXPECT_CALL(proto, set_meta_state(_)).Times(0);
    EXPECT_CALL(proto, set_cursor_position_x(_)).Times(0);
    EXPECT_CALL(proto, set_cursor_position_y(_)).Times(0);
    EXPECT_CALL(proto, set_precision_x(_)).Times(0);
    EXPECT_CALL(proto, set_precision_y(_)).Times(0);
    EXPECT_CALL(axisValue1, set_value(_)).Times(0);
    EXPECT_CALL(axisValue2, set_value(_)).Times(0);
    EXPECT_CALL(axisValue3, set_value(_)).Times(0);
    EXPECT_CALL(axisValue4, set_value(_)).Times(0);
    EXPECT_CALL(axisValue5, set_value(_)).Times(0);
    EXPECT_CALL(axisValue6, set_value(_)).Times(0);

    TestProtoConverter::toProtoMotionEvent(event, proto, /*isRedacted=*/true);
}

// Test any special handling for zero values for pointer events.
TEST(AndroidInputEventProtoConverterTest, ToProtoMotionEvent_ZeroValues) {
    TracedMotionEvent event{};
    event.id = 0;
    event.eventTime = 0;
    event.downTime = 0;
    event.source = AINPUT_SOURCE_MOUSE;
    event.action = AMOTION_EVENT_ACTION_BUTTON_PRESS;
    event.deviceId = 0;
    event.displayId = ui::LogicalDisplayId(0);
    event.classification = {};
    event.flags = 0;
    event.policyFlags = 0;
    event.buttonState = 0;
    event.actionButton = 0;
    event.xCursorPosition = 0.0f;
    event.yCursorPosition = 0.0f;
    event.metaState = 0;
    event.xPrecision = 0.0f;
    event.yPrecision = 0.0f;
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 0,
            .toolType = ToolType::MOUSE,
    });
    event.pointerProperties.emplace_back(PointerProperties{
            .id = 1,
            .toolType = ToolType::FINGER,
    });
    // Zero values for x and y axes are always traced for pointer events.
    // However, zero values for other axes may not necessarily be traced.
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 0.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 1.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 0.0f);
    event.pointerCoords.emplace_back();
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 0.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 0.0f);
    event.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_PRESSURE, 0.0f);

    testing::StrictMock<MockProtoMotion> proto;
    testing::StrictMock<MockProtoPointer> pointer1;
    testing::StrictMock<MockProtoPointer> pointer2;
    testing::StrictMock<MockProtoAxisValue> axisValue1;
    testing::StrictMock<MockProtoAxisValue> axisValue2;
    testing::StrictMock<MockProtoAxisValue> axisValue3;
    testing::StrictMock<MockProtoAxisValue> axisValue4;

    EXPECT_CALL(proto, set_event_id(0));
    EXPECT_CALL(proto, set_event_time_nanos(0));
    EXPECT_CALL(proto, set_down_time_nanos(0));
    EXPECT_CALL(proto, set_source(AINPUT_SOURCE_MOUSE));
    EXPECT_CALL(proto, set_action(AMOTION_EVENT_ACTION_BUTTON_PRESS));
    EXPECT_CALL(proto, set_device_id(0));
    EXPECT_CALL(proto, set_display_id(0));
    EXPECT_CALL(proto, set_classification(0));
    EXPECT_CALL(proto, set_flags(0));
    EXPECT_CALL(proto, set_policy_flags(0));
    EXPECT_CALL(proto, set_button_state(0));
    EXPECT_CALL(proto, set_action_button(0));
    EXPECT_CALL(proto, set_cursor_position_x(0.0f));
    EXPECT_CALL(proto, set_cursor_position_y(0.0f));
    EXPECT_CALL(proto, set_meta_state(0));
    EXPECT_CALL(proto, set_precision_x(0.0f));
    EXPECT_CALL(proto, set_precision_y(0.0f));

    EXPECT_CALL(proto, add_pointer()).WillOnce(Return(&pointer1)).WillOnce(Return(&pointer2));

    EXPECT_CALL(pointer1, set_pointer_id(0));
    EXPECT_CALL(pointer1, set_tool_type(AMOTION_EVENT_TOOL_TYPE_MOUSE));
    EXPECT_CALL(pointer1, add_axis_value())
            .WillOnce(Return(&axisValue1))
            .WillOnce(Return(&axisValue2));
    EXPECT_CALL(axisValue1, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue1, set_value(0.0f));
    EXPECT_CALL(axisValue2, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue2, set_value(1.0f));

    EXPECT_CALL(pointer2, set_pointer_id(1));
    EXPECT_CALL(pointer2, set_tool_type(AMOTION_EVENT_TOOL_TYPE_FINGER));
    EXPECT_CALL(pointer2, add_axis_value())
            .WillOnce(Return(&axisValue3))
            .WillOnce(Return(&axisValue4));
    EXPECT_CALL(axisValue3, set_axis(AMOTION_EVENT_AXIS_X));
    EXPECT_CALL(axisValue3, set_value(0.0f));
    EXPECT_CALL(axisValue4, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue4, set_value(0.0f));

    TestProtoConverter::toProtoMotionEvent(event, proto, /*isRedacted=*/false);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoKeyEvent) {
    TracedKeyEvent event{};
    event.id = 1;
    event.eventTime = 2;
    event.downTime = 3;
    event.source = AINPUT_SOURCE_KEYBOARD;
    event.action = AKEY_EVENT_ACTION_DOWN;
    event.deviceId = 4;
    event.displayId = ui::LogicalDisplayId(5);
    event.repeatCount = 6;
    event.flags = 7;
    event.policyFlags = 8;
    event.keyCode = 9;
    event.scanCode = 10;
    event.metaState = 11;

    testing::StrictMock<MockProtoKey> proto;

    EXPECT_CALL(proto, set_event_id(1));
    EXPECT_CALL(proto, set_event_time_nanos(2));
    EXPECT_CALL(proto, set_down_time_nanos(3));
    EXPECT_CALL(proto, set_source(AINPUT_SOURCE_KEYBOARD));
    EXPECT_CALL(proto, set_action(AKEY_EVENT_ACTION_DOWN));
    EXPECT_CALL(proto, set_device_id(4));
    EXPECT_CALL(proto, set_display_id(5));
    EXPECT_CALL(proto, set_repeat_count(6));
    EXPECT_CALL(proto, set_flags(7));
    EXPECT_CALL(proto, set_policy_flags(8));
    EXPECT_CALL(proto, set_key_code(9));
    EXPECT_CALL(proto, set_scan_code(10));
    EXPECT_CALL(proto, set_meta_state(11));

    TestProtoConverter::toProtoKeyEvent(event, proto, /*isRedacted=*/false);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoKeyEvent_Redacted) {
    TracedKeyEvent event{};
    event.id = 1;
    event.eventTime = 2;
    event.downTime = 3;
    event.source = AINPUT_SOURCE_KEYBOARD;
    event.action = AKEY_EVENT_ACTION_DOWN;
    event.deviceId = 4;
    event.displayId = ui::LogicalDisplayId(5);
    event.repeatCount = 6;
    event.flags = 7;
    event.policyFlags = 8;
    event.keyCode = 9;
    event.scanCode = 10;
    event.metaState = 11;

    testing::StrictMock<MockProtoKey> proto;

    EXPECT_CALL(proto, set_event_id(1));
    EXPECT_CALL(proto, set_event_time_nanos(2));
    EXPECT_CALL(proto, set_down_time_nanos(3));
    EXPECT_CALL(proto, set_source(AINPUT_SOURCE_KEYBOARD));
    EXPECT_CALL(proto, set_action(AKEY_EVENT_ACTION_DOWN));
    EXPECT_CALL(proto, set_device_id(4));
    EXPECT_CALL(proto, set_display_id(5));
    EXPECT_CALL(proto, set_repeat_count(6));
    EXPECT_CALL(proto, set_flags(7));
    EXPECT_CALL(proto, set_policy_flags(8));

    // Redacted fields
    EXPECT_CALL(proto, set_key_code(_)).Times(0);
    EXPECT_CALL(proto, set_scan_code(_)).Times(0);
    EXPECT_CALL(proto, set_meta_state(_)).Times(0);

    TestProtoConverter::toProtoKeyEvent(event, proto, /*isRedacted=*/true);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoWindowDispatchEvent_Motion_IdentityTransform) {
    TracedMotionEvent motion{};
    motion.pointerProperties.emplace_back(PointerProperties{
            .id = 4,
            .toolType = ToolType::MOUSE,
    });
    motion.pointerCoords.emplace_back();
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 5.0f);
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 6.0f);

    WindowDispatchArgs args{};
    args.eventEntry = motion;
    args.vsyncId = 1;
    args.windowId = 2;
    args.resolvedFlags = 3;
    args.rawTransform = ui::Transform{};
    args.transform = ui::Transform{};

    testing::StrictMock<MockProtoDispatch> proto;
    testing::StrictMock<MockProtoDispatchPointer> pointer;

    EXPECT_CALL(proto, set_event_id(0));
    EXPECT_CALL(proto, set_vsync_id(1));
    EXPECT_CALL(proto, set_window_id(2));
    EXPECT_CALL(proto, set_resolved_flags(3));
    EXPECT_CALL(proto, add_dispatched_pointer()).WillOnce(Return(&pointer));
    EXPECT_CALL(pointer, set_pointer_id(4));

    // Since we are using identity transforms, the axis values will be identical to those in the
    // traced event, so they should not be traced here.
    EXPECT_CALL(pointer, add_axis_value_in_window()).Times(0);
    EXPECT_CALL(pointer, set_x_in_display(_)).Times(0);
    EXPECT_CALL(pointer, set_y_in_display(_)).Times(0);

    TestProtoConverter::toProtoWindowDispatchEvent(args, proto, /*isRedacted=*/false);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoWindowDispatchEvent_Motion_CustomTransform) {
    TracedMotionEvent motion{};
    motion.pointerProperties.emplace_back(PointerProperties{
            .id = 4,
            .toolType = ToolType::MOUSE,
    });
    motion.pointerCoords.emplace_back();
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 8.0f);
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 6.0f);

    WindowDispatchArgs args{};
    args.eventEntry = motion;
    args.vsyncId = 1;
    args.windowId = 2;
    args.resolvedFlags = 3;
    args.rawTransform.set(2, 0, 0, 0.5);
    args.transform.set(1.0, 0, 0, 0.5);

    testing::StrictMock<MockProtoDispatch> proto;
    testing::StrictMock<MockProtoDispatchPointer> pointer;
    testing::StrictMock<MockProtoAxisValue> axisValue1;

    EXPECT_CALL(proto, set_event_id(0));
    EXPECT_CALL(proto, set_vsync_id(1));
    EXPECT_CALL(proto, set_window_id(2));
    EXPECT_CALL(proto, set_resolved_flags(3));
    EXPECT_CALL(proto, add_dispatched_pointer()).WillOnce(Return(&pointer));
    EXPECT_CALL(pointer, set_pointer_id(4));

    // Only the transformed axis-values that differ from the traced event will be traced.
    EXPECT_CALL(pointer, add_axis_value_in_window()).WillOnce(Return(&axisValue1));
    EXPECT_CALL(pointer, set_x_in_display(16.0f)); // MotionEvent::getRawX
    EXPECT_CALL(pointer, set_y_in_display(3.0f));  // MotionEvent::getRawY

    EXPECT_CALL(axisValue1, set_axis(AMOTION_EVENT_AXIS_Y));
    EXPECT_CALL(axisValue1, set_value(3.0f));

    TestProtoConverter::toProtoWindowDispatchEvent(args, proto, /*isRedacted=*/false);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoWindowDispatchEvent_Motion_Redacted) {
    TracedMotionEvent motion{};
    motion.pointerProperties.emplace_back(PointerProperties{
            .id = 4,
            .toolType = ToolType::MOUSE,
    });
    motion.pointerCoords.emplace_back();
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_X, 5.0f);
    motion.pointerCoords.back().setAxisValue(AMOTION_EVENT_AXIS_Y, 6.0f);

    WindowDispatchArgs args{};
    args.eventEntry = motion;
    args.vsyncId = 1;
    args.windowId = 2;
    args.resolvedFlags = 3;
    args.rawTransform = ui::Transform{};
    args.transform = ui::Transform{};

    testing::StrictMock<MockProtoDispatch> proto;

    EXPECT_CALL(proto, set_event_id(0));
    EXPECT_CALL(proto, set_vsync_id(1));
    EXPECT_CALL(proto, set_window_id(2));
    EXPECT_CALL(proto, set_resolved_flags(3));

    // Redacted fields
    EXPECT_CALL(proto, add_dispatched_pointer()).Times(0);

    TestProtoConverter::toProtoWindowDispatchEvent(args, proto, /*isRedacted=*/true);
}

TEST(AndroidInputEventProtoConverterTest, ToProtoWindowDispatchEvent_Key) {
    TracedKeyEvent key{};

    WindowDispatchArgs args{};
    args.eventEntry = key;
    args.vsyncId = 1;
    args.windowId = 2;
    args.resolvedFlags = 3;
    args.rawTransform = ui::Transform{};
    args.transform = ui::Transform{};

    testing::StrictMock<MockProtoDispatch> proto;

    EXPECT_CALL(proto, set_event_id(0));
    EXPECT_CALL(proto, set_vsync_id(1));
    EXPECT_CALL(proto, set_window_id(2));
    EXPECT_CALL(proto, set_resolved_flags(3));

    TestProtoConverter::toProtoWindowDispatchEvent(args, proto, /*isRedacted=*/true);
}

} // namespace

} // namespace android::inputdispatcher::trace
