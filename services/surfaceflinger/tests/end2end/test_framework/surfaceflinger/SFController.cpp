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

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android/gui/ISurfaceComposer.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Status.h>
#include <ftl/finalizer.h>
#include <ftl/flags.h>
#include <ftl/ignore.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayId.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/surfaceflinger/DisplayEventReceiver.h"
#include "test_framework/surfaceflinger/PollFdThread.h"
#include "test_framework/surfaceflinger/SFController.h"
#include "test_framework/surfaceflinger/Surface.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

namespace {

auto waitForSurfaceFlingerAIDL() -> sp<gui::ISurfaceComposer> {
    constexpr auto kTimeout = std::chrono::seconds(30);
    constexpr auto kSurfaceFlingerServiceName = "SurfaceFlingerAIDL";
    const sp<android::IServiceManager> serviceManager(android::defaultServiceManager());
    const auto kTimeoutAfter = std::chrono::steady_clock::now() + kTimeout;

    LOG(INFO) << "Waiting " << kTimeout << " for service manager registration....";
    sp<android::IBinder> flingerService;
    while (flingerService == nullptr) {
        if (std::chrono::steady_clock::now() > kTimeoutAfter) {
            LOG(INFO) << "... Timeout!";
            return nullptr;
        }

        constexpr auto sleepTime = std::chrono::milliseconds(10);
        std::this_thread::sleep_for(sleepTime);
        flingerService = serviceManager->checkService(String16(kSurfaceFlingerServiceName));
    }
    LOG(INFO) << "Obtained surfaceflinger interface from service manager.";

    return interface_cast<gui::ISurfaceComposer>(flingerService);
}

}  // namespace

struct SFController::Passkey final {};

void SFController::useHwcService(std::string_view fqn) {
    base::SetProperty("debug.sf.hwc_service_name", std::string(fqn));
}

auto SFController::make() -> base::expected<std::shared_ptr<SFController>, std::string> {
    using namespace std::string_literals;

    auto controller = std::make_unique<SFController>(Passkey{});
    if (controller == nullptr) {
        return base::unexpected("Failed to construct the SFController instance."s);
    }

    if (auto result = controller->init(); !result) {
        return base::unexpected("Failed to init the SFController instance: "s + result.error());
    }

    return controller;
}

SFController::SFController(Passkey passkey) {
    ftl::ignore(passkey);
}

auto SFController::init() -> base::expected<void, std::string> {
    using namespace std::string_literals;

    LOG(INFO) << "Stopping everything to prepare for tests";
    // NOLINTBEGIN(cert-env33-c, concurrency-mt-unsafe)
    system("stop");
    // NOLINTEND(cert-env33-c, concurrency-mt-unsafe)

    auto pollFdThreadResult = PollFdThread::make();
    if (!pollFdThreadResult) {
        return base::unexpected(std::move(pollFdThreadResult).error());
    }
    mPollFdThread = *std::move(pollFdThreadResult);

    mCleanup = ftl::Finalizer([this]() { stop(); });

    return {};
}

auto SFController::startAndConnect() -> base::expected<void, std::string> {
    using namespace std::string_literals;

    start();

    LOG(VERBOSE) << "Getting ISurfaceComposer....";
    auto surfaceComposerAidl = waitForSurfaceFlingerAIDL();
    if (surfaceComposerAidl == nullptr) {
        return base::unexpected("Failed to obtain the surfaceComposerAidl interface."s);
    }
    LOG(VERBOSE) << "Getting ISurfaceComposerClient....";
    sp<gui::ISurfaceComposerClient> surfaceComposerClientAidl;
    if (!surfaceComposerAidl->createConnection(&surfaceComposerClientAidl).isOk()) {
        return base::unexpected("Failed to obtain the surfaceComposerClientAidl interface."s);
    }
    if (surfaceComposerClientAidl == nullptr) {
        return base::unexpected("Failed to obtain a valid surfaceComposerClientAidl interface."s);
    }
    auto surfaceComposerClient = sp<SurfaceComposerClient>::make(surfaceComposerClientAidl);
    if (surfaceComposerClient == nullptr) {
        return base::unexpected(
                "Failed to construct a surfaceComposerClient around the aidl interface."s);
    }

    // We need an internal display event receiver so we can listen for hotplug events.
    auto displayEventReceiverResult = DisplayEventReceiver::make(
            shared_from_this(), surfaceComposerAidl, *mPollFdThread,
            gui::ISurfaceComposer::VsyncSource::eVsyncSourceApp, nullptr, {});
    if (!displayEventReceiverResult) {
        return base::unexpected(std::move(displayEventReceiverResult).error());
    }
    auto displayEventReceiver = *std::move(displayEventReceiverResult);

    mSurfaceComposerAidl = std::move(surfaceComposerAidl);
    mSurfaceComposerClientAidl = std::move(surfaceComposerClientAidl);
    mSurfaceComposerClient = std::move(surfaceComposerClient);
    mDisplayEventReceiver = std::move(displayEventReceiver);

    initializeDisplayIdMapping();

    LOG(INFO) << "Connected to surfaceflinger";
    return {};
}

