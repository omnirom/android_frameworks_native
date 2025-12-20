/*
 * Copyright 2025 The Android Open Source Project
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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <ftl/ignore.h>

#include "test_framework/core/GuardedSharedState.h"
#include "test_framework/hwc3/DisplayVSyncEventService.h"
#include "test_framework/hwc3/MultiDisplayRefreshEventGenerator.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/TimeKeeperThread.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

struct DisplayVSyncEventService::Passkey final {};

auto DisplayVSyncEventService::make()
        -> base::expected<std::shared_ptr<DisplayVSyncEventService>, std::string> {
    using namespace std::string_literals;

    auto service = std::make_unique<DisplayVSyncEventService>(Passkey{});
    if (service == nullptr) {
        return base::unexpected("Failed to construct the DisplayVSyncEventService"s);
    }
    if (auto result = service->init(); !result) {
        return base::unexpected("Failed to init the DisplayVSyncEventService: "s + result.error());
    }
    return service;
}

DisplayVSyncEventService::DisplayVSyncEventService(Passkey passkey) {
    ftl::ignore(passkey);
}

auto DisplayVSyncEventService::init() -> base::expected<void, std::string> {
    auto timekeeperMake = TimeKeeperThread::make();
    if (!timekeeperMake) {
        return base::unexpected(std::move(timekeeperMake.error()));
    }

    mTimeKeeper = *std::move(timekeeperMake);
    mTimeKeeper->editCallbacks().onWakeup.set(
            [this](TimeInterval sleeping) -> TimePoint { return onWakeup(sleeping); })();
    return {};
}

auto DisplayVSyncEventService::editCallbacks() -> Callbacks& {
    return mCallbacks;
}

void DisplayVSyncEventService::addDisplay(DisplayId displayId,
                                          SingleDisplayRefreshSchedule schedule) {
    mGenerator.withExclusiveLock([displayId, schedule](auto& generator) {
        LOG(VERBOSE) << "addDisplay: " << displayId << " schedule: " << toString(schedule);
        generator.addDisplay(displayId, schedule);
    });

    mTimeKeeper->wakeNow();
}

void DisplayVSyncEventService::removeDisplay(DisplayId displayId) {
    mGenerator.withExclusiveLock([displayId](auto& generator) {
        LOG(VERBOSE) << "remove " << displayId;
        generator.removeDisplay(displayId);
    });
}

auto DisplayVSyncEventService::onWakeup(TimeInterval elapsed) -> TimePoint {
    // Obtain all the events to emit.
    auto result = mGenerator.withExclusiveLock(
            [elapsed](auto& generator) { return generator.generateEventsFor(elapsed); });

    // Emit them via the connector interface.
    LOG(VERBOSE) << "emitting " << result.events.size() << " events";
    for (const auto& event : result.events) {
        LOG(VERBOSE) << "displayId: " << event.displayId
                     << " timestamp: " << event.expectedAt.time_since_epoch()
                     << " period: " << event.expectedPeriod;
        mCallbacks.onVSync(event);
    }

    LOG(VERBOSE) << "next wakeup " << result.nextRefreshAt.time_since_epoch();
    return result.nextRefreshAt;
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
