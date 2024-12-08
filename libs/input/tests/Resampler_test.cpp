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

#include <input/Resampler.h>

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <vector>

#include <input/Input.h>
#include <input/InputEventBuilders.h>
#include <input/InputTransport.h>
#include <utils/Timers.h>

namespace android {

namespace {

using namespace std::literals::chrono_literals;

constexpr float EPSILON = MotionEvent::ROUNDING_PRECISION;

struct Pointer {
    int32_t id{0};
    ToolType toolType{ToolType::FINGER};
    float x{0.0f};
    float y{0.0f};
    bool isResampled{false};
    /**
     * Converts from Pointer to PointerCoords. Enables calling LegacyResampler methods and
     * assertions only with the relevant data for tests.
     */
    operator PointerCoords() const;
};

Pointer::operator PointerCoords() const {
    PointerCoords pointerCoords;
    pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_X, x);
    pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_Y, y);
    pointerCoords.isResampled = isResampled;
    return pointerCoords;
}

struct InputSample {
    std::chrono::milliseconds eventTime{0};
    std::vector<Pointer> pointers{};

    explicit InputSample(std::chrono::milliseconds eventTime, const std::vector<Pointer>& pointers)
          : eventTime{eventTime}, pointers{pointers} {}
    /**
     * Converts from InputSample to InputMessage. Enables calling LegacyResampler methods only with
     * the relevant data for tests.
     */
    operator InputMessage() const;
};

InputSample::operator InputMessage() const {
    InputMessageBuilder messageBuilder =
            InputMessageBuilder{InputMessage::Type::MOTION, /*seq=*/0}
                    .eventTime(std::chrono::nanoseconds{eventTime}.count())
                    .source(AINPUT_SOURCE_TOUCHSCREEN)
                    .downTime(0);

    for (const Pointer& pointer : pointers) {
        messageBuilder.pointer(
                PointerBuilder{pointer.id, pointer.toolType}.x(pointer.x).y(pointer.y).isResampled(
                        pointer.isResampled));
    }
    return messageBuilder.build();
}

struct InputStream {
    std::vector<InputSample> samples{};
    int32_t action{0};
    DeviceId deviceId{0};
    /**
     * Converts from InputStream to MotionEvent. Enables calling LegacyResampler methods only with
     * the relevant data for tests.
     */
    operator MotionEvent() const;
};

InputStream::operator MotionEvent() const {
    const InputSample& firstSample{*samples.begin()};
    MotionEventBuilder motionEventBuilder =
            MotionEventBuilder(action, AINPUT_SOURCE_CLASS_POINTER)
                    .downTime(0)
                    .eventTime(static_cast<std::chrono::nanoseconds>(firstSample.eventTime).count())
                    .deviceId(deviceId);
    for (const Pointer& pointer : firstSample.pointers) {
        const PointerBuilder pointerBuilder =
                PointerBuilder(pointer.id, pointer.toolType).x(pointer.x).y(pointer.y);
        motionEventBuilder.pointer(pointerBuilder);
    }
    MotionEvent motionEvent = motionEventBuilder.build();
    const size_t numSamples = samples.size();
    for (size_t i = 1; i < numSamples; ++i) {
        std::vector<PointerCoords> pointersCoords{samples[i].pointers.begin(),
                                                  samples[i].pointers.end()};
        motionEvent.addSample(static_cast<std::chrono::nanoseconds>(samples[i].eventTime).count(),
                              pointersCoords.data(), motionEvent.getId());
    }
    return motionEvent;
}

} // namespace

