/*
 * Copyright 2018 The Android Open Source Project
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
#define LOG_TAG "Scheduler"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "Scheduler.h"

#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <android/hardware/configstore/1.1/ISurfaceFlingerConfigs.h>
#include <common/trace.h>
#include <configstore/Utils.h>
#include <ftl/concat.h>
#include <ftl/enum.h>
#include <ftl/fake_guard.h>
#include <ftl/small_map.h>
#include <gui/WindowInfo.h>
#include <system/window.h>
#include <ui/DisplayMap.h>
#include <utils/Timers.h>

#include <FrameTimeline/FrameTimeline.h>
#include <scheduler/interface/ICompositor.h>

#include <cinttypes>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>

#include <common/FlagManager.h>
#include "EventThread.h"
#include "FrameRateOverrideMappings.h"
#include "FrontEnd/LayerHandle.h"
#include "Layer.h"
#include "OneShotTimer.h"
#include "RefreshRateStats.h"
#include "SurfaceFlingerFactory.h"
#include "SurfaceFlingerProperties.h"
#include "TimeStats/TimeStats.h"
#include "VsyncConfiguration.h"
#include "VsyncController.h"
#include "VsyncSchedule.h"

namespace android::scheduler {

Scheduler::Scheduler(ICompositor& compositor, ISchedulerCallback& callback, FeatureFlags features,
                     surfaceflinger::Factory& factory, Fps activeRefreshRate, TimeStats& timeStats)
      : android::impl::MessageQueue(compositor),
        mFeatures(features),
        mVsyncConfiguration(factory.createVsyncConfiguration(activeRefreshRate)),
        mVsyncModulator(sp<VsyncModulator>::make(mVsyncConfiguration->getCurrentConfigs())),
        mRefreshRateStats(std::make_unique<RefreshRateStats>(timeStats, activeRefreshRate)),
        mSchedulerCallback(callback) {}

Scheduler::~Scheduler() {
    // MessageQueue depends on VsyncSchedule, so first destroy it.
    // Otherwise, MessageQueue will get destroyed after Scheduler's dtor,
    // which will cause a use-after-free issue.
    Impl::destroyVsync();

    // Stop timers and wait for their threads to exit.
    mDisplayPowerTimer.reset();
    mTouchTimer.reset();

    // Stop idle timer and clear callbacks, as the RefreshRateSelector may outlive the Scheduler.
    demotePacesetterDisplay({.toggleIdleTimer = true});
}

void Scheduler::initVsync(frametimeline::TokenManager& tokenManager,
                          std::chrono::nanoseconds workDuration) {
    Impl::initVsyncInternal(getVsyncSchedule()->getDispatch(), tokenManager, workDuration);
}

void Scheduler::startTimers() {
    using namespace sysprop;
    using namespace std::string_literals;

    const int32_t defaultTouchTimerValue =
            FlagManager::getInstance().enable_fro_dependent_features() &&
                    sysprop::enable_frame_rate_override(true)
            ? 200
            : 0;
    if (const int32_t millis = set_touch_timer_ms(defaultTouchTimerValue); millis > 0) {
        // Touch events are coming to SF every 100ms, so the timer needs to be higher than that
        mTouchTimer.emplace(
                "TouchTimer", std::chrono::milliseconds(millis),
                [this] { touchTimerCallback(TimerState::Reset); },
                [this] { touchTimerCallback(TimerState::Expired); });
        mTouchTimer->start();
    }

    if (const int64_t millis = set_display_power_timer_ms(0); millis > 0) {
        mDisplayPowerTimer.emplace(
                "DisplayPowerTimer", std::chrono::milliseconds(millis),
                [this] { displayPowerTimerCallback(TimerState::Reset); },
                [this] { displayPowerTimerCallback(TimerState::Expired); });
        mDisplayPowerTimer->start();
    }
}

void Scheduler::setPacesetterDisplay(PhysicalDisplayId pacesetterId) {
    constexpr PromotionParams kPromotionParams = {.toggleIdleTimer = true};

    demotePacesetterDisplay(kPromotionParams);
    promotePacesetterDisplay(pacesetterId, kPromotionParams);

    // Cancel the pending refresh rate change, if any, before updating the phase configuration.
    mVsyncModulator->cancelRefreshRateChange();

    mVsyncConfiguration->reset();
    updatePhaseConfiguration(pacesetterId, pacesetterSelectorPtr()->getActiveMode().fps);
}

void Scheduler::registerDisplay(PhysicalDisplayId displayId, RefreshRateSelectorPtr selectorPtr,
                                PhysicalDisplayId activeDisplayId) {
    auto schedulePtr =
            std::make_shared<VsyncSchedule>(selectorPtr->getActiveMode().modePtr, mFeatures,
                                            [this](PhysicalDisplayId id, bool enable) {
                                                onHardwareVsyncRequest(id, enable);
                                            });

    registerDisplayInternal(displayId, std::move(selectorPtr), std::move(schedulePtr),
                            activeDisplayId);
}

void Scheduler::registerDisplayInternal(PhysicalDisplayId displayId,
                                        RefreshRateSelectorPtr selectorPtr,
                                        VsyncSchedulePtr schedulePtr,
                                        PhysicalDisplayId activeDisplayId) {
    const bool isPrimary = (ftl::FakeGuard(mDisplayLock), !mPacesetterDisplayId);

    // Start the idle timer for the first registered (i.e. primary) display.
    const PromotionParams promotionParams = {.toggleIdleTimer = isPrimary};

    demotePacesetterDisplay(promotionParams);

    auto [pacesetterVsyncSchedule, isNew] = [&]() REQUIRES(kMainThreadContext) {
        std::scoped_lock lock(mDisplayLock);
        const bool isNew = mDisplays
                                   .emplace_or_replace(displayId, displayId, std::move(selectorPtr),
                                                       std::move(schedulePtr), mFeatures)
                                   .second;

        return std::make_pair(promotePacesetterDisplayLocked(activeDisplayId, promotionParams),
                              isNew);
    }();

    applyNewVsyncSchedule(std::move(pacesetterVsyncSchedule));

    // Disable hardware VSYNC if the registration is new, as opposed to a renewal.
    if (isNew) {
        onHardwareVsyncRequest(displayId, false);
    }

    dispatchHotplug(displayId, Hotplug::Connected);
}

void Scheduler::unregisterDisplay(PhysicalDisplayId displayId, PhysicalDisplayId activeDisplayId) {
    LOG_ALWAYS_FATAL_IF(displayId == activeDisplayId, "Cannot unregister the active display!");

    dispatchHotplug(displayId, Hotplug::Disconnected);

    constexpr PromotionParams kPromotionParams = {.toggleIdleTimer = false};
    demotePacesetterDisplay(kPromotionParams);

    std::shared_ptr<VsyncSchedule> pacesetterVsyncSchedule;
    {
        std::scoped_lock lock(mDisplayLock);
        mDisplays.erase(displayId);

        // Do not allow removing the final display. Code in the scheduler expects
        // there to be at least one display. (This may be relaxed in the future with
        // headless virtual display.)
        LOG_ALWAYS_FATAL_IF(mDisplays.empty(), "Cannot unregister all displays!");

        pacesetterVsyncSchedule = promotePacesetterDisplayLocked(activeDisplayId, kPromotionParams);
    }
    applyNewVsyncSchedule(std::move(pacesetterVsyncSchedule));
}

void Scheduler::run() {
    while (true) {
        waitMessage();
    }
}

void Scheduler::onFrameSignal(ICompositor& compositor, VsyncId vsyncId,
                              TimePoint expectedVsyncTime) {
    const auto debugPresentDelay = mDebugPresentDelay.load();
    mDebugPresentDelay.store(std::nullopt);

    const FrameTargeter::BeginFrameArgs beginFrameArgs =
            {.frameBeginTime = SchedulerClock::now(),
             .vsyncId = vsyncId,
             .expectedVsyncTime = expectedVsyncTime,
             .sfWorkDuration = mVsyncModulator->getVsyncConfig().sfWorkDuration,
             .hwcMinWorkDuration = mVsyncConfiguration->getCurrentConfigs().hwcMinWorkDuration,
             .debugPresentTimeDelay = debugPresentDelay};

    ftl::NonNull<const Display*> pacesetterPtr = pacesetterPtrLocked();
    pacesetterPtr->targeterPtr->beginFrame(beginFrameArgs, *pacesetterPtr->schedulePtr);

    {
        FrameTargets targets;
        targets.try_emplace(pacesetterPtr->displayId, &pacesetterPtr->targeterPtr->target());

        // TODO (b/256196556): Followers should use the next VSYNC after the frontrunner, not the
        // pacesetter.
        // Update expectedVsyncTime, which may have been adjusted by beginFrame.
        expectedVsyncTime = pacesetterPtr->targeterPtr->target().expectedPresentTime();

        for (const auto& [id, display] : mDisplays) {
            if (id == pacesetterPtr->displayId) continue;

            auto followerBeginFrameArgs = beginFrameArgs;
            followerBeginFrameArgs.expectedVsyncTime =
                    display.schedulePtr->vsyncDeadlineAfter(expectedVsyncTime);

            FrameTargeter& targeter = *display.targeterPtr;
            targeter.beginFrame(followerBeginFrameArgs, *display.schedulePtr);
            targets.try_emplace(id, &targeter.target());
        }

        if (!compositor.commit(pacesetterPtr->displayId, targets)) {
            if (FlagManager::getInstance().vrr_config()) {
                compositor.sendNotifyExpectedPresentHint(pacesetterPtr->displayId);
            }
            mSchedulerCallback.onCommitNotComposited();
            return;
        }
    }

    // The pacesetter may have changed or been registered anew during commit.
    pacesetterPtr = pacesetterPtrLocked();

    // TODO(b/256196556): Choose the frontrunner display.
    FrameTargeters targeters;
    targeters.try_emplace(pacesetterPtr->displayId, pacesetterPtr->targeterPtr.get());

    for (auto& [id, display] : mDisplays) {
        if (id == pacesetterPtr->displayId) continue;

        FrameTargeter& targeter = *display.targeterPtr;
        targeters.try_emplace(id, &targeter);
    }

    if (FlagManager::getInstance().vrr_config() &&
        CC_UNLIKELY(mPacesetterFrameDurationFractionToSkip > 0.f)) {
        const auto period = pacesetterPtr->targeterPtr->target().expectedFrameDuration();
        const auto skipDuration = Duration::fromNs(
                static_cast<nsecs_t>(period.ns() * mPacesetterFrameDurationFractionToSkip));
        SFTRACE_FORMAT("Injecting jank for %f%% of the frame (%" PRId64 " ns)",
                       mPacesetterFrameDurationFractionToSkip * 100, skipDuration.ns());
        std::this_thread::sleep_for(skipDuration);
        mPacesetterFrameDurationFractionToSkip = 0.f;
    }

    const auto resultsPerDisplay = compositor.composite(pacesetterPtr->displayId, targeters);
    if (FlagManager::getInstance().vrr_config()) {
        compositor.sendNotifyExpectedPresentHint(pacesetterPtr->displayId);
    }
    compositor.sample();

    for (const auto& [id, targeter] : targeters) {
        const auto resultOpt = resultsPerDisplay.get(id);
        LOG_ALWAYS_FATAL_IF(!resultOpt);
        targeter->endFrame(*resultOpt);
    }
}

std::optional<Fps> Scheduler::getFrameRateOverride(uid_t uid) const {
    const bool supportsFrameRateOverrideByContent =
            pacesetterSelectorPtr()->supportsAppFrameRateOverrideByContent();
    return mFrameRateOverrideMappings
            .getFrameRateOverrideForUid(uid, supportsFrameRateOverrideByContent);
}

bool Scheduler::isVsyncValid(TimePoint expectedVsyncTime, uid_t uid) const {
    const auto frameRate = getFrameRateOverride(uid);
    if (!frameRate.has_value()) {
        return true;
    }

    SFTRACE_FORMAT("%s uid: %d frameRate: %s", __func__, uid, to_string(*frameRate).c_str());
    return getVsyncSchedule()->getTracker().isVSyncInPhase(expectedVsyncTime.ns(), *frameRate);
}

bool Scheduler::isVsyncInPhase(TimePoint expectedVsyncTime, Fps frameRate) const {
    return getVsyncSchedule()->getTracker().isVSyncInPhase(expectedVsyncTime.ns(), frameRate);
}

bool Scheduler::throttleVsync(android::TimePoint expectedPresentTime, uid_t uid) {
    return !isVsyncValid(expectedPresentTime, uid);
}

Period Scheduler::getVsyncPeriod(uid_t uid) {
    const auto [refreshRate, period] = [this] {
        std::scoped_lock lock(mDisplayLock);
        const auto pacesetterOpt = pacesetterDisplayLocked();
        LOG_ALWAYS_FATAL_IF(!pacesetterOpt);
        const Display& pacesetter = *pacesetterOpt;
        const FrameRateMode& frameRateMode = pacesetter.selectorPtr->getActiveMode();
        const auto refreshRate = frameRateMode.fps;
        const auto displayVsync = frameRateMode.modePtr->getVsyncRate();
        const auto numPeriod = RefreshRateSelector::getFrameRateDivisor(displayVsync, refreshRate);
        return std::make_pair(refreshRate, numPeriod * pacesetter.schedulePtr->period());
    }();

    const Period currentPeriod = period != Period::zero() ? period : refreshRate.getPeriod();

    const auto frameRate = getFrameRateOverride(uid);
    if (!frameRate.has_value()) {
        return currentPeriod;
    }

    const auto divisor = RefreshRateSelector::getFrameRateDivisor(refreshRate, *frameRate);
    if (divisor <= 1) {
        return currentPeriod;
    }

    // TODO(b/299378819): the casting is not needed, but we need a flag as it might change
    // behaviour.
    return Period::fromNs(currentPeriod.ns() * divisor);
}
void Scheduler::onExpectedPresentTimePosted(TimePoint expectedPresentTime) {
    const auto frameRateMode = [this] {
        std::scoped_lock lock(mDisplayLock);
        const auto pacesetterOpt = pacesetterDisplayLocked();
        const Display& pacesetter = *pacesetterOpt;
        return pacesetter.selectorPtr->getActiveMode();
    }();

    if (frameRateMode.modePtr->getVrrConfig()) {
        mSchedulerCallback.onExpectedPresentTimePosted(expectedPresentTime, frameRateMode.modePtr,
                                                       frameRateMode.fps);
    }
}

void Scheduler::createEventThread(Cycle cycle, frametimeline::TokenManager* tokenManager,
                                  std::chrono::nanoseconds workDuration,
                                  std::chrono::nanoseconds readyDuration) {
    auto eventThread =
            std::make_unique<android::impl::EventThread>(cycle == Cycle::Render ? "app" : "appSf",
                                                         getVsyncSchedule(), tokenManager, *this,
                                                         workDuration, readyDuration);

    if (cycle == Cycle::Render) {
        mRenderEventThread = std::move(eventThread);
    } else {
        mLastCompositeEventThread = std::move(eventThread);
    }
}

sp<IDisplayEventConnection> Scheduler::createDisplayEventConnection(
        Cycle cycle, EventRegistrationFlags eventRegistration, const sp<IBinder>& layerHandle) {
    const auto connection = eventThreadFor(cycle).createEventConnection(eventRegistration);
    const auto layerId = static_cast<int32_t>(LayerHandle::getLayerId(layerHandle));

    if (layerId != static_cast<int32_t>(UNASSIGNED_LAYER_ID)) {
        // TODO(b/290409668): Moving the choreographer attachment to be a transaction that will be
        // processed on the main thread.
        mSchedulerCallback.onChoreographerAttached();

        std::scoped_lock lock(mChoreographerLock);
        const auto [iter, emplaced] =
                mAttachedChoreographers.emplace(layerId,
                                                AttachedChoreographers{Fps(), {connection}});
        if (!emplaced) {
            iter->second.connections.emplace(connection);
            connection->frameRate = iter->second.frameRate;
        }
    }
    return connection;
}

void Scheduler::dispatchHotplug(PhysicalDisplayId displayId, Hotplug hotplug) {
    if (hasEventThreads()) {
        const bool connected = hotplug == Hotplug::Connected;
        eventThreadFor(Cycle::Render).onHotplugReceived(displayId, connected);
        eventThreadFor(Cycle::LastComposite).onHotplugReceived(displayId, connected);
    }
}

void Scheduler::dispatchHotplugError(int32_t errorCode) {
    if (hasEventThreads()) {
        eventThreadFor(Cycle::Render).onHotplugConnectionError(errorCode);
        eventThreadFor(Cycle::LastComposite).onHotplugConnectionError(errorCode);
    }
}

void Scheduler::enableSyntheticVsync(bool enable) {
    eventThreadFor(Cycle::Render).enableSyntheticVsync(enable);
}

void Scheduler::omitVsyncDispatching(bool omitted) {
    eventThreadFor(Cycle::Render).omitVsyncDispatching(omitted);
    // Note: If we don't couple Cycle::LastComposite event thread, there is a black screen
    // after boot. This is most likely sysui or system_server dependency on sf instance
    // Choreographer
    eventThreadFor(Cycle::LastComposite).omitVsyncDispatching(omitted);
}

void Scheduler::onFrameRateOverridesChanged() {
    const auto [pacesetterId, supportsFrameRateOverrideByContent] = [this] {
        std::scoped_lock lock(mDisplayLock);
        const auto pacesetterOpt = pacesetterDisplayLocked();
        LOG_ALWAYS_FATAL_IF(!pacesetterOpt);
        const Display& pacesetter = *pacesetterOpt;
        return std::make_pair(FTL_FAKE_GUARD(kMainThreadContext, *mPacesetterDisplayId),
                              pacesetter.selectorPtr->supportsAppFrameRateOverrideByContent());
    }();

    std::vector<FrameRateOverride> overrides =
            mFrameRateOverrideMappings.getAllFrameRateOverrides(supportsFrameRateOverrideByContent);

    eventThreadFor(Cycle::Render).onFrameRateOverridesChanged(pacesetterId, std::move(overrides));
}

void Scheduler::onHdcpLevelsChanged(Cycle cycle, PhysicalDisplayId displayId,
                                    int32_t connectedLevel, int32_t maxLevel) {
    eventThreadFor(cycle).onHdcpLevelsChanged(displayId, connectedLevel, maxLevel);
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value" // b/369277774
bool Scheduler::onDisplayModeChanged(PhysicalDisplayId displayId, const FrameRateMode& mode,
                                     bool clearContentRequirements) {
    const bool isPacesetter =
            FTL_FAKE_GUARD(kMainThreadContext,
                           (std::scoped_lock(mDisplayLock), displayId == mPacesetterDisplayId));

    if (isPacesetter) {
        std::lock_guard<std::mutex> lock(mPolicyLock);
        mPolicy.emittedModeOpt = mode;

        if (clearContentRequirements) {
            // Invalidate content based refresh rate selection so it could be calculated
            // again for the new refresh rate.
            mPolicy.contentRequirements.clear();
        }
    }

    if (hasEventThreads()) {
        eventThreadFor(Cycle::Render).onModeChanged(mode);
    }

    return isPacesetter;
}
#pragma clang diagnostic pop

void Scheduler::onDisplayModeRejected(PhysicalDisplayId displayId, DisplayModeId modeId) {
    if (hasEventThreads()) {
        eventThreadFor(Cycle::Render).onModeRejected(displayId, modeId);
    }
}

void Scheduler::emitModeChangeIfNeeded() {
    if (!mPolicy.modeOpt || !mPolicy.emittedModeOpt) {
        ALOGW("No mode change to emit");
        return;
    }

    const auto& mode = *mPolicy.modeOpt;

    if (mode != pacesetterSelectorPtr()->getActiveMode()) {
        // A mode change is pending. The event will be emitted when the mode becomes active.
        return;
    }

    if (mode == *mPolicy.emittedModeOpt) {
        // The event was already emitted.
        return;
    }

    mPolicy.emittedModeOpt = mode;

    if (hasEventThreads()) {
        eventThreadFor(Cycle::Render).onModeChanged(mode);
    }
}

void Scheduler::dump(Cycle cycle, std::string& result) const {
    eventThreadFor(cycle).dump(result);
}

void Scheduler::setDuration(Cycle cycle, std::chrono::nanoseconds workDuration,
                            std::chrono::nanoseconds readyDuration) {
    if (hasEventThreads()) {
        eventThreadFor(cycle).setDuration(workDuration, readyDuration);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value" // b/369277774
void Scheduler::updatePhaseConfiguration(PhysicalDisplayId displayId, Fps refreshRate) {
    const bool isPacesetter =
            FTL_FAKE_GUARD(kMainThreadContext,
                           (std::scoped_lock(mDisplayLock), displayId == mPacesetterDisplayId));
    if (!isPacesetter) return;

    mRefreshRateStats->setRefreshRate(refreshRate);
    mVsyncConfiguration->setRefreshRateFps(refreshRate);
    setVsyncConfig(mVsyncModulator->setVsyncConfigSet(mVsyncConfiguration->getCurrentConfigs()),
                   refreshRate.getPeriod());
}
#pragma clang diagnostic pop

void Scheduler::setActiveDisplayPowerModeForRefreshRateStats(hal::PowerMode powerMode) {
    mRefreshRateStats->setPowerMode(powerMode);
}

void Scheduler::setVsyncConfig(const VsyncConfig& config, Period vsyncPeriod) {
    setDuration(Cycle::Render,
                /* workDuration */ config.appWorkDuration,
                /* readyDuration */ config.sfWorkDuration);
    setDuration(Cycle::LastComposite,
                /* workDuration */ vsyncPeriod,
                /* readyDuration */ config.sfWorkDuration);
    setDuration(config.sfWorkDuration);
}

