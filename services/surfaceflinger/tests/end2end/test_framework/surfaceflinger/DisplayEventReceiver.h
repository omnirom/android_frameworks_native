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

#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include <sys/types.h>

#include <android-base/expected.h>
#include <android/gui/ISurfaceComposer.h>
#include <binder/IBinder.h>
#include <ftl/finalizer.h>
#include <ftl/flags.h>
#include <ftl/function.h>
#include <gui/DisplayEventReceiver.h>
#include <gui/VsyncEventData.h>
#include <ui/DisplayId.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/surfaceflinger/PollFdThread.h"
#include "test_framework/surfaceflinger/events/Hotplug.h"
#include "test_framework/surfaceflinger/events/VSyncTiming.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class SFController;
class PollFdThread;

class DisplayEventReceiver final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    // The callbacks provided from a surface instance.
    struct Callbacks final {
        using Timestamp = std::chrono::steady_clock::time_point;

        // The vsync timing display event from SurfaceFlinger (slightly simplified).
        events::VSyncTiming::AsyncConnector onVSyncTiming;

        // The hotplug display event from SurfaceFlinger (slightly simplified).
        events::Hotplug::AsyncConnector onHotplug;
    };

    using PhysicalIdToTestDisplayId = ftl::Function<
            auto(PhysicalDisplayId)->std::optional<test_framework::core::DisplayConfiguration::Id>>;

    [[nodiscard]] static auto make(
            std::weak_ptr<SFController> controller, const sp<gui::ISurfaceComposer>& client,
            PollFdThread& pollFdThread, gui::ISurfaceComposer::VsyncSource source,
            const sp<IBinder>& layerHandle,
            const ftl::Flags<gui::ISurfaceComposer::EventRegistration>& events)
            -> base::expected<std::shared_ptr<DisplayEventReceiver>, std::string>;

    explicit DisplayEventReceiver(Passkey pass);

    // Allows the callbacks to be set by the caller, by modifying the values in the returned
    // structure.
    [[nodiscard]] auto editCallbacks() -> Callbacks&;

    // Sets the vsync rate for events.
    void setVsyncRate(int32_t count) const;

    // Requests a vsync callback.
    void requestNextVsync() const;

    // Obtains the latest vsync data immediately.
    [[nodiscard]] auto getLatestVsyncEventData() const -> std::optional<ParcelableVsyncEventData>;

  private:
    using Event = android::DisplayEventReceiver::Event;
    using EventType = android::DisplayEventType;
    using DisplayId = PhysicalDisplayId;
    using Timestamp = std::chrono::steady_clock::time_point;
    using VsyncEventData = android::VsyncEventData;

    [[nodiscard]] auto init(std::weak_ptr<SFController> controller,
                            const sp<gui::ISurfaceComposer>& client, PollFdThread& pollFdThread,
                            gui::ISurfaceComposer::VsyncSource source,
                            const sp<IBinder>& layerHandle,
                            const ftl::Flags<gui::ISurfaceComposer::EventRegistration>& events)
            -> base::expected<void, std::string>;

    void processReceivedEvents();
    void processReceivedEvents(std::span<Event> events);
    auto mapPhysicalDisplayIdToTestDisplayId(PhysicalDisplayId displayId)
            -> std::optional<core::DisplayConfiguration::Id>;
    void onVsync(DisplayId displayId, Timestamp timestamp, uint32_t count,
                 VsyncEventData vsyncData);
    void onHotplug(DisplayId displayId, Timestamp timestamp, bool connected,
                   int32_t connectionError);
    static void onHdcpLevelsChange(DisplayId displayId, Timestamp timestamp, int32_t connectedLevel,
                                   int32_t maxLevel);

    std::weak_ptr<SFController> mController;
    Callbacks mCallbacks;

    sp<gui::IDisplayEventConnection> mDisplayEventConnection;
    std::unique_ptr<gui::BitTube> mDataChannel;
    PollFdThread* mPollFdThread = nullptr;

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl mCleanup;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
