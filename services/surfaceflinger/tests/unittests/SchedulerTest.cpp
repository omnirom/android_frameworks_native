/*
 * Copyright 2019 The Android Open Source Project
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

#include <common/test/FlagUtils.h>
#include <ftl/fake_guard.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <log/log.h>

#include <mutex>

#include "Scheduler/EventThread.h"
#include "Scheduler/RefreshRateSelector.h"
#include "Scheduler/VSyncPredictor.h"
#include "Scheduler/VSyncReactor.h"
#include "TestableScheduler.h"
#include "TestableSurfaceFlinger.h"
#include "fake/FakeClock.h"
#include "mock/DisplayHardware/MockDisplayMode.h"
#include "mock/MockEventThread.h"
#include "mock/MockLayer.h"
#include "mock/MockSchedulerCallback.h"
#include "mock/MockVSyncTracker.h"

#include <FrontEnd/LayerHierarchy.h>
#include <scheduler/FrameTime.h>

#include <com_android_graphics_surfaceflinger_flags.h>
#include "FpsOps.h"

using namespace com::android::graphics::surfaceflinger;

namespace android::scheduler {

using android::mock::createDisplayMode;
using android::mock::createVrrDisplayMode;
using display::DisplayModeRequest;
using testing::_;
using testing::Field;
using testing::Return;
using testing::UnorderedElementsAre;

namespace {

using MockEventThread = android::mock::EventThread;
using MockLayer = android::mock::MockLayer;

using LayerHierarchy = surfaceflinger::frontend::LayerHierarchy;
using LayerHierarchyBuilder = surfaceflinger::frontend::LayerHierarchyBuilder;
using RequestedLayerState = surfaceflinger::frontend::RequestedLayerState;

class ZeroClock : public Clock {
public:
    nsecs_t now() const override { return 0; }
};

class SchedulerTest : public testing::Test {
public:
    static constexpr PhysicalDisplayId kDisplayId1 = PhysicalDisplayId::fromPort(255u);
    static constexpr PhysicalDisplayId kDisplayId2 = PhysicalDisplayId::fromPort(254u);
    static constexpr PhysicalDisplayId kDisplayId3 = PhysicalDisplayId::fromPort(253u);

protected:
    class MockEventThreadConnection : public android::EventThreadConnection {
    public:
        explicit MockEventThreadConnection(EventThread* eventThread)
              : EventThreadConnection(eventThread, /*callingUid*/ static_cast<uid_t>(0)) {}
        ~MockEventThreadConnection() = default;

        MOCK_METHOD1(stealReceiveChannel, binder::Status(gui::BitTube* outChannel));
        MOCK_METHOD1(setVsyncRate, binder::Status(int count));
        MOCK_METHOD0(requestNextVsync, binder::Status());
    };

    SchedulerTest();

    static constexpr RefreshRateSelector::LayerRequirement kLayer = {.weight = 1.f};

    static inline const ftl::NonNull<DisplayModePtr> kDisplay1Mode60 =
            ftl::as_non_null(createDisplayMode(kDisplayId1, DisplayModeId(0), 60_Hz));
    static inline const ftl::NonNull<DisplayModePtr> kDisplay1Mode120 =
            ftl::as_non_null(createDisplayMode(kDisplayId1, DisplayModeId(1), 120_Hz));
    static inline const DisplayModes kDisplay1Modes = makeModes(kDisplay1Mode60, kDisplay1Mode120);

    static inline FrameRateMode kDisplay1Mode60_60{60_Hz, kDisplay1Mode60};
    static inline FrameRateMode kDisplay1Mode120_120{120_Hz, kDisplay1Mode120};

    static inline const ftl::NonNull<DisplayModePtr> kDisplay2Mode60 =
            ftl::as_non_null(createDisplayMode(kDisplayId2, DisplayModeId(0), 60_Hz));
    static inline const ftl::NonNull<DisplayModePtr> kDisplay2Mode120 =
            ftl::as_non_null(createDisplayMode(kDisplayId2, DisplayModeId(1), 120_Hz));
    static inline const ftl::NonNull<DisplayModePtr> kDisplay2Mode60point01 =
            ftl::as_non_null(createDisplayMode(kDisplayId2, DisplayModeId(2), 60.01_Hz));
    static inline const DisplayModes kDisplay2Modes =
            makeModes(kDisplay2Mode60, kDisplay2Mode120, kDisplay2Mode60point01);

    static inline FrameRateMode kDisplay2Mode60_60{60_Hz, kDisplay2Mode60};
    static inline FrameRateMode kDisplay2Mode120_120{120_Hz, kDisplay2Mode120};

    static inline const ftl::NonNull<DisplayModePtr> kDisplay3Mode60 =
            ftl::as_non_null(createDisplayMode(kDisplayId3, DisplayModeId(0), 60_Hz));
    static inline const DisplayModes kDisplay3Modes = makeModes(kDisplay3Mode60);

    std::shared_ptr<RefreshRateSelector> mSelector =
            std::make_shared<RefreshRateSelector>(makeModes(kDisplay1Mode60),
                                                  kDisplay1Mode60->getId());

    mock::SchedulerCallback mSchedulerCallback;
    TestableSurfaceFlinger mFlinger;
    TestableScheduler* mScheduler = new TestableScheduler{mSelector, mFlinger, mSchedulerCallback};
    surfaceflinger::frontend::LayerHierarchyBuilder mLayerHierarchyBuilder;

    MockEventThread* mEventThread;
    sp<MockEventThreadConnection> mEventThreadConnection;
};

SchedulerTest::SchedulerTest() {
    auto eventThread = std::make_unique<MockEventThread>();
    mEventThread = eventThread.get();
    EXPECT_CALL(*mEventThread, registerDisplayEventConnection(_)).WillOnce(Return(0));

    mEventThreadConnection = sp<MockEventThreadConnection>::make(mEventThread);

    // createConnection call to scheduler makes a createEventConnection call to EventThread. Make
    // sure that call gets executed and returns an EventThread::Connection object.
    EXPECT_CALL(*mEventThread, createEventConnection(_))
            .WillRepeatedly(Return(mEventThreadConnection));

    mScheduler->setEventThread(Cycle::Render, std::move(eventThread));
    mScheduler->setEventThread(Cycle::LastComposite, std::make_unique<MockEventThread>());

    mFlinger.resetScheduler(mScheduler);
}

using VsyncIds = std::vector<std::pair<PhysicalDisplayId, VsyncId>>;

struct FakeMultiDisplayCompositor final : ICompositor {
    explicit FakeMultiDisplayCompositor(TestableScheduler& scheduler) : scheduler(scheduler) {}

    TestableScheduler& scheduler;

    struct {
        PhysicalDisplayId commit;
        PhysicalDisplayId composite;
    } pacesetterIds;

    struct {
        VsyncIds commit;
        VsyncIds composite;
    } vsyncIds;

    bool committed = true;
    bool changePacesetter = false;

    struct PresentationFence {
        sp<Fence> fence;
        FenceTimePtr fenceTime;
        VsyncId vsyncId;
    };
    std::unordered_map<PhysicalDisplayId, PresentationFence> lastPresentationFences;
    FenceToFenceTimeMap fenceMap;

    void configure() override {}

    bool commit(PhysicalDisplayId pacesetterId, const scheduler::FrameTargets& targets) override {
        pacesetterIds.commit = pacesetterId;

        vsyncIds.commit.clear();
        vsyncIds.composite.clear();

        for (const auto& [id, target] : targets) {
            vsyncIds.commit.emplace_back(id, target->vsyncId());
        }

        if (changePacesetter) {
            scheduler.designatePacesetterDisplay(SchedulerTest::kDisplayId2);
        }

        return committed;
    }

    CompositeResultsPerDisplay composite(PhysicalDisplayId pacesetterId,
                                         const scheduler::FrameTargeters& targeters) override {
        pacesetterIds.composite = pacesetterId;

        CompositeResultsPerDisplay results;

        for (const auto& [id, targeter] : targeters) {
            vsyncIds.composite.emplace_back(id, targeter->target().vsyncId());
            results.try_emplace(id,
                                CompositeResult{.compositionCoverage = CompositionCoverage::Hwc});

            auto [fence, fenceTime] = fenceMap.makePendingFenceForTest();
            lastPresentationFences[id] = {fence, fenceTime, targeter->target().vsyncId()};
            scheduler.setPresentFenceForFrameTargeter(targeter, fence, fenceTime);
        }

        return results;
    }

    void sample() override {}
    void sendNotifyExpectedPresentHint(PhysicalDisplayId) override {}
};

} // namespace

