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
#include <string>

#include <fmt/chrono.h>  // NOLINT(misc-include-cleaner)
#include <fmt/format.h>
#include <gui/VsyncEventData.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class DisplayEventReceiver;

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger::events {

struct VSyncTiming final {
    using AsyncConnector = core::AsyncFunctionStd<void(VSyncTiming)>;

    using DisplayId = core::DisplayConfiguration::Id;
    using Timestamp = std::chrono::steady_clock::time_point;

    DisplayEventReceiver* receiver{};
    DisplayId displayId{};
    Timestamp sfEventAt;
    uint32_t count{};
    gui::VsyncEventData data{};
    Timestamp receivedAt{std::chrono::steady_clock::now()};
};

inline auto toString(const VSyncTiming& event) -> std::string {
    return fmt::format(
            "sfVSyncTiming{{"
            " receiver: {},"
            " displayId: {},"
            " sfEventAt: {}"
            " count: {}"
            " data: ..."
            " receivedAt: {}"
            " }}",
            static_cast<void*>(event.receiver), event.displayId, event.sfEventAt.time_since_epoch(),
            event.count, event.receivedAt.time_since_epoch());
}

inline auto format_as(const VSyncTiming& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger::events
