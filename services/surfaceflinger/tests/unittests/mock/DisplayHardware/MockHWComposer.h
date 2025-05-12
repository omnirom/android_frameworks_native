/*
 * Copyright 2024 The Android Open Source Project
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

#include <gmock/gmock.h>

#include "DisplayHardware/HWComposer.h"

namespace android::mock {

class HWComposer : public android::HWComposer {
public:
    using HWDisplayId = android::hardware::graphics::composer::hal::HWDisplayId;
    using PowerMode = android::hardware::graphics::composer::hal::PowerMode;

    HWComposer();
    ~HWComposer() override;

    MOCK_METHOD(void, setCallback, (HWC2::ComposerCallback&), (override));
    MOCK_METHOD(bool, getDisplayIdentificationData,
                (HWDisplayId, uint8_t*, DisplayIdentificationData*), (const, override));
    MOCK_METHOD(bool, hasCapability, (aidl::android::hardware::graphics::composer3::Capability),
                (const, override));
    MOCK_METHOD(bool, hasDisplayCapability,
                (HalDisplayId, aidl::android::hardware::graphics::composer3::DisplayCapability),
                (const, override));

    MOCK_METHOD(size_t, getMaxVirtualDisplayCount, (), (const, override));
    MOCK_METHOD(size_t, getMaxVirtualDisplayDimension, (), (const, override));
    MOCK_METHOD(bool, allocateVirtualDisplay, (HalVirtualDisplayId, ui::Size, ui::PixelFormat*),
                (override));
    MOCK_METHOD(void, allocatePhysicalDisplay,
                (hal::HWDisplayId, PhysicalDisplayId, std::optional<ui::Size>), (override));

    MOCK_METHOD(std::shared_ptr<HWC2::Layer>, createLayer, (HalDisplayId), (override));
    MOCK_METHOD(status_t, getDeviceCompositionChanges,
                (HalDisplayId, bool, std::optional<std::chrono::steady_clock::time_point>, nsecs_t,
                 Fps, std::optional<android::HWComposer::DeviceRequestedChanges>*));
    MOCK_METHOD(status_t, setClientTarget,
                (HalDisplayId, uint32_t, const sp<Fence>&, const sp<GraphicBuffer>&, ui::Dataspace,
                 float),
                (override));
    MOCK_METHOD(status_t, presentAndGetReleaseFences,
                (HalDisplayId, std::optional<std::chrono::steady_clock::time_point>), (override));
    MOCK_METHOD(status_t, executeCommands, (HalDisplayId));
    MOCK_METHOD(status_t, setPowerMode, (PhysicalDisplayId, PowerMode), (override));
    MOCK_METHOD(status_t, setColorTransform, (HalDisplayId, const mat4&), (override));
    MOCK_METHOD(void, disconnectDisplay, (HalDisplayId), (override));
    MOCK_METHOD(sp<Fence>, getPresentFence, (HalDisplayId), (const, override));
    MOCK_METHOD(nsecs_t, getPresentTimestamp, (PhysicalDisplayId), (const, override));
    MOCK_METHOD(sp<Fence>, getLayerReleaseFence, (HalDisplayId, HWC2::Layer*), (const, override));
    MOCK_METHOD(status_t, setOutputBuffer,
                (HalVirtualDisplayId, const sp<Fence>&, const sp<GraphicBuffer>&), (override));
    MOCK_METHOD(void, clearReleaseFences, (HalDisplayId), (override));
    MOCK_METHOD(status_t, getHdrCapabilities, (HalDisplayId, HdrCapabilities*), (override));
    MOCK_METHOD(int32_t, getSupportedPerFrameMetadata, (HalDisplayId), (const, override));
    MOCK_METHOD(std::vector<ui::RenderIntent>, getRenderIntents, (HalDisplayId, ui::ColorMode),
                (const, override));
    MOCK_METHOD(mat4, getDataspaceSaturationMatrix, (HalDisplayId, ui::Dataspace), (override));
    MOCK_METHOD(status_t, getDisplayedContentSamplingAttributes,
                (HalDisplayId, ui::PixelFormat*, ui::Dataspace*, uint8_t*), (override));
    MOCK_METHOD(status_t, setDisplayContentSamplingEnabled, (HalDisplayId, bool, uint8_t, uint64_t),
                (override));
    MOCK_METHOD(status_t, getDisplayedContentSample,
                (HalDisplayId, uint64_t, uint64_t, DisplayedFrameStats*), (override));
    MOCK_METHOD(ftl::Future<status_t>, setDisplayBrightness,
                (PhysicalDisplayId, float, float, const Hwc2::Composer::DisplayBrightnessOptions&),
                (override));
    MOCK_METHOD(std::optional<DisplayIdentificationInfo>, onHotplug,
                (hal::HWDisplayId, hal::Connection), (override));
    MOCK_METHOD(bool, updatesDeviceProductInfoOnHotplugReconnect, (), (const, override));
    MOCK_METHOD(std::optional<PhysicalDisplayId>, onVsync, (hal::HWDisplayId, int64_t));
    MOCK_METHOD(void, setVsyncEnabled, (PhysicalDisplayId, hal::Vsync), (override));
    MOCK_METHOD(bool, isConnected, (PhysicalDisplayId), (const, override));
    MOCK_METHOD(std::vector<HWComposer::HWCDisplayMode>, getModes, (PhysicalDisplayId, int32_t),
                (const, override));
    MOCK_METHOD((ftl::Expected<hal::HWConfigId, status_t>), getActiveMode, (PhysicalDisplayId),
                (const, override));
    MOCK_METHOD(std::vector<ui::ColorMode>, getColorModes, (PhysicalDisplayId), (const, override));
    MOCK_METHOD(status_t, setActiveColorMode, (PhysicalDisplayId, ui::ColorMode, ui::RenderIntent),
                (override));
    MOCK_METHOD(ui::DisplayConnectionType, getDisplayConnectionType, (PhysicalDisplayId),
                (const, override));
    MOCK_METHOD(bool, isVsyncPeriodSwitchSupported, (PhysicalDisplayId), (const, override));
    MOCK_METHOD((ftl::Expected<nsecs_t, status_t>), getDisplayVsyncPeriod, (PhysicalDisplayId),
                (const, override));
    MOCK_METHOD(status_t, setActiveModeWithConstraints,
                (PhysicalDisplayId, hal::HWConfigId, const hal::VsyncPeriodChangeConstraints&,
                 hal::VsyncPeriodChangeTimeline*),
                (override));
    MOCK_METHOD(status_t, setBootDisplayMode, (PhysicalDisplayId, hal::HWConfigId), (override));
    MOCK_METHOD(status_t, clearBootDisplayMode, (PhysicalDisplayId), (override));
    MOCK_METHOD(std::optional<hal::HWConfigId>, getPreferredBootDisplayMode, (PhysicalDisplayId),
                (override));

    MOCK_METHOD(std::vector<aidl::android::hardware::graphics::common::HdrConversionCapability>,
                getHdrConversionCapabilities, (), (const, override));
    MOCK_METHOD(status_t, setHdrConversionStrategy,
                (aidl::android::hardware::graphics::common::HdrConversionStrategy,
                 aidl::android::hardware::graphics::common::Hdr*),
                (override));
    MOCK_METHOD(status_t, setAutoLowLatencyMode, (PhysicalDisplayId, bool), (override));
    MOCK_METHOD(status_t, getSupportedContentTypes,
                (PhysicalDisplayId, std::vector<hal::ContentType>*), (const, override));
    MOCK_METHOD(status_t, setContentType, (PhysicalDisplayId, hal::ContentType)), (override);
    MOCK_METHOD((const std::unordered_map<std::string, bool>&), getSupportedLayerGenericMetadata,
                (), (const, override));
    MOCK_METHOD(void, dump, (std::string&), (const, override));
    MOCK_METHOD(void, dumpOverlayProperties, (std::string&), (const, override));
    MOCK_METHOD(android::Hwc2::Composer*, getComposer, (), (const, override));

    MOCK_METHOD(hal::HWDisplayId, getPrimaryHwcDisplayId, (), (const, override));
    MOCK_METHOD(PhysicalDisplayId, getPrimaryDisplayId, (), (const, override));
    MOCK_METHOD(bool, isHeadless, (), (const, override));

    MOCK_METHOD(std::optional<PhysicalDisplayId>, toPhysicalDisplayId, (hal::HWDisplayId),
                (const, override));
    MOCK_METHOD(std::optional<hal::HWDisplayId>, fromPhysicalDisplayId, (PhysicalDisplayId),
                (const, override));
    MOCK_METHOD(status_t, getDisplayDecorationSupport,
                (PhysicalDisplayId,
                 std::optional<aidl::android::hardware::graphics::common::DisplayDecorationSupport>*
                         support),
                (override));
    MOCK_METHOD(status_t, setIdleTimerEnabled, (PhysicalDisplayId, std::chrono::milliseconds),
                (override));
    MOCK_METHOD(bool, hasDisplayIdleTimerCapability, (PhysicalDisplayId), (const, override));
    MOCK_METHOD(Hwc2::AidlTransform, getPhysicalDisplayOrientation, (PhysicalDisplayId),
                (const, override));
    MOCK_METHOD(bool, getValidateSkipped, (HalDisplayId), (const, override));
    MOCK_METHOD(const aidl::android::hardware::graphics::composer3::OverlayProperties&,
                getOverlaySupport, (), (const, override));
    MOCK_METHOD(status_t, setRefreshRateChangedCallbackDebugEnabled, (PhysicalDisplayId, bool));
    MOCK_METHOD(status_t, notifyExpectedPresent, (PhysicalDisplayId, TimePoint, Fps));
    MOCK_METHOD(HWC2::Display::LutFileDescriptorMapper&, getLutFileDescriptorMapper, (),
                (override));
    MOCK_METHOD(int32_t, getMaxLayerPictureProfiles, (PhysicalDisplayId));
    MOCK_METHOD(status_t, setDisplayPictureProfileHandle,
                (PhysicalDisplayId, const PictureProfileHandle&));
};

} // namespace android::mock
