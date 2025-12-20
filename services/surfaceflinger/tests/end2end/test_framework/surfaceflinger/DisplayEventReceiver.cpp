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

#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>

#include <sys/types.h>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android/gui/IDisplayEventConnection.h>
#include <android/gui/ISurfaceComposer.h>
#include <binder/IBinder.h>
#include <fmt/format.h>
#include <ftl/finalizer.h>
#include <ftl/flags.h>
#include <ftl/ignore.h>
#include <gui/DisplayEventReceiver.h>
#include <private/gui/BitTube.h>
#include <ui/DisplayId.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/surfaceflinger/DisplayEventReceiver.h"
#include "test_framework/surfaceflinger/PollFdThread.h"
#include "test_framework/surfaceflinger/SFController.h"
#include "test_framework/surfaceflinger/events/Hotplug.h"
#include "test_framework/surfaceflinger/events/VSyncTiming.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

struct DisplayEventReceiver::Passkey final {};

auto DisplayEventReceiver::make(std::weak_ptr<SFController> controller,
                                const sp<gui::ISurfaceComposer>& client, PollFdThread& pollFdThread,
                                gui::ISurfaceComposer::VsyncSource source,
                                const sp<IBinder>& layerHandle,
                                const ftl::Flags<gui::ISurfaceComposer::EventRegistration>& events)
        -> base::expected<std::shared_ptr<DisplayEventReceiver>, std::string> {
    using namespace std::string_literals;

    auto instance = std::make_unique<DisplayEventReceiver>(Passkey{});
    if (instance == nullptr) {
        return base::unexpected("Failed to construct a DisplayEventReceiver"s);
    }
    if (auto result = instance->init(std::move(controller), client, pollFdThread, source,
                                     layerHandle, events);
        !result) {
        return base::unexpected("Failed to init a DisplayEventReceiver: " + result.error());
    }
    return std::move(instance);
}

DisplayEventReceiver::DisplayEventReceiver(Passkey passkey) {
    ftl::ignore(passkey);
}

[[nodiscard]] auto DisplayEventReceiver::init(
        std::weak_ptr<SFController> controller, const sp<gui::ISurfaceComposer>& client,
        PollFdThread& pollFdThread,

        gui::ISurfaceComposer::VsyncSource source, const sp<IBinder>& layerHandle,
        const ftl::Flags<gui::ISurfaceComposer::EventRegistration>& events)
        -> base::expected<void, std::string> {
    using namespace std::string_literals;

    sp<gui::IDisplayEventConnection> displayEventConnection;
    auto result = client->createDisplayEventConnection(
            source, std::bit_cast<gui::ISurfaceComposer::EventRegistration>(events.get()),
            layerHandle, &displayEventConnection);

    if (!result.isOk() || displayEventConnection == nullptr) {
        return base::unexpected("failed to create the display event connection"s);
    }
    auto dataChannel = std::make_unique<gui::BitTube>();
    result = displayEventConnection->stealReceiveChannel(dataChannel.get());
    if (!result.isOk()) {
        return base::unexpected("failed to steal the receive channel"s);
    }

    mController = std::move(controller);
    mPollFdThread = &pollFdThread;
    mDisplayEventConnection = std::move(displayEventConnection);
    mDataChannel = std::move(dataChannel);
    pollFdThread.addFileDescriptor(mDataChannel->getFd(), {PollFdThread::PollFlags::IN},
                                   [this]() { processReceivedEvents(); });

    mCleanup = ftl::Finalizer(
            [this]() { mPollFdThread->removeFileDescriptor(mDataChannel->getFd()); });

    LOG(VERBOSE) << "initialized";
    return {};
}

auto DisplayEventReceiver::editCallbacks() -> Callbacks& {
    return mCallbacks;
}

void DisplayEventReceiver::setVsyncRate(int32_t count) const {
    LOG(VERBOSE) << __func__ << " count " << count;
    CHECK(count >= 0);
    mDisplayEventConnection->setVsyncRate(count);
}

void DisplayEventReceiver::requestNextVsync() const {
    LOG(VERBOSE) << __func__;
    mDisplayEventConnection->requestNextVsync();
}

[[nodiscard]] auto DisplayEventReceiver::getLatestVsyncEventData() const
        -> std::optional<ParcelableVsyncEventData> {
    LOG(VERBOSE) << __func__;
    ParcelableVsyncEventData data;
    auto status = mDisplayEventConnection->getLatestVsyncEventData(&data);
    if (!status.isOk()) {
        LOG(ERROR) << "Failed to get latest vsync event data: " << status.toString8();
        return {};
    }
    return data;
}

