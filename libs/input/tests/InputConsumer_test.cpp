/**
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

#include <input/InputConsumerNoResampling.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <optional>

#include <TestEventMatchers.h>
#include <TestInputChannel.h>
#include <android-base/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <input/BlockingQueue.h>
#include <input/Input.h>
#include <input/InputEventBuilders.h>
#include <input/Resampler.h>
#include <utils/Looper.h>
#include <utils/StrongPointer.h>

namespace android {

namespace {

using std::chrono::nanoseconds;

using ::testing::AllOf;
using ::testing::Matcher;

constexpr auto ACTION_DOWN = AMOTION_EVENT_ACTION_DOWN;
constexpr auto ACTION_MOVE = AMOTION_EVENT_ACTION_MOVE;

struct Pointer {
    int32_t id{0};
    ToolType toolType{ToolType::FINGER};
    float x{0.0f};
    float y{0.0f};
    bool isResampled{false};

    PointerBuilder asPointerBuilder() const {
        return PointerBuilder{id, toolType}.x(x).y(y).isResampled(isResampled);
    }
};
} // namespace

class InputConsumerTest : public testing::Test, public InputConsumerCallbacks {
protected:
    InputConsumerTest()
          : mClientTestChannel{std::make_shared<TestInputChannel>("TestChannel")},
            mLooper{sp<Looper>::make(/*allowNonCallbacks=*/false)} {
        Looper::setForThread(mLooper);
        mConsumer = std::make_unique<
                InputConsumerNoResampling>(mClientTestChannel, mLooper, *this,
                                           []() { return std::make_unique<LegacyResampler>(); });
    }

    bool invokeLooperCallback() const {
        sp<LooperCallback> callback;
        const bool found =
                mLooper->getFdStateDebug(mClientTestChannel->getFd(), /*ident=*/nullptr,
                                         /*events=*/nullptr, &callback, /*data=*/nullptr);
        if (!found) {
            return false;
        }
        if (callback == nullptr) {
            LOG(FATAL) << "Looper has the fd of interest, but the callback is null!";
            return false;
        }
        callback->handleEvent(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT, /*data=*/nullptr);
        return true;
    }

    void assertOnBatchedInputEventPendingWasCalled() {
        ASSERT_GT(mOnBatchedInputEventPendingInvocationCount, 0UL)
                << "onBatchedInputEventPending has not been called.";
        --mOnBatchedInputEventPendingInvocationCount;
    }

    std::unique_ptr<MotionEvent> assertReceivedMotionEvent(const Matcher<MotionEvent>& matcher) {
        if (mMotionEvents.empty()) {
            ADD_FAILURE() << "No motion events received";
            return nullptr;
        }
        std::unique_ptr<MotionEvent> motionEvent = std::move(mMotionEvents.front());
        mMotionEvents.pop();
        if (motionEvent == nullptr) {
            ADD_FAILURE() << "The consumed motion event should never be null";
            return nullptr;
        }
        EXPECT_THAT(*motionEvent, matcher);
        return motionEvent;
    }

    InputMessage nextPointerMessage(std::chrono::nanoseconds eventTime, DeviceId deviceId,
                                    int32_t action, const Pointer& pointer);

    std::shared_ptr<TestInputChannel> mClientTestChannel;
    sp<Looper> mLooper;
    std::unique_ptr<InputConsumerNoResampling> mConsumer;

    std::queue<std::unique_ptr<KeyEvent>> mKeyEvents;
    std::queue<std::unique_ptr<MotionEvent>> mMotionEvents;
    std::queue<std::unique_ptr<FocusEvent>> mFocusEvents;
    std::queue<std::unique_ptr<CaptureEvent>> mCaptureEvents;
    std::queue<std::unique_ptr<DragEvent>> mDragEvents;
    std::queue<std::unique_ptr<TouchModeEvent>> mTouchModeEvents;

    // Whether or not to automatically call "finish" whenever a motion event is received.
    bool mShouldFinishMotions{true};