TEST_F(SchedulerTest, registerDisplay) FTL_FAKE_GUARD(kMainThreadContext) {
    // Hardware VSYNC should not change if the display is already registered.
    EXPECT_CALL(mSchedulerCallback, requestHardwareVsync(kDisplayId1, false)).Times(0);
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()));

    // TODO(b/241285191): Restore once VsyncSchedule::getPendingHardwareVsyncState is called by
    // Scheduler::setDisplayPowerMode rather than SF::setPhysicalDisplayPowerMode.
#if 0
    // Hardware VSYNC should be disabled for newly registered displays.
    EXPECT_CALL(mSchedulerCallback, requestHardwareVsync(kDisplayId2, false)).Times(1);
    EXPECT_CALL(mSchedulerCallback, requestHardwareVsync(kDisplayId3, false)).Times(1);
#endif

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()));
    mScheduler->registerDisplay(kDisplayId3, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay3Modes,
                                                                      kDisplay3Mode60->getId()));

    EXPECT_FALSE(mScheduler->getVsyncSchedule(kDisplayId1)->getPendingHardwareVsyncState());
    EXPECT_FALSE(mScheduler->getVsyncSchedule(kDisplayId2)->getPendingHardwareVsyncState());
    EXPECT_FALSE(mScheduler->getVsyncSchedule(kDisplayId3)->getPendingHardwareVsyncState());
}

TEST_F(SchedulerTest, chooseRefreshRateForContentIsNoopWhenModeSwitchingIsNotSupported) {
    // The layer is registered at creation time and deregistered at destruction time.
    sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());

    // recordLayerHistory should be a noop
    ASSERT_EQ(0u, mScheduler->getNumActiveLayers());
    scheduler::LayerProps layerProps = {
            .visible = true,
            .bounds = {0, 0, 100, 100},
            .transform = {},
            .setFrameRateVote = {},
            .frameRateSelectionPriority = Layer::PRIORITY_UNSET,
            .isSmallDirty = false,
            .isFrontBuffered = false,
    };

    mScheduler->recordLayerHistory(layer->getSequence(), layerProps, 0, 0,
                                   LayerHistory::LayerUpdateType::Buffer);
    ASSERT_EQ(0u, mScheduler->getNumActiveLayers());

    constexpr hal::PowerMode kPowerModeOn = hal::PowerMode::ON;
    FTL_FAKE_GUARD(kMainThreadContext, mScheduler->setDisplayPowerMode(kDisplayId1, kPowerModeOn));

    const ui::Size kDisplaySize = ui::Size(9, 111'111);
    mScheduler->onPacesetterDisplaySizeChanged(kDisplaySize);

    EXPECT_CALL(mSchedulerCallback, requestDisplayModes(_)).Times(0);
    mScheduler->chooseRefreshRateForContent(/*LayerHierarchy*/ nullptr,
                                            /*updateAttachedChoreographer*/ false);
}

TEST_F(SchedulerTest, updateDisplayModes) {
    ASSERT_EQ(0u, mScheduler->layerHistorySize());
    sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    ASSERT_EQ(1u, mScheduler->layerHistorySize());

    // Replace `mSelector` with a new `RefreshRateSelector` that has different display modes.
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()));

    ASSERT_EQ(0u, mScheduler->getNumActiveLayers());
    scheduler::LayerProps layerProps = {
            .visible = true,
            .bounds = {0, 0, 100, 100},
            .transform = {},
            .setFrameRateVote = {},
            .frameRateSelectionPriority = Layer::PRIORITY_UNSET,
            .isSmallDirty = false,
            .isFrontBuffered = false,
    };
    mScheduler->recordLayerHistory(layer->getSequence(), layerProps, 0, 0,
                                   LayerHistory::LayerUpdateType::Buffer);
    ASSERT_EQ(1u, mScheduler->getNumActiveLayers());
}

TEST_F(SchedulerTest, emitModeChangeEvent) {
    const auto selectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode120->getId());
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal, selectorPtr);
    mScheduler->onDisplayModeChanged(kDisplayId1, kDisplay1Mode120_120, true);

    mScheduler->setContentRequirements({kLayer});

    // No event is emitted in response to idle.
    EXPECT_CALL(*mEventThread, onModeAndFrameRateOverridesChanged(_, _, _, _, _)).Times(0);

    using TimerState = TestableScheduler::TimerState;

    mScheduler->idleTimerCallback(kDisplayId1, TimerState::Expired);
    selectorPtr->setActiveMode(kDisplay1Mode60->getId(), 60_Hz);

    auto layer = kLayer;
    layer.vote = RefreshRateSelector::LayerVoteType::ExplicitExact;
    layer.desiredRefreshRate = 60_Hz;
    mScheduler->setContentRequirements({layer});

    // An event is emitted implicitly despite choosing the same mode as when idle.
    EXPECT_CALL(*mEventThread, onModeAndFrameRateOverridesChanged(_, kDisplay1Mode60_60, _, _, _))
            .Times(1);

    mScheduler->idleTimerCallback(kDisplayId1, TimerState::Reset);

    mScheduler->setContentRequirements({kLayer});

    // An event is emitted explicitly for the mode change.
    EXPECT_CALL(*mEventThread, onModeAndFrameRateOverridesChanged(_, kDisplay1Mode120_120, _, _, _))
            .Times(1);

    mScheduler->touchTimerCallback(TimerState::Reset);
}

TEST_F(SchedulerTest, emitModeAndFrameRateOverrideChangeEvent) {
    const auto selectorPtr = std::make_shared<
            RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode60->getId(),
                                 RefreshRateSelector::Config{.enableFrameRateOverride = true,
                                                             .kernelIdleTimerController = {}});
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal, selectorPtr);
    std::vector<RefreshRateSelector::LayerRequirement> layers = {kLayer, kLayer};
    auto& lr1 = layers[0];
    auto& lr2 = layers[1];
    lr1.vote = RefreshRateSelector::LayerVoteType::ExplicitExact;
    lr1.desiredRefreshRate = 120_Hz;
    lr1.name = "120Hz ExplicitExactOrMultiple";
    lr2.vote = RefreshRateSelector::LayerVoteType::ExplicitExactOrMultiple;
    lr2.desiredRefreshRate = 60_Hz;
    lr2.name = "60Hz ExplicitExactOrMultiple";

    // Emit Mode and Frame Rate override changed call
    EXPECT_CALL(*mEventThread, onModeAndFrameRateOverridesChanged(_, kDisplay1Mode120_120, _, _, _))
            .Times(1);
    mScheduler->setContentRequirements(layers);
    mScheduler->touchTimerCallback(TestableScheduler::TimerState::Reset);
}

TEST_F(SchedulerTest, emitModeChangeEventOnReloadPhaseConfiguration) {
    const auto selectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode120->getId());
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal, selectorPtr);
    constexpr auto kMinSfDuration = Duration::fromNs(1000000);
    constexpr auto kMaxSfDuration = Duration::fromNs(2000000);
    constexpr auto kAppDuration = Duration::fromNs(1500000);

    EXPECT_CALL(*mEventThread, onModeAndFrameRateOverridesChanged(_, kDisplay1Mode120_120, _, _, _))
            .Times(1);
    mScheduler->reloadPhaseConfiguration(kDisplay1Mode120_120, kMinSfDuration, kMaxSfDuration,
                                         kAppDuration);
}

TEST_F(SchedulerTest, calculateMaxAcquiredBufferCount) {
    struct TestCase {
        Fps refreshRate;
        std::chrono::nanoseconds presentLatency;
        int expectedBufferCount;
    };

    const auto verifyTestCases = [&](std::vector<TestCase> tests) {
        for (const auto testCase : tests) {
            EXPECT_EQ(testCase.expectedBufferCount,
                      mFlinger.calculateMaxAcquiredBufferCount(testCase.refreshRate,
                                                               testCase.presentLatency));
        }
    };

    std::vector<TestCase> testCases{{60_Hz, 30ms, 1},
                                    {90_Hz, 30ms, 2},
                                    {120_Hz, 30ms, 3},
                                    {60_Hz, 40ms, 2},
                                    {60_Hz, 10ms, 1}};
    verifyTestCases(testCases);

    const auto savedMinAcquiredBuffers = mFlinger.mutableMinAcquiredBuffers();
    mFlinger.mutableMinAcquiredBuffers() = 2;
    verifyTestCases({{60_Hz, 10ms, 2}});
    mFlinger.mutableMinAcquiredBuffers() = savedMinAcquiredBuffers;

    const auto savedMaxAcquiredBuffers = mFlinger.mutableMaxAcquiredBuffers();
    mFlinger.mutableMaxAcquiredBuffers() = 2;
    testCases = {{60_Hz, 30ms, 1},
                 {90_Hz, 30ms, 2},
                 {120_Hz, 30ms, 2}, // max buffers allowed is 2
                 {60_Hz, 40ms, 2},
                 {60_Hz, 10ms, 1}};
    verifyTestCases(testCases);
    mFlinger.mutableMaxAcquiredBuffers() = 3; // max buffers allowed is 3
    verifyTestCases({{120_Hz, 30ms, 3}});
    mFlinger.mutableMaxAcquiredBuffers() = savedMaxAcquiredBuffers;
}

