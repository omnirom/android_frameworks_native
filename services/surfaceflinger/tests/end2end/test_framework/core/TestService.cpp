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
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <android-base/expected.h>
#include <ftl/ignore.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/core/TestService.h"
#include "test_framework/fake_hwc3/Hwc3Controller.h"
#include "test_framework/surfaceflinger/SFController.h"

namespace android::surfaceflinger::tests::end2end::test_framework::core {

struct TestService::Passkey final {};

auto TestService::startWithDisplays(const std::vector<DisplayConfiguration>& displays)
        -> base::expected<std::unique_ptr<TestService>, std::string> {
    using namespace std::string_literals;

    auto service = std::make_unique<TestService>(TestService::Passkey{});
    if (service == nullptr) {
        return base::unexpected("Failed to construct the TestService instance."s);
    }

    if (auto result = service->init(displays); !result) {
        return base::unexpected("Failed to init the TestService instance: "s + result.error());
    }

    return service;
}

TestService::TestService(Passkey passkey) {
    ftl::ignore(passkey);
}

auto TestService::init(std::span<const DisplayConfiguration> displays)
        -> base::expected<void, std::string> {
    using namespace std::string_literals;

    auto hwcResult = fake_hwc3::Hwc3Controller::make(displays);
    if (!hwcResult) {
        return base::unexpected(std::move(hwcResult).error());
    }
    auto hwc = *std::move(hwcResult);

    auto flingerResult = surfaceflinger::SFController::make();
    if (!flingerResult) {
        return base::unexpected(std::move(flingerResult).error());
    }
    auto flinger = *std::move(flingerResult);

    surfaceflinger::SFController::useHwcService(fake_hwc3::Hwc3Controller::getServiceName());

    if (auto result = flinger->startAndConnect(); !result) {
        return base::unexpected(std::move(result).error());
    }

    mHwc = std::move(hwc);
    mFlinger = std::move(flinger);
    return {};
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