private:
    uint32_t mLastSeq{0};
    size_t mOnBatchedInputEventPendingInvocationCount{0};

    // InputConsumerCallbacks interface
    void onKeyEvent(std::unique_ptr<KeyEvent> event, uint32_t seq) override {
        mKeyEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }
    void onMotionEvent(std::unique_ptr<MotionEvent> event, uint32_t seq) override {
        mMotionEvents.push(std::move(event));
        if (mShouldFinishMotions) {
            mConsumer->finishInputEvent(seq, /*handled=*/true);
        }
    }
    void onBatchedInputEventPending(int32_t pendingBatchSource) override {
        if (!mConsumer->probablyHasInput()) {
            ADD_FAILURE() << "should deterministically have input because there is a batch";
        }
        ++mOnBatchedInputEventPendingInvocationCount;
    };
    void onFocusEvent(std::unique_ptr<FocusEvent> event, uint32_t seq) override {
        mFocusEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    };
    void onCaptureEvent(std::unique_ptr<CaptureEvent> event, uint32_t seq) override {
        mCaptureEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    };
    void onDragEvent(std::unique_ptr<DragEvent> event, uint32_t seq) override {
        mDragEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }
    void onTouchModeEvent(std::unique_ptr<TouchModeEvent> event, uint32_t seq) override {
        mTouchModeEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    };
};

InputMessage InputConsumerTest::nextPointerMessage(std::chrono::nanoseconds eventTime,
                                                   DeviceId deviceId, int32_t action,
                                                   const Pointer& pointer) {
    ++mLastSeq;
    return InputMessageBuilder{InputMessage::Type::MOTION, mLastSeq}
            .eventTime(eventTime.count())
            .deviceId(deviceId)
            .source(AINPUT_SOURCE_TOUCHSCREEN)
            .action(action)
            .pointer(pointer.asPointerBuilder())
            .build();
}

TEST_F(InputConsumerTest, MessageStreamBatchedInMotionEvent) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .eventTime(nanoseconds{0ms}.count())
                                               .action(ACTION_DOWN)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .eventTime(nanoseconds{5ms}.count())
                                               .action(ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .eventTime(nanoseconds{10ms}.count())
                                               .action(ACTION_MOVE)
                                               .build());

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();

    assertOnBatchedInputEventPendingWasCalled();

    mConsumer->consumeBatchedInputEvents(/*frameTime=*/std::nullopt);

    assertReceivedMotionEvent(WithMotionAction(ACTION_DOWN));

    std::unique_ptr<MotionEvent> moveMotionEvent =
            assertReceivedMotionEvent(WithMotionAction(ACTION_MOVE));
    ASSERT_NE(moveMotionEvent, nullptr);
    EXPECT_EQ(moveMotionEvent->getHistorySize() + 1, 2UL);

    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
}

TEST_F(InputConsumerTest, LastBatchedSampleIsLessThanResampleTime) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .eventTime(nanoseconds{0ms}.count())
                                               .action(ACTION_DOWN)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .eventTime(nanoseconds{5ms}.count())
                                               .action(ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .eventTime(nanoseconds{10ms}.count())
                                               .action(ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/3}
                                               .eventTime(nanoseconds{15ms}.count())
                                               .action(ACTION_MOVE)
                                               .build());

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();

    assertOnBatchedInputEventPendingWasCalled();

    mConsumer->consumeBatchedInputEvents(16'000'000 /*ns*/);

    assertReceivedMotionEvent(WithMotionAction(ACTION_DOWN));

    std::unique_ptr<MotionEvent> moveMotionEvent =
            assertReceivedMotionEvent(WithMotionAction(ACTION_MOVE));
    ASSERT_NE(moveMotionEvent, nullptr);
    const size_t numSamples = moveMotionEvent->getHistorySize() + 1;
    EXPECT_LT(moveMotionEvent->getHistoricalEventTime(numSamples - 2),
              moveMotionEvent->getEventTime());

    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    // The event with seq=3 remains unconsumed, and therefore finish will not be called for it until
    // after the consumer is destroyed.
    mConsumer.reset();
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/false);
    mClientTestChannel->assertNoSentMessages();
}