MATCHER(Is120Hz, "") {
    return isApproxEqual(arg.front().mode.fps, 120_Hz);
}

TEST_F(SchedulerTest, chooseRefreshRateForContentSelectsMaxRefreshRate) {
    auto selector = std::make_shared<RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode60->getId());
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal, selector);

    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    scheduler::LayerProps layerProps = {
            .visible = true,
            .bounds = {0, 0, 0, 0},
            .transform = {},
            .setFrameRateVote = {},
            .frameRateSelectionPriority = Layer::PRIORITY_UNSET,
            .isSmallDirty = false,
            .isFrontBuffered = false,
            .refreshRateSelector = selector.get(),
    };
    mScheduler->recordLayerHistory(layer->getSequence(), layerProps, 0, systemTime(),
                                   LayerHistory::LayerUpdateType::Buffer);

    constexpr hal::PowerMode kPowerModeOn = hal::PowerMode::ON;
    FTL_FAKE_GUARD(kMainThreadContext, mScheduler->setDisplayPowerMode(kDisplayId1, kPowerModeOn));

    const ui::Size kDisplaySize = ui::Size(9, 111'111);
    mScheduler->onPacesetterDisplaySizeChanged(kDisplaySize);

    EXPECT_CALL(mSchedulerCallback, requestDisplayModes(Is120Hz())).Times(1);
    mScheduler->chooseRefreshRateForContent(/*LayerHierarchy*/ nullptr,
                                            /*updateAttachedChoreographer*/ false);

    // No-op if layer requirements have not changed.
    EXPECT_CALL(mSchedulerCallback, requestDisplayModes(_)).Times(0);
    mScheduler->chooseRefreshRateForContent(/*LayerHierarchy*/ nullptr,
                                            /*updateAttachedChoreographer*/ false);
}

TEST_F(SchedulerTest, chooseRefreshRateForContentFollowerModeChangeRequest) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);
    SET_FLAG_FOR_TEST(flags::follower_arbitrary_refresh_rate_selection, true);

    // Configure pacesetter display to 120Hz.
    const LayerFilter pacesetterLayerStack = {.layerStack = {.id = 0}};
    const auto pacesetterSelectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode120->getId());
    pacesetterSelectorPtr->setLayerFilter(pacesetterLayerStack);
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                pacesetterSelectorPtr);
    FTL_FAKE_GUARD(kMainThreadContext,
                   mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON));
    mScheduler->onDisplayModeChanged(kDisplayId1, kDisplay1Mode120_120, true);

    // Configure follower display to 60Hz, even though it is also capable of 120Hz.
    const LayerFilter followerLayerStack = {.layerStack = {.id = 1}};
    const auto followerSelectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay2Modes, kDisplay2Mode60->getId());
    followerSelectorPtr->setLayerFilter(followerLayerStack);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                followerSelectorPtr);
    FTL_FAKE_GUARD(kMainThreadContext,
                   mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON));
    mScheduler->onDisplayModeChanged(kDisplayId2, kDisplay2Mode60_60, true);

    ASSERT_EQ(mScheduler->getPacesetterDisplayId(), kDisplayId1);
    ASSERT_EQ(mScheduler->getPreferredDisplayMode(), kDisplay1Mode120_120);

    // Add a layer requesting 120Hz on the follower display.
    auto layer = kLayer;
    layer.vote = RefreshRateSelector::LayerVoteType::ExplicitExact;
    layer.desiredRefreshRate = 120_Hz;
    layer.layerFilter = followerLayerStack;
    mScheduler->setContentRequirements({layer});

    EXPECT_CALL(mSchedulerCallback,
                requestDisplayModes(
                        UnorderedElementsAre(Field(&DisplayModeRequest::mode, kDisplay1Mode120_120),
                                             // Trigger mode change request even if only
                                             // the follower display is changing.
                                             Field(&DisplayModeRequest::mode,
                                                   kDisplay2Mode120_120))))
            .Times(1);

    mScheduler->chooseRefreshRateForContent(/*LayerHierarchy*/ nullptr,
                                            /*updateAttachedChoreographer*/ false);
}

TEST_F(SchedulerTest, chooseRefreshRateForContentFollowerModeChangeRequestPacesetterCantSwitch) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);
    SET_FLAG_FOR_TEST(flags::follower_arbitrary_refresh_rate_selection, true);

    // Configure pacesetter display to 120Hz.
    const DisplayModes kDisplay1ModesOneMode = makeModes(kDisplay1Mode120);
    const auto pacesetterSelectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay1ModesOneMode, kDisplay1Mode120->getId());

    ASSERT_FALSE(pacesetterSelectorPtr->canSwitch());

    const LayerFilter pacesetterLayerStack = {.layerStack = {.id = 0}};
    pacesetterSelectorPtr->setLayerFilter(pacesetterLayerStack);
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                pacesetterSelectorPtr);
    FTL_FAKE_GUARD(kMainThreadContext,
                   mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON));
    mScheduler->onDisplayModeChanged(kDisplayId1, kDisplay1Mode120_120, true);

    // Configure follower display to 60Hz, even though it is also capable of 120Hz.
    const LayerFilter followerLayerStack = {.layerStack = {.id = 1}};
    const auto followerSelectorPtr =
            std::make_shared<RefreshRateSelector>(kDisplay2Modes, kDisplay2Mode60->getId());
    followerSelectorPtr->setLayerFilter(followerLayerStack);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                followerSelectorPtr);
    FTL_FAKE_GUARD(kMainThreadContext,
                   mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON));
    mScheduler->onDisplayModeChanged(kDisplayId2, kDisplay2Mode60_60, true);

    ASSERT_EQ(mScheduler->getPacesetterDisplayId(), kDisplayId1);
    ASSERT_EQ(mScheduler->getPreferredDisplayMode(), kDisplay1Mode120_120);

    // Add a layer requesting 120Hz on the follower display.
    auto layer = kLayer;
    layer.vote = RefreshRateSelector::LayerVoteType::ExplicitExact;
    layer.desiredRefreshRate = 120_Hz;
    layer.layerFilter = followerLayerStack;
    mScheduler->setContentRequirements({layer});

    EXPECT_CALL(mSchedulerCallback,
                requestDisplayModes(
                        testing::UnorderedElementsAre(testing::Field(&display::DisplayModeRequest::
                                                                             mode,
                                                                     kDisplay1Mode120_120),
                                                      // Trigger mode change request even if only
                                                      // the follower display is changing mode and
                                                      // pacesetter display is not capable of
                                                      // switching.
                                                      testing::Field(&display::DisplayModeRequest::
                                                                             mode,
                                                                     kDisplay2Mode120_120))))
            .Times(1);

    mScheduler->chooseRefreshRateForContent(/*LayerHierarchy*/ nullptr,
                                            /*updateAttachedChoreographer*/ false);
}

TEST_F(SchedulerTest, chooseDisplayModes) {
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()));

    mScheduler->setContentRequirements({kLayer, kLayer});
    GlobalSignals globalSignals = {.idle = true};
    mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);

    using DisplayModeChoice = TestableScheduler::DisplayModeChoice;

    auto modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());

    auto choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({60_Hz, kDisplay1Mode60}, globalSignals));

    globalSignals = {.idle = false};
    mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);

    modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());

    choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({120_Hz, kDisplay1Mode120}, globalSignals));

    globalSignals = {.touch = true};
    mScheduler->replaceTouchTimer(10);
    mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);

    modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());

    choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({120_Hz, kDisplay1Mode120}, globalSignals));
}

