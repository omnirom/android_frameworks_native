/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "../dispatcher/LatencyTracker.h"
#include "../InputDeviceMetricsSource.h"
#include "NotifyArgsBuilders.h"
#include "android/input.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <binder/Binder.h>
#include <gtest/gtest.h>
#include <input/PrintTools.h>
#include <inttypes.h>
#include <linux/input.h>
#include <log/log.h>

#define TAG "LatencyTracker_test"

using android::base::HwTimeoutMultiplier;
using android::inputdispatcher::InputEventTimeline;
using android::inputdispatcher::LatencyTracker;

namespace android::inputdispatcher {

namespace {

constexpr DeviceId DEVICE_ID = 100;

static InputDeviceInfo generateTestDeviceInfo(uint16_t vendorId, uint16_t productId,
                                              DeviceId deviceId) {
    InputDeviceIdentifier identifier;
    identifier.vendor = vendorId;
    identifier.product = productId;
    auto info = InputDeviceInfo();
    info.initialize(deviceId, /*generation=*/1, /*controllerNumber=*/1, identifier, "Test Device",
                    /*isExternal=*/false, /*hasMic=*/false, ui::LogicalDisplayId::INVALID);
    return info;
}

void setDefaultInputDeviceInfo(LatencyTracker& tracker) {
    InputDeviceInfo deviceInfo = generateTestDeviceInfo(/*vendorId=*/0, /*productId=*/0, DEVICE_ID);
    tracker.setInputDevices({deviceInfo});
}

const auto FIRST_TOUCH_POINTER = PointerBuilder(/*id=*/0, ToolType::FINGER).x(100).y(200);

/**
 * This is a convenience method for comparing timelines that also prints the difference between
 * the two structures. This helps debugging when the timelines don't match.
 * @param received the timeline that was actually received
 * @param expected the timeline that we expected to receive
 * @return true if the two timelines match, false otherwise.
 */
bool timelinesAreEqual(const InputEventTimeline& received, const InputEventTimeline& expected) {
    LOG_IF(ERROR, expected.eventTime != received.eventTime)
            << "Received timeline with eventTime=" << received.eventTime
            << " instead of expected eventTime=" << expected.eventTime;
    LOG_IF(ERROR, expected.readTime != received.readTime)
            << "Received timeline with readTime=" << received.readTime
            << " instead of expected readTime=" << expected.readTime;
    LOG_IF(ERROR, expected.vendorId != received.vendorId)
            << "Received timeline with vendorId=" << received.vendorId
            << " instead of expected vendorId=" << expected.vendorId;
    LOG_IF(ERROR, expected.productId != received.productId)
            << "Received timeline with productId=" << received.productId
            << " instead of expected productId=" << expected.productId;
    LOG_IF(ERROR, expected.sources != received.sources)
            << "Received timeline with sources=" << dumpSet(received.sources, ftl::enum_string)
            << " instead of expected sources=" << dumpSet(expected.sources, ftl::enum_string);
    LOG_IF(ERROR, expected.inputEventActionType != received.inputEventActionType)
            << "Received timeline with inputEventActionType="
            << ftl::enum_string(received.inputEventActionType)
            << " instead of expected inputEventActionType="
            << ftl::enum_string(expected.inputEventActionType);

    return received == expected;
}

} // namespace

const std::chrono::duration ANR_TIMEOUT = std::chrono::milliseconds(
        android::os::IInputConstants::UNMULTIPLIED_DEFAULT_DISPATCHING_TIMEOUT_MILLIS *
        HwTimeoutMultiplier());

InputEventTimeline getTestTimeline() {
    InputEventTimeline t(
            /*eventTime=*/2,
            /*readTime=*/3,
            /*vendorId=*/0,
            /*productId=*/0, {InputDeviceUsageSource::TOUCHSCREEN},
            /*inputEventActionType=*/InputEventActionType::UNKNOWN_INPUT_EVENT);
    ConnectionTimeline expectedCT(/*deliveryTime=*/6, /*consumeTime=*/7, /*finishTime=*/8);
    std::array<nsecs_t, GraphicsTimeline::SIZE> graphicsTimeline{};
    graphicsTimeline[GraphicsTimeline::GPU_COMPLETED_TIME] = 9;
    graphicsTimeline[GraphicsTimeline::PRESENT_TIME] = 10;
    expectedCT.setGraphicsTimeline(graphicsTimeline);
    t.connectionTimelines.emplace(sp<BBinder>::make(), expectedCT);
    return t;
}

// --- LatencyTrackerTest ---
class LatencyTrackerTest : public testing::Test, public InputEventTimelineProcessor {
protected:
    std::unique_ptr<LatencyTracker> mTracker;
    sp<IBinder> connection1;
    sp<IBinder> connection2;

