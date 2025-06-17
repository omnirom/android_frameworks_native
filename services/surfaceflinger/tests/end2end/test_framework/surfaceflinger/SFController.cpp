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
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <android/gui/ISurfaceComposer.h>
#include <binder/IBinder.h>
#include <binder/IInterface.h>
#include <binder/IServiceManager.h>
#include <binder/Status.h>
#include <ftl/finalizer.h>
#include <ftl/ignore.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <utils/String16.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>

#include "test_framework/surfaceflinger/SFController.h"

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
    LOG(INFO) << "Stopping everything to prepare for tests";
    // NOLINTBEGIN(cert-env33-c)
    system("stop");
    // NOLINTEND(cert-env33-c)

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

    mSurfaceComposerAidl = std::move(surfaceComposerAidl);
    mSurfaceComposerClientAidl = std::move(surfaceComposerClientAidl);
    mSurfaceComposerClient = std::move(surfaceComposerClient);

    LOG(INFO) << "Connected to surfaceflinger";
    return {};
}

void SFController::start() {
    LOG(INFO) << "Starting surfaceflinger";
    // NOLINTBEGIN(cert-env33-c)
    system("start surfaceflinger");
    // NOLINTEND(cert-env33-c)
}

void SFController::stop() {
    LOG(INFO) << "Stopping surfaceflinger";
    // NOLINTBEGIN(cert-env33-c)
    system("stop surfaceflinger");
    // NOLINTEND(cert-env33-c)

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

    mSurfaceComposerClient = nullptr;
    mSurfaceComposerClientAidl = nullptr;
    mSurfaceComposerAidl = nullptr;
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