/**
 * The testing setup assumes an input rate of 200 Hz and a display rate of 60 Hz. This implies that
 * input events are received every 5 milliseconds, while the display consumes batched events every
 * ~16 milliseconds. The resampler's RESAMPLE_LATENCY constant determines the resample time, which
 * is calculated as frameTime - RESAMPLE_LATENCY. resampleTime specifies the time used for
 * resampling. For example, if the desired frame time consumption is ~16 milliseconds, the resample
 * time would be ~11 milliseconds. Consequenly, the last added sample to the motion event has an
 * event time of ~11 milliseconds. Note that there are specific scenarios where resampleMotionEvent
 * is not called with a multiple of ~16 milliseconds. These cases are primarily for data addition
 * or to test other functionalities of the resampler.
 *
 * Coordinates are calculated using linear interpolation (lerp) based on the last two available
 * samples. Linear interpolation is defined as (a + alpha*(b - a)). Let t_b and t_a be the
 * timestamps of samples a and b, respectively. The interpolation factor alpha is calculated as
 * (resampleTime - t_a) / (t_b - t_a). The value of alpha determines whether the resampled
 * coordinates are interpolated or extrapolated. If alpha falls within the semi-closed interval [0,
 * 1), the coordinates are interpolated. If alpha is greater than or equal to 1, the coordinates are
 * extrapolated.
 *
 * The timeline below depics an interpolation scenario
 * -----------------------------------|---------|---------|---------|----------
 *                                   10ms      11ms      15ms      16ms
 *                                   MOVE       |        MOVE       |
 *                                         resampleTime         frameTime
 * Based on the timeline alpha is (11 - 10)/(15 - 10) = 1/5. Thus, coordinates are interpolated.
 *
 * The following timeline portrays an extrapolation scenario
 * -------------------------|---------|---------|-------------------|----------
 *                          5ms      10ms      11ms                16ms
 *                          MOVE     MOVE       |                   |
 *                                         resampleTime         frameTime
 * Likewise, alpha = (11 - 5)/(10 - 5) = 6/5. Hence, coordinates are extrapolated.
 *
 * If a motion event was resampled, the tests will check that the following conditions are satisfied
 * to guarantee resampling correctness:
 * - The motion event metadata must not change.
 * - The number of samples in the motion event must only increment by 1.
 * - The resampled values must be at the end of motion event coordinates.
 * - The rasamples values must be near the hand calculations.
 * - The resampled time must be the most recent one in motion event.
 */
class ResamplerTest : public testing::Test {
protected:
    ResamplerTest() : mResampler(std::make_unique<LegacyResampler>()) {}

    ~ResamplerTest() override {}

    void SetUp() override {}

    void TearDown() override {}

    std::unique_ptr<Resampler> mResampler;

    /**
     * Checks that beforeCall and afterCall are equal except for the mutated attributes by addSample
     * member function.
     * @param beforeCall MotionEvent before passing it to resampleMotionEvent
     * @param afterCall MotionEvent after passing it to resampleMotionEvent
     */
    void assertMotionEventMetaDataDidNotMutate(const MotionEvent& beforeCall,
                                               const MotionEvent& afterCall);

    /**
     * Asserts the MotionEvent is resampled by checking an increment in history size and that the
     * resampled coordinates are near the expected ones.
     */
    void assertMotionEventIsResampledAndCoordsNear(
            const MotionEvent& original, const MotionEvent& resampled,
            const std::vector<PointerCoords>& expectedCoords);

    void assertMotionEventIsNotResampled(const MotionEvent& original,
                                         const MotionEvent& notResampled);
};

void ResamplerTest::assertMotionEventMetaDataDidNotMutate(const MotionEvent& beforeCall,
                                                          const MotionEvent& afterCall) {
    EXPECT_EQ(beforeCall.getDeviceId(), afterCall.getDeviceId());
    EXPECT_EQ(beforeCall.getAction(), afterCall.getAction());
    EXPECT_EQ(beforeCall.getActionButton(), afterCall.getActionButton());
    EXPECT_EQ(beforeCall.getButtonState(), afterCall.getButtonState());
    EXPECT_EQ(beforeCall.getFlags(), afterCall.getFlags());
    EXPECT_EQ(beforeCall.getEdgeFlags(), afterCall.getEdgeFlags());
    EXPECT_EQ(beforeCall.getClassification(), afterCall.getClassification());
    EXPECT_EQ(beforeCall.getPointerCount(), afterCall.getPointerCount());
    EXPECT_EQ(beforeCall.getMetaState(), afterCall.getMetaState());
    EXPECT_EQ(beforeCall.getSource(), afterCall.getSource());
    EXPECT_EQ(beforeCall.getXPrecision(), afterCall.getXPrecision());
    EXPECT_EQ(beforeCall.getYPrecision(), afterCall.getYPrecision());
    EXPECT_EQ(beforeCall.getDownTime(), afterCall.getDownTime());
    EXPECT_EQ(beforeCall.getDisplayId(), afterCall.getDisplayId());
}