    void SetUp() override {
        connection1 = sp<BBinder>::make();
        connection2 = sp<BBinder>::make();

        mTracker = std::make_unique<LatencyTracker>(*this);
        setDefaultInputDeviceInfo(*mTracker);
    }
    void TearDown() override {}

    void triggerEventReporting(nsecs_t lastEventTime);

    void assertReceivedTimeline(const InputEventTimeline& timeline);
    /**
     * Timelines can be received in any order (order is not guaranteed). So if we are expecting more
     * than 1 timeline, use this function to check that the set of received timelines matches
     * what we expected.
     */
    void assertReceivedTimelines(const std::vector<InputEventTimeline>& timelines);

private:
    void processTimeline(const InputEventTimeline& timeline) override {
        mReceivedTimelines.push_back(timeline);
    }
    void pushLatencyStatistics() override {}
    std::string dump(const char* prefix) const { return ""; };
    std::deque<InputEventTimeline> mReceivedTimelines;
};

/**
 * Send an event that would trigger the reporting of all of the events that are at least as old as
 * the provided 'lastEventTime'.
 */
void LatencyTrackerTest::triggerEventReporting(nsecs_t lastEventTime) {
    const nsecs_t triggerEventTime =
            lastEventTime + std::chrono::nanoseconds(ANR_TIMEOUT).count() + 1;
    mTracker->trackListener(MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL,
                                              AINPUT_SOURCE_TOUCHSCREEN, /*inputEventId=*/1)
                                    .eventTime(triggerEventTime)
                                    .readTime(3)
                                    .deviceId(DEVICE_ID)
                                    .pointer(FIRST_TOUCH_POINTER)
                                    .build());
}

void LatencyTrackerTest::assertReceivedTimeline(const InputEventTimeline& expectedTimeline) {
    ASSERT_FALSE(mReceivedTimelines.empty());
    const InputEventTimeline& received = mReceivedTimelines.front();
    ASSERT_TRUE(timelinesAreEqual(received, expectedTimeline));
    mReceivedTimelines.pop_front();
}

/**
 * We are essentially comparing two multisets, but without constructing them.
 * This comparison is inefficient, but it avoids having to construct a set, and also avoids the
 * declaration of copy constructor for ConnectionTimeline.
 * We ensure that collections A and B have the same size, that for every element in A, there is an
 * equal element in B, and for every element in B there is an equal element in A.
 */
void LatencyTrackerTest::assertReceivedTimelines(const std::vector<InputEventTimeline>& timelines) {
    ASSERT_EQ(timelines.size(), mReceivedTimelines.size());
    for (const InputEventTimeline& expectedTimeline : timelines) {
        bool found = false;
        for (const InputEventTimeline& receivedTimeline : mReceivedTimelines) {
            if (receivedTimeline == expectedTimeline) {
                found = true;
                break;
            }
        }
        if (!found) {
            for (const InputEventTimeline& receivedTimeline : mReceivedTimelines) {
                LOG(ERROR) << "Received timeline with eventTime=" << receivedTimeline.eventTime;
            }
        }
        ASSERT_TRUE(found) << "Could not find expected timeline with eventTime="
                           << expectedTimeline.eventTime;
    }
    for (const InputEventTimeline& receivedTimeline : mReceivedTimelines) {
        bool found = false;
        for (const InputEventTimeline& expectedTimeline : timelines) {
            if (receivedTimeline == expectedTimeline) {
                found = true;
                break;
            }
        }
        ASSERT_TRUE(found) << "Could not find received timeline with eventTime="
                           << receivedTimeline.eventTime;
    }
    mReceivedTimelines.clear();
}

