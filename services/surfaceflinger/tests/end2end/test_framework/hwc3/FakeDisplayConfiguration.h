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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <aidl/android/hardware/graphics/common/Transform.h>
#include <aidl/android/hardware/graphics/composer3/ColorMode.h>
#include <aidl/android/hardware/graphics/composer3/ContentType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConfiguration.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConnectionType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayIdentification.h>
#include <aidl/android/hardware/graphics/composer3/HdrCapabilities.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataKey.h>
#include <aidl/android/hardware/graphics/composer3/RenderIntent.h>
#include <android-base/logging.h>
#include <ftl/ignore.h>
#include <ui/Size.h>

#include "test_framework/core/EdidBuilder.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

// An interface for tests to control the fake HWC implementation.
struct FakeDisplayConfiguration final {
    using Hwc3ColorMode = aidl::android::hardware::graphics::composer3::ColorMode;
    using Hwc3ContentType = aidl::android::hardware::graphics::composer3::ContentType;
    using Hwc3DisplayCapability = aidl::android::hardware::graphics::composer3::DisplayCapability;
    using Hwc3DisplayConfiguration =
            aidl::android::hardware::graphics::composer3::DisplayConfiguration;
    using Hwc3DisplayConnectionType =
            aidl::android::hardware::graphics::composer3::DisplayConnectionType;
    using Hwc3DisplayIdentification =
            aidl::android::hardware::graphics::composer3::DisplayIdentification;
    using Hwc3HdrCapabilities = aidl::android::hardware::graphics::composer3::HdrCapabilities;
    using Hwc3PerFrameMetadataKey =
            aidl::android::hardware::graphics::composer3::PerFrameMetadataKey;
    using Hwc3RenderIntent = aidl::android::hardware::graphics::composer3::RenderIntent;
    using Hwc3Transform = aidl::android::hardware::graphics::common::Transform;
    using Hwc3VrrConfig = aidl::android::hardware::graphics::composer3::VrrConfig;

    using Hwc3DisplayCapabilities = std::vector<Hwc3DisplayCapability>;
    using Hwc3DisplayConfigurations = std::vector<Hwc3DisplayConfiguration>;
    using Hwc3PerFrameMetadataKeys = std::vector<Hwc3PerFrameMetadataKey>;
    using Hwc3ContentTypes = std::vector<Hwc3ContentType>;

    struct Builder final {
        // NOLINTBEGIN(readability-convert-member-functions-to-static)
        // clang-tidy incorrectly shows this error for C++23 "deducing this" methods.

        [[nodiscard]] auto activeConfigId(this Builder&& self,
                                          int32_t activeConfigId) -> Builder&& {
            self.mActiveConfig = activeConfigId;
            return std::move(self);
        }

        [[nodiscard]] auto setCapabilities(this Builder&& self,
                                           Hwc3DisplayCapabilities& values) -> Builder&& {
            self.mCapabilities = std::move(values);
            return std::move(self);
        }

        [[nodiscard]] auto setConnectionType(this Builder&& self,
                                             Hwc3DisplayConnectionType value) -> Builder&& {
            self.mConnectionType = value;
            return std::move(self);
        }

        [[nodiscard]] auto addDisplayConfig(this Builder&& self, int32_t configId,
                                            ui::Size physicalPixels,
                                            int32_t refreshRateHz) -> Builder&& {
            constexpr auto kDefaultDpi = 160;
            constexpr std::optional<Hwc3VrrConfig> kDefaultVrrConfig = {};
            constexpr auto kDefaultConfigGroup = 0;

            const auto vsyncPeriod =
                    (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) /
                     refreshRateHz);

            self.mDisplayConfigs.push_back(Hwc3DisplayConfiguration{
                    .configId = configId,
                    .width = physicalPixels.width,
                    .height = physicalPixels.height,
                    .dpi = Hwc3DisplayConfiguration::Dpi{.x = kDefaultDpi, .y = kDefaultDpi},
                    .configGroup = kDefaultConfigGroup,
                    .vsyncPeriod = static_cast<int32_t>(vsyncPeriod.count()),
                    .vrrConfig = kDefaultVrrConfig,
            });

