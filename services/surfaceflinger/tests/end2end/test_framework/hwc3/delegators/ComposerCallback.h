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

#include <aidl/android/hardware/graphics/common/DisplayHotplugEvent.h>
#include <aidl/android/hardware/graphics/composer3/BnComposerCallback.h>
#include <aidl/android/hardware/graphics/composer3/IComposerCallback.h>
#include <android/binder_auto_utils.h>

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators {

// Implements the complete ComposerCallback interface in such a way that the default behavior can be
// specified by defining a templated `operator()` in the derived class `Derived`.
//
// The derived class should override all interface functions where the default behavior isn't
// desired, or where an extension to the default is needed. This can lead to a leaner derived class
// especially when only a handful of interface functions need to be overridden.
//
// Example:
//
//   class MyDelegator : public hwc3::delegators::ComposerCallback<MyDelegator> {
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
class ComposerCallback : public ::aidl::android::hardware::graphics::composer3::BnComposerCallback {
    friend Delegator;
    ComposerCallback() = default;

  public:
    auto delegator() -> Delegator* { return static_cast<Delegator*>(this); }

    // clang-format off
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"

    auto onHotplug(int64_t in_display, bool in_connected) -> ::ndk::ScopedAStatus override __attribute__((deprecated(": Use instead onHotplugEvent"))) {
        return delegator()->template operator()<&IComposerCallback::onHotplug>(in_display, in_connected);
    }
    auto onRefresh(int64_t in_display) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onRefresh>(in_display);
    }
    auto onSeamlessPossible(int64_t in_display) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onSeamlessPossible>(in_display);
    }
    auto onVsync(int64_t in_display, int64_t in_timestamp, int32_t in_vsyncPeriodNanos) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onVsync>(in_display, in_timestamp, in_vsyncPeriodNanos);
    }
    auto onVsyncPeriodTimingChanged(int64_t in_display, const ::aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline& in_updatedTimeline) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onVsyncPeriodTimingChanged>(in_display, in_updatedTimeline);
    }
    auto onVsyncIdle(int64_t in_display) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onVsyncIdle>(in_display);
    }
    auto onRefreshRateChangedDebug(const ::aidl::android::hardware::graphics::composer3::RefreshRateChangedDebugData& in_data) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onRefreshRateChangedDebug>(in_data);
    }
    auto onHotplugEvent(int64_t in_display, ::aidl::android::hardware::graphics::common::DisplayHotplugEvent in_event) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposerCallback::onHotplugEvent>(in_display, in_event);
    }

    // Note: When updating this interface, a useful starting point is the generated
    // ::aidl::android::hardware::graphics::composer3::IComposerCallbackDelegator class defined in
    // BnComposerCallback.h

    #pragma clang diagnostic pop
    // clang-format on
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators
