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
#include <vector>

#include <android-base/expected.h>
#include <android-base/logging.h>

#include "test_framework/core/DisplayConfiguration.h"

namespace android::surfaceflinger::tests::end2end::test_framework {

namespace surfaceflinger {

class SFController;

}  // namespace surfaceflinger

namespace fake_hwc3 {

class Hwc3Controller;

}  // namespace fake_hwc3

namespace core {

class TestService final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    // Constructs the test service, and starts it with the given displays as connected at boot.
    [[nodiscard]] static auto startWithDisplays(const std::vector<DisplayConfiguration>& displays)
            -> base::expected<std::unique_ptr<TestService>, std::string>;

    explicit TestService(Passkey passkey);

    // Obtains the HWC3 back-end controller
    [[nodiscard]] auto hwc() -> fake_hwc3::Hwc3Controller& {
        CHECK(mHwc);
        return *mHwc;
    }

    // Obtains the SurfaceFlinger front-end controller
    [[nodiscard]] auto flinger() -> surfaceflinger::SFController& {
        CHECK(mFlinger);
        return *mFlinger;
    }

  private:
    [[nodiscard]] auto init(std::span<const DisplayConfiguration> displays)
            -> base::expected<void, std::string>;

    std::shared_ptr<fake_hwc3::Hwc3Controller> mHwc;
    std::shared_ptr<surfaceflinger::SFController> mFlinger;
};

}  // namespace core
}  // namespace android::surfaceflinger::tests::end2end::test_framework