TEST_F(SchedulerTest, chooseDisplayModesHighHintTouchSignal) {
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()));

    using DisplayModeChoice = TestableScheduler::DisplayModeChoice;

    std::vector<RefreshRateSelector::LayerRequirement> layers = {kLayer, kLayer};
    auto& lr1 = layers[0];
    auto& lr2 = layers[1];

    // Scenario that is similar to game. Expects no touch boost.
    lr1.vote = RefreshRateSelector::LayerVoteType::ExplicitCategory;
    lr1.frameRateCategory = FrameRateCategory::HighHint;
    lr1.name = "ExplicitCategory HighHint";
    lr2.vote = RefreshRateSelector::LayerVoteType::ExplicitDefault;
    lr2.desiredRefreshRate = 30_Hz;
    lr2.name = "30Hz ExplicitDefault";
    mScheduler->setContentRequirements(layers);
    auto modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());
    auto choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({60_Hz, kDisplay1Mode60}, {.touch = false}));

    // Scenario that is similar to video playback and interaction. Expects touch boost.
    lr1.vote = RefreshRateSelector::LayerVoteType::ExplicitCategory;
    lr1.frameRateCategory = FrameRateCategory::HighHint;
    lr1.name = "ExplicitCategory HighHint";
    lr2.vote = RefreshRateSelector::LayerVoteType::ExplicitExactOrMultiple;
    lr2.desiredRefreshRate = 30_Hz;
    lr2.name = "30Hz ExplicitExactOrMultiple";
    mScheduler->setContentRequirements(layers);
    modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());
    choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({120_Hz, kDisplay1Mode120}, {.touch = true}));

    // Scenario with explicit category and HighHint. Expects touch boost.
    lr1.vote = RefreshRateSelector::LayerVoteType::ExplicitCategory;
    lr1.frameRateCategory = FrameRateCategory::HighHint;
    lr1.name = "ExplicitCategory HighHint";
    lr2.vote = RefreshRateSelector::LayerVoteType::ExplicitCategory;
    lr2.frameRateCategory = FrameRateCategory::Low;
    lr2.name = "ExplicitCategory Low";
    mScheduler->setContentRequirements(layers);
    modeChoices = mScheduler->chooseDisplayModes();
    ASSERT_EQ(1u, modeChoices.size());
    choice = modeChoices.get(kDisplayId1);
    ASSERT_TRUE(choice);
    EXPECT_EQ(choice->get(), DisplayModeChoice({120_Hz, kDisplay1Mode120}, {.touch = true}));
}

TEST_F(SchedulerTest, chooseDisplayModesMultipleDisplays) {
    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    using DisplayModeChoice = TestableScheduler::DisplayModeChoice;
    TestableScheduler::DisplayModeChoiceMap expectedChoices;

    const bool follower_arbitrary_refresh_rate =
            FlagManager::getInstance().follower_arbitrary_refresh_rate_selection();
    {
        const GlobalSignals globalSignals = {.idle = true};
        const GlobalSignals display2GlobalSignals = GlobalSignals{};
        const FrameRateMode display2Mode = follower_arbitrary_refresh_rate
                ? FrameRateMode{120_Hz, kDisplay2Mode120}
                : FrameRateMode{60_Hz, kDisplay2Mode60};
        expectedChoices =
                ftl::init::map<const PhysicalDisplayId&,
                               DisplayModeChoice>(kDisplayId1,
                                                  FrameRateMode{60_Hz, kDisplay1Mode60},
                                                  globalSignals)(kDisplayId2, display2Mode,
                                                                 display2GlobalSignals);

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, display2GlobalSignals);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        const GlobalSignals globalSignals = {.idle = false};
        const GlobalSignals display2GlobalSignals = GlobalSignals{};
        expectedChoices =
                ftl::init::map<const PhysicalDisplayId&,
                               DisplayModeChoice>(kDisplayId1,
                                                  FrameRateMode{120_Hz, kDisplay1Mode120},
                                                  globalSignals)(kDisplayId2,
                                                                 FrameRateMode{120_Hz,
                                                                               kDisplay2Mode120},
                                                                 display2GlobalSignals);

        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, display2GlobalSignals);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        const GlobalSignals globalSignals = {.touch = true};
        const GlobalSignals display2GlobalSignals =
                follower_arbitrary_refresh_rate ? globalSignals : GlobalSignals{};

        mScheduler->replaceTouchTimer(10);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);

        expectedChoices =
                ftl::init::map<const PhysicalDisplayId&,
                               DisplayModeChoice>(kDisplayId1,
                                                  FrameRateMode{120_Hz, kDisplay1Mode120},
                                                  globalSignals)(kDisplayId2,
                                                                 FrameRateMode{120_Hz,
                                                                               kDisplay2Mode120},
                                                                 display2GlobalSignals);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        // The kDisplayId3 does not support 120Hz, The pacesetter display rate is chosen to be 120
        // Hz. In this case only the display kDisplayId3 choose 60Hz as it does not support 120Hz.
        mScheduler->registerDisplay(kDisplayId3, ui::DisplayConnectionType::Internal,
                                    std::make_shared<RefreshRateSelector>(kDisplay3Modes,
                                                                          kDisplay3Mode60->getId()),
                                    kActiveDisplayId);
        mScheduler->setDisplayPowerMode(kDisplayId3, hal::PowerMode::ON);

        const GlobalSignals globalSignals = {.touch = true};
        mScheduler->replaceTouchTimer(10);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId3, globalSignals);

        expectedChoices = ftl::init::map<const PhysicalDisplayId&, DisplayModeChoice>(
                kDisplayId1, FrameRateMode{120_Hz, kDisplay1Mode120},
                globalSignals)(kDisplayId2, FrameRateMode{120_Hz, kDisplay2Mode120},
                               follower_arbitrary_refresh_rate
                                       ? globalSignals
                                       : GlobalSignals{})(kDisplayId3,
                                                          FrameRateMode{60_Hz, kDisplay3Mode60},
                                                          follower_arbitrary_refresh_rate
                                                                  ? globalSignals
                                                                  : GlobalSignals{});

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        // We should choose 60Hz despite the touch signal as pacesetter only supports 60Hz
        mScheduler->designatePacesetterDisplay(kDisplayId3);
        const GlobalSignals globalSignals = {.touch = true};
        const GlobalSignals followerSignals =
                follower_arbitrary_refresh_rate ? globalSignals : GlobalSignals{};
        mScheduler->replaceTouchTimer(10);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId3, globalSignals);

        const FrameRateMode display1Mode = follower_arbitrary_refresh_rate
                ? FrameRateMode{120_Hz, kDisplay1Mode120}
                : FrameRateMode{60_Hz, kDisplay1Mode60};
        const FrameRateMode display2Mode = follower_arbitrary_refresh_rate
                ? FrameRateMode{120_Hz, kDisplay2Mode120}
                : FrameRateMode{60_Hz, kDisplay2Mode60};
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, display1Mode,
                                   followerSignals)(kDisplayId2, display2Mode,
                                                    followerSignals)(kDisplayId3,
                                                                     FrameRateMode{60_Hz,
                                                                                   kDisplay3Mode60},
                                                                     globalSignals);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
}

TEST_F(SchedulerTest, chooseDisplayModesMultipleDisplaysArbitraryFollowersIdle) {
    SET_FLAG_FOR_TEST(flags::follower_arbitrary_refresh_rate_selection, true);
    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    using DisplayModeChoice = TestableScheduler::DisplayModeChoice;
    TestableScheduler::DisplayModeChoiceMap expectedChoices;

    {
        const GlobalSignals globalSignals = {.idle = true};
        expectedChoices =
                ftl::init::map<const PhysicalDisplayId&,
                               DisplayModeChoice>(kDisplayId1,
                                                  FrameRateMode{60_Hz, kDisplay1Mode60},
                                                  globalSignals)(kDisplayId2,
                                                                 FrameRateMode{60_Hz,
                                                                               kDisplay2Mode60},
                                                                 globalSignals);

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, globalSignals);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, globalSignals);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, FrameRateMode{60_Hz, kDisplay1Mode60},
                                   GlobalSignals{.idle = true})(kDisplayId2,
                                                                FrameRateMode{120_Hz,
                                                                              kDisplay2Mode120},
                                                                GlobalSignals{.idle = false});

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, {.idle = true});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, {.idle = false});

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, FrameRateMode{120_Hz, kDisplay1Mode120},
                                   GlobalSignals{.idle = false})(kDisplayId2,
                                                                 FrameRateMode{60_Hz,
                                                                               kDisplay2Mode60},
                                                                 GlobalSignals{.idle = true});

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, {.idle = false});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, {.idle = true});

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, FrameRateMode{120_Hz, kDisplay1Mode120},
                                   GlobalSignals{.idle = false})(kDisplayId2,
                                                                 FrameRateMode{120_Hz,
                                                                               kDisplay2Mode120},
                                                                 GlobalSignals{.idle = false});

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, {.idle = false});
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, {.idle = false});

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
}

