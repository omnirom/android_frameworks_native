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

#include <chrono>
#include <iostream>
#include <memory>
#include <queue>

#include <TestEventMatchers.h>
#include <TestInputChannel.h>
#include <android-base/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
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

const int32_t ACTION_DOWN = AMOTION_EVENT_ACTION_DOWN;
const int32_t ACTION_MOVE = AMOTION_EVENT_ACTION_MOVE;

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

class InputConsumerFilteredResamplingTest : public ::testing::Test, public InputConsumerCallbacks {
protected:
    InputConsumerFilteredResamplingTest()
          : mClientTestChannel{std::make_shared<TestInputChannel>("TestChannel")},
            mLooper{sp<Looper>::make(/*allowNonCallbacks=*/false)} {
        Looper::setForThread(mLooper);
        mConsumer = std::make_unique<
                InputConsumerNoResampling>(mClientTestChannel, mLooper, *this, []() {
            return std::make_unique<FilteredLegacyResampler>(/*minCutoffFreq=*/4.7, /*beta=*/0.01);
        });
    }

    void invokeLooperCallback() const {
        sp<LooperCallback> callback;
        ASSERT_TRUE(mLooper->getFdStateDebug(mClientTestChannel->getFd(), /*ident=*/nullptr,
                                             /*events=*/nullptr, &callback, /*data=*/nullptr));
        ASSERT_NE(callback, nullptr);
        callback->handleEvent(mClientTestChannel->getFd(), ALOOPER_EVENT_INPUT, /*data=*/nullptr);
    }

    void assertOnBatchedInputEventPendingWasCalled() {
        ASSERT_GT(mOnBatchedInputEventPendingInvocationCount, 0UL)
                << "onBatchedInputEventPending was not called";
        --mOnBatchedInputEventPendingInvocationCount;
    }

    void assertReceivedMotionEvent(const Matcher<MotionEvent>& matcher) {
        ASSERT_TRUE(!mMotionEvents.empty()) << "No motion events were received";
        std::unique_ptr<MotionEvent> motionEvent = std::move(mMotionEvents.front());
        mMotionEvents.pop();
        ASSERT_NE(motionEvent, nullptr) << "The consumed motion event must not be nullptr";
        EXPECT_THAT(*motionEvent, matcher);
    }

    InputMessage nextPointerMessage(nanoseconds eventTime, int32_t action, const Pointer& pointer);

    std::shared_ptr<TestInputChannel> mClientTestChannel;
    sp<Looper> mLooper;
    std::unique_ptr<InputConsumerNoResampling> mConsumer;

    // Batched input events
    std::queue<std::unique_ptr<KeyEvent>> mKeyEvents;
    std::queue<std::unique_ptr<MotionEvent>> mMotionEvents;
    std::queue<std::unique_ptr<FocusEvent>> mFocusEvents;
    std::queue<std::unique_ptr<CaptureEvent>> mCaptureEvents;
    std::queue<std::unique_ptr<DragEvent>> mDragEvents;
    std::queue<std::unique_ptr<TouchModeEvent>> mTouchModeEvents;

private:
    // InputConsumer callbacks
    void onKeyEvent(std::unique_ptr<KeyEvent> event, uint32_t seq) override {
        mKeyEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    void onMotionEvent(std::unique_ptr<MotionEvent> event, uint32_t seq) override {
        mMotionEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    void onBatchedInputEventPending(int32_t pendingBatchSource) override {
        if (!mConsumer->probablyHasInput()) {
            ADD_FAILURE() << "Should deterministically have input because there is a batch";
        }
        ++mOnBatchedInputEventPendingInvocationCount;
    }

    void onFocusEvent(std::unique_ptr<FocusEvent> event, uint32_t seq) override {
        mFocusEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    void onCaptureEvent(std::unique_ptr<CaptureEvent> event, uint32_t seq) override {
        mCaptureEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    void onDragEvent(std::unique_ptr<DragEvent> event, uint32_t seq) override {
        mDragEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    void onTouchModeEvent(std::unique_ptr<TouchModeEvent> event, uint32_t seq) override {
        mTouchModeEvents.push(std::move(event));
        mConsumer->finishInputEvent(seq, /*handled=*/true);
    }

    uint32_t mLastSeq{0};
    size_t mOnBatchedInputEventPendingInvocationCount{0};
};

InputMessage InputConsumerFilteredResamplingTest::nextPointerMessage(nanoseconds eventTime,
                                                                     int32_t action,
                                                                     const Pointer& pointer) {
    ++mLastSeq;
    return InputMessageBuilder{InputMessage::Type::MOTION, mLastSeq}
            .eventTime(eventTime.count())
            .source(AINPUT_SOURCE_TOUCHSCREEN)
            .action(action)
            .pointer(pointer.asPointerBuilder())
            .build();
}

TEST_F(InputConsumerFilteredResamplingTest, NeighboringTimestampsDoNotResultInZeroDivision) {
    mClientTestChannel->enqueueMessage(
            nextPointerMessage(0ms, ACTION_DOWN, Pointer{.x = 0.0f, .y = 0.0f}));

    invokeLooperCallback();

    assertReceivedMotionEvent(AllOf(WithMotionAction(ACTION_DOWN), WithSampleCount(1)));

    const std::chrono::nanoseconds initialTime{56'821'700'000'000};

    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 4'929'000ns, ACTION_MOVE,
                                                          Pointer{.x = 1.0f, .y = 1.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 9'352'000ns, ACTION_MOVE,
                                                          Pointer{.x = 2.0f, .y = 2.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 14'531'000ns, ACTION_MOVE,
                                                          Pointer{.x = 3.0f, .y = 3.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(initialTime.count() + 18'849'395 /*ns*/);

    assertOnBatchedInputEventPendingWasCalled();
    // Three samples are expected. The first two of the batch, and the resampled one. The
    // coordinates of the resampled sample are hardcoded because the matcher requires them. However,
    // the primary intention here is to check that the last sample is resampled.
    assertReceivedMotionEvent(AllOf(WithMotionAction(ACTION_MOVE), WithSampleCount(3),
                                    WithSample(/*sampleIndex=*/2,
                                               Sample{initialTime + 13'849'395ns,
                                                      {PointerArgs{.x = 1.3286f,
                                                                   .y = 1.3286f,
                                                                   .isResampled = true}}})));

    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 20'363'000ns, ACTION_MOVE,
                                                          Pointer{.x = 4.0f, .y = 4.0f}));
    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 25'745'000ns, ACTION_MOVE,
                                                          Pointer{.x = 5.0f, .y = 5.0f}));
    // This sample is part of the stream of messages, but should not be consumed because its
    // timestamp is greater than the ajusted frame time.
    mClientTestChannel->enqueueMessage(nextPointerMessage(initialTime + 31'337'000ns, ACTION_MOVE,
                                                          Pointer{.x = 6.0f, .y = 6.0f}));

    invokeLooperCallback();
    mConsumer->consumeBatchedInputEvents(initialTime.count() + 35'516'062 /*ns*/);

    assertOnBatchedInputEventPendingWasCalled();
    // Four samples are expected because the last sample of the previous batch was not consumed.
    assertReceivedMotionEvent(AllOf(WithMotionAction(ACTION_MOVE), WithSampleCount(4)));

    mClientTestChannel->assertFinishMessage(/*seq=*/1, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/2, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/3, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/4, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/5, /*handled=*/true);
    mClientTestChannel->assertFinishMessage(/*seq=*/6, /*handled=*/true);
}

} // namespace android