/**
 * Ensure that calling 'trackListener' in isolation only creates an inputflinger timeline, without
 * any additional ConnectionTimeline's.
 */
TEST_F(LatencyTrackerTest, TrackListener_DoesNotTriggerReporting) {
    mTracker->trackListener(MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL,
                                              AINPUT_SOURCE_TOUCHSCREEN, /*inputEventId=*/1)
                                    .eventTime(2)
                                    .readTime(3)
                                    .deviceId(DEVICE_ID)
                                    .pointer(FIRST_TOUCH_POINTER)
                                    .build());
    triggerEventReporting(/*eventTime=*/2);
    assertReceivedTimeline(
            InputEventTimeline{/*eventTime=*/2,
                               /*readTime=*/3,
                               /*vendorId=*/0,
                               /*productID=*/0,
                               {InputDeviceUsageSource::TOUCHSCREEN},
                               /*inputEventActionType=*/InputEventActionType::UNKNOWN_INPUT_EVENT});
}

/**
 * A single call to trackFinishedEvent should not cause a timeline to be reported.
 */
TEST_F(LatencyTrackerTest, TrackFinishedEvent_DoesNotTriggerReporting) {
    mTracker->trackFinishedEvent(/*inputEventId=*/1, connection1, /*deliveryTime=*/2,
                                 /*consumeTime=*/3, /*finishTime=*/4);
    triggerEventReporting(/*eventTime=*/4);
    assertReceivedTimelines({});
}

/**
 * A single call to trackGraphicsLatency should not cause a timeline to be reported.
 */
TEST_F(LatencyTrackerTest, TrackGraphicsLatency_DoesNotTriggerReporting) {
    std::array<nsecs_t, GraphicsTimeline::SIZE> graphicsTimeline;
    graphicsTimeline[GraphicsTimeline::GPU_COMPLETED_TIME] = 2;
    graphicsTimeline[GraphicsTimeline::PRESENT_TIME] = 3;
    mTracker->trackGraphicsLatency(/*inputEventId=*/1, connection2, graphicsTimeline);
    triggerEventReporting(/*eventTime=*/3);
    assertReceivedTimelines({});
}

TEST_F(LatencyTrackerTest, TrackAllParameters_ReportsFullTimeline) {
    constexpr int32_t inputEventId = 1;
    InputEventTimeline expected = getTestTimeline();

    const auto& [connectionToken, expectedCT] = *expected.connectionTimelines.begin();

    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId)
                    .eventTime(expected.eventTime)
                    .readTime(expected.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    mTracker->trackFinishedEvent(inputEventId, connectionToken, expectedCT.deliveryTime,
                                 expectedCT.consumeTime, expectedCT.finishTime);
    mTracker->trackGraphicsLatency(inputEventId, connectionToken, expectedCT.graphicsTimeline);

    triggerEventReporting(expected.eventTime);
    assertReceivedTimeline(expected);
}

/**
 * Send 2 events with the same inputEventId, but different eventTime's. Ensure that no crash occurs,
 * and that the tracker drops such events completely.
 */
TEST_F(LatencyTrackerTest, WhenDuplicateEventsAreReported_DoesNotCrash) {
    constexpr nsecs_t inputEventId = 1;
    constexpr nsecs_t readTime = 3; // does not matter for this test

    // In the following 2 calls to trackListener, the inputEventId's are the same, but event times
    // are different.
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId)
                    .eventTime(1)
                    .readTime(readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId)
                    .eventTime(2)
                    .readTime(readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());

    triggerEventReporting(/*eventTime=*/2);
    // Since we sent duplicate input events, the tracker should just delete all of them, because it
    // does not have enough information to properly track them.
    assertReceivedTimelines({});
}

