/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android_os_vibrator.h>
#include <flag_macros.h>
#include <gtest/gtest.h>
#include <vibrator/ExternalVibrationUtils.h>

#include "test_utils.h"

#define FLAG_NS android::os::vibrator

using namespace android;
using namespace testing;

using HapticScale = os::HapticScale;
using HapticLevel = os::HapticLevel;

static constexpr float TEST_TOLERANCE = 1e-2f;
static constexpr size_t TEST_BUFFER_LENGTH = 4;
static float TEST_BUFFER[TEST_BUFFER_LENGTH] = { 1, -1, 0.5f, -0.2f };

class ExternalVibrationUtilsTest : public Test {
public:
    void SetUp() override {
        std::copy(std::begin(TEST_BUFFER), std::end(TEST_BUFFER), std::begin(mBuffer));
    }

protected:
    void scaleBuffer(HapticLevel hapticLevel) {
        scaleBuffer(HapticScale(hapticLevel));
    }

    void scaleBuffer(HapticLevel hapticLevel, float adaptiveScaleFactor) {
        scaleBuffer(hapticLevel, adaptiveScaleFactor, 0 /* limit */);
    }

    void scaleBuffer(HapticLevel hapticLevel, float adaptiveScaleFactor, float limit) {
        scaleBuffer(HapticScale(hapticLevel, -1 /* scaleFactor */, adaptiveScaleFactor), limit);
    }

    void scaleBuffer(HapticScale hapticScale) {
        scaleBuffer(hapticScale, 0 /* limit */);
    }

    void scaleBuffer(HapticScale hapticScale, float limit) {
        std::copy(std::begin(TEST_BUFFER), std::end(TEST_BUFFER), std::begin(mBuffer));
        os::scaleHapticData(&mBuffer[0], TEST_BUFFER_LENGTH, hapticScale, limit);
    }

