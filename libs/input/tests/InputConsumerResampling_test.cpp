/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <input/InputConsumerNoResampling.h>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <TestEventMatchers.h>
#include <TestInputChannel.h>
#include <attestation/HmacKeyManager.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <input/BlockingQueue.h>
#include <input/InputEventBuilders.h>
#include <input/Resampler.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {
namespace {

using std::chrono::nanoseconds;
using namespace std::chrono_literals;

const std::chrono::milliseconds RESAMPLE_LATENCY{5};

struct Pointer {
    int32_t id{0};
    float x{0.0f};
    float y{0.0f};
    ToolType toolType{ToolType::FINGER};
    bool isResampled{false};

    PointerBuilder asPointerBuilder() const {
        return PointerBuilder{id, toolType}.x(x).y(y).isResampled(isResampled);
    }
};

struct InputEventEntry {
    std::chrono::nanoseconds eventTime{0};
    std::vector<Pointer> pointers{};
    int32_t action{-1};
};

} // namespace

class InputConsumerResamplingTest : public ::testing::Test, public InputConsumerCallbacks {
protected:
    InputConsumerResamplingTest()
          : mClientTestChannel{std::make_shared<TestInputChannel>("TestChannel")},
            mLooper{sp<Looper>::make(/*allowNonCallbacks=*/false)} {
        Looper::setForThread(mLooper);
        mConsumer = std::make_unique<
                InputConsumerNoResampling>(mClientTestChannel, mLooper, *this,
                                           []() { return std::make_unique<LegacyResampler>(); });
    }

    void invokeLooperCallback() const {
        sp<LooperCallback> callback;
        ASSERT_TRUE(mLooper->getFdStateDebug(mClientTestChannel->getFd(), /*ident=*/nullptr,
                                             /*events=*/nullptr, &callback, /*data=*/nullptr));
        ASSERT_NE(callback, nullptr);
        callback->handleEvent(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT, /*data=*/nullptr);
    }

    InputMessage nextPointerMessage(const InputEventEntry& entry);

    void assertReceivedMotionEvent(const std::vector<InputEventEntry>& expectedEntries);

    std::shared_ptr<TestInputChannel> mClientTestChannel;
    sp<Looper> mLooper;
    std::unique_ptr<InputConsumerNoResampling> mConsumer;

    BlockingQueue<std::unique_ptr<KeyEvent>> mKeyEvents;
    BlockingQueue<std::unique_ptr<MotionEvent>> mMotionEvents;
    BlockingQueue<std::unique_ptr<FocusEvent>> mFocusEvents;
    BlockingQueue<std::unique_ptr<CaptureEvent>> mCaptureEvents;
    BlockingQueue<std::unique_ptr<DragEvent>> mDragEvents;
    BlockingQueue<std::unique_ptr<TouchModeEvent>> mTouchModeEvents;

private:
    uint32_t mLastSeq{0};
    size_t mOnBatchedInputEventPendingInvocationCount{0};