/**
 * During normal operation, the user of InputConsumer (callbacks) is expected to call "finish"
 * for each input event received in InputConsumerCallbacks.
 * If the InputConsumer is destroyed, the events that were already sent to the callbacks will not
 * be finished automatically.
 */
TEST_F(InputConsumerTest, UnhandledEventsNotFinishedInDestructor) {
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}.action(ACTION_DOWN).build());
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}.action(ACTION_MOVE).build());
    mShouldFinishMotions = false;
    invokeLooperCallback();
    assertOnBatchedInputEventPendingWasCalled();
    assertReceivedMotionEvent(WithMotionAction(ACTION_DOWN));
    mClientTestChannel->assertNoSentMessages();
    // The "finishInputEvent" was not called by the InputConsumerCallbacks.
    // Now, destroy the consumer and check that the "finish" was not called automatically for the
    // DOWN event, but was called for the undelivered MOVE event.
    mConsumer.reset();
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/false);
    mClientTestChannel->assertNoSentMessages();
}

/**
 * Check what happens when looper invokes callback after consumer has been destroyed.
 * This reproduces a crash where the LooperEventCallback was added back to the Looper during
 * destructor, thus allowing the looper callback to be invoked onto a null consumer object.
 */
TEST_F(InputConsumerTest, LooperCallbackInvokedAfterConsumerDestroyed) {
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}.action(ACTION_DOWN).build());
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}.action(ACTION_MOVE).build());
    ASSERT_TRUE(invokeLooperCallback());
    assertOnBatchedInputEventPendingWasCalled();
    assertReceivedMotionEvent(WithMotionAction(ACTION_DOWN));
    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);

    // Now, destroy the consumer and invoke the looper callback again after it's been destroyed.
    mConsumer.reset();
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/false);
    ASSERT_FALSE(invokeLooperCallback());
}

/**
 * Send an event to the InputConsumer, but do not invoke "consumeBatchedInputEvents", thus leaving
 * the input event unconsumed by the callbacks. Ensure that no crash occurs when the consumer is
 * destroyed.
 * This test is similar to the one above, but here we are calling "finish"
 * automatically for any event received in the callbacks.
 */
TEST_F(InputConsumerTest, UnconsumedEventDoesNotCauseACrash) {
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}.action(ACTION_DOWN).build());
    invokeLooperCallback();
    assertReceivedMotionEvent(WithMotionAction(ACTION_DOWN));
    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->enqueueMessage(
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}.action(ACTION_MOVE).build());
    invokeLooperCallback();
    mConsumer.reset();
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/false);
}

TEST_F(InputConsumerTest, BatchedEventsMultiDeviceConsumption) {
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                                               .deviceId(0)
                                               .action(ACTION_DOWN)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(0), WithMotionAction(ACTION_DOWN)));

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/1}
                                               .deviceId(0)
                                               .action(ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/2}
                                               .deviceId(0)
                                               .action(ACTION_MOVE)
                                               .build());
    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/3}
                                               .deviceId(0)
                                               .action(ACTION_MOVE)
                                               .build());

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/4}
                                               .deviceId(1)
                                               .action(ACTION_DOWN)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(1), WithMotionAction(ACTION_DOWN)));

    mClientTestChannel->enqueueMessage(InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/5}
                                               .deviceId(0)
                                               .action(AMOTION_EVENT_ACTION_UP)
                                               .build());

    invokeLooperCallback();
    assertReceivedMotionEvent(AllOf(WithDeviceId(0), WithMotionAction(ACTION_MOVE)));

    mClientTestChannel->assertFinishMessage(/*seq=*/0, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
}