TEST_F(SchedulerTest, chooseDisplayModesMultipleDisplaysArbitraryFollowersPowerMode) {
    SET_FLAG_FOR_TEST(flags::follower_arbitrary_refresh_rate_selection, true);
    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    using DisplayModeChoice = TestableScheduler::DisplayModeChoice;
    TestableScheduler::DisplayModeChoiceMap expectedChoices;

    {
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, FrameRateMode{120_Hz, kDisplay1Mode120},
                                   GlobalSignals{.powerOnImminent =
                                                         true})(kDisplayId2,
                                                                FrameRateMode{60_Hz,
                                                                              kDisplay2Mode60},
                                                                GlobalSignals{.idle = true});

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setPowerTimerPolicy(kDisplayId1, TestableScheduler::TimerState::Reset);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, {.idle = true});

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
    {
        expectedChoices = ftl::init::map<
                const PhysicalDisplayId&,
                DisplayModeChoice>(kDisplayId1, FrameRateMode{60_Hz, kDisplay1Mode60},
                                   GlobalSignals{
                                           .idle = true})(kDisplayId2,
                                                          FrameRateMode{120_Hz, kDisplay2Mode120},
                                                          GlobalSignals{.powerOnImminent = true});

        mScheduler->setContentRequirements({kLayer, kLayer});
        mScheduler->setPowerTimerPolicy(kDisplayId1, TestableScheduler::TimerState::Expired);
        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId1, {.idle = true});

        mScheduler->setTouchStateAndIdleTimerPolicy(kDisplayId2, {.idle = false});
        mScheduler->setPowerTimerPolicy(kDisplayId2, TestableScheduler::TimerState::Reset);

        const auto actualChoices = mScheduler->chooseDisplayModes();
        EXPECT_EQ(expectedChoices, actualChoices);
    }
}

TEST_F(SchedulerTest, forcePacesetterDisplay) FTL_FAKE_GUARD(kMainThreadContext) {
    constexpr PhysicalDisplayId kFrontInternalDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode120->getId()),
                                kFrontInternalDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kFrontInternalDisplayId);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    // The front internal display (display 1) should be the pacesetter.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);

    // Force set display 2 as pacesetter.
    EXPECT_TRUE(mScheduler->forcePacesetterDisplay(kDisplayId2));
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId2);

    // Try to set display 1 as pacesetter without forcing, check that it failed.
    EXPECT_FALSE(mScheduler->designatePacesetterDisplay(kDisplayId1));
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId2);
}

TEST_F(SchedulerTest, resetForcedPacesetterDisplay) FTL_FAKE_GUARD(kMainThreadContext) {
    constexpr PhysicalDisplayId kFrontInternalDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode120->getId()),
                                kFrontInternalDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kFrontInternalDisplayId);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    // The front internal display (display 1) should be the pacesetter.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);

    // Since no display was forced a pacesetter, resetForcedPacesetterDisplay() should be a no-op.
    EXPECT_FALSE(mScheduler->resetForcedPacesetterDisplay(kDisplayId2));
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);

    // Force set display 2 as pacesetter.
    EXPECT_TRUE(mScheduler->forcePacesetterDisplay(kDisplayId2));
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId2);

    // The forced pacesetter display should be reset to display 1.
    EXPECT_TRUE(mScheduler->resetForcedPacesetterDisplay(kDisplayId1));
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
}

TEST_F(SchedulerTest, onFrameSignalMultipleDisplays) {
    SET_FLAG_FOR_TEST(flags::follower_display_backpressure, false);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId);

    FakeMultiDisplayCompositor compositor(*mScheduler);

    mScheduler->doFrameSignal(compositor, VsyncId(42));

    const auto makeVsyncIds = [](VsyncId vsyncId, bool swap = false) -> VsyncIds {
        if (swap) {
            return {{kDisplayId2, vsyncId}, {kDisplayId1, vsyncId}};
        } else {
            return {{kDisplayId1, vsyncId}, {kDisplayId2, vsyncId}};
        }
    };

    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.commit);
    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.composite);
    EXPECT_EQ(makeVsyncIds(VsyncId(42)), compositor.vsyncIds.commit);
    EXPECT_EQ(makeVsyncIds(VsyncId(42)), compositor.vsyncIds.composite);

    // FrameTargets should be updated despite the skipped commit.
    compositor.committed = false;
    mScheduler->doFrameSignal(compositor, VsyncId(43));

    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.commit);
    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.composite);
    EXPECT_EQ(makeVsyncIds(VsyncId(43)), compositor.vsyncIds.commit);
    EXPECT_TRUE(compositor.vsyncIds.composite.empty());

    // The pacesetter may change during commit.
    compositor.committed = true;
    compositor.changePacesetter = true;
    mScheduler->doFrameSignal(compositor, VsyncId(44));

    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.commit);
    EXPECT_EQ(kDisplayId2, compositor.pacesetterIds.composite);
    EXPECT_EQ(makeVsyncIds(VsyncId(44)), compositor.vsyncIds.commit);
    EXPECT_EQ(makeVsyncIds(VsyncId(44), true), compositor.vsyncIds.composite);
}

TEST_F(SchedulerTest, onFrameSignalMultipleDisplaysSkipFollowerCompositionOnBackpressure) {
    SET_FLAG_FOR_TEST(flags::follower_display_backpressure, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    constexpr Fps k120Hz = 120_Hz;
    auto mockVsyncTracker1 = std::make_shared<android::mock::VSyncTracker>();
    ON_CALL(*mockVsyncTracker1, currentPeriod).WillByDefault(Return(k120Hz.getPeriodNsecs()));
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode120->getId()),
                                kActiveDisplayId, mockVsyncTracker1);
    auto mockVsyncTracker2 = std::make_shared<android::mock::VSyncTracker>();
    constexpr Fps k60Hz = 60_Hz;
    ON_CALL(*mockVsyncTracker2, currentPeriod).WillByDefault(Return(k60Hz.getPeriodNsecs()));
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId, mockVsyncTracker2);

    FakeMultiDisplayCompositor compositor(*mScheduler);

    fake::FakeClock* fakeClock = mScheduler->injectFakeClock();
    TimePoint now(fakeClock->now());
    TimePoint expectedPacesetterPresentTime(now + k120Hz.getPeriod());
    TimePoint expectedFollowerPresentTime(now + k60Hz.getPeriod());

    // Advance time by some offset to simulate wakeup.
    fakeClock->advanceTime(1ms);

    auto advanceMockVsyncTrackers = [&]() {
        EXPECT_CALL(*mockVsyncTracker1,
                    nextAnticipatedVSyncTimeFrom(expectedPacesetterPresentTime.ns(), _))
                .WillRepeatedly(Return(expectedPacesetterPresentTime.ns()));
        EXPECT_CALL(*mockVsyncTracker2,
                    nextAnticipatedVSyncTimeFrom(expectedPacesetterPresentTime.ns(), _))
                .WillRepeatedly(Return(expectedFollowerPresentTime.ns()));
        EXPECT_CALL(*mockVsyncTracker2,
                    nextAnticipatedVSyncTimeFrom(TimePoint(fakeClock->now()).ns(), _))
                .WillRepeatedly(Return(expectedFollowerPresentTime.ns()));
    };
    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(42), expectedPacesetterPresentTime);

    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.commit);
    EXPECT_EQ(kDisplayId1, compositor.pacesetterIds.composite);
    VsyncIds expectedTargets = {{kDisplayId1, VsyncId(42)}, {kDisplayId2, VsyncId(42)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(42));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // pending presentation due to slower nature of the second display.
    }

    // Advance to the next pacestter vsync wakeup time.
    fakeClock->advanceTime(k120Hz.getPeriod());

    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());
    // `expectedFollowerPresentTime` remains the same

    advanceMockVsyncTrackers();
    // Only the pacesetter should commit/composite as there's follower backpressure.
    compositor.committed = true;
    mScheduler->doFrameSignal(compositor, VsyncId(43), expectedPacesetterPresentTime);

    expectedTargets = {{kDisplayId1, VsyncId(43)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [_, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(43));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [_, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        fenceTimePtr->signalForTest(expectedFollowerPresentTime.ns());
    }

    // Advance to the next pacestter vsync wakeup time.
    fakeClock->advanceTime(k120Hz.getPeriod());

    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());
    expectedFollowerPresentTime = TimePoint(expectedFollowerPresentTime + k60Hz.getPeriod());

    advanceMockVsyncTrackers();
    // Both pacesetter and follower should commit
    compositor.committed = true;
    mScheduler->doFrameSignal(compositor, VsyncId(44), expectedPacesetterPresentTime);

    expectedTargets = {{kDisplayId1, VsyncId(44)}, {kDisplayId2, VsyncId(44)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);
}

TEST_F(SchedulerTest, onFrameSignalMultipleDisplaysSkipFollowerCompositionOnMissedPresentation) {
    SET_FLAG_FOR_TEST(flags::follower_display_backpressure, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    constexpr Fps k120Hz = 120_Hz;
    auto mockVsyncTracker1 = std::make_shared<android::mock::VSyncTracker>();
    ON_CALL(*mockVsyncTracker1, currentPeriod).WillByDefault(Return(k120Hz.getPeriodNsecs()));
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode120->getId()),
                                kActiveDisplayId, mockVsyncTracker1);
    auto mockVsyncTracker2 = std::make_shared<android::mock::VSyncTracker>();
    constexpr Fps k60Hz = 60_Hz;
    ON_CALL(*mockVsyncTracker2, currentPeriod).WillByDefault(Return(k60Hz.getPeriodNsecs()));
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()),
                                kActiveDisplayId, mockVsyncTracker2);

    FakeMultiDisplayCompositor compositor(*mScheduler);

    fake::FakeClock* fakeClock = mScheduler->injectFakeClock();
    TimePoint now(fakeClock->now());
    TimePoint expectedPacesetterPresentTime(now + k120Hz.getPeriod());
    TimePoint expectedFollowerPresentTime(now + k60Hz.getPeriod());

    // Advance time by some offset to simulate wakeup.
    fakeClock->advanceTime(1ms);

    auto advanceMockVsyncTrackers = [&]() {
        EXPECT_CALL(*mockVsyncTracker1,
                    nextAnticipatedVSyncTimeFrom(expectedPacesetterPresentTime.ns(), _))
                .WillRepeatedly(Return(expectedPacesetterPresentTime.ns()));
        EXPECT_CALL(*mockVsyncTracker2,
                    nextAnticipatedVSyncTimeFrom(expectedPacesetterPresentTime.ns(), _))
                .WillRepeatedly(Return(expectedFollowerPresentTime.ns()));
        EXPECT_CALL(*mockVsyncTracker2,
                    nextAnticipatedVSyncTimeFrom(TimePoint(fakeClock->now()).ns(), _))
                .WillRepeatedly(Return(expectedFollowerPresentTime.ns()));
    };
    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(42), expectedPacesetterPresentTime);

    VsyncIds expectedTargets = {{kDisplayId1, VsyncId(42)}, {kDisplayId2, VsyncId(42)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(42));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // pending presentation due to slower nature of the second display.
    }

    // Advance to the next pacestter vsync wakeup time.
    fakeClock->advanceTime(k120Hz.getPeriod());
    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());

    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(43), expectedPacesetterPresentTime);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(43));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // pending presentation due to slower nature of the second display.
    }
    expectedTargets = {{kDisplayId1, VsyncId(43)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(43));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    FenceTimePtr firstFollowerPresentFenceTime;
    sp<Fence> firstFollowerPresentFence;
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // Don't fire the follower's presentation fence to simulate a miss.
        firstFollowerPresentFence = fence;
        firstFollowerPresentFenceTime = fenceTimePtr;
    }

    fakeClock->advanceTime(k120Hz.getPeriod());
    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());
    expectedFollowerPresentTime = TimePoint(expectedFollowerPresentTime + k60Hz.getPeriod());

    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(44), expectedPacesetterPresentTime);

    // Follower backpressure mitigation kicks in.
    expectedTargets = {{kDisplayId1, VsyncId(44)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(44));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // pending presentation due to slower nature of the second display.
    }

    // Advance to the next pacestter vsync wakeup time.
    fakeClock->advanceTime(k120Hz.getPeriod());
    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());

    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(45), expectedPacesetterPresentTime);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(45));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(42));
        // Fire the first present fence here instead to simulate backpressure. with 300us of slop.
        firstFollowerPresentFenceTime->signalForTest(expectedFollowerPresentTime.ns() + 300000);
    }

    fakeClock->advanceTime(k120Hz.getPeriod());
    expectedPacesetterPresentTime = TimePoint(expectedPacesetterPresentTime + k120Hz.getPeriod());
    expectedFollowerPresentTime = TimePoint(expectedFollowerPresentTime + k60Hz.getPeriod());

    advanceMockVsyncTrackers();

    mScheduler->doFrameSignal(compositor, VsyncId(46), expectedPacesetterPresentTime);

    // Follower backpressure mitigation should not kick in for display 2.
    expectedTargets = {{kDisplayId1, VsyncId(46)}, {kDisplayId2, VsyncId(46)}};
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.commit);
    EXPECT_EQ(expectedTargets, compositor.vsyncIds.composite);

    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId1];
        EXPECT_EQ(vsyncId, VsyncId(46));
        fenceTimePtr->signalForTest(expectedPacesetterPresentTime.ns());
    }
    {
        auto& [fence, fenceTimePtr, vsyncId] = compositor.lastPresentationFences[kDisplayId2];
        EXPECT_EQ(vsyncId, VsyncId(46));
        // pending presentation due to slower nature of the second display.
    }
}