void ResamplerTest::assertMotionEventIsResampledAndCoordsNear(
        const MotionEvent& original, const MotionEvent& resampled,
        const std::vector<PointerCoords>& expectedCoords) {
    assertMotionEventMetaDataDidNotMutate(original, resampled);

    const size_t originalSampleSize = original.getHistorySize() + 1;
    const size_t resampledSampleSize = resampled.getHistorySize() + 1;
    EXPECT_EQ(originalSampleSize + 1, resampledSampleSize);

    const size_t numPointers = resampled.getPointerCount();
    const size_t beginLatestSample = resampledSampleSize - 1;
    for (size_t i = 0; i < numPointers; ++i) {
        SCOPED_TRACE(i);
        EXPECT_EQ(original.getPointerId(i), resampled.getPointerId(i));
        EXPECT_EQ(original.getToolType(i), resampled.getToolType(i));

        const PointerCoords& resampledCoords =
                resampled.getSamplePointerCoords()[beginLatestSample * numPointers + i];

        EXPECT_TRUE(resampledCoords.isResampled);
        EXPECT_NEAR(expectedCoords[i].getX(), resampledCoords.getX(), EPSILON);
        EXPECT_NEAR(expectedCoords[i].getY(), resampledCoords.getY(), EPSILON);
    }
}

void ResamplerTest::assertMotionEventIsNotResampled(const MotionEvent& original,
                                                    const MotionEvent& notResampled) {
    assertMotionEventMetaDataDidNotMutate(original, notResampled);
    const size_t originalSampleSize = original.getHistorySize() + 1;
    const size_t notResampledSampleSize = notResampled.getHistorySize() + 1;
    EXPECT_EQ(originalSampleSize, notResampledSampleSize);
}