/**
 * The test supposes a 60Hz Vsync rate and a 200Hz input rate. The InputMessages are intertwined as
 * in a real use cases. The test's two devices should be resampled independently. Moreover, the
 * InputMessage stream layout for the test is:
 *
 * DOWN(0, 0ms)
 * MOVE(0, 5ms)
 * MOVE(0, 10ms)
 * DOWN(1, 15ms)
 *
 * CONSUME(16ms)
 *
 * MOVE(1, 20ms)
 * MOVE(1, 25ms)
 * MOVE(0, 30ms)
 *
 * CONSUME(32ms)
 *
 * MOVE(0, 35ms)
 * UP(1, 40ms)
 * UP(0, 45ms)
 *
 * CONSUME(48ms)
 *
 * The first field is device ID, and the second field is event time.
 */
TEST_F(InputConsumerTest, MultiDeviceResampling) {
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(0ms, /*deviceId=*/0, ACTION_DOWN, Pointer{.x = 0, .y = 0}));

    mClientTestChannel->assertNoSentMessages();

    invokeLooperCallback();
    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(ACTION_DOWN), WithSampleCount(1)));

    mClientTestChannel->enqueueMessage(
            nextPointerMessage(5ms, /*deviceId=*/0, ACTION_MOVE, Pointer{.x = 1.0f, .y = 2.0f}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(10ms, /*deviceId=*/0, ACTION_MOVE, Pointer{.x = 2.0f, .y = 4.0f}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(15ms, /*deviceId=*/1, ACTION_DOWN, Pointer{.x = 10.0f, .y = 10.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(16'000'000 /*ns*/);

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(1), WithMotionAction(ACTION_DOWN), WithSampleCount(1)));
    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{11ms,
                                    {PointerArgs{.x = 2.2f, .y = 4.4f, .isResampled = true}}})));

    mClientTestChannel->enqueueMessage(
            nextPointerMessage(20ms, /*deviceId=*/1, ACTION_MOVE, Pointer{.x = 11.0f, .y = 12.0f}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(25ms, /*deviceId=*/1, ACTION_MOVE, Pointer{.x = 12.0f, .y = 14.0f}));
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(30ms, /*deviceId=*/0, ACTION_MOVE, Pointer{.x = 5.0f, .y = 6.0f}));

    invokeLooperCallback();
    assertOnBatchedInputEventPendingWasCalled();
    mConsumer->consumeBatchedInputEvents(32'000'000 /*ns*/);

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(1), WithMotionAction(ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{27ms,
                                    {PointerArgs{.x = 12.4f, .y = 14.8f, .isResampled = true}}})));

    mClientTestChannel->enqueueMessage(
            nextPointerMessage(35ms, /*deviceId=*/0, ACTION_MOVE, Pointer{.x = 8.0f, .y = 9.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(40ms, /*deviceId=*/1,
                                                          AMOTION_EVENT_ACTION_UP,
                                                          Pointer{.x = 12.0f, .y = 14.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(45ms, /*deviceId=*/0,
                                                          AMOTION_EVENT_ACTION_UP,
                                                          Pointer{.x = 8.0f, .y = 9.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(48'000'000 /*ns*/);

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(1), WithMotionAction(AMOTION_EVENT_ACTION_UP), WithSampleCount(1)));

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(ACTION_MOVE), WithSampleCount(3),
                  WithSample(/*sampleIndex=*/2,
                             Sample{37'500'000ns,
                                    {PointerArgs{.x = 9.5f, .y = 10.5f, .isResampled = true}}})));

    assertReceivedMotionEvent(
            AllOf(WithDeviceId(0), WithMotionAction(AMOTION_EVENT_ACTION_UP), WithSampleCount(1)));

    // The sequence order is based on the expected consumption. Each sequence number corresponds to
    // one of the previously enqueued messages.
    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/5, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/6, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/9, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/7, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/8, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/10, /*handled=*/true);
}

} // namespace android