    // InputConsumerCallbacks interface
    void onKeyEvent(std::unique_ptr<KeyEvent> event, uint32_t seq) override {
        mKeyEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onMotionEvent(std::unique_ptr<MotionEvent> event, uint32_t seq) override {
        mMotionEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onBatchedInputEventPending(int32_t pendingBatchSource) override {
        if (!mConsumer->probablyHasInput()) {
            ADD_FAILURE() << "should deterministically have input because there is a batch";
        }
        ++mOnBatchedInputEventPendingInvocationCount;
    }
    void onFocusEvent(std::unique_ptr<FocusEvent> event, uint32_t seq) override {
        mFocusEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onCaptureEvent(std::unique_ptr<CaptureEvent> event, uint32_t seq) override {
        mCaptureEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onDragEvent(std::unique_ptr<DragEvent> event, uint32_t seq) override {
        mDragEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
    void onTouchModeEvent(std::unique_ptr<TouchModeEvent> event, uint32_t seq) override {
        mTouchModeEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, true);
    }
};

InputMessage InputConsumerResamplingTest::nextPointerMessage(const InputEventEntry& entry) {
    ++mLastSeq;
    InputMessageBuilder messageBuilder = InputMessageBuilder{InputMessage::Type::MOTION, mLastSeq}
                                                 .eventTime(entry.eventTime.count())
                                                 .deviceId(1)
                                                 .action(entry.action)
                                                 .downTime(0);
    for (const Pointer& pointer : entry.pointers) {
        messageBuilder.pointer(pointer.asPointerBuilder());
    }
    return messageBuilder.build();
}

void InputConsumerResamplingTest::assertReceivedMotionEvent(
        const std::vector<InputEventEntry>& expectedEntries) {
    std::unique_ptr<MotionEvent> motionEvent = mMotionEvents.pop();
    ASSERT_NE(motionEvent, nullptr);

    ASSERT_EQ(motionEvent->getHistorySize() + 1, expectedEntries.size());

    for (size_t sampleIndex = 0; sampleIndex < expectedEntries.size(); ++sampleIndex) {
        SCOPED_TRACE("sampleIndex: " + std::to_string(sampleIndex));
        const InputEventEntry& expectedEntry = expectedEntries[sampleIndex];
        EXPECT_EQ(motionEvent->getHistoricalEventTime(sampleIndex),
                  expectedEntry.eventTime.count());
        EXPECT_EQ(motionEvent->getPointerCount(), expectedEntry.pointers.size());
        EXPECT_EQ(motionEvent->getAction(), expectedEntry.action);

        for (size_t pointerIndex = 0; pointerIndex < expectedEntry.pointers.size();
             ++pointerIndex) {
            SCOPED_TRACE("pointerIndex: " + std::to_string(pointerIndex));
            ssize_t eventPointerIndex =
                    motionEvent->findPointerIndex(expectedEntry.pointers[pointerIndex].id);
            EXPECT_EQ(motionEvent->getHistoricalRawX(eventPointerIndex, sampleIndex),
                      expectedEntry.pointers[pointerIndex].x);
            EXPECT_EQ(motionEvent->getHistoricalRawY(eventPointerIndex, sampleIndex),
                      expectedEntry.pointers[pointerIndex].y);
            EXPECT_EQ(motionEvent->getHistoricalX(eventPointerIndex, sampleIndex),
                      expectedEntry.pointers[pointerIndex].x);
            EXPECT_EQ(motionEvent->getHistoricalY(eventPointerIndex, sampleIndex),
                      expectedEntry.pointers[pointerIndex].y);
            EXPECT_EQ(motionEvent->isResampled(pointerIndex, sampleIndex),
                      expectedEntry.pointers[pointerIndex].isResampled);
        }
    }
}

/**
 * Timeline
 * ---------+------------------+------------------+--------+-----------------+----------------------
 *          0 ms               10 ms              20 ms    25 ms            35 ms
 *          ACTION_DOWN       ACTION_MOVE      ACTION_MOVE  ^                ^
 *                                                          |                |
 *                                                         resampled value   |
 *                                                                          frameTime
 * Typically, the prediction is made for time frameTime - RESAMPLE_LATENCY, or 30 ms in this case,
 * where RESAMPLE_LATENCY equals 5 milliseconds. However, that would be 10 ms later than the last
 * real sample (which came in at 20 ms). Therefore, the resampling should happen at 20 ms +
 * RESAMPLE_MAX_PREDICTION = 28 ms, where RESAMPLE_MAX_PREDICTION equals 8 milliseconds. In this
 * situation, though, resample time is further limited by taking half of the difference between the
 * last two real events, which would put this time at: 20 ms + (20 ms - 10 ms) / 2 = 25 ms.
 */
TEST_F(InputConsumerResamplingTest, EventIsResampled) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{10ms,
                             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{20ms,
                             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{25ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Same as above test, but use pointer id=1 instead of 0 to make sure that system does not
 * have these hardcoded.
 */
TEST_F(InputConsumerResamplingTest, EventIsResampledWithDifferentId) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 1, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 1, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 1, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 1, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{10ms,
                             {Pointer{.id = 1, .x = 20.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{20ms,
                             {Pointer{.id = 1, .x = 30.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{25ms,
                             {Pointer{.id = 1, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Stylus pointer coordinates are resampled.
 */
TEST_F(InputConsumerResamplingTest, StylusEventIsResampled) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms,
             {Pointer{.id = 0, .x = 10.0f, .y = 20.0f, .toolType = ToolType::STYLUS}},
             AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0,
                                                        .x = 10.0f,
                                                        .y = 20.0f,
                                                        .toolType = ToolType::STYLUS}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms,
             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f, .toolType = ToolType::STYLUS}},
             AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms,
             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f, .toolType = ToolType::STYLUS}},
             AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent({InputEventEntry{10ms,
                                               {Pointer{.id = 0,
                                                        .x = 20.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::STYLUS}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{20ms,
                                               {Pointer{.id = 0,
                                                        .x = 30.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::STYLUS}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{25ms,
                                               {Pointer{.id = 0,
                                                        .x = 35.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::STYLUS,
                                                        .isResampled = true}},
                                               AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Mouse pointer coordinates are resampled.
 */
TEST_F(InputConsumerResamplingTest, MouseEventIsResampled) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms,
             {Pointer{.id = 0, .x = 10.0f, .y = 20.0f, .toolType = ToolType::MOUSE}},
             AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0,
                                                        .x = 10.0f,
                                                        .y = 20.0f,
                                                        .toolType = ToolType::MOUSE}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms,
             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f, .toolType = ToolType::MOUSE}},
             AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms,
             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f, .toolType = ToolType::MOUSE}},
             AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent({InputEventEntry{10ms,
                                               {Pointer{.id = 0,
                                                        .x = 20.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::MOUSE}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{20ms,
                                               {Pointer{.id = 0,
                                                        .x = 30.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::MOUSE}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{25ms,
                                               {Pointer{.id = 0,
                                                        .x = 35.0f,
                                                        .y = 30.0f,
                                                        .toolType = ToolType::MOUSE,
                                                        .isResampled = true}},
                                               AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Motion events with palm tool type are not resampled.
 */
TEST_F(InputConsumerResamplingTest, PalmEventIsNotResampled) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms,
             {Pointer{.id = 0, .x = 10.0f, .y = 20.0f, .toolType = ToolType::PALM}},
             AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent(
            {InputEventEntry{0ms,
                             {Pointer{.id = 0, .x = 10.0f, .y = 20.0f, .toolType = ToolType::PALM}},
                             AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms,
             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f, .toolType = ToolType::PALM}},
             AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms,
             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f, .toolType = ToolType::PALM}},
             AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{10ms,
                             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f, .toolType = ToolType::PALM}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{20ms,
                             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f, .toolType = ToolType::PALM}},
                             AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Event should not be resampled when sample time is equal to event time.
 */
TEST_F(InputConsumerResamplingTest, SampleTimeEqualsEventTime) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{20ms + RESAMPLE_LATENCY}.count());

    // MotionEvent should not resampled because the resample time falls exactly on the existing
    // event time.
    assertReceivedMotionEvent({InputEventEntry{10ms,
                                               {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{20ms,
                                               {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}},
                                               AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * Once we send a resampled value to the app, we should continue to send the last predicted value if
 * a pointer does not move. Only real values are used to determine if a pointer does not move.
 */
TEST_F(InputConsumerResamplingTest, ResampledValueIsUsedForIdenticalCoordinates) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{10ms,
                             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{20ms,
                             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{25ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    // Coordinate value 30 has been resampled to 35. When a new event comes in with value 30 again,
    // the system should still report 35.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {40ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{45ms + RESAMPLE_LATENCY}.count());
    // Original and resampled event should be both overwritten.
    assertReceivedMotionEvent(
            {InputEventEntry{40ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{45ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
}

TEST_F(InputConsumerResamplingTest, OldEventReceivedAfterResampleOccurs) {
    // Send the initial ACTION_DOWN separately, so that the first consumed event will only return an
    // InputEvent with a single action.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    // Two ACTION_MOVE events 10 ms apart that move in X direction and stay still in Y
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{35ms}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{10ms,
                             {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{20ms,
                             {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{25ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    // Above, the resampled event is at 25ms rather than at 30 ms = 35ms - RESAMPLE_LATENCY
    // because we are further bound by how far we can extrapolate by the "last time delta".
    // That's 50% of (20 ms - 10ms) => 5ms. So we can't predict more than 5 ms into the future
    // from the event at 20ms, which is why the resampled event is at t = 25 ms.

    // We resampled the event to 25 ms. Now, an older 'real' event comes in.
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {24ms, {Pointer{.id = 0, .x = 40.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{50ms}.count());
    // Original and resampled event should be both overwritten.
    assertReceivedMotionEvent(
            {InputEventEntry{24ms,
                             {Pointer{.id = 0, .x = 35.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{26ms,
                             {Pointer{.id = 0, .x = 45.0f, .y = 30.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
}

TEST_F(InputConsumerResamplingTest, DoNotResampleWhenFrameTimeIsNotAvailable) {
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 10.0f, .y = 20.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {20ms, {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(std::nullopt);
    assertReceivedMotionEvent({InputEventEntry{10ms,
                                               {Pointer{.id = 0, .x = 20.0f, .y = 30.0f}},
                                               AMOTION_EVENT_ACTION_MOVE},
                               InputEventEntry{20ms,
                                               {Pointer{.id = 0, .x = 30.0f, .y = 30.0f}},
                                               AMOTION_EVENT_ACTION_MOVE}});

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

TEST_F(InputConsumerResamplingTest, TwoPointersAreResampledIndependently) {
    // Full action for when a pointer with index=1 appears (some other pointer must already be
    // present)
    const int32_t actionPointer1Down =
            AMOTION_EVENT_ACTION_POINTER_DOWN + (1 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    // Full action for when a pointer with index=0 disappears (some other pointer must still remain)
    const int32_t actionPointer0Up =
            AMOTION_EVENT_ACTION_POINTER_UP + (0 << AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);

    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {0ms, {Pointer{.id = 0, .x = 100.0f, .y = 100.0f}}, AMOTION_EVENT_ACTION_DOWN}));

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();
    assertReceivedMotionEvent({InputEventEntry{0ms,
                                               {Pointer{.id = 0, .x = 100.0f, .y = 100.0f}},
                                               AMOTION_EVENT_ACTION_DOWN}});

    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {10ms, {Pointer{.id = 0, .x = 100.0f, .y = 100.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{10ms + RESAMPLE_LATENCY}.count());
    // Not resampled value because requestedFrameTime - RESAMPLE_LATENCY == eventTime
    assertReceivedMotionEvent({InputEventEntry{10ms,
                                               {Pointer{.id = 0, .x = 100.0f, .y = 100.0f}},
                                               AMOTION_EVENT_ACTION_MOVE}});

    // Second pointer id=1 appears
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({15ms,
                                {Pointer{.id = 0, .x = 100.0f, .y = 100.0f},
                                 Pointer{.id = 1, .x = 500.0f, .y = 500.0f}},
                                actionPointer1Down}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{20ms + RESAMPLE_LATENCY}.count());
    // Not resampled value because requestedFrameTime - RESAMPLE_LATENCY == eventTime.
    assertReceivedMotionEvent({InputEventEntry{15ms,
                                               {Pointer{.id = 0, .x = 100.0f, .y = 100.0f},
                                                Pointer{.id = 1, .x = 500.0f, .y = 500.0f}},
                                               actionPointer1Down}});

    // Both pointers move
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({30ms,
                                {Pointer{.id = 0, .x = 100.0f, .y = 100.0f},
                                 Pointer{.id = 1, .x = 500.0f, .y = 500.0f}},
                                AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({40ms,
                                {Pointer{.id = 0, .x = 120.0f, .y = 120.0f},
                                 Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                                AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{45ms + RESAMPLE_LATENCY}.count());
    assertReceivedMotionEvent(
            {InputEventEntry{30ms,
                             {Pointer{.id = 0, .x = 100.0f, .y = 100.0f},
                              Pointer{.id = 1, .x = 500.0f, .y = 500.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{40ms,
                             {Pointer{.id = 0, .x = 120.0f, .y = 120.0f},
                              Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{45ms,
                             {Pointer{.id = 0, .x = 130.0f, .y = 130.0f, .isResampled = true},
                              Pointer{.id = 1, .x = 650.0f, .y = 650.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    // Both pointers move again
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({60ms,
                                {Pointer{.id = 0, .x = 120.0f, .y = 120.0f},
                                 Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                                AMOTION_EVENT_ACTION_MOVE}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({70ms,
                                {Pointer{.id = 0, .x = 130.0f, .y = 130.0f},
                                 Pointer{.id = 1, .x = 700.0f, .y = 700.0f}},
                                AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{75ms + RESAMPLE_LATENCY}.count());

    /*
     * The pointer id 0 at t = 60 should not be equal to 120 because the value was received twice,
     * and resampled to 130. Therefore, if we reported 130, then we should continue to report it as
     * such. Likewise, with pointer id 1.
     */

    // Not 120 because it matches a previous real event.
    assertReceivedMotionEvent(
            {InputEventEntry{60ms,
                             {Pointer{.id = 0, .x = 130.0f, .y = 130.0f, .isResampled = true},
                              Pointer{.id = 1, .x = 650.0f, .y = 650.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{70ms,
                             {Pointer{.id = 0, .x = 130.0f, .y = 130.0f},
                              Pointer{.id = 1, .x = 700.0f, .y = 700.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{75ms,
                             {Pointer{.id = 0, .x = 135.0f, .y = 135.0f, .isResampled = true},
                              Pointer{.id = 1, .x = 750.0f, .y = 750.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});

    // First pointer id=0 leaves the screen
    mClientTestChannel->enqueueMessage(
            nextPointerMessage({80ms,
                                {Pointer{.id = 0, .x = 120.0f, .y = 120.0f},
                                 Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                                actionPointer0Up}));

    invokeLooperCallback();
    // Not resampled event for ACTION_POINTER_UP
    assertReceivedMotionEvent({InputEventEntry{80ms,
                                               {Pointer{.id = 0, .x = 120.0f, .y = 120.0f},
                                                Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                                               actionPointer0Up}});

    // Remaining pointer id=1 is still present, but doesn't move
    mClientTestChannel->enqueueMessage(nextPointerMessage(
            {90ms, {Pointer{.id = 1, .x = 600.0f, .y = 600.0f}}, AMOTION_EVENT_ACTION_MOVE}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(nanoseconds{100ms}.count());

    /*
     * The latest event with ACTION_MOVE was at t = 70 with value = 700. Thus, the resampled value
     * is 700 + ((95 - 70)/(90 - 70))*(600 - 700) = 575.
     */
    assertReceivedMotionEvent(
            {InputEventEntry{90ms,
                             {Pointer{.id = 1, .x = 600.0f, .y = 600.0f}},
                             AMOTION_EVENT_ACTION_MOVE},
             InputEventEntry{95ms,
                             {Pointer{.id = 1, .x = 575.0f, .y = 575.0f, .isResampled = true}},
                             AMOTION_EVENT_ACTION_MOVE}});
}

} // namespace android