TEST_F(LatencyTrackerTest, MultipleEvents_AreReportedConsistently) {
    constexpr int32_t inputEventId1 = 1;
    InputEventTimeline timeline1(
            /*eventTime*/ 2,
            /*readTime*/ 3,
            /*vendorId=*/0,
            /*productId=*/0, {InputDeviceUsageSource::TOUCHSCREEN},
            /*inputEventType=*/InputEventActionType::UNKNOWN_INPUT_EVENT);
    timeline1.connectionTimelines.emplace(connection1,
                                          ConnectionTimeline(/*deliveryTime*/ 6, /*consumeTime*/ 7,
                                                             /*finishTime*/ 8));
    ConnectionTimeline& connectionTimeline1 = timeline1.connectionTimelines.begin()->second;
    std::array<nsecs_t, GraphicsTimeline::SIZE> graphicsTimeline1;
    graphicsTimeline1[GraphicsTimeline::GPU_COMPLETED_TIME] = 9;
    graphicsTimeline1[GraphicsTimeline::PRESENT_TIME] = 10;
    connectionTimeline1.setGraphicsTimeline(std::move(graphicsTimeline1));

    constexpr int32_t inputEventId2 = 10;
    InputEventTimeline timeline2(
            /*eventTime=*/20,
            /*readTime=*/30,
            /*vendorId=*/0,
            /*productId=*/0, {InputDeviceUsageSource::TOUCHSCREEN},
            /*inputEventActionType=*/InputEventActionType::UNKNOWN_INPUT_EVENT);
    timeline2.connectionTimelines.emplace(connection2,
                                          ConnectionTimeline(/*deliveryTime=*/60,
                                                             /*consumeTime=*/70,
                                                             /*finishTime=*/80));
    ConnectionTimeline& connectionTimeline2 = timeline2.connectionTimelines.begin()->second;
    std::array<nsecs_t, GraphicsTimeline::SIZE> graphicsTimeline2;
    graphicsTimeline2[GraphicsTimeline::GPU_COMPLETED_TIME] = 90;
    graphicsTimeline2[GraphicsTimeline::PRESENT_TIME] = 100;
    connectionTimeline2.setGraphicsTimeline(std::move(graphicsTimeline2));

    // Start processing first event
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId1)
                    .eventTime(timeline1.eventTime)
                    .readTime(timeline1.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    // Start processing second event
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId2)
                    .eventTime(timeline2.eventTime)
                    .readTime(timeline2.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    mTracker->trackFinishedEvent(inputEventId1, connection1, connectionTimeline1.deliveryTime,
                                 connectionTimeline1.consumeTime, connectionTimeline1.finishTime);

    mTracker->trackFinishedEvent(inputEventId2, connection2, connectionTimeline2.deliveryTime,
                                 connectionTimeline2.consumeTime, connectionTimeline2.finishTime);
    mTracker->trackGraphicsLatency(inputEventId1, connection1,
                                   connectionTimeline1.graphicsTimeline);
    mTracker->trackGraphicsLatency(inputEventId2, connection2,
                                   connectionTimeline2.graphicsTimeline);
    // Now both events should be completed
    triggerEventReporting(timeline2.eventTime);
    assertReceivedTimelines({timeline1, timeline2});
}

/**
 * Check that LatencyTracker consistently tracks events even if there are many incomplete events.
 */