void Scheduler::enableHardwareVsync(PhysicalDisplayId id) {
    auto schedule = getVsyncSchedule(id);
    LOG_ALWAYS_FATAL_IF(!schedule);
    schedule->enableHardwareVsync();
}

void Scheduler::disableHardwareVsync(PhysicalDisplayId id, bool disallow) {
    auto schedule = getVsyncSchedule(id);
    LOG_ALWAYS_FATAL_IF(!schedule);
    schedule->disableHardwareVsync(disallow);
}

void Scheduler::resyncAllToHardwareVsync(bool allowToEnable) {
    SFTRACE_CALL();
    std::scoped_lock lock(mDisplayLock);
    ftl::FakeGuard guard(kMainThreadContext);

    for (const auto& [id, display] : mDisplays) {
        if (display.powerMode != hal::PowerMode::OFF ||
            !FlagManager::getInstance().multithreaded_present()) {
            resyncToHardwareVsyncLocked(id, allowToEnable);
        }
    }
}

void Scheduler::resyncToHardwareVsyncLocked(PhysicalDisplayId id, bool allowToEnable,
                                            DisplayModePtr modePtr) {
    const auto displayOpt = mDisplays.get(id);
    if (!displayOpt) {
        ALOGW("%s: Invalid display %s!", __func__, to_string(id).c_str());
        return;
    }
    const Display& display = *displayOpt;

    if (display.schedulePtr->isHardwareVsyncAllowed(allowToEnable)) {
        if (!modePtr) {
            modePtr = display.selectorPtr->getActiveMode().modePtr.get();
        }
        if (modePtr->getVsyncRate().isValid()) {
            constexpr bool kForce = false;
            display.schedulePtr->onDisplayModeChanged(ftl::as_non_null(modePtr), kForce);
        }
    }
}