TEST_F(SchedulerTest, nextFrameIntervalTest) {
    static constexpr size_t kHistorySize = 10;
    static constexpr size_t kMinimumSamplesForPrediction = 6;
    static constexpr size_t kOutlierTolerancePercent = 25;
    const auto refreshRate = Fps::fromPeriodNsecs(500);
    auto frameRate = Fps::fromPeriodNsecs(1000);

    const ftl::NonNull<DisplayModePtr> kMode = ftl::as_non_null(
            createVrrDisplayMode(DisplayModeId(0), refreshRate,
                                 hal::VrrConfig{.minFrameIntervalNs = static_cast<int32_t>(
                                                        frameRate.getPeriodNsecs())}));
    std::shared_ptr<VSyncPredictor> vrrTracker =
            std::make_shared<VSyncPredictor>(std::make_unique<ZeroClock>(), kMode, kHistorySize,
                                             kMinimumSamplesForPrediction,
                                             kOutlierTolerancePercent);
    std::shared_ptr<RefreshRateSelector> vrrSelectorPtr =
            std::make_shared<RefreshRateSelector>(makeModes(kMode), kMode->getId(),
                                                  RefreshRateSelector::Config{
                                                          .enableFrameRateOverride = true});
    TestableScheduler scheduler{std::make_unique<android::mock::VsyncController>(),
                                vrrTracker,
                                vrrSelectorPtr,
                                mFlinger.getFactory(),
                                mFlinger.getTimeStats(),
                                mSchedulerCallback};

    scheduler.registerDisplay(kMode->getPhysicalDisplayId(), ui::DisplayConnectionType::Internal,
                              vrrSelectorPtr, std::nullopt, vrrTracker);
    vrrSelectorPtr->setActiveMode(kMode->getId(), frameRate);
    scheduler.setRenderRate(kMode->getPhysicalDisplayId(), frameRate, /*applyImmediately*/ false);
    vrrTracker->addVsyncTimestamp(0);
    // Set 1000 as vsync seq #0
    vrrTracker->nextAnticipatedVSyncTimeFrom(700);

    EXPECT_EQ(Fps::fromPeriodNsecs(1000),
              scheduler.getNextFrameInterval(kMode->getPhysicalDisplayId(),
                                             TimePoint::fromNs(1000)));
    EXPECT_EQ(Fps::fromPeriodNsecs(1000),
              scheduler.getNextFrameInterval(kMode->getPhysicalDisplayId(),
                                             TimePoint::fromNs(2000)));

    // Not crossing the min frame period
    vrrTracker->onFrameBegin(TimePoint::fromNs(2000),
                             {TimePoint::fromNs(1500), TimePoint::fromNs(1500)});
    EXPECT_EQ(Fps::fromPeriodNsecs(1000),
              scheduler.getNextFrameInterval(kMode->getPhysicalDisplayId(),
                                             TimePoint::fromNs(2500)));
    // Change render rate
    frameRate = Fps::fromPeriodNsecs(2000);
    vrrSelectorPtr->setActiveMode(kMode->getId(), frameRate);
    scheduler.setRenderRate(kMode->getPhysicalDisplayId(), frameRate, /*applyImmediately*/ false);

    EXPECT_EQ(Fps::fromPeriodNsecs(2000),
              scheduler.getNextFrameInterval(kMode->getPhysicalDisplayId(),
                                             TimePoint::fromNs(5500)));
    EXPECT_EQ(Fps::fromPeriodNsecs(2000),
              scheduler.getNextFrameInterval(kMode->getPhysicalDisplayId(),
                                             TimePoint::fromNs(7500)));
}

TEST_F(SchedulerTest, resyncAllToHardwareVsync) FTL_FAKE_GUARD(kMainThreadContext) {
    // resyncAllToHardwareVsync will result in requesting hardware VSYNC on both displays, since
    // they are both on.
    EXPECT_CALL(mScheduler->mockRequestHardwareVsync, Call(kDisplayId1, true)).Times(1);
    EXPECT_CALL(mScheduler->mockRequestHardwareVsync, Call(kDisplayId2, true)).Times(1);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()));
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    static constexpr bool kDisallow = true;
    mScheduler->disableHardwareVsync(kDisplayId1, kDisallow);
    mScheduler->disableHardwareVsync(kDisplayId2, kDisallow);

    static constexpr bool kAllowToEnable = true;
    mScheduler->resyncAllToHardwareVsync(kAllowToEnable);
}

TEST_F(SchedulerTest, resyncAllDoNotAllow) FTL_FAKE_GUARD(kMainThreadContext) {
    // Without setting allowToEnable to true, resyncAllToHardwareVsync does not
    // result in requesting hardware VSYNC.
    EXPECT_CALL(mScheduler->mockRequestHardwareVsync, Call(kDisplayId1, _)).Times(0);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    static constexpr bool kDisallow = true;
    mScheduler->disableHardwareVsync(kDisplayId1, kDisallow);

    static constexpr bool kAllowToEnable = false;
    mScheduler->resyncAllToHardwareVsync(kAllowToEnable);
}