TEST_F(LatencyTrackerTest, IncompleteEvents_AreHandledConsistently) {
    InputEventTimeline timeline = getTestTimeline();
    std::vector<InputEventTimeline> expectedTimelines;
    const ConnectionTimeline& expectedCT = timeline.connectionTimelines.begin()->second;
    const sp<IBinder>& token = timeline.connectionTimelines.begin()->first;

    for (size_t i = 1; i <= 100; i++) {
        mTracker->trackListener(MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL,
                                                  AINPUT_SOURCE_TOUCHSCREEN, /*inputEventId=*/i)
                                        .eventTime(timeline.eventTime)
                                        .readTime(timeline.readTime)
                                        .deviceId(DEVICE_ID)
                                        .pointer(FIRST_TOUCH_POINTER)
                                        .build());
        expectedTimelines.push_back(InputEventTimeline{timeline.eventTime, timeline.readTime,
                                                       timeline.vendorId, timeline.productId,
                                                       timeline.sources,
                                                       timeline.inputEventActionType});
    }
    // Now, complete the first event that was sent.
    mTracker->trackFinishedEvent(/*inputEventId=*/1, token, expectedCT.deliveryTime,
                                 expectedCT.consumeTime, expectedCT.finishTime);
    mTracker->trackGraphicsLatency(/*inputEventId=*/1, token, expectedCT.graphicsTimeline);

    expectedTimelines[0].connectionTimelines.emplace(token, std::move(expectedCT));
    triggerEventReporting(timeline.eventTime);
    assertReceivedTimelines(expectedTimelines);
}

/**
 * For simplicity of the implementation, LatencyTracker only starts tracking an event when
 * 'trackListener' is invoked.
 * Both 'trackFinishedEvent' and 'trackGraphicsLatency' should not start a new event.
 * If they are received before 'trackListener' (which should not be possible), they are ignored.
 */
TEST_F(LatencyTrackerTest, EventsAreTracked_WhenTrackListenerIsCalledFirst) {
    constexpr int32_t inputEventId = 1;
    InputEventTimeline expected = getTestTimeline();
    const ConnectionTimeline& expectedCT = expected.connectionTimelines.begin()->second;
    mTracker->trackFinishedEvent(inputEventId, connection1, expectedCT.deliveryTime,
                                 expectedCT.consumeTime, expectedCT.finishTime);
    mTracker->trackGraphicsLatency(inputEventId, connection1, expectedCT.graphicsTimeline);

    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL, AINPUT_SOURCE_TOUCHSCREEN, inputEventId)
                    .eventTime(expected.eventTime)
                    .readTime(expected.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    triggerEventReporting(expected.eventTime);
    assertReceivedTimeline(InputEventTimeline{expected.eventTime, expected.readTime,
                                              expected.vendorId, expected.productId,
                                              expected.sources, expected.inputEventActionType});
}

/**
 * Check that LatencyTracker has the received timeline that contains the correctly
 * resolved product ID, vendor ID and source for a particular device ID from
 * among a list of devices.
 */
TEST_F(LatencyTrackerTest, TrackListenerCheck_DeviceInfoFieldsInputEventTimeline) {
    constexpr int32_t inputEventId = 1;
    InputEventTimeline timeline(
            /*eventTime*/ 2, /*readTime*/ 3,
            /*vendorId=*/50, /*productId=*/60, {InputDeviceUsageSource::STYLUS_DIRECT},
            /*inputEventActionType=*/InputEventActionType::UNKNOWN_INPUT_EVENT);
    InputDeviceInfo deviceInfo1 = generateTestDeviceInfo(
            /*vendorId=*/5, /*productId=*/6, /*deviceId=*/DEVICE_ID + 1);
    InputDeviceInfo deviceInfo2 = generateTestDeviceInfo(
            /*vendorId=*/50, /*productId=*/60, /*deviceId=*/DEVICE_ID);
    deviceInfo2.addSource(AINPUT_SOURCE_TOUCHSCREEN);
    deviceInfo2.addSource(AINPUT_SOURCE_STYLUS);

    mTracker->setInputDevices({deviceInfo1, deviceInfo2});
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_CANCEL,
                              AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_STYLUS, inputEventId)

                    .eventTime(timeline.eventTime)
                    .readTime(timeline.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(PointerBuilder(/*id=*/0, ToolType::STYLUS).x(100).y(200))
                    .build());
    triggerEventReporting(timeline.eventTime);
    assertReceivedTimeline(timeline);
}

/**
 * Check that InputEventActionType is correctly assigned to InputEventTimeline in trackListener.
 */
