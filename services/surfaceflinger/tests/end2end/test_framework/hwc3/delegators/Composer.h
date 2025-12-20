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
#include <vector>

#include <aidl/android/hardware/graphics/composer3/BnComposer.h>
#include <aidl/android/hardware/graphics/composer3/Capability.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <android/binder_auto_utils.h>

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators {

// Implements the complete Composer interface in such a way that the default behavior can be
// specified by defining a templated `operator()` in the derived class `Derived`.
//
// The derived class should override all interface functions where the default behavior isn't
// desired, or where an extension to the default is needed. This can lead to a leaner derived class
// especially when only a handful of interface functions need to be overridden.
//
// Example:
//
//   class MyDelegator : public hwc3::delegators::Composer<MyDelegator> {
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
template <class Derived>
class Composer : public ::aidl::android::hardware::graphics::composer3::BnComposer {
    friend Derived;
    Composer() = default;

  public:
    auto delegator() -> Derived* { return static_cast<Derived*>(this); }

    // clang-format off
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"

    auto createClient(std::shared_ptr<::aidl::android::hardware::graphics::composer3::IComposerClient>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposer::createClient>(_aidl_return);
    }
    auto getCapabilities(std::vector<::aidl::android::hardware::graphics::composer3::Capability>* _aidl_return) -> ::ndk::ScopedAStatus override {
        return delegator()->template operator()<&IComposer::getCapabilities>(_aidl_return);
    }

    // Note: When updating this interface, a useful starting point is the generated
    // ::aidl::android::hardware::graphics::composer3::IComposerDelegator class defined in
    // BnComposer.h

    #pragma clang diagnostic pop
    // clang-format on
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3::delegators