    float mBuffer[TEST_BUFFER_LENGTH];
};

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestLegacyScaleMute,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expected), std::end(expected), 0);

    scaleBuffer(HapticLevel::MUTE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestFixedScaleMute,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expected), std::end(expected), 0);

    scaleBuffer(HapticLevel::MUTE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestScaleV2Mute,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expected), std::end(expected), 0);

    scaleBuffer(HapticLevel::MUTE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestLegacyScaleNone,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::copy(std::begin(TEST_BUFFER), std::end(TEST_BUFFER), std::begin(expected));

    scaleBuffer(HapticLevel::NONE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestFixedScaleNone,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::copy(std::begin(TEST_BUFFER), std::end(TEST_BUFFER), std::begin(expected));

    scaleBuffer(HapticLevel::NONE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestScaleV2None,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expected[TEST_BUFFER_LENGTH];
    std::copy(std::begin(TEST_BUFFER), std::end(TEST_BUFFER), std::begin(expected));

    scaleBuffer(HapticLevel::NONE);
    EXPECT_FLOATS_NEARLY_EQ(expected, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestLegacyScaleToHapticLevel,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.84f, -0.66f };
    scaleBuffer(HapticLevel::VERY_HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.7f, -0.44f };
    scaleBuffer(HapticLevel::HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedLow[TEST_BUFFER_LENGTH] = { 0.75f, -0.75f, 0.26f, -0.06f };
    scaleBuffer(HapticLevel::LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 0.66f, -0.66f, 0.16f, -0.02f };
    scaleBuffer(HapticLevel::VERY_LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestFixedScaleToHapticLevel,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.79f, -0.39f };
    scaleBuffer(HapticLevel::VERY_HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.62f, -0.27f };
    scaleBuffer(HapticLevel::HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedLow[TEST_BUFFER_LENGTH] = { 0.70f, -0.70f, 0.35f, -0.14f };
    scaleBuffer(HapticLevel::LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 0.45f, -0.45f, 0.22f, -0.09f };
    scaleBuffer(HapticLevel::VERY_LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestScaleV2ToHapticLevel,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.8f, -0.38f };
    scaleBuffer(HapticLevel::VERY_HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.63f, -0.27f };
    scaleBuffer(HapticLevel::HIGH);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedLow[TEST_BUFFER_LENGTH] = { 0.71f, -0.71f, 0.35f, -0.14f };
    scaleBuffer(HapticLevel::LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 0.51f, -0.51f, 0.25f, -0.1f };
    scaleBuffer(HapticLevel::VERY_LOW);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestScaleV2ToScaleFactorUndefinedUsesHapticLevel,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    constexpr float adaptiveScaleNone = 1.0f;
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = {1, -1, 0.8f, -0.38f};
    scaleBuffer(HapticScale(HapticLevel::VERY_HIGH, -1.0f /* scaleFactor */, adaptiveScaleNone));
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestScaleV2ToScaleFactorIgnoresLevel,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    constexpr float adaptiveScaleNone = 1.0f;

    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 1, -1, 1, -0.55f };
    scaleBuffer(HapticScale(HapticLevel::LOW, 3.0f /* scaleFactor */, adaptiveScaleNone));
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedHigh[TEST_BUFFER_LENGTH] = { 1, -1, 0.66f, -0.29f };
    scaleBuffer(HapticScale(HapticLevel::LOW, 1.5f /* scaleFactor */, adaptiveScaleNone));
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedLow[TEST_BUFFER_LENGTH] = { 0.8f, -0.8f, 0.4f, -0.16f };
    scaleBuffer(HapticScale(HapticLevel::HIGH, 0.8f /* scaleFactor */, adaptiveScaleNone));
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 0.4f, -0.4f, 0.2f, -0.08f };
    scaleBuffer(HapticScale(HapticLevel::HIGH, 0.4f /* scaleFactor */, adaptiveScaleNone));
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestAdaptiveScaleFactorUndefinedIsIgnoredLegacyScale,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = {1, -1, 0.79f, -0.39f};
    scaleBuffer(HapticLevel::VERY_HIGH, -1.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestAdaptiveScaleFactorAppliedAfterLegacyScale,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Adaptive scale mutes vibration
    float expectedMuted[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expectedMuted), std::end(expectedMuted), 0);
    scaleBuffer(HapticLevel::VERY_HIGH, 0.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedMuted, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale down
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 0.2, -0.2, 0.16f, -0.13f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale up
    float expectedHigh[TEST_BUFFER_LENGTH] = { 1.5f, -1.5f, 1.06f, -0.67f };
    scaleBuffer(HapticLevel::HIGH, 1.5f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale down
    float expectedLow[TEST_BUFFER_LENGTH] = { 0.45f, -0.45f, 0.15f, -0.04f };
    scaleBuffer(HapticLevel::LOW, 0.6f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale up
    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 1.33f, -1.33f, 0.33f, -0.05f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestAdaptiveScaleFactorUndefinedIgnoredFixedScale,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = {1, -1, 0.79f, -0.39f};
    scaleBuffer(HapticLevel::VERY_HIGH, -1.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestAdaptiveScaleFactorAppliedAfterFixedScale,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Adaptive scale mutes vibration
    float expectedMuted[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expectedMuted), std::end(expectedMuted), 0);
    scaleBuffer(HapticLevel::VERY_HIGH, 0.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedMuted, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale down
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 0.2, -0.2, 0.16f, -0.07f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale up
    float expectedHigh[TEST_BUFFER_LENGTH] = { 1.5f, -1.5f, 0.93f, -0.41f };
    scaleBuffer(HapticLevel::HIGH, 1.5f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale down
    float expectedLow[TEST_BUFFER_LENGTH] = { 0.42f, -0.42f, 0.21f, -0.08f };
    scaleBuffer(HapticLevel::LOW, 0.6f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale up
    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 0.91f, -0.91f, 0.45f, -0.18f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestAdaptiveScaleFactorUndefinedIgnoredScaleV2,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = {1, -1, 0.8f, -0.38f};
    scaleBuffer(HapticLevel::VERY_HIGH, -1.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestAdaptiveScaleFactorAppliedAfterScaleV2,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Adaptive scale mutes vibration
    float expectedMuted[TEST_BUFFER_LENGTH];
    std::fill(std::begin(expectedMuted), std::end(expectedMuted), 0);
    scaleBuffer(HapticLevel::VERY_HIGH, 0.0f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedMuted, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale down
    float expectedVeryHigh[TEST_BUFFER_LENGTH] = { 0.2, -0.2, 0.15f, -0.07f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale up then adaptive scale up
    float expectedHigh[TEST_BUFFER_LENGTH] = { 1.5f, -1.5f, 0.95f, -0.41f };
    scaleBuffer(HapticLevel::HIGH, 1.5f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale down
    float expectedLow[TEST_BUFFER_LENGTH] = { 0.42f, -0.42f, 0.21f, -0.08f };
    scaleBuffer(HapticLevel::LOW, 0.6f /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Haptic level scale down then adaptive scale up
    float expectedVeryLow[TEST_BUFFER_LENGTH] = { 1.02f, -1.02f, 0.51f, -0.2f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */);
    EXPECT_FLOATS_NEARLY_EQ(expectedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestLimitAppliedAfterLegacyScale,
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling),
                                          ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Scaled = { 0.2, -0.2, 0.16f, -0.13f };
    float expectedClippedVeryHigh[TEST_BUFFER_LENGTH] = { 0.15f, -0.15f, 0.15f, -0.13f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */, 0.15f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Scaled = { 1, -1, 0.5f, -0.2f };
    float expectedClippedVeryLow[TEST_BUFFER_LENGTH] = { 0.7f, -0.7f, 0.33f, -0.05f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */, 0.7f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(ExternalVibrationUtilsTest, TestLimitAppliedAfterFixedScale,
                  REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, fix_audio_coupled_haptics_scaling)),
                  REQUIRES_FLAGS_DISABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Scaled = { 0.2, -0.2, 0.16f, -0.13f };
    float expectedClippedVeryHigh[TEST_BUFFER_LENGTH] = { 0.15f, -0.15f, 0.15f, -0.07f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */, 0.15f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Scaled = { 1, -1, 0.5f, -0.2f };
    float expectedClippedVeryLow[TEST_BUFFER_LENGTH] = { 0.7f, -0.7f, 0.45f, -0.18f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */, 0.7f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}

TEST_F_WITH_FLAGS(
        ExternalVibrationUtilsTest, TestLimitAppliedAfterScaleV2,
        // Value of fix_audio_coupled_haptics_scaling is not important, should work with either
        REQUIRES_FLAGS_ENABLED(ACONFIG_FLAG(FLAG_NS, haptics_scale_v2_enabled))) {
    // Scaled = { 0.2, -0.2, 0.15f, -0.07f };
    float expectedClippedVeryHigh[TEST_BUFFER_LENGTH] = { 0.15f, -0.15f, 0.15f, -0.07f };
    scaleBuffer(HapticLevel::VERY_HIGH, 0.2f /* adaptiveScaleFactor */, 0.15f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryHigh, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);

    // Scaled = { 1.02f, -1.02f, 0.51f, -0.2f }
    float expectedClippedVeryLow[TEST_BUFFER_LENGTH] = { 0.7f, -0.7f, 0.51f, -0.2f };
    scaleBuffer(HapticLevel::VERY_LOW, 2 /* adaptiveScaleFactor */, 0.7f /* limit */);
    EXPECT_FLOATS_NEARLY_EQ(expectedClippedVeryLow, mBuffer, TEST_BUFFER_LENGTH, TEST_TOLERANCE);
}
