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

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <aidl/android/hardware/graphics/common/Dataspace.h>

#include <aidl/android/hardware/graphics/common/Hdr.h>
#include <aidl/android/hardware/graphics/common/HdrConversionStrategy.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/Transform.h>
#include <aidl/android/hardware/graphics/composer3/BnComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/ColorMode.h>
#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>
#include <aidl/android/hardware/graphics/composer3/ContentType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayAttribute.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConnectionType.h>
#include <aidl/android/hardware/graphics/composer3/FormatColorComponent.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataKey.h>
#include <aidl/android/hardware/graphics/composer3/PowerMode.h>
#include <aidl/android/hardware/graphics/composer3/RenderIntent.h>
#include <android/binder_auto_utils.h>

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators {

// Implements the complete ComposerClient interface in such a way that the default behavior can be
// specified by defining a templated `operator()` in the derived class `Derived`.
//
// The derived class should override all interface functions where the default behavior isn't
// desired, or where an extension to the default is needed. This can lead to a leaner derived class
// especially when only a handful of interface functions need to be overridden.
//
// Example:
//
//   class MyDelegator : public hwc3::delegators::ComposerClient<MyDelegator> {
//      template <auto MemberFn, typename... Args>
//      auto operator()(Args&& args...) -> ::ndk::ScopedAStatus {
//        // A forwarding delegator would do something like:
//        // return (mImpl.get()->*MemberFn)(std::forward<decltype(args)>(args)...);
//
//        // A no-op stub might just return a default status, like:
//        return ::ndk::ScopedAStatus::ok();
//      }
//   };
//
template <class Delegator>
class ComposerClient : public ::aidl::android::hardware::graphics::composer3::BnComposerClient {
    friend Delegator;
    ComposerClient() = default;

  public:
    auto delegator() -> Delegator* { return static_cast<Delegator*>(this); }

    // clang-format off
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"

    auto createLayer(int64_t in_display, int32_t in_bufferSlotCount, int64_t* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::createLayer>(in_display, in_bufferSlotCount, _aidl_return);
    }
    auto createVirtualDisplay(int32_t in_width, int32_t in_height, ::aidl::android::hardware::graphics::common::PixelFormat in_formatHint, int32_t in_outputBufferSlotCount, ::aidl::android::hardware::graphics::composer3::VirtualDisplay* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::createVirtualDisplay>(in_width, in_height, in_formatHint, in_outputBufferSlotCount, _aidl_return);
    }
    auto destroyLayer(int64_t in_display, int64_t in_layer) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::destroyLayer>(in_display, in_layer);
    }
    auto destroyVirtualDisplay(int64_t in_display) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::destroyVirtualDisplay>(in_display);
    }
    auto executeCommands(const std::vector<::aidl::android::hardware::graphics::composer3::DisplayCommand>& in_commands, std::vector<::aidl::android::hardware::graphics::composer3::CommandResultPayload>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::executeCommands>(in_commands, _aidl_return);
    }
    auto getActiveConfig(int64_t in_display, int32_t* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getActiveConfig>(in_display, _aidl_return);
    }
    auto getColorModes(int64_t in_display, std::vector<::aidl::android::hardware::graphics::composer3::ColorMode>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getColorModes>(in_display, _aidl_return);
    }
    auto getDataspaceSaturationMatrix(::aidl::android::hardware::graphics::common::Dataspace in_dataspace, std::vector<float>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDataspaceSaturationMatrix>(in_dataspace, _aidl_return);
    }
    auto getDisplayAttribute(int64_t in_display, int32_t in_config, ::aidl::android::hardware::graphics::composer3::DisplayAttribute in_attribute, int32_t* _aidl_return) -> ::ndk::ScopedAStatus override __attribute__((deprecated("use getDisplayConfigurations instead. Returns a display attribute value for a particular display configuration. For legacy support getDisplayAttribute should return valid values for any requested DisplayAttribute, and for all of the configs obtained either through getDisplayConfigs or getDisplayConfigurations."))) {
        return delegator()->template operator()<&IComposerClient::getDisplayAttribute>(in_display, in_config, in_attribute, _aidl_return);
    }
    auto getDisplayCapabilities(int64_t in_display, std::vector<::aidl::android::hardware::graphics::composer3::DisplayCapability>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayCapabilities>(in_display, _aidl_return);
    }
    auto getDisplayConfigs(int64_t in_display, std::vector<int32_t>* _aidl_return) -> ::ndk::ScopedAStatus override __attribute__((deprecated("use getDisplayConfigurations instead. For legacy support getDisplayConfigs should return at least one valid config. All the configs returned from the getDisplayConfigs should also be returned from getDisplayConfigurations."))) {
        return delegator()->template operator()<&IComposerClient::getDisplayConfigs>(in_display, _aidl_return);
    }
    auto getDisplayConnectionType(int64_t in_display, ::aidl::android::hardware::graphics::composer3::DisplayConnectionType* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayConnectionType>(in_display, _aidl_return);
    }
    auto getDisplayIdentificationData(int64_t in_display, ::aidl::android::hardware::graphics::composer3::DisplayIdentification* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayIdentificationData>(in_display, _aidl_return);
    }
    auto getDisplayName(int64_t in_display, std::string* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayName>(in_display, _aidl_return);
    }
    auto getDisplayVsyncPeriod(int64_t in_display, int32_t* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayVsyncPeriod>(in_display, _aidl_return);
    }
    auto getDisplayedContentSample(int64_t in_display, int64_t in_maxFrames, int64_t in_timestamp, ::aidl::android::hardware::graphics::composer3::DisplayContentSample* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayedContentSample>(in_display, in_maxFrames, in_timestamp, _aidl_return);
    }
    auto getDisplayedContentSamplingAttributes(int64_t in_display, ::aidl::android::hardware::graphics::composer3::DisplayContentSamplingAttributes* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayedContentSamplingAttributes>(in_display, _aidl_return);
    }
    auto getDisplayPhysicalOrientation(int64_t in_display, ::aidl::android::hardware::graphics::common::Transform* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayPhysicalOrientation>(in_display, _aidl_return);
    }
    auto getHdrCapabilities(int64_t in_display, ::aidl::android::hardware::graphics::composer3::HdrCapabilities* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getHdrCapabilities>(in_display, _aidl_return);
    }
    auto getMaxVirtualDisplayCount(int32_t* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getMaxVirtualDisplayCount>(_aidl_return);
    }
    auto getPerFrameMetadataKeys(int64_t in_display, std::vector<::aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getPerFrameMetadataKeys>(in_display, _aidl_return);
    }
    auto getReadbackBufferAttributes(int64_t in_display, ::aidl::android::hardware::graphics::composer3::ReadbackBufferAttributes* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getReadbackBufferAttributes>(in_display, _aidl_return);
    }
    auto getReadbackBufferFence(int64_t in_display, ::ndk::ScopedFileDescriptor* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getReadbackBufferFence>(in_display, _aidl_return);
    }
    auto getRenderIntents(int64_t in_display, ::aidl::android::hardware::graphics::composer3::ColorMode in_mode, std::vector<::aidl::android::hardware::graphics::composer3::RenderIntent>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getRenderIntents>(in_display, in_mode, _aidl_return);
    }
    auto getSupportedContentTypes(int64_t in_display, std::vector<::aidl::android::hardware::graphics::composer3::ContentType>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getSupportedContentTypes>(in_display, _aidl_return);
    }
    auto getDisplayDecorationSupport(int64_t in_display, std::optional<::aidl::android::hardware::graphics::common::DisplayDecorationSupport>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayDecorationSupport>(in_display, _aidl_return);
    }
    auto registerCallback(const std::shared_ptr<::aidl::android::hardware::graphics::composer3::IComposerCallback>& in_callback) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::registerCallback>(in_callback);
    }
    auto setActiveConfig(int64_t in_display, int32_t in_config) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setActiveConfig>(in_display, in_config);
    }
    auto setActiveConfigWithConstraints(int64_t in_display, int32_t in_config, const ::aidl::android::hardware::graphics::composer3::VsyncPeriodChangeConstraints& in_vsyncPeriodChangeConstraints, ::aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setActiveConfigWithConstraints>(in_display, in_config, in_vsyncPeriodChangeConstraints, _aidl_return);
    }
    auto setBootDisplayConfig(int64_t in_display, int32_t in_config) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setBootDisplayConfig>(in_display, in_config);
    }
    auto clearBootDisplayConfig(int64_t in_display) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::clearBootDisplayConfig>(in_display);
    }
    auto getPreferredBootDisplayConfig(int64_t in_display, int32_t* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getPreferredBootDisplayConfig>(in_display, _aidl_return);
    }
    auto setAutoLowLatencyMode(int64_t in_display, bool in_on) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setAutoLowLatencyMode>(in_display, in_on);
    }
    auto setClientTargetSlotCount(int64_t in_display, int32_t in_clientTargetSlotCount) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setClientTargetSlotCount>(in_display, in_clientTargetSlotCount);
    }
    auto setColorMode(int64_t in_display, ::aidl::android::hardware::graphics::composer3::ColorMode in_mode, ::aidl::android::hardware::graphics::composer3::RenderIntent in_intent) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setColorMode>(in_display, in_mode, in_intent);
    }
    auto setContentType(int64_t in_display, ::aidl::android::hardware::graphics::composer3::ContentType in_type) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setContentType>(in_display, in_type);
    }
    auto setDisplayedContentSamplingEnabled(int64_t in_display, bool in_enable, ::aidl::android::hardware::graphics::composer3::FormatColorComponent in_componentMask, int64_t in_maxFrames) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setDisplayedContentSamplingEnabled>(in_display, in_enable, in_componentMask, in_maxFrames);
    }
    auto setPowerMode(int64_t in_display, ::aidl::android::hardware::graphics::composer3::PowerMode in_mode) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setPowerMode>(in_display, in_mode);
    }
    auto setReadbackBuffer(int64_t in_display, const ::aidl::android::hardware::common::NativeHandle& in_buffer, const ::ndk::ScopedFileDescriptor& in_releaseFence) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setReadbackBuffer>(in_display, in_buffer, in_releaseFence);
    }
    auto setVsyncEnabled(int64_t in_display, bool in_enabled) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setVsyncEnabled>(in_display, in_enabled);
    }
    auto setIdleTimerEnabled(int64_t in_display, int32_t in_timeoutMs) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setIdleTimerEnabled>(in_display, in_timeoutMs);
    }
    auto getOverlaySupport(::aidl::android::hardware::graphics::composer3::OverlayProperties* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getOverlaySupport>(_aidl_return);
    }
    auto getHdrConversionCapabilities(std::vector<::aidl::android::hardware::graphics::common::HdrConversionCapability>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getHdrConversionCapabilities>(_aidl_return);
    }
    auto setHdrConversionStrategy(const ::aidl::android::hardware::graphics::common::HdrConversionStrategy& in_conversionStrategy, ::aidl::android::hardware::graphics::common::Hdr* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setHdrConversionStrategy>(in_conversionStrategy, _aidl_return);
    }
    auto setRefreshRateChangedCallbackDebugEnabled(int64_t in_display, bool in_enabled) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::setRefreshRateChangedCallbackDebugEnabled>(in_display, in_enabled);
    }
    auto getDisplayConfigurations(int64_t in_display, int32_t in_maxFrameIntervalNs, std::vector<::aidl::android::hardware::graphics::composer3::DisplayConfiguration>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::getDisplayConfigurations>(in_display, in_maxFrameIntervalNs, _aidl_return);
    }
    auto notifyExpectedPresent(int64_t in_display, const ::aidl::android::hardware::graphics::composer3::ClockMonotonicTimestamp& in_expectedPresentTime, int32_t in_frameIntervalNs) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerClient::notifyExpectedPresent>(in_display, in_expectedPresentTime, in_frameIntervalNs);
    }

    // Note: When updating this interface, a useful starting point is the generated
    // ::aidl::android::hardware::graphics::composer3::IComposerClientDelegator class defined in
    // BnComposerClient.h

    #pragma clang diagnostic pop
    // clang-format on
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators