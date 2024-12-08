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

#include <binder/Parcel.h>
#include <gtest/gtest.h>
#include <vibrator/ExternalVibration.h>

using namespace android;
using namespace testing;

using HapticLevel = os::HapticLevel;
using ScaleLevel = os::ExternalVibrationScale::ScaleLevel;

class TestVibrationController : public os::IExternalVibrationController {
public:
    explicit TestVibrationController() {}
    IBinder *onAsBinder() override { return nullptr; }
    binder::Status mute(/*out*/ bool *ret) override {
        *ret = false;
        return binder::Status::ok();
    };
    binder::Status unmute(/*out*/ bool *ret) override {
        *ret = false;
        return binder::Status::ok();
    };
};

class ExternalVibrationTest : public Test {
protected:
    HapticLevel toHapticLevel(ScaleLevel level) {
        os::ExternalVibrationScale externalVibrationScale;
        externalVibrationScale.scaleLevel = level;
        os::HapticScale hapticScale =
                os::ExternalVibration::externalVibrationScaleToHapticScale(externalVibrationScale);
        return hapticScale.getLevel();
    }
};

TEST_F(ExternalVibrationTest, TestReadAndWriteToParcel) {
    int32_t uid = 1;
    std::string pkg("package.name");
    audio_attributes_t originalAttrs;
    originalAttrs.content_type = AUDIO_CONTENT_TYPE_SONIFICATION;
    originalAttrs.usage = AUDIO_USAGE_ASSISTANCE_SONIFICATION;
    originalAttrs.source = AUDIO_SOURCE_VOICE_COMMUNICATION;
    originalAttrs.flags = AUDIO_FLAG_BYPASS_MUTE;

    sp<TestVibrationController> vibrationController = new TestVibrationController();
    ASSERT_NE(vibrationController, nullptr);

    sp<os::ExternalVibration> original =
            new os::ExternalVibration(uid, pkg, originalAttrs, vibrationController);
    ASSERT_NE(original, nullptr);

    EXPECT_EQ(original->getUid(), uid);
    EXPECT_EQ(original->getPackage(), pkg);
    EXPECT_EQ(original->getAudioAttributes().content_type, originalAttrs.content_type);
    EXPECT_EQ(original->getAudioAttributes().usage, originalAttrs.usage);
    EXPECT_EQ(original->getAudioAttributes().source, originalAttrs.source);
    EXPECT_EQ(original->getAudioAttributes().flags, originalAttrs.flags);
    EXPECT_EQ(original->getController(), vibrationController);

    audio_attributes_t defaultAttrs;
    defaultAttrs.content_type = AUDIO_CONTENT_TYPE_UNKNOWN;
    defaultAttrs.usage = AUDIO_USAGE_UNKNOWN;
    defaultAttrs.source = AUDIO_SOURCE_DEFAULT;
    defaultAttrs.flags = AUDIO_FLAG_NONE;

    sp<os::ExternalVibration> parceled =
            new os::ExternalVibration(0, std::string(""), defaultAttrs, nullptr);
    ASSERT_NE(parceled, nullptr);

    Parcel parcel;
    original->writeToParcel(&parcel);
    parcel.setDataPosition(0);
    parceled->readFromParcel(&parcel);

    EXPECT_EQ(parceled->getUid(), uid);
    EXPECT_EQ(parceled->getPackage(), pkg);
    EXPECT_EQ(parceled->getAudioAttributes().content_type, originalAttrs.content_type);
    EXPECT_EQ(parceled->getAudioAttributes().usage, originalAttrs.usage);
    EXPECT_EQ(parceled->getAudioAttributes().source, originalAttrs.source);
    EXPECT_EQ(parceled->getAudioAttributes().flags, originalAttrs.flags);
    // TestVibrationController does not implement onAsBinder, skip controller parcel in this test.
}

TEST_F(ExternalVibrationTest, TestExternalVibrationScaleToHapticScale) {
    os::ExternalVibrationScale externalVibrationScale;
    externalVibrationScale.scaleLevel = ScaleLevel::SCALE_HIGH;
    externalVibrationScale.scaleFactor = 0.5f;
    externalVibrationScale.adaptiveHapticsScale = 0.8f;

    os::HapticScale hapticScale =
            os::ExternalVibration::externalVibrationScaleToHapticScale(externalVibrationScale);

    // Check scale factors are forwarded.
    EXPECT_EQ(hapticScale.getLevel(), HapticLevel::HIGH);
    EXPECT_EQ(hapticScale.getScaleFactor(), 0.5f);
    EXPECT_EQ(hapticScale.getAdaptiveScaleFactor(), 0.8f);

    // Check conversion for all levels.
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_MUTE), HapticLevel::MUTE);
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_VERY_LOW), HapticLevel::VERY_LOW);
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_LOW), HapticLevel::LOW);
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_NONE), HapticLevel::NONE);
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_HIGH), HapticLevel::HIGH);
    EXPECT_EQ(toHapticLevel(ScaleLevel::SCALE_VERY_HIGH), HapticLevel::VERY_HIGH);
}
