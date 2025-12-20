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
#include <string>

#include <sys/stat.h>

#include <aidl/android/hardware/common/NativeHandle.h>
#include <android-base/logging.h>
#include <cutils/native_handle.h>
#include <fmt/format.h>
#include <ftl/cast.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/BufferId.h"

namespace android::surfaceflinger::tests::end2end::test_framework::core {
namespace {

auto bufferFdToBufferId(int rawFd) -> BufferId {
    struct stat stat{};
    const int result = fstat(rawFd, &stat);
    CHECK(result == 0);
    CHECK(ftl::cast_safety<uint64_t>(stat.st_ino) == ftl::CastSafety::kSafe);
    CHECK(ftl::cast_safety<uint64_t>(stat.st_dev) == ftl::CastSafety::kSafe);
    return {.inode = static_cast<uint64_t>(stat.st_ino),
            .device = static_cast<uint64_t>(stat.st_dev)};
}

}  // namespace

auto toBufferId(const aidl::android::hardware::common::NativeHandle& handle) -> BufferId {
    CHECK(!handle.fds.empty());
    return bufferFdToBufferId(handle.fds[0].get());
}

auto toBufferId(const native_handle_t* handle) -> BufferId {
    CHECK(handle != nullptr);
    CHECK(handle->numFds > 0);
    return bufferFdToBufferId(handle->data[0]);
}

auto toBufferId(const sp<GraphicBuffer>& buffer) -> BufferId {
    CHECK(buffer != nullptr);
    return toBufferId(buffer->getNativeBuffer()->handle);
}

auto toString(const BufferId& value) -> std::string {
    return fmt::format("BufferId{{inode: {}, device: {}}}", value.inode, value.device);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core