void Scheduler::onHardwareVsyncRequest(PhysicalDisplayId id, bool enabled) {
    static const auto& whence = __func__;
    SFTRACE_NAME(ftl::Concat(whence, ' ', id.value, ' ', enabled).c_str());

    // On main thread to serialize reads/writes of pending hardware VSYNC state.
    static_cast<void>(
            schedule([=, this]() FTL_FAKE_GUARD(mDisplayLock) FTL_FAKE_GUARD(kMainThreadContext) {
                SFTRACE_NAME(ftl::Concat(whence, ' ', id.value, ' ', enabled).c_str());

                if (const auto displayOpt = mDisplays.get(id)) {
                    auto& display = displayOpt->get();
                    display.schedulePtr->setPendingHardwareVsyncState(enabled);

                    if (display.powerMode != hal::PowerMode::OFF) {
                        mSchedulerCallback.requestHardwareVsync(id, enabled);
                    }
                }
            }));
}

void Scheduler::setRenderRate(PhysicalDisplayId id, Fps renderFrameRate, bool applyImmediately) {
    std::scoped_lock lock(mDisplayLock);
    ftl::FakeGuard guard(kMainThreadContext);

    const auto displayOpt = mDisplays.get(id);
    if (!displayOpt) {
        ALOGW("%s: Invalid display %s!", __func__, to_string(id).c_str());
        return;
    }
    const Display& display = *displayOpt;
    const auto mode = display.selectorPtr->getActiveMode();

    using fps_approx_ops::operator!=;
    LOG_ALWAYS_FATAL_IF(renderFrameRate != mode.fps,
                        "Mismatch in render frame rates. Selector: %s, Scheduler: %s, Display: "
                        "%" PRIu64,
                        to_string(mode.fps).c_str(), to_string(renderFrameRate).c_str(), id.value);

    ALOGV("%s %s (%s)", __func__, to_string(mode.fps).c_str(),
          to_string(mode.modePtr->getVsyncRate()).c_str());

    display.schedulePtr->getTracker().setRenderRate(renderFrameRate, applyImmediately);
}

