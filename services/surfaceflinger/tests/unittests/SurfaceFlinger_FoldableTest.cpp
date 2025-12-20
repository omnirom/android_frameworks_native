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

#undef LOG_TAG
#define LOG_TAG "LibSurfaceFlingerUnittests"

#include <com_android_graphics_surfaceflinger_flags.h>
#include <common/test/FlagUtils.h>
#include "DualDisplayTransactionTest.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace com::android::graphics::surfaceflinger;

namespace android {
namespace {

constexpr bool kExpectSetPowerModeOnce = false;
struct FoldableTest : DualDisplayTransactionTest<hal::PowerMode::OFF, hal::PowerMode::OFF,
                                                 kExpectSetPowerModeOnce>,
                      public testing::WithParamInterface<bool> {};

TEST_P(FoldableTest, promotesPacesetterOnBoot) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    // When the device boots, the inner display should be the pacesetter.
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());

    // ...and should still be after powering on.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());
}

TEST_P(FoldableTest, promotesPacesetterOnFoldUnfold) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);

    // The outer display should become the pacesetter after folding.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::OFF);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());

    // The inner display should become the pacesetter after unfolding.
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::OFF);
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());
}

TEST_F(FoldableTest, promotesPacesetterOnConcurrentPowerOn) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, false);

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);

    // The inner display should stay the pacesetter if both are powered on.
    // TODO(b/255635821): The pacesetter should depend on the displays' refresh rates.
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());

    // The outer display should become the pacesetter if designated.
    mFlinger.scheduler()->designatePacesetterDisplay(getOuterDisplayId());
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());

    // The inner display should become the pacesetter if designated.
    mFlinger.scheduler()->designatePacesetterDisplay(getInnerDisplayId());
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());
}

TEST_P(FoldableTest, promotesPacesetterOnConcurrentPowerOff) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);

    // The outer display should become the pacesetter if the inner display powers off.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::OFF);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());

    // The outer display should stay the pacesetter if both are powered on.
    // TODO(b/255635821): The pacesetter should depend on the displays' refresh rates.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());

    // The inner display should become the pacesetter if the outer display powers off.
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::OFF);
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());
}

TEST_P(FoldableTest, doesNotRequestHardwareVsyncIfPoweredOff) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    // Both displays are powered off.
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getInnerDisplayId(), _))
            .Times(0);
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getOuterDisplayId(), _))
            .Times(0);

    EXPECT_FALSE(mInnerDisplay->isPoweredOn());
    EXPECT_FALSE(mOuterDisplay->isPoweredOn());

    auto& scheduler = *mFlinger.scheduler();
    scheduler.onHardwareVsyncRequest(getInnerDisplayId(), true);
    scheduler.onHardwareVsyncRequest(getOuterDisplayId(), true);
}

TEST_P(FoldableTest, requestsHardwareVsyncForInnerDisplay) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    // Only inner display is powered on.
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getInnerDisplayId(), true))
            .Times(1);
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getOuterDisplayId(), _))
            .Times(0);

    // The injected VsyncSchedule uses TestableScheduler::mockRequestHardwareVsync, so no calls to
    // ISchedulerCallback::requestHardwareVsync are expected during setPhysicalDisplayPowerMode.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);

    EXPECT_TRUE(mInnerDisplay->isPoweredOn());
    EXPECT_FALSE(mOuterDisplay->isPoweredOn());

    auto& scheduler = *mFlinger.scheduler();
    scheduler.onHardwareVsyncRequest(getInnerDisplayId(), true);
    scheduler.onHardwareVsyncRequest(getOuterDisplayId(), true);
}

TEST_P(FoldableTest, requestsHardwareVsyncForOuterDisplay) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    // Only outer display is powered on.
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getInnerDisplayId(), _))
            .Times(0);
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getOuterDisplayId(), true))
            .Times(1);

    // The injected VsyncSchedule uses TestableScheduler::mockRequestHardwareVsync, so no calls to
    // ISchedulerCallback::requestHardwareVsync are expected during setPhysicalDisplayPowerMode.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::OFF);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);

    EXPECT_FALSE(mInnerDisplay->isPoweredOn());
    EXPECT_TRUE(mOuterDisplay->isPoweredOn());

    auto& scheduler = *mFlinger.scheduler();
    scheduler.onHardwareVsyncRequest(getInnerDisplayId(), true);
    scheduler.onHardwareVsyncRequest(getOuterDisplayId(), true);
}