TEST_F(SchedulerTest, resyncAllSkipsOffDisplays) FTL_FAKE_GUARD(kMainThreadContext) {
    // resyncAllToHardwareVsync will result in requesting hardware VSYNC on display 1, which is on,
    // but not on display 2, which is off.
    EXPECT_CALL(mScheduler->mockRequestHardwareVsync, Call(kDisplayId1, true)).Times(1);
    EXPECT_CALL(mScheduler->mockRequestHardwareVsync, Call(kDisplayId2, _)).Times(0);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()));
    ASSERT_EQ(hal::PowerMode::OFF, mScheduler->getDisplayPowerMode(kDisplayId2));

    static constexpr bool kDisallow = true;
    mScheduler->disableHardwareVsync(kDisplayId1, kDisallow);
    mScheduler->disableHardwareVsync(kDisplayId2, kDisallow);

    static constexpr bool kAllowToEnable = true;
    mScheduler->resyncAllToHardwareVsync(kAllowToEnable);
}

TEST_F(SchedulerTest, enablesLayerCachingTexturePoolForPacesetter) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode120->getId()));
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId2);

    EXPECT_CALL(mSchedulerCallback, enableLayerCachingTexturePool(kDisplayId1, true));
    EXPECT_CALL(mSchedulerCallback, enableLayerCachingTexturePool(kDisplayId2, false));
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::OFF);
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);

    // Flush EXPECT_CALLs here so that additional mock calls in the dtor does not trigger a failure.
    testing::Mock::VerifyAndClearExpectations(&mSchedulerCallback);
}

TEST_F(SchedulerTest, pendingModeChangeSingleDisplay) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId1, true);
    EXPECT_TRUE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId1, false);
    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());
}

TEST_F(SchedulerTest, pendingModeChangeMultiDisplay) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode60->getId()));
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId1, true);
    EXPECT_TRUE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId2, true);
    EXPECT_TRUE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId1, false);
    EXPECT_TRUE(mScheduler->layerHistoryModeChangePending());

    mScheduler->setModeChangePending(kDisplayId2, false);
    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());
}

TEST_F(SchedulerTest, pendingModeChangeInvalidDisplay) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());

    PhysicalDisplayId invalidDisplayId = PhysicalDisplayId::fromPort(123);
    mScheduler->setModeChangePending(invalidDisplayId, true);
    EXPECT_FALSE(mScheduler->layerHistoryModeChangePending());
}

class AttachedChoreographerTest : public SchedulerTest {
protected:
    void frameRateTestScenario(Fps layerFps, int8_t frameRateCompatibility, Fps displayFps,
                               Fps expectedChoreographerFps);
};

TEST_F(AttachedChoreographerTest, registerSingle) {
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());

    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    const sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    EXPECT_EQ(1u, mScheduler->mutableAttachedChoreographers().size());
    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));
    EXPECT_EQ(1u,
              mScheduler->mutableAttachedChoreographers()[layer->getSequence()].connections.size());
    EXPECT_FALSE(
            mScheduler->mutableAttachedChoreographers()[layer->getSequence()].frameRate.isValid());
}

TEST_F(AttachedChoreographerTest, registerMultipleOnSameLayer) {
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());

    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    const auto handle = layer->getHandle();

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached).Times(2);

    EXPECT_CALL(*mEventThread, registerDisplayEventConnection(_))
            .WillOnce(Return(0))
            .WillOnce(Return(0));

    const auto mockConnection1 = sp<MockEventThreadConnection>::make(mEventThread);
    const auto mockConnection2 = sp<MockEventThreadConnection>::make(mEventThread);
    EXPECT_CALL(*mEventThread, createEventConnection(_))
            .WillOnce(Return(mockConnection1))
            .WillOnce(Return(mockConnection2));

    const sp<IDisplayEventConnection> connection1 =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, handle);
    const sp<IDisplayEventConnection> connection2 =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, handle);

    EXPECT_EQ(1u, mScheduler->mutableAttachedChoreographers().size());
    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));
    EXPECT_EQ(2u,
              mScheduler->mutableAttachedChoreographers()[layer->getSequence()].connections.size());
    EXPECT_FALSE(
            mScheduler->mutableAttachedChoreographers()[layer->getSequence()].frameRate.isValid());
}

TEST_F(AttachedChoreographerTest, registerMultipleOnDifferentLayers) {
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());

    const sp<MockLayer> layer1 = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> layer2 = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached).Times(2);
    const sp<IDisplayEventConnection> connection1 =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer1->getHandle());
    const sp<IDisplayEventConnection> connection2 =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer2->getHandle());

    EXPECT_EQ(2u, mScheduler->mutableAttachedChoreographers().size());

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer1->getSequence()));
    EXPECT_EQ(1u,
              mScheduler->mutableAttachedChoreographers()[layer1->getSequence()]
                      .connections.size());
    EXPECT_FALSE(
            mScheduler->mutableAttachedChoreographers()[layer1->getSequence()].frameRate.isValid());

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer2->getSequence()));
    EXPECT_EQ(1u,
              mScheduler->mutableAttachedChoreographers()[layer2->getSequence()]
                      .connections.size());
    EXPECT_FALSE(
            mScheduler->mutableAttachedChoreographers()[layer2->getSequence()].frameRate.isValid());
}

TEST_F(AttachedChoreographerTest, removedWhenConnectionIsGone) {
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());

    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);

    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));
    EXPECT_EQ(1u,
              mScheduler->mutableAttachedChoreographers()[layer->getSequence()].connections.size());

    // The connection is used all over this test, so it is quite hard to release it from here.
    // Instead, we just do a small shortcut.
    {
        EXPECT_CALL(*mEventThread, registerDisplayEventConnection(_)).WillOnce(Return(0));
        sp<MockEventThreadConnection> mockConnection =
                sp<MockEventThreadConnection>::make(mEventThread);
        mScheduler->mutableAttachedChoreographers()[layer->getSequence()].connections.clear();
        mScheduler->mutableAttachedChoreographers()[layer->getSequence()].connections.emplace(
                mockConnection);
    }

    RequestedLayerState layerState(LayerCreationArgs(layer->getSequence()));
    LayerHierarchy hierarchy(&layerState);
    mScheduler->updateAttachedChoreographers(hierarchy, 60_Hz);
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());
}

TEST_F(AttachedChoreographerTest, removedWhenLayerIsGone) {
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());

    sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    const sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    layer.clear();
    EXPECT_TRUE(mScheduler->mutableAttachedChoreographers().empty());
}

void AttachedChoreographerTest::frameRateTestScenario(Fps layerFps, int8_t frameRateCompatibility,
                                                      Fps displayFps,
                                                      Fps expectedChoreographerFps) {
    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    RequestedLayerState layerState(LayerCreationArgs(layer->getSequence()));
    LayerHierarchy hierarchy(&layerState);

    layerState.frameRate = layerFps.getValue();
    layerState.frameRateCompatibility = frameRateCompatibility;

    mScheduler->updateAttachedChoreographers(hierarchy, displayFps);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));
    EXPECT_EQ(expectedChoreographerFps,
              mScheduler->mutableAttachedChoreographers()[layer->getSequence()].frameRate);
    EXPECT_EQ(expectedChoreographerFps, mEventThreadConnection->frameRate);
}

TEST_F(AttachedChoreographerTest, setsFrameRateDefault) {
    Fps layerFps = 30_Hz;
    int8_t frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;
    Fps displayFps = 60_Hz;
    Fps expectedChoreographerFps = 30_Hz;

    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);

    layerFps = Fps::fromValue(32.7f);
    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);
}

TEST_F(AttachedChoreographerTest, setsFrameRateExact) {
    Fps layerFps = 30_Hz;
    int8_t frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_EXACT;
    Fps displayFps = 60_Hz;
    Fps expectedChoreographerFps = 30_Hz;

    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);

    layerFps = Fps::fromValue(32.7f);
    expectedChoreographerFps = {};
    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);
}

TEST_F(AttachedChoreographerTest, setsFrameRateExactOrMultiple) {
    Fps layerFps = 30_Hz;
    int8_t frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_FIXED_SOURCE;
    Fps displayFps = 60_Hz;
    Fps expectedChoreographerFps = 30_Hz;

    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);

    layerFps = Fps::fromValue(32.7f);
    expectedChoreographerFps = {};
    frameRateTestScenario(layerFps, frameRateCompatibility, displayFps, expectedChoreographerFps);
}