            return std::move(self);
        }

        [[nodiscard]] auto setHdrCapabilities(this Builder&& self,
                                              Hwc3HdrCapabilities& value) -> Builder&& {
            self.mHdrCapabilities = std::move(value);
            return std::move(self);
        }

        [[nodiscard]] auto setIdentification(this Builder&& self,
                                             Hwc3DisplayIdentification value) -> Builder&& {
            self.mIdentification = std::move(value);
            return std::move(self);
        }

        [[nodiscard]] auto setPerFrameMetadataKeys(this Builder&& self,
                                                   Hwc3PerFrameMetadataKeys& value) -> Builder&& {
            self.mPerFrameMetadataKeys = std::move(value);
            return std::move(self);
        }

        [[nodiscard]] auto addColorModeAndRenderIntents(
                this Builder&& self, Hwc3ColorMode colorMode,
                std::vector<Hwc3RenderIntent> renderIntents) -> Builder&& {
            self.mRenderIntentsForColorMode.emplace(colorMode, std::move(renderIntents));
            return std::move(self);
        }

        [[nodiscard]] auto setPerFrameMetadataKeys(this Builder&& self,
                                                   Hwc3Transform value) -> Builder&& {
            self.mTransform = value;
            return std::move(self);
        }

        [[nodiscard]] auto setSupportedContentTypes(this Builder&& self,
                                                    Hwc3ContentTypes values) -> Builder&& {
            self.mSupportedContentTypes = std::move(values);
            return std::move(self);
        }

        [[nodiscard]] auto operator()(this Builder&& self) -> FakeDisplayConfiguration {
            auto initCapabilities =
                    !self.mCapabilities.empty() ? self.mCapabilities : getDefaultCapabilities();

            auto initDisplayConfigs = getDefaultDisplayConfigs();
            CHECK(!initDisplayConfigs.empty());
            const auto initActiveConfigId =
                    self.mActiveConfig.value_or(initDisplayConfigs[0].configId);

            const bool activeConfigIsAConfig = std::ranges::any_of(
                    initDisplayConfigs, [initActiveConfigId](const auto& config) {
                        return initActiveConfigId == config.configId;
                    });
            CHECK(activeConfigIsAConfig);

            auto initHdrCapabilities = self.mHdrCapabilities.value_or(getDefaultHdrCapabilities());

            auto initIdentification =
                    self.mIdentification.value_or(getDefaultDisplayIdentification());

            auto initPerFrameMetadataKeys = self.mPerFrameMetadataKeys.empty()
                                                    ? self.mPerFrameMetadataKeys
                                                    : getDefaultPerFrameMetadataKeys();

            auto initRenderIntentMap = !self.mRenderIntentsForColorMode.empty()
                                               ? self.mRenderIntentsForColorMode
                                               : getDefaultRenderIntentMap();

            auto initSupportedContentTypes = !self.mSupportedContentTypes.empty()
                                                     ? self.mSupportedContentTypes
                                                     : getDefaultSupportedContentTypes();

            auto result = FakeDisplayConfiguration{
                    .activeConfig = initActiveConfigId,
                    .capabilities = std::move(initCapabilities),
                    .connectionType = self.mConnectionType,
                    .displayConfigs = std::move(initDisplayConfigs),
                    .hdrCapabilities = std::move(initHdrCapabilities),
                    .identification = std::move(initIdentification),
                    .perFrameMetadataKeys = std::move(initPerFrameMetadataKeys),
                    .renderIntentsForColorMode = std::move(initRenderIntentMap),
                    .transform = self.mTransform,
                    .supportedContentTypes = std::move(initSupportedContentTypes)};
            ftl::ignore(std::move(self));
            return result;
        }

        // NOLINTEND(readability-convert-member-functions-to-static)

      private:
        [[nodiscard]] static constexpr auto getDefaultCapabilities()
                -> std::vector<Hwc3DisplayCapability> {
            return {Hwc3DisplayCapability::PROTECTED_CONTENTS,
                    Hwc3DisplayCapability::MULTI_THREADED_PRESENT};
        }