Fps Scheduler::getNextFrameInterval(PhysicalDisplayId id,
                                    TimePoint currentExpectedPresentTime) const {
    std::scoped_lock lock(mDisplayLock);
    ftl::FakeGuard guard(kMainThreadContext);

    const auto displayOpt = mDisplays.get(id);
    if (!displayOpt) {
        ALOGW("%s: Invalid display %s!", __func__, to_string(id).c_str());
        return Fps{};
    }
    const Display& display = *displayOpt;
    const Duration threshold =
            display.selectorPtr->getActiveMode().modePtr->getVsyncRate().getPeriod() / 2;
    const TimePoint nextVsyncTime =
            display.schedulePtr->vsyncDeadlineAfter(currentExpectedPresentTime + threshold,
                                                    currentExpectedPresentTime);
    const Duration frameInterval = nextVsyncTime - currentExpectedPresentTime;
    return Fps::fromPeriodNsecs(frameInterval.ns());
}

void Scheduler::resync() {
    static constexpr nsecs_t kIgnoreDelay = ms2ns(750);

    const nsecs_t now = systemTime();
    const nsecs_t last = mLastResyncTime.exchange(now);

    if (now - last > kIgnoreDelay) {
        resyncAllToHardwareVsync(false /* allowToEnable */);
    }
}

bool Scheduler::addResyncSample(PhysicalDisplayId id, nsecs_t timestamp,
                                std::optional<nsecs_t> hwcVsyncPeriodIn) {
    const auto hwcVsyncPeriod = ftl::Optional(hwcVsyncPeriodIn).transform([](nsecs_t nanos) {
        return Period::fromNs(nanos);
    });
    auto schedule = getVsyncSchedule(id);
    if (!schedule) {
        ALOGW("%s: Invalid display %s!", __func__, to_string(id).c_str());
        return false;
    }
    return schedule->addResyncSample(TimePoint::fromNs(timestamp), hwcVsyncPeriod);
}

void Scheduler::addPresentFence(PhysicalDisplayId id, std::shared_ptr<FenceTime> fence) {
    SFTRACE_NAME(ftl::Concat(__func__, ' ', id.value).c_str());
    const auto scheduleOpt =
            (ftl::FakeGuard(mDisplayLock), mDisplays.get(id)).and_then([](const Display& display) {
                return display.powerMode == hal::PowerMode::OFF
                        ? std::nullopt
                        : std::make_optional(display.schedulePtr);
            });

    if (!scheduleOpt) return;
    const auto& schedule = scheduleOpt->get();

    const bool needMoreSignals = schedule->getController().addPresentFence(std::move(fence));
    if (needMoreSignals) {
        schedule->enableHardwareVsync();
    } else {
        constexpr bool kDisallow = false;
        schedule->disableHardwareVsync(kDisallow);
    }
}

void Scheduler::registerLayer(Layer* layer, FrameRateCompatibility frameRateCompatibility) {
    // If the content detection feature is off, we still keep the layer history,
    // since we use it for other features (like Frame Rate API), so layers
    // still need to be registered.
    mLayerHistory.registerLayer(layer, mFeatures.test(Feature::kContentDetection),
                                frameRateCompatibility);
}

void Scheduler::deregisterLayer(Layer* layer) {
    mLayerHistory.deregisterLayer(layer);
}

void Scheduler::onLayerDestroyed(Layer* layer) {
    std::scoped_lock lock(mChoreographerLock);
    mAttachedChoreographers.erase(layer->getSequence());
}

void Scheduler::recordLayerHistory(int32_t id, const LayerProps& layerProps, nsecs_t presentTime,
                                   nsecs_t now, LayerHistory::LayerUpdateType updateType) {
    if (pacesetterSelectorPtr()->canSwitch()) {
        mLayerHistory.record(id, layerProps, presentTime, now, updateType);
    }
}

void Scheduler::setModeChangePending(bool pending) {
    mLayerHistory.setModeChangePending(pending);
}

void Scheduler::setDefaultFrameRateCompatibility(
        int32_t id, scheduler::FrameRateCompatibility frameRateCompatibility) {
    mLayerHistory.setDefaultFrameRateCompatibility(id, frameRateCompatibility,
                                                   mFeatures.test(Feature::kContentDetection));
}

void Scheduler::setLayerProperties(int32_t id, const android::scheduler::LayerProps& properties) {
    mLayerHistory.setLayerProperties(id, properties);
}

void Scheduler::chooseRefreshRateForContent(
        const surfaceflinger::frontend::LayerHierarchy* hierarchy,
        bool updateAttachedChoreographer) {
    const auto selectorPtr = pacesetterSelectorPtr();
    if (!selectorPtr->canSwitch()) return;

    SFTRACE_CALL();

    LayerHistory::Summary summary = mLayerHistory.summarize(*selectorPtr, systemTime());
    applyPolicy(&Policy::contentRequirements, std::move(summary));

    if (updateAttachedChoreographer) {
        LOG_ALWAYS_FATAL_IF(!hierarchy);

        // update the attached choreographers after we selected the render rate.
        const ftl::Optional<FrameRateMode> modeOpt = [&] {
            std::scoped_lock lock(mPolicyLock);
            return mPolicy.modeOpt;
        }();

        if (modeOpt) {
            updateAttachedChoreographers(*hierarchy, modeOpt->fps);
        }
    }
}

void Scheduler::resetIdleTimer() {
    pacesetterSelectorPtr()->resetIdleTimer();
}

void Scheduler::onTouchHint() {
    if (mTouchTimer) {
        mTouchTimer->reset();
        pacesetterSelectorPtr()->resetKernelIdleTimer();
    }
}

void Scheduler::setDisplayPowerMode(PhysicalDisplayId id, hal::PowerMode powerMode) {
    const bool isPacesetter = [this, id]() REQUIRES(kMainThreadContext) {
        ftl::FakeGuard guard(mDisplayLock);
        return id == mPacesetterDisplayId;
    }();
    if (isPacesetter) {
        // TODO (b/255657128): This needs to be handled per display.
        std::lock_guard<std::mutex> lock(mPolicyLock);
        mPolicy.displayPowerMode = powerMode;
    }
    {
        std::scoped_lock lock(mDisplayLock);

        const auto displayOpt = mDisplays.get(id);
        LOG_ALWAYS_FATAL_IF(!displayOpt);
        auto& display = displayOpt->get();

        display.powerMode = powerMode;
        display.schedulePtr->getController().setDisplayPowerMode(powerMode);
    }
    if (!isPacesetter) return;

    if (mDisplayPowerTimer) {
        mDisplayPowerTimer->reset();
    }

    // Display Power event will boost the refresh rate to performance.
    // Clear Layer History to get fresh FPS detection
    mLayerHistory.clear();
}

auto Scheduler::getVsyncSchedule(std::optional<PhysicalDisplayId> idOpt) const
        -> ConstVsyncSchedulePtr {
    std::scoped_lock lock(mDisplayLock);
    return getVsyncScheduleLocked(idOpt);
}

auto Scheduler::getVsyncScheduleLocked(std::optional<PhysicalDisplayId> idOpt) const
        -> ConstVsyncSchedulePtr {
    ftl::FakeGuard guard(kMainThreadContext);

    if (!idOpt) {
        LOG_ALWAYS_FATAL_IF(!mPacesetterDisplayId, "Missing a pacesetter!");
        idOpt = mPacesetterDisplayId;
    }

    const auto displayOpt = mDisplays.get(*idOpt);
    if (!displayOpt) {
        return nullptr;
    }
    return displayOpt->get().schedulePtr;
}

void Scheduler::kernelIdleTimerCallback(TimerState state) {
    SFTRACE_INT("ExpiredKernelIdleTimer", static_cast<int>(state));

    // TODO(145561154): cleanup the kernel idle timer implementation and the refresh rate
    // magic number
    const Fps refreshRate = pacesetterSelectorPtr()->getActiveMode().modePtr->getPeakFps();

    constexpr Fps FPS_THRESHOLD_FOR_KERNEL_TIMER = 65_Hz;
    using namespace fps_approx_ops;

    if (state == TimerState::Reset && refreshRate > FPS_THRESHOLD_FOR_KERNEL_TIMER) {
        // If we're not in performance mode then the kernel timer shouldn't do
        // anything, as the refresh rate during DPU power collapse will be the
        // same.
        resyncAllToHardwareVsync(true /* allowToEnable */);
    } else if (state == TimerState::Expired && refreshRate <= FPS_THRESHOLD_FOR_KERNEL_TIMER) {
        // Disable HW VSYNC if the timer expired, as we don't need it enabled if
        // we're not pushing frames, and if we're in PERFORMANCE mode then we'll
        // need to update the VsyncController model anyway.
        std::scoped_lock lock(mDisplayLock);
        ftl::FakeGuard guard(kMainThreadContext);
        for (const auto& [_, display] : mDisplays) {
            constexpr bool kDisallow = false;
            display.schedulePtr->disableHardwareVsync(kDisallow);
        }
    }

    mSchedulerCallback.kernelTimerChanged(state == TimerState::Expired);
}

