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

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_stability.h>
#include <android/binder_status.h>
#include <fmt/format.h>
#include <ftl/ignore.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/fake_hwc3/Hwc3Composer.h"
#include "test_framework/fake_hwc3/Hwc3Controller.h"

namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3 {

struct Hwc3Controller::Passkey final {};

auto Hwc3Controller::make(std::span<const core::DisplayConfiguration> displays)
        -> base::expected<std::shared_ptr<fake_hwc3::Hwc3Controller>, std::string> {
    using namespace std::string_literals;

    auto controller = std::make_unique<Hwc3Controller>(Passkey{});
    if (controller == nullptr) {
        return base::unexpected("Failed to construct the Hwc3Controller instance"s);
    }

    if (auto result = controller->init(displays); !result) {
        return base::unexpected("Failed to construct the Hwc3Controller instance: "s +
                                result.error());
    }

    return controller;
}

Hwc3Controller::Hwc3Controller(Passkey passkey) {
    ftl::ignore(passkey);
}

auto Hwc3Controller::init(const std::span<const core::DisplayConfiguration> displays)
        -> base::expected<void, std::string> {
    using namespace std::string_literals;

    auto qualifiedServiceName = Hwc3Composer::getServiceName(baseServiceName);

    auto composerResult = Hwc3Composer::make();
    if (!composerResult) {
        return base::unexpected(std::move(composerResult).error());
    }
    auto composer = *std::move(composerResult);

    for (const auto& display : displays) {
        composer->addDisplay(display);
    }

    auto binder = composer->getComposer()->asBinder();

    // This downgrade allows us to use the fake service name without it being defined in the
    // VINTF manifest.
    AIBinder_forceDowngradeToLocalStability(binder.get());

    auto status = AServiceManager_addService(binder.get(), qualifiedServiceName.c_str());
    if (status != STATUS_OK) {
        return base::unexpected(fmt::format("Failed to register service {}. Error {}.",
                                            qualifiedServiceName, status));
    }
    LOG(INFO) << "Registered service " << qualifiedServiceName << ". Error: " << status;

    mComposer = std::move(composer);
    return {};
}

auto Hwc3Controller::getServiceName() -> std::string {
    return Hwc3Composer::getServiceName(baseServiceName);
}

void Hwc3Controller::addDisplay(const core::DisplayConfiguration& config) {
    CHECK(mComposer);
    mComposer->addDisplay(config);
}

void Hwc3Controller::removeDisplay(core::DisplayConfiguration::Id displayId) {
    CHECK(mComposer);
    mComposer->removeDisplay(displayId);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3