void DisplayEventReceiver::processReceivedEvents() {
    for (;;) {
        constexpr auto kMaxEventBatchSize = 10;
        std::array<Event, kMaxEventBatchSize> events{};
        const ssize_t eventCount =
                gui::BitTube::recvObjects(mDataChannel.get(), events.data(), events.size());
        LOG(VERBOSE) << __func__ << " received " << eventCount << " events";
        CHECK(eventCount >= 0);
        if (eventCount == 0) {
            return;
        }
        processReceivedEvents({events.data(), static_cast<size_t>(eventCount)});
    }
}

void DisplayEventReceiver::processReceivedEvents(std::span<Event> events) {
    LOG(VERBOSE) << __func__ << " " << events.size() << " events";
    for (const auto& event : events) {
        LOG(VERBOSE) << fmt::format("event {:#x}", fmt::underlying(event.header.type));

        auto timestamp = Timestamp(std::chrono::steady_clock::duration(event.header.timestamp));

        // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
        switch (event.header.type) {
            default:
                LOG(FATAL) << fmt::format("Unknown event type {:#x}",
                                          fmt::underlying(event.header.type));
                break;

            case EventType::DISPLAY_EVENT_VSYNC:
                onVsync(event.header.displayId, timestamp, event.vsync.count,
                        event.vsync.vsyncData);
                break;

            case EventType::DISPLAY_EVENT_HOTPLUG:
                onHotplug(event.header.displayId, timestamp, event.hotplug.connected,
                          event.hotplug.connectionError);
                break;

            case EventType::DISPLAY_EVENT_HDCP_LEVELS_CHANGE:
                onHdcpLevelsChange(event.header.displayId, timestamp,
                                   event.hdcpLevelsChange.connectedLevel,
                                   event.hdcpLevelsChange.maxLevel);
                break;
        }
        // NOLINTEND(cppcoreguidelines-pro-type-union-access)
    }
}

auto DisplayEventReceiver::mapPhysicalDisplayIdToTestDisplayId(PhysicalDisplayId displayId)
        -> std::optional<core::DisplayConfiguration::Id> {
    auto controller = mController.lock();
    if (!controller) {
        LOG(WARNING) << "Unable to map physical displayId " << displayId
                     << " to a test display id. SFController gone.";
        return {};
    }

    auto testDisplayId = controller->mapPhysicalDisplayIdToTestDisplayId(displayId);
    if (!testDisplayId) {
        LOG(WARNING) << "Unable to map physical displayId " << displayId << " to a test display id";
        return {};
    }

    return testDisplayId;
}

void DisplayEventReceiver::onVsync(DisplayId displayId, Timestamp timestamp, uint32_t count,
                                   VsyncEventData vsyncData) {
    ftl::ignore(displayId, timestamp, count, vsyncData);
    LOG(VERBOSE) << "onVsync() display " << displayId << " timestamp "
                 << timestamp.time_since_epoch() << " count " << count;

    auto testDisplayId = mapPhysicalDisplayIdToTestDisplayId(displayId);
    if (!testDisplayId) {
        return;
    }

    mCallbacks.onVSyncTiming(events::VSyncTiming{
            .receiver = this,
            .displayId = *testDisplayId,
            .sfEventAt = timestamp,
            .count = count,
            .data = vsyncData,
    });
}

void DisplayEventReceiver::onHotplug(DisplayId displayId, Timestamp timestamp, bool connected,
                                     int32_t connectionError) {
    ftl::ignore(displayId, timestamp, connected, connectionError);
    LOG(VERBOSE) << "onHotplug() display " << displayId << " timestamp "
                 << timestamp.time_since_epoch() << " connected " << connected
                 << " connectionError " << connectionError;

    if (auto controller = mController.lock()) {
        controller->onDisplayConnectionChanged(displayId, connected);
    }

    auto testDisplayId = mapPhysicalDisplayIdToTestDisplayId(displayId);
    if (!testDisplayId) {
        return;
    }

    if (connectionError == 0) {
        mCallbacks.onHotplug(events::Hotplug{
                .receiver = this,
                .displayId = *testDisplayId,
                .sfEventAt = timestamp,
                .connected = connected,
        });
    }
}

void DisplayEventReceiver::onHdcpLevelsChange(DisplayId displayId, Timestamp timestamp,
                                              int32_t connectedLevel, int32_t maxLevel) {
    ftl::ignore(displayId, timestamp, connectedLevel, maxLevel);
    LOG(VERBOSE) << "onHdcpLevelsChange() display " << displayId << " timestamp "
                 << timestamp.time_since_epoch() << " connectedLevel " << connectedLevel
                 << " maxLevel " << maxLevel;
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