TEST_P(FoldableTest, requestsHardwareVsyncForBothDisplays) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    // Both displays are powered on.
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getInnerDisplayId(), true))
            .Times(1);
    EXPECT_CALL(mFlinger.mockSchedulerCallback(), requestHardwareVsync(getOuterDisplayId(), true))
            .Times(1);

    // The injected VsyncSchedule uses TestableScheduler::mockRequestHardwareVsync, so no calls to
    // ISchedulerCallback::requestHardwareVsync are expected during setPhysicalDisplayPowerMode.
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);

    EXPECT_TRUE(mInnerDisplay->isPoweredOn());
    EXPECT_TRUE(mOuterDisplay->isPoweredOn());

    auto& scheduler = *mFlinger.scheduler();
    scheduler.onHardwareVsyncRequest(mInnerDisplay->getPhysicalId(), true);
    scheduler.onHardwareVsyncRequest(mOuterDisplay->getPhysicalId(), true);
}

TEST_P(FoldableTest, requestVsyncOnPowerOn) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());
    EXPECT_CALL(mFlinger.scheduler()->mockRequestHardwareVsync, Call(getInnerDisplayId(), true))
            .Times(1);
    EXPECT_CALL(mFlinger.scheduler()->mockRequestHardwareVsync, Call(getOuterDisplayId(), true))
            .Times(1);

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);
}

TEST_P(FoldableTest, disableVsyncOnPowerOffPacesetter) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());
    SET_FLAG_FOR_TEST(flags::multithreaded_present, true);
    // When the device boots, the inner display should be the pacesetter.
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());

    testing::InSequence seq;
    EXPECT_CALL(mFlinger.scheduler()->mockRequestHardwareVsync, Call(getInnerDisplayId(), true))
            .Times(1);
    EXPECT_CALL(mFlinger.scheduler()->mockRequestHardwareVsync, Call(getOuterDisplayId(), true))
            .Times(1);

    // Turning off the pacesetter will result in disabling VSYNC.
    EXPECT_CALL(mFlinger.scheduler()->mockRequestHardwareVsync, Call(getInnerDisplayId(), false))
            .Times(1);

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);

    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::OFF);

    // Other display is now the pacesetter.
    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());
}

TEST_P(FoldableTest, layerCachingTexturePoolOnFrontInternal) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, GetParam());

    ON_CALL(mFlinger.mockSchedulerCallback(), enableLayerCachingTexturePool)
            .WillByDefault([&](PhysicalDisplayId displayId, bool enable) {
                mFlinger.enableLayerCachingTexturePool(displayId, enable);
            });

    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getInnerDisplayId());

    // In order for TexturePool to be enabled, layer caching needs to be enabled.
    mInnerDisplay->getCompositionDisplay()->setLayerCachingEnabled(true);
    mOuterDisplay->getCompositionDisplay()->setLayerCachingEnabled(true);

    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::OFF);
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);

    // Switching to outer display as the front-internal display should disable the inner display's
    // pool and enable the outer display's pool.
    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::ON);
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::OFF);

    ASSERT_EQ(mFlinger.scheduler()->pacesetterDisplayId(), getOuterDisplayId());

    EXPECT_FALSE(mInnerDisplay->getCompositionDisplay()->plannerTexturePoolEnabled());
    EXPECT_TRUE(mOuterDisplay->getCompositionDisplay()->plannerTexturePoolEnabled());

    mFlinger.setPhysicalDisplayPowerMode(mOuterDisplay, PowerMode::OFF);
    mFlinger.setPhysicalDisplayPowerMode(mInnerDisplay, PowerMode::ON);

    EXPECT_TRUE(mInnerDisplay->getCompositionDisplay()->plannerTexturePoolEnabled());
    EXPECT_FALSE(mOuterDisplay->getCompositionDisplay()->plannerTexturePoolEnabled());
}

INSTANTIATE_TEST_SUITE_P(Foldable, FoldableTest, testing::Bool(),
                         [](const testing::TestParamInfo<FoldableTest::ParamType>& info) {
                             return info.param ? "PacesetterSelectionEnabled"
                                               : "PacesetterSelectionDisabled";
                         });

} // namespace
} // namespace android
