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

#include <memory>
#include <span>
#include <string>

#include <android-base/expected.h>

#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3 {

class Hwc3Composer;

class Hwc3Controller final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    // Gets the service name for the HWC3 instance that will be created and registered
    [[nodiscard]] static auto getServiceName() -> std::string;

    // Makes the HWC3 controller instance.
    [[nodiscard]] static auto make(std::span<const core::DisplayConfiguration> displays)
            -> base::expected<std::shared_ptr<fake_hwc3::Hwc3Controller>, std::string>;

    explicit Hwc3Controller(Passkey passkey);

    // Adds a new display to the HWC3, which will become a hotplug connect event.
    void addDisplay(const core::DisplayConfiguration& config);

    // Removes a new display from the HWC3, which will become a hotplug disconnect event.
    void removeDisplay(core::DisplayConfiguration::Id displayId);

  private:
    static constexpr std::string baseServiceName = "fake";

    [[nodiscard]] auto init(std::span<const core::DisplayConfiguration> displays)
            -> base::expected<void, std::string>;

    std::shared_ptr<Hwc3Composer> mComposer;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3