TEST_F(LatencyTrackerTest, TrackListenerCheck_InputEventActionTypeFieldInputEventTimeline) {
    constexpr int32_t inputEventId = 1;
    // Create timelines for different event types (Motion, Key)
    InputEventTimeline motionDownTimeline(
            /*eventTime*/ 2, /*readTime*/ 3,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::TOUCHSCREEN},
            InputEventActionType::MOTION_ACTION_DOWN);

    InputEventTimeline motionMoveTimeline(
            /*eventTime*/ 4, /*readTime*/ 5,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::TOUCHSCREEN},
            InputEventActionType::MOTION_ACTION_MOVE);

    InputEventTimeline motionUpTimeline(
            /*eventTime*/ 6, /*readTime*/ 7,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::TOUCHSCREEN},
            InputEventActionType::MOTION_ACTION_UP);

    InputEventTimeline keyDownTimeline(
            /*eventTime*/ 8, /*readTime*/ 9,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::BUTTONS},
            InputEventActionType::KEY);

    InputEventTimeline keyUpTimeline(
            /*eventTime*/ 10, /*readTime*/ 11,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::BUTTONS},
            InputEventActionType::KEY);

    InputEventTimeline unknownTimeline(
            /*eventTime*/ 12, /*readTime*/ 13,
            /*vendorId*/ 0, /*productId*/ 0, {InputDeviceUsageSource::TOUCHSCREEN},
            InputEventActionType::UNKNOWN_INPUT_EVENT);

    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_DOWN, AINPUT_SOURCE_TOUCHSCREEN, inputEventId)
                    .eventTime(motionDownTimeline.eventTime)
                    .readTime(motionDownTimeline.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    mTracker->trackListener(MotionArgsBuilder(AMOTION_EVENT_ACTION_MOVE, AINPUT_SOURCE_TOUCHSCREEN,
                                              inputEventId + 1)
                                    .eventTime(motionMoveTimeline.eventTime)
                                    .readTime(motionMoveTimeline.readTime)
                                    .deviceId(DEVICE_ID)
                                    .pointer(FIRST_TOUCH_POINTER)
                                    .build());
    mTracker->trackListener(
            MotionArgsBuilder(AMOTION_EVENT_ACTION_UP, AINPUT_SOURCE_TOUCHSCREEN, inputEventId + 2)
                    .eventTime(motionUpTimeline.eventTime)
                    .readTime(motionUpTimeline.readTime)
                    .deviceId(DEVICE_ID)
                    .pointer(FIRST_TOUCH_POINTER)
                    .build());
    mTracker->trackListener(
            KeyArgsBuilder(AKEY_EVENT_ACTION_DOWN, AINPUT_SOURCE_KEYBOARD, inputEventId + 3)
                    .eventTime(keyDownTimeline.eventTime)
                    .readTime(keyDownTimeline.readTime)
                    .deviceId(DEVICE_ID)
                    .build());
    mTracker->trackListener(
            KeyArgsBuilder(AKEY_EVENT_ACTION_UP, AINPUT_SOURCE_KEYBOARD, inputEventId + 4)
                    .eventTime(keyUpTimeline.eventTime)
                    .readTime(keyUpTimeline.readTime)
                    .deviceId(DEVICE_ID)
                    .build());
    mTracker->trackListener(MotionArgsBuilder(AMOTION_EVENT_ACTION_POINTER_DOWN,
                                              AINPUT_SOURCE_TOUCHSCREEN, inputEventId + 5)
                                    .eventTime(unknownTimeline.eventTime)
                                    .readTime(unknownTimeline.readTime)
                                    .deviceId(DEVICE_ID)
                                    .pointer(FIRST_TOUCH_POINTER)
                                    .build());

    triggerEventReporting(unknownTimeline.eventTime);

    std::vector<InputEventTimeline> expectedTimelines = {motionDownTimeline, motionMoveTimeline,
                                                         motionUpTimeline,   keyDownTimeline,
                                                         keyUpTimeline,      unknownTimeline};
    assertReceivedTimelines(expectedTimelines);
}

} // namespace android::inputdispatcher