TEST_F(ResamplerTest, NonResampledAxesArePreserved) {
    constexpr float TOUCH_MAJOR_VALUE = 1.0f;

    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    constexpr std::chrono::nanoseconds eventTime{10ms};
    PointerCoords pointerCoords{};
    pointerCoords.isResampled = false;
    pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_X, 2.0f);
    pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_Y, 2.0f);
    pointerCoords.setAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR, TOUCH_MAJOR_VALUE);

    motionEvent.addSample(eventTime.count(), &pointerCoords, motionEvent.getId());

    const InputMessage futureSample =
            InputSample{15ms, {{.id = 0, .x = 3.0f, .y = 4.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    EXPECT_EQ(motionEvent.getTouchMajor(0), TOUCH_MAJOR_VALUE);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 2.2f,
                                                       .y = 2.4f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, SinglePointerNotEnoughDataToResample) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, SinglePointerDifferentDeviceIdBetweenMotionEvents) {
    MotionEvent motionFromFirstDevice =
            InputStream{{InputSample{4ms, {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false}}},
                         InputSample{8ms, {{.id = 0, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE,
                        .deviceId = 0};

    mResampler->resampleMotionEvent(10ms, motionFromFirstDevice, nullptr);

    MotionEvent motionFromSecondDevice =
            InputStream{{InputSample{11ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE,
                        .deviceId = 1};
    const MotionEvent originalMotionEvent = motionFromSecondDevice;

    mResampler->resampleMotionEvent(12ms, motionFromSecondDevice, nullptr);
    // The MotionEvent should not be resampled because the second event came from a different device
    // than the previous event.
    assertMotionEventIsNotResampled(originalMotionEvent, motionFromSecondDevice);
}

TEST_F(ResamplerTest, SinglePointerSingleSampleInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};
    const InputMessage futureSample =
            InputSample{15ms, {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 1.2f,
                                                       .y = 2.4f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, SinglePointerDeltaTooSmallInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};
    const InputMessage futureSample =
            InputSample{11ms, {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(10'500'000ns, motionEvent, &futureSample);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

/**
 * Tests extrapolation given two MotionEvents with a single sample.
 */
TEST_F(ResamplerTest, SinglePointerSingleSampleExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, nullptr);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, secondMotionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 2.2f,
                                                       .y = 4.4f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, SinglePointerMultipleSampleInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{10ms,
                                     {{.id = 0, .x = 2.0f, .y = 3.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms, {{.id = 0, .x = 3.0f, .y = 5.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 2.2f,
                                                       .y = 3.4f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, SinglePointerMultipleSampleExtrapolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{10ms,
                                     {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, nullptr);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 2.2f,
                                                       .y = 4.4f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, SinglePointerDeltaTooSmallExtrapolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{9ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{10ms,
                                     {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, nullptr);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, SinglePointerDeltaTooLargeExtrapolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{26ms,
                                     {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(32ms, motionEvent, nullptr);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, SinglePointerResampleTimeTooFarExtrapolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms, {{.id = 0, .x = 1.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{25ms,
                                     {{.id = 0, .x = 2.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(48ms, motionEvent, nullptr);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.id = 0,
                                                       .x = 2.4f,
                                                       .y = 4.8f,
                                                       .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerSingleSampleInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                         {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.x = 2.2f, .y = 2.2f, .isResampled = true},
                                               Pointer{.x = 3.2f, .y = 3.2f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerSingleSampleExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, secondMotionEvent,
                                              {Pointer{.x = 3.4f, .y = 3.4f, .isResampled = true},
                                               Pointer{.x = 4.4f, .y = 4.4f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerMultipleSampleInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};
    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 0, .x = 5.0f, .y = 5.0f, .isResampled = false},
                         {.id = 1, .x = 6.0f, .y = 6.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.x = 3.4f, .y = 3.4f, .isResampled = true},
                                               Pointer{.x = 4.4f, .y = 4.4f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerMultipleSampleExtrapolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}},
                         InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.x = 3.4f, .y = 3.4f, .isResampled = true},
                                               Pointer{.x = 4.4f, .y = 4.4f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerIncreaseNumPointersInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                         {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                         {.id = 2, .x = 5.0f, .y = 5.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsResampledAndCoordsNear(originalMotionEvent, motionEvent,
                                              {Pointer{.x = 1.4f, .y = 1.4f, .isResampled = true},
                                               Pointer{.x = 2.4f, .y = 2.4f, .isResampled = true}});

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{25ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                                      {.id = 2, .x = 5.0f, .y = 5.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage secondFutureSample =
            InputSample{30ms,
                        {{.id = 0, .x = 5.0f, .y = 5.0f, .isResampled = false},
                         {.id = 1, .x = 6.0f, .y = 6.0f, .isResampled = false},
                         {.id = 2, .x = 7.0f, .y = 7.0f, .isResampled = false}}};

    const MotionEvent originalSecondMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(32ms, secondMotionEvent, &secondFutureSample);

    assertMotionEventIsResampledAndCoordsNear(originalSecondMotionEvent, secondMotionEvent,
                                              {Pointer{.x = 3.8f, .y = 3.8f, .isResampled = true},
                                               Pointer{.x = 4.8f, .y = 4.8f, .isResampled = true},
                                               Pointer{.x = 5.8f, .y = 5.8f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerIncreaseNumPointersExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                                      {.id = 2, .x = 5.0f, .y = 5.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent secondOriginalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(secondOriginalMotionEvent, secondMotionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDecreaseNumPointersInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                                      {.id = 2, .x = 5.0f, .y = 5.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 0, .x = 4.0f, .y = 4.0f, .isResampled = false},
                         {.id = 1, .x = 5.0f, .y = 5.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDecreaseNumPointersExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false},
                                      {.id = 2, .x = 3.0f, .y = 3.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false},
                                      {.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent secondOriginalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsResampledAndCoordsNear(secondOriginalMotionEvent, secondMotionEvent,
                                              {Pointer{.x = 3.4f, .y = 3.4f, .isResampled = true},
                                               Pointer{.x = 4.4f, .y = 4.4f, .isResampled = true}});
}

TEST_F(ResamplerTest, MultiplePointerDifferentIdOrderInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                         {.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDifferentIdOrderExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                                      {.id = 0, .x = 3.0f, .y = 3.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent secondOriginalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(secondOriginalMotionEvent, secondMotionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDifferentIdsInterpolation) {
    MotionEvent motionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample =
            InputSample{15ms,
                        {{.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                         {.id = 2, .x = 3.0f, .y = 3.0f, .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDifferentIdsExtrapolation) {
    MotionEvent firstMotionEvent =
            InputStream{{InputSample{5ms,
                                     {{.id = 0, .x = 1.0f, .y = 1.0f, .isResampled = false},
                                      {.id = 1, .x = 2.0f, .y = 2.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent =
            InputStream{{InputSample{10ms,
                                     {{.id = 1, .x = 4.0f, .y = 4.0f, .isResampled = false},
                                      {.id = 2, .x = 3.0f, .y = 3.0f, .isResampled = false}}}},
                        AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent secondOriginalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(secondOriginalMotionEvent, secondMotionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDifferentToolTypeInterpolation) {
    MotionEvent motionEvent = InputStream{{InputSample{10ms,
                                                       {{.id = 0,
                                                         .toolType = ToolType::FINGER,
                                                         .x = 1.0f,
                                                         .y = 1.0f,
                                                         .isResampled = false},
                                                        {.id = 1,
                                                         .toolType = ToolType::FINGER,
                                                         .x = 2.0f,
                                                         .y = 2.0f,
                                                         .isResampled = false}}}},
                                          AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample = InputSample{15ms,
                                                  {{.id = 0,
                                                    .toolType = ToolType::FINGER,
                                                    .x = 3.0,
                                                    .y = 3.0,
                                                    .isResampled = false},
                                                   {.id = 1,
                                                    .toolType = ToolType::STYLUS,
                                                    .x = 4.0,
                                                    .y = 4.0,
                                                    .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, &futureSample);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, MultiplePointerDifferentToolTypeExtrapolation) {
    MotionEvent firstMotionEvent = InputStream{{InputSample{5ms,
                                                            {{.id = 0,
                                                              .toolType = ToolType::FINGER,
                                                              .x = 1.0f,
                                                              .y = 1.0f,
                                                              .isResampled = false},
                                                             {.id = 1,
                                                              .toolType = ToolType::FINGER,
                                                              .x = 2.0f,
                                                              .y = 2.0f,
                                                              .isResampled = false}}}},
                                               AMOTION_EVENT_ACTION_MOVE};

    mResampler->resampleMotionEvent(9ms, firstMotionEvent, /*futureSample=*/nullptr);

    MotionEvent secondMotionEvent = InputStream{{InputSample{10ms,
                                                             {{.id = 0,
                                                               .toolType = ToolType::FINGER,
                                                               .x = 1.0f,
                                                               .y = 1.0f,
                                                               .isResampled = false},
                                                              {.id = 1,
                                                               .toolType = ToolType::STYLUS,
                                                               .x = 2.0f,
                                                               .y = 2.0f,
                                                               .isResampled = false}}}},
                                                AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent secondOriginalMotionEvent = secondMotionEvent;

    mResampler->resampleMotionEvent(16ms, secondMotionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(secondOriginalMotionEvent, secondMotionEvent);
}

TEST_F(ResamplerTest, MultiplePointerShouldNotResampleToolTypeInterpolation) {
    MotionEvent motionEvent = InputStream{{InputSample{10ms,
                                                       {{.id = 0,
                                                         .toolType = ToolType::PALM,
                                                         .x = 1.0f,
                                                         .y = 1.0f,
                                                         .isResampled = false},
                                                        {.id = 1,
                                                         .toolType = ToolType::PALM,
                                                         .x = 2.0f,
                                                         .y = 2.0f,
                                                         .isResampled = false}}}},
                                          AMOTION_EVENT_ACTION_MOVE};

    const InputMessage futureSample = InputSample{15ms,
                                                  {{.id = 0,
                                                    .toolType = ToolType::PALM,
                                                    .x = 3.0,
                                                    .y = 3.0,
                                                    .isResampled = false},
                                                   {.id = 1,
                                                    .toolType = ToolType::PALM,
                                                    .x = 4.0,
                                                    .y = 4.0,
                                                    .isResampled = false}}};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}

TEST_F(ResamplerTest, MultiplePointerShouldNotResampleToolTypeExtrapolation) {
    MotionEvent motionEvent = InputStream{{InputSample{5ms,
                                                       {{.id = 0,
                                                         .toolType = ToolType::PALM,
                                                         .x = 1.0f,
                                                         .y = 1.0f,
                                                         .isResampled = false},
                                                        {.id = 1,
                                                         .toolType = ToolType::PALM,
                                                         .x = 2.0f,
                                                         .y = 2.0f,
                                                         .isResampled = false}}},
                                           InputSample{10ms,
                                                       {{.id = 0,
                                                         .toolType = ToolType::PALM,
                                                         .x = 3.0f,
                                                         .y = 3.0f,
                                                         .isResampled = false},
                                                        {.id = 1,
                                                         .toolType = ToolType::PALM,
                                                         .x = 4.0f,
                                                         .y = 4.0f,
                                                         .isResampled = false}}}},
                                          AMOTION_EVENT_ACTION_MOVE};

    const MotionEvent originalMotionEvent = motionEvent;

    mResampler->resampleMotionEvent(16ms, motionEvent, /*futureSample=*/nullptr);

    assertMotionEventIsNotResampled(originalMotionEvent, motionEvent);
}
} // namespace android