TEST_F(AttachedChoreographerTest, setsFrameRateParent) {
    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> parent = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, parent->getHandle());

    RequestedLayerState parentState(LayerCreationArgs(parent->getSequence()));
    LayerHierarchy parentHierarchy(&parentState);

    RequestedLayerState layerState(LayerCreationArgs(layer->getSequence()));
    LayerHierarchy hierarchy(&layerState);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&hierarchy, LayerHierarchy::Variant::Attached));

    layerState.frameRate = (30_Hz).getValue();
    layerState.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    mScheduler->updateAttachedChoreographers(parentHierarchy, 120_Hz);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(parent->getSequence()));

    EXPECT_EQ(30_Hz, mScheduler->mutableAttachedChoreographers()[parent->getSequence()].frameRate);
}

TEST_F(AttachedChoreographerTest, setsFrameRateParent2Children) {
    const sp<MockLayer> layer1 = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> layer2 = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> parent = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, parent->getHandle());

    RequestedLayerState parentState(LayerCreationArgs(parent->getSequence()));
    LayerHierarchy parentHierarchy(&parentState);

    RequestedLayerState layer1State(LayerCreationArgs(layer1->getSequence()));
    LayerHierarchy layer1Hierarchy(&layer1State);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&layer1Hierarchy, LayerHierarchy::Variant::Attached));

    RequestedLayerState layer2State(LayerCreationArgs(layer1->getSequence()));
    LayerHierarchy layer2Hierarchy(&layer2State);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&layer2Hierarchy, LayerHierarchy::Variant::Attached));

    layer1State.frameRate = (30_Hz).getValue();
    layer1State.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    layer2State.frameRate = (20_Hz).getValue();
    layer2State.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    mScheduler->updateAttachedChoreographers(parentHierarchy, 120_Hz);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(parent->getSequence()));

    EXPECT_EQ(60_Hz, mScheduler->mutableAttachedChoreographers()[parent->getSequence()].frameRate);
}

TEST_F(AttachedChoreographerTest, setsFrameRateParentConflictingChildren) {
    const sp<MockLayer> layer1 = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> layer2 = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> parent = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, parent->getHandle());

    RequestedLayerState parentState(LayerCreationArgs(parent->getSequence()));
    LayerHierarchy parentHierarchy(&parentState);

    RequestedLayerState layer1State(LayerCreationArgs(layer1->getSequence()));
    LayerHierarchy layer1Hierarchy(&layer1State);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&layer1Hierarchy, LayerHierarchy::Variant::Attached));

    RequestedLayerState layer2State(LayerCreationArgs(layer1->getSequence()));
    LayerHierarchy layer2Hierarchy(&layer2State);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&layer2Hierarchy, LayerHierarchy::Variant::Attached));

    layer1State.frameRate = (30_Hz).getValue();
    layer1State.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    layer2State.frameRate = (25_Hz).getValue();
    layer2State.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    mScheduler->updateAttachedChoreographers(parentHierarchy, 120_Hz);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(parent->getSequence()));

    EXPECT_EQ(Fps(), mScheduler->mutableAttachedChoreographers()[parent->getSequence()].frameRate);
}

TEST_F(AttachedChoreographerTest, setsFrameRateChild) {
    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> parent = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    RequestedLayerState parentState(LayerCreationArgs(parent->getSequence()));
    LayerHierarchy parentHierarchy(&parentState);

    RequestedLayerState layerState(LayerCreationArgs(layer->getSequence()));
    LayerHierarchy hierarchy(&layerState);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&hierarchy, LayerHierarchy::Variant::Attached));

    parentState.frameRate = (30_Hz).getValue();
    parentState.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    mScheduler->updateAttachedChoreographers(parentHierarchy, 120_Hz);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));

    EXPECT_EQ(30_Hz, mScheduler->mutableAttachedChoreographers()[layer->getSequence()].frameRate);
}

TEST_F(AttachedChoreographerTest, setsFrameRateChildNotOverriddenByParent) {
    const sp<MockLayer> layer = sp<MockLayer>::make(mFlinger.flinger());
    const sp<MockLayer> parent = sp<MockLayer>::make(mFlinger.flinger());

    EXPECT_CALL(mSchedulerCallback, onChoreographerAttached);
    sp<IDisplayEventConnection> connection =
            mScheduler->createDisplayEventConnection(Cycle::Render, {}, layer->getHandle());

    RequestedLayerState parentState(LayerCreationArgs(parent->getSequence()));
    LayerHierarchy parentHierarchy(&parentState);

    RequestedLayerState layerState(LayerCreationArgs(layer->getSequence()));
    LayerHierarchy hierarchy(&layerState);
    parentHierarchy.mChildren.push_back(
            std::make_pair(&hierarchy, LayerHierarchy::Variant::Attached));

    parentState.frameRate = (30_Hz).getValue();
    parentState.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    layerState.frameRate = (60_Hz).getValue();
    layerState.frameRateCompatibility = ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT;

    mScheduler->updateAttachedChoreographers(parentHierarchy, 120_Hz);

    ASSERT_EQ(1u, mScheduler->mutableAttachedChoreographers().count(layer->getSequence()));

    EXPECT_EQ(60_Hz, mScheduler->mutableAttachedChoreographers()[layer->getSequence()].frameRate);
}

class SelectPacesetterDisplayTest : public SchedulerTest {};

TEST_F(SelectPacesetterDisplayTest, SingleDisplay) FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);
    mScheduler->designatePacesetterDisplay();

    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
}

TEST_F(SelectPacesetterDisplayTest, TwoDisplaysDifferentRefreshRates)
FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode120->getId()),
                                kActiveDisplayId);
    // setDisplayPowerMode() should trigger pacesetter migration to display 2.
    EXPECT_TRUE(mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON));

    // Display2 has the higher refresh rate so should be the pacesetter.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId2);
}

TEST_F(SelectPacesetterDisplayTest, TwoDisplaysHigherIgnoredPowerOff)
FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode120->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::OFF);

    mScheduler->designatePacesetterDisplay();

    // Display2 has the higher refresh rate but is off so should not be considered.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
}

TEST_F(SelectPacesetterDisplayTest, TwoDisplaysAllOffFirstUsed) FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::OFF);

    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::External,
                                std::make_shared<RefreshRateSelector>(kDisplay2Modes,
                                                                      kDisplay2Mode120->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::OFF);

    mScheduler->designatePacesetterDisplay();

    // When all displays are off just use the first display as pacesetter.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
}

TEST_F(SelectPacesetterDisplayTest, TwoDisplaysWithinEpsilon) FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::pacesetter_selection, true);

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal,
                                std::make_shared<RefreshRateSelector>(kDisplay1Modes,
                                                                      kDisplay1Mode60->getId()),
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    auto selector2 =
            std::make_shared<RefreshRateSelector>(kDisplay2Modes, kDisplay2Mode60->getId());
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal, selector2,
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId2, hal::PowerMode::ON);

    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);

    // If the highest refresh rate is within a small epsilon of the current pacesetter display's
    // refresh rate, let the current pacesetter stay.
    selector2->setActiveMode(kDisplay2Mode60point01->getId(), 60.01_Hz);
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
}

TEST_F(SchedulerTest, selectorPtrForLayerStack) FTL_FAKE_GUARD(kMainThreadContext) {
    SET_FLAG_FOR_TEST(flags::follower_arbitrary_refresh_rate_selection, true);

    auto selector1 =
            std::make_shared<RefreshRateSelector>(kDisplay1Modes, kDisplay1Mode60->getId());
    ui::LayerStack stack1 = ui::LayerStack::fromValue(123);
    selector1->setLayerFilter({stack1, false});

    constexpr PhysicalDisplayId kActiveDisplayId = kDisplayId1;
    mScheduler->registerDisplay(kDisplayId1, ui::DisplayConnectionType::Internal, selector1,
                                kActiveDisplayId);
    mScheduler->setDisplayPowerMode(kDisplayId1, hal::PowerMode::ON);

    auto selector2 =
            std::make_shared<RefreshRateSelector>(kDisplay2Modes, kDisplay2Mode60->getId());
    ui::LayerStack stack2 = ui::LayerStack::fromValue(467);
    selector2->setLayerFilter({stack2, false});
    mScheduler->registerDisplay(kDisplayId2, ui::DisplayConnectionType::Internal, selector2,
                                kActiveDisplayId);

    EXPECT_EQ(mScheduler->selectorPtrForLayerStack(stack1), selector1.get());

    EXPECT_EQ(mScheduler->selectorPtrForLayerStack(stack2), selector2.get());

    // Expect the pacesetter selector if the argument does not match any known selector stack.
    EXPECT_EQ(mScheduler->pacesetterDisplayId(), kDisplayId1);
    EXPECT_EQ(mScheduler->selectorPtrForLayerStack(ui::LayerStack::fromValue(11111u)),
              selector1.get());
}

} // namespace android::scheduler
