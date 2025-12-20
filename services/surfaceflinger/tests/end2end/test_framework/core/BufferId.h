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
#include <string>

#include <aidl/android/hardware/common/NativeHandle.h>
#include <cutils/native_handle.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

namespace android::surfaceflinger::tests::end2end::test_framework::core {

struct BufferId final {
    uint64_t inode;
    uint64_t device;

    friend auto operator==(const BufferId&, const BufferId&) -> bool = default;
};

auto toBufferId(const native_handle_t* handle) -> BufferId;
auto toBufferId(const sp<GraphicBuffer>& buffer) -> BufferId;
auto toBufferId(const aidl::android::hardware::common::NativeHandle& handle) -> BufferId;

auto toString(const BufferId& value) -> std::string;

inline auto format_as(const BufferId& event) -> std::string {
    return toString(event);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