void Scheduler::idleTimerCallback(TimerState state) {
    applyPolicy(&Policy::idleTimer, state);
    SFTRACE_INT("ExpiredIdleTimer", static_cast<int>(state));
}

void Scheduler::touchTimerCallback(TimerState state) {
    const TouchState touch = state == TimerState::Reset ? TouchState::Active : TouchState::Inactive;
    // Touch event will boost the refresh rate to performance.
    // Clear layer history to get fresh FPS detection.
    // NOTE: Instead of checking all the layers, we should be checking the layer
    // that is currently on top. b/142507166 will give us this capability.
    if (applyPolicy(&Policy::touch, touch).touch) {
        mLayerHistory.clear();
    }
    SFTRACE_INT("TouchState", static_cast<int>(touch));
}

void Scheduler::displayPowerTimerCallback(TimerState state) {
    applyPolicy(&Policy::displayPowerTimer, state);
    SFTRACE_INT("ExpiredDisplayPowerTimer", static_cast<int>(state));
}

void Scheduler::dump(utils::Dumper& dumper) const {
    using namespace std::string_view_literals;

    {
        utils::Dumper::Section section(dumper, "Features"sv);

        for (Feature feature : ftl::enum_range<Feature>()) {
            if (const auto flagOpt = ftl::flag_name(feature)) {
                dumper.dump(flagOpt->substr(1), mFeatures.test(feature));
            }
        }
    }
    {
        utils::Dumper::Section section(dumper, "Policy"sv);
        {
            std::scoped_lock lock(mDisplayLock);
            ftl::FakeGuard guard(kMainThreadContext);
            dumper.dump("pacesetterDisplayId"sv, mPacesetterDisplayId);
        }
        dumper.dump("layerHistory"sv, mLayerHistory.dump());
        dumper.dump("touchTimer"sv, mTouchTimer.transform(&OneShotTimer::interval));
        dumper.dump("displayPowerTimer"sv, mDisplayPowerTimer.transform(&OneShotTimer::interval));
    }

    mFrameRateOverrideMappings.dump(dumper);
    dumper.eol();

    mVsyncConfiguration->dump(dumper.out());
    dumper.eol();

    mRefreshRateStats->dump(dumper.out());
    dumper.eol();

    std::scoped_lock lock(mDisplayLock);
    ftl::FakeGuard guard(kMainThreadContext);

    for (const auto& [id, display] : mDisplays) {
        utils::Dumper::Section
                section(dumper,
                        id == mPacesetterDisplayId
                                ? ftl::Concat("Pacesetter Display ", id.value).c_str()
                                : ftl::Concat("Follower Display ", id.value).c_str());

        display.selectorPtr->dump(dumper);
        display.targeterPtr->dump(dumper);
        dumper.eol();
    }
}

