/*
 * Copyright 2020 The Android Open Source Project
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

#include <aidl/android/hardware/graphics/common/DisplayHotplugEvent.h>
#include <com_android_graphics_surfaceflinger_flags.h>
#include <common/test/FlagUtils.h>
#include "DisplayTransactionTestHelpers.h"

using namespace com::android::graphics::surfaceflinger;
using ::aidl::android::hardware::graphics::common::DisplayHotplugEvent;

namespace android {

class HotplugTest : public DisplayTransactionTest {};

TEST_F(HotplugTest, schedulesConfigureToProcessHotplugEvents) {
    EXPECT_CALL(*mFlinger.scheduler(), scheduleConfigure()).Times(2);

    constexpr HWDisplayId hwcDisplayId1 = 456;
    mFlinger.onComposerHalHotplugEvent(hwcDisplayId1, DisplayHotplugEvent::CONNECTED);

    constexpr HWDisplayId hwcDisplayId2 = 654;
    mFlinger.onComposerHalHotplugEvent(hwcDisplayId2, DisplayHotplugEvent::DISCONNECTED);

    const auto& pendingEvents = mFlinger.mutablePendingHotplugEvents();
    ASSERT_EQ(2u, pendingEvents.size());
    EXPECT_EQ(hwcDisplayId1, pendingEvents[0].hwcDisplayId);
    EXPECT_EQ(HWComposer::HotplugEvent::Connected, pendingEvents[0].event);
    EXPECT_EQ(hwcDisplayId2, pendingEvents[1].hwcDisplayId);
    EXPECT_EQ(HWComposer::HotplugEvent::Disconnected, pendingEvents[1].event);
}

TEST_F(HotplugTest, schedulesFrameToCommitDisplayTransaction) {
    EXPECT_CALL(*mFlinger.scheduler(), scheduleConfigure()).Times(1);
    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    constexpr HWDisplayId displayId1 = 456;
    mFlinger.onComposerHalHotplugEvent(displayId1, DisplayHotplugEvent::DISCONNECTED);
    mFlinger.configure();

    // The configure stage should consume the hotplug queue and produce a display transaction.
    EXPECT_TRUE(mFlinger.mutablePendingHotplugEvents().empty());
    EXPECT_TRUE(hasTransactionFlagSet(eDisplayTransactionNeeded));
}

TEST_F(HotplugTest, createsDisplaySnapshotsForDisplaysWithIdentificationData) {
    // Configure a primary display with identification data.
    using PrimaryDisplay = InnerDisplayVariant;
    PrimaryDisplay::setupHwcHotplugCallExpectations(this);
    PrimaryDisplay::setupHwcGetActiveConfigCallExpectations(this);
    PrimaryDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(PrimaryDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    // A single commit should be scheduled.
    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    mFlinger.configure();

    // Configure an external display with identification info.
    using ExternalDisplay = ExternalDisplayWithIdentificationVariant<>;
    ExternalDisplay::setupHwcHotplugCallExpectations(this);
    ExternalDisplay::setupHwcGetActiveConfigCallExpectations(this);
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(ExternalDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    mFlinger.configure();

    EXPECT_TRUE(hasPhysicalHwcDisplay(PrimaryDisplay::HWC_DISPLAY_ID));
    const auto primaryDisplayId = asPhysicalDisplayId(PrimaryDisplay::DISPLAY_ID::get());
    ASSERT_TRUE(primaryDisplayId);
    EXPECT_TRUE(mFlinger.getHwComposer().isConnected(*primaryDisplayId));
    const auto primaryDisplayIdOpt =
            mFlinger.getHwComposer().toPhysicalDisplayId(PrimaryDisplay::HWC_DISPLAY_ID);
    ASSERT_TRUE(primaryDisplayIdOpt.has_value());
    const auto primaryPhysicalDisplayOpt =
            mFlinger.physicalDisplays().get(primaryDisplayIdOpt.value());
    ASSERT_TRUE(primaryPhysicalDisplayOpt.has_value());
    const auto primaryDisplaySnapshotRef = primaryPhysicalDisplayOpt->get().snapshotRef();
    EXPECT_EQ(*primaryDisplayId, primaryDisplaySnapshotRef.get().displayId());
    EXPECT_EQ(PrimaryDisplay::PORT::value, primaryDisplaySnapshotRef.get().port());
    EXPECT_EQ(PrimaryDisplay::CONNECTION_TYPE::value,
              primaryDisplaySnapshotRef.get().connectionType());

    EXPECT_TRUE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));
    const auto externalDisplayId = asPhysicalDisplayId(ExternalDisplay::DISPLAY_ID::get());
    ASSERT_TRUE(externalDisplayId);
    EXPECT_TRUE(mFlinger.getHwComposer().isConnected(*externalDisplayId));
    const auto externalDisplayIdOpt =
            mFlinger.getHwComposer().toPhysicalDisplayId(ExternalDisplay::HWC_DISPLAY_ID);
    ASSERT_TRUE(externalDisplayIdOpt.has_value());
    const auto externalPhysicalDisplayOpt =
            mFlinger.physicalDisplays().get(externalDisplayIdOpt.value());
    ASSERT_TRUE(externalPhysicalDisplayOpt.has_value());
    const auto externalDisplaySnapshotRef = externalPhysicalDisplayOpt->get().snapshotRef();
    EXPECT_EQ(*externalDisplayId, externalDisplaySnapshotRef.get().displayId());
    EXPECT_EQ(ExternalDisplay::PORT::value, externalDisplaySnapshotRef.get().port());
    EXPECT_EQ(ExternalDisplay::CONNECTION_TYPE::value,
              externalDisplaySnapshotRef.get().connectionType());
}

TEST_F(HotplugTest, createsDisplaySnapshotsForDisplaysWithoutIdentificationData) {
    // Configure a primary display without identification data.
    using PrimaryDisplay = PrimaryDisplayVariant;
    PrimaryDisplay::setupHwcHotplugCallExpectations(this);
    PrimaryDisplay::setupHwcGetActiveConfigCallExpectations(this);
    PrimaryDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(PrimaryDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    // A single commit should be scheduled.
    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    mFlinger.configure();

    // Configure an external display with identification info.
    using ExternalDisplay = ExternalDisplayWithIdentificationVariant<>;
    ExternalDisplay::setupHwcHotplugCallExpectations(this);
    ExternalDisplay::setupHwcGetActiveConfigCallExpectations(this);
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(ExternalDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    mFlinger.configure();

    // Both ID and port are expected to be equal to 0 for primary internal display due to no
    // identification data.
    constexpr uint8_t primaryInternalDisplayPort = 0u;
    constexpr PhysicalDisplayId primaryInternalDisplayId =
            PhysicalDisplayId::fromPort(primaryInternalDisplayPort);
    EXPECT_TRUE(hasPhysicalHwcDisplay(PrimaryDisplay::HWC_DISPLAY_ID));
    ASSERT_EQ(primaryInternalDisplayId, asPhysicalDisplayId(PrimaryDisplay::DISPLAY_ID::get()));
    EXPECT_TRUE(mFlinger.getHwComposer().isConnected(primaryInternalDisplayId));
    const auto primaryDisplayIdOpt =
            mFlinger.getHwComposer().toPhysicalDisplayId(PrimaryDisplay::HWC_DISPLAY_ID);
    ASSERT_TRUE(primaryDisplayIdOpt.has_value());
    const auto primaryPhysicalDisplayOpt =
            mFlinger.physicalDisplays().get(primaryDisplayIdOpt.value());
    ASSERT_TRUE(primaryPhysicalDisplayOpt.has_value());
    const auto primaryDisplaySnapshotRef = primaryPhysicalDisplayOpt->get().snapshotRef();
    EXPECT_EQ(primaryInternalDisplayId, primaryDisplaySnapshotRef.get().displayId());
    EXPECT_EQ(primaryInternalDisplayPort, primaryDisplaySnapshotRef.get().port());
    EXPECT_EQ(PrimaryDisplay::CONNECTION_TYPE::value,
              primaryDisplaySnapshotRef.get().connectionType());

    // Even though the external display has identification data available, the lack of data for the
    // internal display has set of the legacy multi-display mode in SF and therefore the external
    // display's identification data will be ignored.
    // Both ID and port are expected to be equal to 1 for external internal display.
    constexpr uint8_t externalDisplayPort = 1u;
    constexpr PhysicalDisplayId externalDisplayId =
            PhysicalDisplayId::fromPort(externalDisplayPort);
    EXPECT_TRUE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));
    EXPECT_TRUE(mFlinger.getHwComposer().isConnected(externalDisplayId));
    const auto externalDisplayIdOpt =
            mFlinger.getHwComposer().toPhysicalDisplayId(ExternalDisplay::HWC_DISPLAY_ID);
    ASSERT_TRUE(externalDisplayIdOpt.has_value());
    const auto externalPhysicalDisplayOpt =
            mFlinger.physicalDisplays().get(externalDisplayIdOpt.value());
    ASSERT_TRUE(externalPhysicalDisplayOpt.has_value());
    const auto externalDisplaySnapshotRef = externalPhysicalDisplayOpt->get().snapshotRef();
    EXPECT_EQ(externalDisplayId, externalDisplaySnapshotRef.get().displayId());
    EXPECT_EQ(externalDisplayPort, externalDisplaySnapshotRef.get().port());
    EXPECT_EQ(ExternalDisplay::CONNECTION_TYPE::value,
              externalDisplaySnapshotRef.get().connectionType());
}

TEST_F(HotplugTest, ignoresDuplicateDisconnection) {
    // Inject a primary display.
    PrimaryDisplayVariant::injectHwcDisplay(this);

    using ExternalDisplay = ExternalDisplayVariant;
    ExternalDisplay::setupHwcHotplugCallExpectations(this);
    ExternalDisplay::setupHwcGetActiveConfigCallExpectations(this);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(ExternalDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    // A single commit should be scheduled for both configure calls.
    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);
    mFlinger.configure();

    EXPECT_TRUE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));

    // Disconnecting a display that was already disconnected should be a no-op.
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Disconnected);
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Disconnected);
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Disconnected);
    mFlinger.configure();

    // The display should be scheduled for removal during the next commit. At this point, it should
    // still exist but be marked as disconnected.
    EXPECT_TRUE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));
    const auto externalDisplayId = asPhysicalDisplayId(ExternalDisplay::DISPLAY_ID::get());
    ASSERT_TRUE(externalDisplayId);
    EXPECT_FALSE(mFlinger.getHwComposer().isConnected(*externalDisplayId));
}

TEST_F(HotplugTest, rejectsHotplugIfFailedToLoadDisplayModes) {
    // Inject a primary display.
    PrimaryDisplayVariant::injectHwcDisplay(this);

    using ExternalDisplay = ExternalDisplayVariant;
    constexpr bool kFailedHotplug = true;
    ExternalDisplay::setupHwcHotplugCallExpectations<kFailedHotplug>(this);

    EXPECT_CALL(*mEventThread,
                onHotplugConnectionError(static_cast<int32_t>(DisplayHotplugEvent::ERROR_UNKNOWN)))
            .Times(1);

    // Simulate a connect event that fails to load display modes due to HWC already having
    // disconnected the display but SF yet having to process the queued disconnect event.
    EXPECT_CALL(*mComposer, getActiveConfig(ExternalDisplay::HWC_DISPLAY_ID, _))
            .WillRepeatedly(Return(Error::BAD_DISPLAY));

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(ExternalDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);
    mFlinger.configure();

    // The hotplug should be rejected, so no HWComposer::DisplayData should be created.
    EXPECT_FALSE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));

    // Disconnecting a display that does not exist should be a no-op.
    ExternalDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Disconnected);
    mFlinger.configure();

    EXPECT_FALSE(hasPhysicalHwcDisplay(ExternalDisplay::HWC_DISPLAY_ID));
}

TEST_F(HotplugTest, rejectsHotplugOnActivePortsDuplicate) {
    // Inject a primary display.
    PrimaryDisplayVariant::injectHwcDisplay(this);

    // Second display should come up properly.
    using SecondDisplay = ExternalDisplayWithIdentificationVariant<>;
    SecondDisplay::setupHwcHotplugCallExpectations(this);
    SecondDisplay::setupHwcGetActiveConfigCallExpectations(this);

    // TODO: b/241286146 - Remove this unnecessary call.
    EXPECT_CALL(*mComposer,
                setVsyncEnabled(SecondDisplay::HWC_DISPLAY_ID, IComposerClient::Vsync::DISABLE))
            .WillOnce(Return(Error::NONE));

    EXPECT_CALL(*mFlinger.scheduler(), scheduleFrame(_)).Times(1);

    SecondDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);
    mFlinger.configure();

    EXPECT_TRUE(hasPhysicalHwcDisplay(SecondDisplay::HWC_DISPLAY_ID));

    // Third display will return the same port ID as the second, and the hotplug
    // should fail.
    constexpr HWDisplayId kHwDisplayId = 1234;
    using DuplicatePortDisplay = ExternalDisplayWithIdentificationVariant<kHwDisplayId>;

    // We expect display identification to be fetched correctly, since EDID and
    // port are available and successfully retrieved from HAL.
    EXPECT_CALL(*mComposer,
                getDisplayIdentificationData(DuplicatePortDisplay::HWC_DISPLAY_ID, _, _))
            .WillOnce(DoAll(SetArgPointee<1>(*DuplicatePortDisplay::PORT::value),
                            SetArgPointee<2>(getExternalEedid()), Return(Error::NONE)));

    DuplicatePortDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Connected);
    mFlinger.configure();

    // The hotplug should be rejected due to an attempt to connect a display to an already active
    // port. No HWComposer::DisplayData should be created.
    EXPECT_FALSE(hasPhysicalHwcDisplay(DuplicatePortDisplay::HWC_DISPLAY_ID));

    // Disconnecting a display that was not successfully configured should be a no-op.
    DuplicatePortDisplay::injectPendingHotplugEvent(this, HWComposer::HotplugEvent::Disconnected);
    mFlinger.configure();

    EXPECT_FALSE(hasPhysicalHwcDisplay(DuplicatePortDisplay::HWC_DISPLAY_ID));
}

} // namespace android