void SFController::start() {
    LOG(INFO) << "Starting surfaceflinger";
    // NOLINTBEGIN(cert-env33-c, concurrency-mt-unsafe)
    system("start surfaceflinger");
    // NOLINTEND(cert-env33-c, concurrency-mt-unsafe)
}

void SFController::stop() {
    LOG(INFO) << "Stopping surfaceflinger";
    // NOLINTBEGIN(cert-env33-c, concurrency-mt-unsafe)
    system("stop surfaceflinger");
    // NOLINTEND(cert-env33-c, concurrency-mt-unsafe)

    if (mSurfaceComposerAidl != nullptr) {
        LOG(INFO) << "Waiting for SF AIDL interface to die";

        constexpr auto kTimeout = std::chrono::seconds(30);
        const auto binder = android::gui::ISurfaceComposer::asBinder(mSurfaceComposerAidl);
        const auto kTimeoutAfter = std::chrono::steady_clock::now() + kTimeout;

        while (binder->isBinderAlive()) {
            if (std::chrono::steady_clock::now() > kTimeoutAfter) {
                LOG(INFO) << "... Timeout!";
                break;
            }

            ftl::ignore = binder->pingBinder();

            constexpr auto kPollInterval = std::chrono::milliseconds(10);
            std::this_thread::sleep_for(kPollInterval);
        }

        constexpr auto kShutdownWait = std::chrono::milliseconds(500);
        std::this_thread::sleep_for(kShutdownWait);
    }

    mDisplayEventReceiver = nullptr;
    mSurfaceComposerClient = nullptr;
    mSurfaceComposerClientAidl = nullptr;
    mSurfaceComposerAidl = nullptr;
}

auto SFController::makeDisplayEventReceiver()
        -> base::expected<std::shared_ptr<DisplayEventReceiver>, std::string> {
    CHECK(mSurfaceComposerAidl != nullptr);
    return DisplayEventReceiver::make(shared_from_this(), mSurfaceComposerAidl, *mPollFdThread,
                                      gui::ISurfaceComposer::VsyncSource::eVsyncSourceApp, nullptr,
                                      {gui::ISurfaceComposer::EventRegistration::frameRateOverride,
                                       gui::ISurfaceComposer::EventRegistration::modeChanged});
}

auto SFController::makeSurface(const Surface::CreationArgs& args)
        -> base::expected<std::shared_ptr<Surface>, std::string> {
    CHECK(mSurfaceComposerClient != nullptr);
    return Surface::make(mSurfaceComposerClient, args);
}

auto SFController::mapPhysicalDisplayIdToTestDisplayId(PhysicalDisplayId displayId)
        -> std::optional<core::DisplayConfiguration::Id> {
    const std::lock_guard lock(mMutex);
    auto iter = mPhysicalDisplayIdToTestDisplayId.find(displayId);
    return (iter != mPhysicalDisplayIdToTestDisplayId.end()) ? std::make_optional(iter->second)
                                                             : std::nullopt;
}

void SFController::addDisplayIdToMapping(PhysicalDisplayId displayId) {
    gui::StaticDisplayInfo staticInfo;
    CHECK(mSurfaceComposerAidl->getStaticDisplayInfo(displayId.value, &staticInfo).isOk());

    auto name = staticInfo.deviceProductInfo.and_then(
            [](const auto& info) -> std::optional<std::string> { return info.name; });

    constexpr std::string kMagicPrefix = "fake";
    if (!name || !name->starts_with(kMagicPrefix)) {
        LOG(ERROR) << "Unexpected: physical display id " << displayId
                   << " is not a test fake display: " << name.value_or("<missing>");
        return;
    }

    auto remainder = name->substr(kMagicPrefix.size());

    uint32_t testId = 0;
    if (!android::base::ParseUint(remainder.c_str(), &testId)) {
        LOG(ERROR) << "Failed to parse as int: " << remainder;
        return;
    }

    const std::lock_guard lock(mMutex);
    mPhysicalDisplayIdToTestDisplayId[displayId] = testId;
}

void SFController::onDisplayConnectionChanged(PhysicalDisplayId displayId, bool connected) {
    if (connected) {
        addDisplayIdToMapping(displayId);
    } else {
        const std::lock_guard lock(mMutex);
        mPhysicalDisplayIdToTestDisplayId.erase(displayId);
    }
}

void SFController::initializeDisplayIdMapping() {
    CHECK(mSurfaceComposerAidl);

    std::vector<int64_t> rawDisplayIds;
    CHECK(mSurfaceComposerAidl->getPhysicalDisplayIds(&rawDisplayIds).isOk());

    for (auto displayIdValue : rawDisplayIds) {
        addDisplayIdToMapping(PhysicalDisplayId::fromValue(displayIdValue));
    }
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