void Scheduler::dumpVsync(std::string& out) const {
    std::scoped_lock lock(mDisplayLock);
    ftl::FakeGuard guard(kMainThreadContext);
    if (mPacesetterDisplayId) {
        base::StringAppendF(&out, "VsyncSchedule for pacesetter %s:\n",
                            to_string(*mPacesetterDisplayId).c_str());
        getVsyncScheduleLocked()->dump(out);
    }
    for (auto& [id, display] : mDisplays) {
        if (id == mPacesetterDisplayId) {
            continue;
        }
        base::StringAppendF(&out, "VsyncSchedule for follower %s:\n", to_string(id).c_str());
        display.schedulePtr->dump(out);
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value" // b/369277774
void Scheduler::updateFrameRateOverrides(GlobalSignals consideredSignals, Fps displayRefreshRate) {
    const bool changed = (std::scoped_lock(mPolicyLock),
                          updateFrameRateOverridesLocked(consideredSignals, displayRefreshRate));

    if (changed) {
        onFrameRateOverridesChanged();
    }
}
#pragma clang diagnostic pop

bool Scheduler::updateFrameRateOverridesLocked(GlobalSignals consideredSignals,
                                               Fps displayRefreshRate) {
    if (consideredSignals.idle) return false;

    const auto frameRateOverrides =
            pacesetterSelectorPtr()->getFrameRateOverrides(mPolicy.contentRequirements,
                                                           displayRefreshRate, consideredSignals);

    // Note that RefreshRateSelector::supportsFrameRateOverrideByContent is checked when querying
    // the FrameRateOverrideMappings rather than here.
    return mFrameRateOverrideMappings.updateFrameRateOverridesByContent(frameRateOverrides);
}

void Scheduler::addBufferStuffedUids(BufferStuffingMap bufferStuffedUids) {
    if (!mRenderEventThread) return;
    mRenderEventThread->addBufferStuffedUids(std::move(bufferStuffedUids));
}

void Scheduler::promotePacesetterDisplay(PhysicalDisplayId pacesetterId, PromotionParams params) {
    std::shared_ptr<VsyncSchedule> pacesetterVsyncSchedule;
    {
        std::scoped_lock lock(mDisplayLock);
        pacesetterVsyncSchedule = promotePacesetterDisplayLocked(pacesetterId, params);
    }

    applyNewVsyncSchedule(std::move(pacesetterVsyncSchedule));
}

std::shared_ptr<VsyncSchedule> Scheduler::promotePacesetterDisplayLocked(
        PhysicalDisplayId pacesetterId, PromotionParams params) {
    // TODO: b/241286431 - Choose the pacesetter among mDisplays.
    mPacesetterDisplayId = pacesetterId;
    ALOGI("Display %s is the pacesetter", to_string(pacesetterId).c_str());

    std::shared_ptr<VsyncSchedule> newVsyncSchedulePtr;
    if (const auto pacesetterOpt = pacesetterDisplayLocked()) {
        const Display& pacesetter = *pacesetterOpt;

        if (!FlagManager::getInstance().connected_display() || params.toggleIdleTimer) {
            pacesetter.selectorPtr->setIdleTimerCallbacks(
                    {.platform = {.onReset = [this] { idleTimerCallback(TimerState::Reset); },
                                  .onExpired = [this] { idleTimerCallback(TimerState::Expired); }},
                     .kernel = {.onReset = [this] { kernelIdleTimerCallback(TimerState::Reset); },
                                .onExpired =
                                        [this] { kernelIdleTimerCallback(TimerState::Expired); }},
                     .vrr = {.onReset = [this] { mSchedulerCallback.vrrDisplayIdle(false); },
                             .onExpired = [this] { mSchedulerCallback.vrrDisplayIdle(true); }}});

            pacesetter.selectorPtr->startIdleTimer();
        }

        newVsyncSchedulePtr = pacesetter.schedulePtr;

        constexpr bool kForce = true;
        newVsyncSchedulePtr->onDisplayModeChanged(pacesetter.selectorPtr->getActiveMode().modePtr,
                                                  kForce);
    }
    return newVsyncSchedulePtr;
}

void Scheduler::applyNewVsyncSchedule(std::shared_ptr<VsyncSchedule> vsyncSchedule) {
    onNewVsyncSchedule(vsyncSchedule->getDispatch());

    if (hasEventThreads()) {
        eventThreadFor(Cycle::Render).onNewVsyncSchedule(vsyncSchedule);
        eventThreadFor(Cycle::LastComposite).onNewVsyncSchedule(vsyncSchedule);
    }
}

void Scheduler::demotePacesetterDisplay(PromotionParams params) {
    if (!FlagManager::getInstance().connected_display() || params.toggleIdleTimer) {
        // No need to lock for reads on kMainThreadContext.
        if (const auto pacesetterPtr =
                    FTL_FAKE_GUARD(mDisplayLock, pacesetterSelectorPtrLocked())) {
            pacesetterPtr->stopIdleTimer();
            pacesetterPtr->clearIdleTimerCallbacks();
        }
    }

    // Clear state that depends on the pacesetter's RefreshRateSelector.
    std::scoped_lock lock(mPolicyLock);
    mPolicy = {};
}

void Scheduler::updateAttachedChoreographersFrameRate(
        const surfaceflinger::frontend::RequestedLayerState& layer, Fps fps) {
    std::scoped_lock lock(mChoreographerLock);

    const auto layerId = static_cast<int32_t>(layer.id);
    const auto choreographers = mAttachedChoreographers.find(layerId);
    if (choreographers == mAttachedChoreographers.end()) {
        return;
    }

    auto& layerChoreographers = choreographers->second;

    layerChoreographers.frameRate = fps;
    SFTRACE_FORMAT_INSTANT("%s: %s for %s", __func__, to_string(fps).c_str(), layer.name.c_str());
    ALOGV("%s: %s for %s", __func__, to_string(fps).c_str(), layer.name.c_str());

    auto it = layerChoreographers.connections.begin();
    while (it != layerChoreographers.connections.end()) {
        sp<EventThreadConnection> choreographerConnection = it->promote();
        if (choreographerConnection) {
            choreographerConnection->frameRate = fps;
            it++;
        } else {
            it = choreographers->second.connections.erase(it);
        }
    }

    if (layerChoreographers.connections.empty()) {
        mAttachedChoreographers.erase(choreographers);
    }
}

int Scheduler::updateAttachedChoreographersInternal(
        const surfaceflinger::frontend::LayerHierarchy& layerHierarchy, Fps displayRefreshRate,
        int parentDivisor) {
    const char* name = layerHierarchy.getLayer() ? layerHierarchy.getLayer()->name.c_str() : "Root";

    int divisor = 0;
    if (layerHierarchy.getLayer()) {
        const auto frameRateCompatibility = layerHierarchy.getLayer()->frameRateCompatibility;
        const auto frameRate = Fps::fromValue(layerHierarchy.getLayer()->frameRate);
        ALOGV("%s: %s frameRate %s parentDivisor=%d", __func__, name, to_string(frameRate).c_str(),
              parentDivisor);

        if (frameRate.isValid()) {
            if (frameRateCompatibility == ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_FIXED_SOURCE ||
                frameRateCompatibility == ANATIVEWINDOW_FRAME_RATE_EXACT) {
                // Since this layer wants an exact match, we would only set a frame rate if the
                // desired rate is a divisor of the display refresh rate.
                divisor = RefreshRateSelector::getFrameRateDivisor(displayRefreshRate, frameRate);
            } else if (frameRateCompatibility == ANATIVEWINDOW_FRAME_RATE_COMPATIBILITY_DEFAULT) {
                // find the closest frame rate divisor for the desired frame rate.
                divisor = static_cast<int>(
                        std::round(displayRefreshRate.getValue() / frameRate.getValue()));
            }
        }
    }

    // We start by traversing the children, updating their choreographers, and getting back the
    // aggregated frame rate.
    int childrenDivisor = 0;
    for (const auto& [child, _] : layerHierarchy.mChildren) {
        LOG_ALWAYS_FATAL_IF(child == nullptr || child->getLayer() == nullptr);

        ALOGV("%s: %s traversing child %s", __func__, name, child->getLayer()->name.c_str());

        const int childDivisor =
                updateAttachedChoreographersInternal(*child, displayRefreshRate, divisor);
        childrenDivisor = childrenDivisor > 0 ? childrenDivisor : childDivisor;
        if (childDivisor > 0) {
            childrenDivisor = std::gcd(childrenDivisor, childDivisor);
        }
        ALOGV("%s: %s childrenDivisor=%d", __func__, name, childrenDivisor);
    }

    ALOGV("%s: %s divisor=%d", __func__, name, divisor);

    // If there is no explicit vote for this layer. Use the children's vote if exists
    divisor = (divisor == 0) ? childrenDivisor : divisor;
    ALOGV("%s: %s divisor=%d with children", __func__, name, divisor);

    // If there is no explicit vote for this layer or its children, Use the parent vote if exists
    divisor = (divisor == 0) ? parentDivisor : divisor;
    ALOGV("%s: %s divisor=%d with parent", __func__, name, divisor);

    if (layerHierarchy.getLayer()) {
        Fps fps = divisor > 1 ? displayRefreshRate / (unsigned int)divisor : Fps();
        updateAttachedChoreographersFrameRate(*layerHierarchy.getLayer(), fps);
    }

    return divisor;
}

void Scheduler::updateAttachedChoreographers(
        const surfaceflinger::frontend::LayerHierarchy& layerHierarchy, Fps displayRefreshRate) {
    SFTRACE_CALL();
    updateAttachedChoreographersInternal(layerHierarchy, displayRefreshRate, 0);
}

template <typename S, typename T>
auto Scheduler::applyPolicy(S Policy::*statePtr, T&& newState) -> GlobalSignals {
    SFTRACE_CALL();
    std::vector<display::DisplayModeRequest> modeRequests;
    GlobalSignals consideredSignals;

    bool refreshRateChanged = false;
    bool frameRateOverridesChanged;

    {
        std::scoped_lock lock(mPolicyLock);

        auto& currentState = mPolicy.*statePtr;
        if (currentState == newState) return {};
        currentState = std::forward<T>(newState);

        DisplayModeChoiceMap modeChoices;
        ftl::Optional<FrameRateMode> modeOpt;
        {
            std::scoped_lock lock(mDisplayLock);
            ftl::FakeGuard guard(kMainThreadContext);

            modeChoices = chooseDisplayModes();

            // TODO(b/240743786): The pacesetter display's mode must change for any
            // DisplayModeRequest to go through. Fix this by tracking per-display Scheduler::Policy
            // and timers.
            std::tie(modeOpt, consideredSignals) =
                    modeChoices.get(*mPacesetterDisplayId)
                            .transform([](const DisplayModeChoice& choice) {
                                return std::make_pair(choice.mode, choice.consideredSignals);
                            })
                            .value();
        }

        modeRequests.reserve(modeChoices.size());
        for (auto& [id, choice] : modeChoices) {
            modeRequests.emplace_back(
                    display::DisplayModeRequest{.mode = std::move(choice.mode),
                                                .emitEvent = choice.consideredSignals
                                                                     .shouldEmitEvent()});
        }

        if (!FlagManager::getInstance().vrr_bugfix_dropped_frame()) {
            frameRateOverridesChanged =
                    updateFrameRateOverridesLocked(consideredSignals, modeOpt->fps);
        }
        if (mPolicy.modeOpt != modeOpt) {
            mPolicy.modeOpt = modeOpt;
            refreshRateChanged = true;
        } else if (consideredSignals.shouldEmitEvent()) {
            // The mode did not change, but we may need to emit if DisplayModeRequest::emitEvent was
            // previously false.
            emitModeChangeIfNeeded();
        }
    }
    if (refreshRateChanged) {
        mSchedulerCallback.requestDisplayModes(std::move(modeRequests));
    }

    if (FlagManager::getInstance().vrr_bugfix_dropped_frame()) {
        std::scoped_lock lock(mPolicyLock);
        frameRateOverridesChanged =
                updateFrameRateOverridesLocked(consideredSignals, mPolicy.modeOpt->fps);
    }
    if (frameRateOverridesChanged) {
        onFrameRateOverridesChanged();
    }
    return consideredSignals;
}

auto Scheduler::chooseDisplayModes() const -> DisplayModeChoiceMap {
    SFTRACE_CALL();

    DisplayModeChoiceMap modeChoices;
    const auto globalSignals = makeGlobalSignals();

    const Fps pacesetterFps = [&]() REQUIRES(mPolicyLock, mDisplayLock, kMainThreadContext) {
        auto rankedFrameRates =
                pacesetterSelectorPtrLocked()->getRankedFrameRates(mPolicy.contentRequirements,
                                                                   globalSignals);

        const Fps pacesetterFps = rankedFrameRates.ranking.front().frameRateMode.fps;

        modeChoices.try_emplace(*mPacesetterDisplayId,
                                DisplayModeChoice::from(std::move(rankedFrameRates)));
        return pacesetterFps;
    }();

    // Choose a mode for powered-on follower displays.
    for (const auto& [id, display] : mDisplays) {
        if (id == *mPacesetterDisplayId) continue;
        if (display.powerMode != hal::PowerMode::ON) continue;

        auto rankedFrameRates =
                display.selectorPtr->getRankedFrameRates(mPolicy.contentRequirements, globalSignals,
                                                         pacesetterFps);

        modeChoices.try_emplace(id, DisplayModeChoice::from(std::move(rankedFrameRates)));
    }

    return modeChoices;
}

GlobalSignals Scheduler::makeGlobalSignals() const {
    const bool powerOnImminent = mDisplayPowerTimer &&
            (mPolicy.displayPowerMode != hal::PowerMode::ON ||
             mPolicy.displayPowerTimer == TimerState::Reset);

    return {.touch = mTouchTimer && mPolicy.touch == TouchState::Active,
            .idle = mPolicy.idleTimer == TimerState::Expired,
            .powerOnImminent = powerOnImminent};
}

FrameRateMode Scheduler::getPreferredDisplayMode() {
    std::lock_guard<std::mutex> lock(mPolicyLock);
    const auto frameRateMode =
            pacesetterSelectorPtr()
                    ->getRankedFrameRates(mPolicy.contentRequirements, makeGlobalSignals())
                    .ranking.front()
                    .frameRateMode;

    // Make sure the stored mode is up to date.
    mPolicy.modeOpt = frameRateMode;

    return frameRateMode;
}

void Scheduler::onNewVsyncPeriodChangeTimeline(const hal::VsyncPeriodChangeTimeline& timeline) {
    std::lock_guard<std::mutex> lock(mVsyncTimelineLock);
    mLastVsyncPeriodChangeTimeline = std::make_optional(timeline);

    const auto maxAppliedTime = systemTime() + MAX_VSYNC_APPLIED_TIME.count();
    if (timeline.newVsyncAppliedTimeNanos > maxAppliedTime) {
        mLastVsyncPeriodChangeTimeline->newVsyncAppliedTimeNanos = maxAppliedTime;
    }
}

bool Scheduler::onCompositionPresented(nsecs_t presentTime) {
    std::lock_guard<std::mutex> lock(mVsyncTimelineLock);
    if (mLastVsyncPeriodChangeTimeline && mLastVsyncPeriodChangeTimeline->refreshRequired) {
        if (presentTime < mLastVsyncPeriodChangeTimeline->refreshTimeNanos) {
            // We need to composite again as refreshTimeNanos is still in the future.
            return true;
        }

        mLastVsyncPeriodChangeTimeline->refreshRequired = false;
    }
    return false;
}

void Scheduler::onActiveDisplayAreaChanged(uint32_t displayArea) {
    mLayerHistory.setDisplayArea(displayArea);
}

void Scheduler::setGameModeFrameRateForUid(FrameRateOverride frameRateOverride) {
    if (frameRateOverride.frameRateHz > 0.f && frameRateOverride.frameRateHz < 1.f) {
        return;
    }

    if (FlagManager::getInstance().game_default_frame_rate()) {
        // update the frame rate override mapping in LayerHistory
        mLayerHistory.updateGameModeFrameRateOverride(frameRateOverride);
    } else {
        mFrameRateOverrideMappings.setGameModeRefreshRateForUid(frameRateOverride);
    }

    onFrameRateOverridesChanged();
}

void Scheduler::setGameDefaultFrameRateForUid(FrameRateOverride frameRateOverride) {
    if (!FlagManager::getInstance().game_default_frame_rate() ||
        (frameRateOverride.frameRateHz > 0.f && frameRateOverride.frameRateHz < 1.f)) {
        return;
    }

    // update the frame rate override mapping in LayerHistory
    mLayerHistory.updateGameDefaultFrameRateOverride(frameRateOverride);
}

void Scheduler::setPreferredRefreshRateForUid(FrameRateOverride frameRateOverride) {
    if (frameRateOverride.frameRateHz > 0.f && frameRateOverride.frameRateHz < 1.f) {
        return;
    }

    mFrameRateOverrideMappings.setPreferredRefreshRateForUid(frameRateOverride);
    onFrameRateOverridesChanged();
}

void Scheduler::updateSmallAreaDetection(
        std::vector<std::pair<int32_t, float>>& uidThresholdMappings) {
    mSmallAreaDetectionAllowMappings.update(uidThresholdMappings);
}

void Scheduler::setSmallAreaDetectionThreshold(int32_t appId, float threshold) {
    mSmallAreaDetectionAllowMappings.setThresholdForAppId(appId, threshold);
}

bool Scheduler::isSmallDirtyArea(int32_t appId, uint32_t dirtyArea) {
    std::optional<float> oThreshold = mSmallAreaDetectionAllowMappings.getThresholdForAppId(appId);
    if (oThreshold) {
        return mLayerHistory.isSmallDirtyArea(dirtyArea, oThreshold.value());
    }
    return false;
}

} // namespace android::scheduler
