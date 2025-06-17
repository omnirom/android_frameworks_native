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
#include <string>
#include <string_view>

#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <android-base/expected.h>

#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3 {

class Hwc3Composer final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

    class Hwc3ComposerImpl;  // An internal class implements the AIDL interface.

  public:
    using Hwc3IComposer = aidl::android::hardware::graphics::composer3::IComposer;

    // Gets the full qualified service name given a base name for the service.
    [[nodiscard]] static auto getServiceName(std::string_view baseServiceName) -> std::string;

    // Constructs a Hwc3Composer instance.
    [[nodiscard]] static auto make() -> base::expected<std::shared_ptr<Hwc3Composer>, std::string>;

    explicit Hwc3Composer(Passkey passkey);

    // Obtains the AIDL composer3::IComposer interface for the internal instance.
    [[nodiscard]] auto getComposer() -> std::shared_ptr<Hwc3IComposer>;

    // Adds a display to the composer. This will sent a hotplug connect event.
    void addDisplay(const core::DisplayConfiguration& display);

    // Removes a display from the composer. This will sent a hotplug disconnect event.
    void removeDisplay(core::DisplayConfiguration::Id displayId);

  private:
    [[nodiscard]] auto init() -> base::expected<void, std::string>;

    std::shared_ptr<Hwc3ComposerImpl> mImpl;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3