        [[nodiscard]] static auto getDefaultDisplayConfigs()
                -> std::vector<Hwc3DisplayConfiguration> {
            constexpr auto kDefaultWidth = 3840;
            constexpr auto kDefaultHeight = 2160;
            constexpr auto kDefaultDpi = 160;
            constexpr auto kDefaultHz = 60;
            constexpr auto kDefaultPeriod =
                    (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(1)) /
                     kDefaultHz);
            constexpr std::optional<Hwc3VrrConfig> kDefaultVrrConfig = {};

            return {
                    Hwc3DisplayConfiguration{
                            .configId = 0,
                            .width = kDefaultWidth,
                            .height = kDefaultHeight,
                            .dpi = Hwc3DisplayConfiguration::Dpi{.x = kDefaultDpi,
                                                                 .y = kDefaultDpi},
                            .configGroup = 0,
                            .vsyncPeriod = kDefaultPeriod.count(),
                            .vrrConfig = kDefaultVrrConfig,
                    },
            };
        }

        [[nodiscard]] static constexpr auto getDefaultDisplayIdentification()
                -> Hwc3DisplayIdentification {
            constexpr auto displayPort = 0;
            const auto& displayEdid = core::EdidBuilder::kDefaultEdid;
            return {
                    .port = displayPort,
                    .data = {displayEdid.begin(), displayEdid.end()},
            };
        }

        [[nodiscard]] static constexpr auto getDefaultHdrCapabilities() -> Hwc3HdrCapabilities {
            constexpr auto defaultMaxLuminance = 0.0F;
            constexpr auto defaultMaxAverageLuminance = 0.0F;
            constexpr auto defaultMinLuminance = 0.0F;

            return {
                    .types = {},
                    .maxLuminance = defaultMaxLuminance,
                    .maxAverageLuminance = defaultMaxAverageLuminance,
                    .minLuminance = defaultMinLuminance,
            };
        }

        [[nodiscard]] static constexpr auto getDefaultPerFrameMetadataKeys()
                -> std::vector<Hwc3PerFrameMetadataKey> {
            return {};
        }

        [[nodiscard]] static auto getDefaultRenderIntentMap()
                -> std::unordered_map<Hwc3ColorMode, std::vector<Hwc3RenderIntent>> {
            return {{Hwc3ColorMode::NATIVE, {Hwc3RenderIntent::ENHANCE}}};
        }

        [[nodiscard]] static auto getDefaultSupportedContentTypes()
                -> std::vector<Hwc3ContentType> {
            return {Hwc3ContentType::GRAPHICS, Hwc3ContentType::PHOTO, Hwc3ContentType::CINEMA,
                    Hwc3ContentType::GAME};
        }

        std::optional<int32_t> mActiveConfig;
        Hwc3DisplayCapabilities mCapabilities;
        Hwc3DisplayConnectionType mConnectionType = Hwc3DisplayConnectionType::INTERNAL;
        Hwc3DisplayConfigurations mDisplayConfigs;
        std::optional<Hwc3HdrCapabilities> mHdrCapabilities;
        std::optional<Hwc3DisplayIdentification> mIdentification;
        Hwc3PerFrameMetadataKeys mPerFrameMetadataKeys;
        std::unordered_map<Hwc3ColorMode, std::vector<Hwc3RenderIntent>> mRenderIntentsForColorMode;
        Hwc3Transform mTransform = Hwc3Transform::NONE;
        Hwc3ContentTypes mSupportedContentTypes;
    };

    int32_t activeConfig = 0;
    Hwc3DisplayCapabilities capabilities;
    Hwc3DisplayConnectionType connectionType = Hwc3DisplayConnectionType::INTERNAL;
    Hwc3DisplayConfigurations displayConfigs;
    Hwc3HdrCapabilities hdrCapabilities;
    Hwc3DisplayIdentification identification;
    Hwc3PerFrameMetadataKeys perFrameMetadataKeys;
    std::unordered_map<Hwc3ColorMode, std::vector<Hwc3RenderIntent>> renderIntentsForColorMode;
    Hwc3Transform transform = Hwc3Transform::NONE;
    Hwc3ContentTypes supportedContentTypes;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
