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

#include <android-base/expected.h>
#include <ftl/finalizer.h>
#include <utils/StrongPointer.h>

namespace android::gui {

class ISurfaceComposer;
class ISurfaceComposerClient;

}  // namespace android::gui

namespace android {

class SurfaceComposerClient;

}  // namespace android

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class SFController final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    // Sets a property so that SurfaceFlinger uses the named HWC service.
    static void useHwcService(std::string_view fqn);

    // Makes an instance of the SFController.
    [[nodiscard]] static auto make() -> base::expected<std::shared_ptr<SFController>, std::string>;

    explicit SFController(Passkey pass);

    // Starts SurfaceFlinger and establishes the AIDL interface connections.
    [[nodiscard]] auto startAndConnect() -> base::expected<void, std::string>;

  private:
    [[nodiscard]] auto init() -> base::expected<void, std::string>;
    static void start();
    void stop();

    sp<gui::ISurfaceComposer> mSurfaceComposerAidl;
    sp<gui::ISurfaceComposerClient> mSurfaceComposerClientAidl;
    sp<SurfaceComposerClient> mSurfaceComposerClient;

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl mCleanup;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
