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
#include "test_framework/hwc3/FakeComposer.h"
#include "test_framework/hwc3/Hwc3Controller.h"
#include "test_framework/hwc3/ObservingComposer.h"
#include "test_framework/hwc3/events/BufferPendingDisplay.h"
#include "test_framework/hwc3/events/BufferPendingRelease.h"
#include "test_framework/hwc3/events/ClientDestroyed.h"
#include "test_framework/hwc3/events/DisplayPresented.h"
#include "test_framework/hwc3/events/PowerMode.h"
#include "test_framework/hwc3/events/VSync.h"
#include "test_framework/hwc3/events/VSyncEnabled.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

struct Hwc3Controller::Passkey final {};

auto Hwc3Controller::make(std::span<const core::DisplayConfiguration> displays)
        -> base::expected<std::shared_ptr<hwc3::Hwc3Controller>, std::string> {
    using namespace std::string_literals;

    auto controller = std::make_shared<Hwc3Controller>(Passkey{});
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

    auto fakeComposerResult = FakeComposer::make();
    if (!fakeComposerResult) {
        return base::unexpected(std::move(fakeComposerResult).error());
    }
    auto fakeComposer = *std::move(fakeComposerResult);

    for (const auto& display : displays) {
        fakeComposer->addDisplay(display);
    }

    auto observingComposerResult =
            ObservingComposer::make(shared_from_this(), fakeComposer->getComposer());
    if (!observingComposerResult) {
        return base::unexpected(std::move(observingComposerResult).error());
    }
    auto observingComposer = *std::move(observingComposerResult);

    const auto qualifiedServiceName = ObservingComposer::getServiceName(baseServiceName);
    auto binder = observingComposer->getComposer()->asBinder();

    // This downgrade allows us to use the fake service name without it being defined in the
    // VINTF manifest.
    AIBinder_forceDowngradeToLocalStability(binder.get());

    auto status = AServiceManager_addService(binder.get(), qualifiedServiceName.c_str());
    if (status != STATUS_OK) {
        return base::unexpected(fmt::format("Failed to register service {}. Error {}.",
                                            qualifiedServiceName, status));
    }
    LOG(INFO) << "Registered service " << qualifiedServiceName << ". Error: " << status;

    mFakeComposer = std::move(fakeComposer);
    mObservingComposer = std::move(observingComposer);
    return {};
}

auto Hwc3Controller::editCallbacks() -> Callbacks& {
    return mCallbacks;
}

auto Hwc3Controller::callbacks() const -> const Callbacks& {
    return mCallbacks;
}

auto Hwc3Controller::getServiceName() -> std::string {
    return FakeComposer::getServiceName(baseServiceName);
}

void Hwc3Controller::addDisplay(const core::DisplayConfiguration& config) {
    CHECK(mFakeComposer);
    mFakeComposer->addDisplay(config);
}

void Hwc3Controller::removeDisplay(core::DisplayConfiguration::Id displayId) {
    CHECK(mFakeComposer);
    mFakeComposer->removeDisplay(displayId);
}

void Hwc3Controller::onClientDestroyed(const events::ClientDestroyed& event) const {
    mCallbacks.onClientDestroyed(event);
}

void Hwc3Controller::onPowerModeChanged(const events::PowerMode& event) const {
    mCallbacks.onPowerModeChanged(event);
}

void Hwc3Controller::onVsyncEnabledChanged(const events::VSyncEnabled& event) const {
    mCallbacks.onVsyncEnabledChanged(event);
}

void Hwc3Controller::onDisplayPresented(const events::DisplayPresented& event) const {
    mCallbacks.onDisplayPresented(event);
}

void Hwc3Controller::onBufferPendingDisplay(const events::BufferPendingDisplay& event) const {
    mCallbacks.onBufferPendingDisplay(event);
}
void Hwc3Controller::onBufferPendingRelease(const events::BufferPendingRelease& event) const {
    mCallbacks.onBufferPendingRelease(event);
}

void Hwc3Controller::onVSyncCallbackSent(const events::VSync& event) const {
    mCallbacks.onVSyncCallbackSent(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
