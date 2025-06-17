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

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <aidl/android/hardware/graphics/composer3/BnComposer.h>
#include <aidl/android/hardware/graphics/composer3/Capability.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/PowerMode.h>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <android/binder_status.h>
#include <ftl/ignore.h>

#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/fake_hwc3/Hwc3Composer.h"

namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3 {

class Hwc3Composer::Hwc3ComposerImpl final
    : public aidl::android::hardware::graphics::composer3::BnComposer {
    using Capability = aidl::android::hardware::graphics::composer3::Capability;
    using IComposerClient = aidl::android::hardware::graphics::composer3::IComposerClient;
    using Hwc3PowerMode = aidl::android::hardware::graphics::composer3::PowerMode;

    // begin IComposer overrides

    auto dump(int dumpFd, const char** args, uint32_t num_args) -> binder_status_t override {
        UNIMPLEMENTED(WARNING);
        ftl::ignore(dumpFd, args, num_args);
        return static_cast<binder_status_t>(STATUS_NO_MEMORY);
    }

    auto createClient(std::shared_ptr<IComposerClient>* out_client) -> ndk::ScopedAStatus override {
        UNIMPLEMENTED(WARNING);
        ftl::ignore(out_client);
        return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                IComposer::EX_NO_RESOURCES, "Client failed to initialize");
    }

    auto getCapabilities(std::vector<Capability>* out_capabilities) -> ndk::ScopedAStatus override {
        UNIMPLEMENTED(WARNING);
        ftl::ignore(out_capabilities);
        return ndk::ScopedAStatus::ok();
    }

    // end IComposer overrides
};

struct Hwc3Composer::Passkey final {};

auto Hwc3Composer::getServiceName(std::string_view baseServiceName) -> std::string {
    return Hwc3ComposerImpl::makeServiceName(baseServiceName);
}

auto Hwc3Composer::make() -> base::expected<std::shared_ptr<Hwc3Composer>, std::string> {
    using namespace std::string_literals;

    auto composer = std::make_shared<Hwc3Composer>(Passkey{});
    if (composer == nullptr) {
        return base::unexpected("Failed to construct the Hwc3Composer instance."s);
    }

    if (auto result = composer->init(); !result) {
        return base::unexpected("Failed to init the Hwc3Composer instance: "s + result.error());
    }

    return composer;
}

Hwc3Composer::Hwc3Composer(Hwc3Composer::Passkey passkey) {
    ftl::ignore(passkey);
}

auto Hwc3Composer::init() -> base::expected<void, std::string> {
    using namespace std::string_literals;

    auto impl = ndk::SharedRefBase::make<Hwc3ComposerImpl>();
    if (!impl) {
        return base::unexpected("Failed to construct the Hwc3ComposerImpl instance."s);
    }

    mImpl = std::move(impl);

    return {};
}

auto Hwc3Composer::getComposer() -> std::shared_ptr<Hwc3IComposer> {
    return mImpl;
}

void Hwc3Composer::addDisplay(const core::DisplayConfiguration& display) {
    UNIMPLEMENTED(WARNING);
    ftl::ignore(display, mImpl);
}

void Hwc3Composer::removeDisplay(core::DisplayConfiguration::Id displayId) {
    UNIMPLEMENTED(WARNING);
    ftl::ignore(displayId, mImpl);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::fake_hwc3
