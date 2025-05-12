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

#include <optional>
#include <string>

#include <ui/DisplayId.h>

#include "Utils/Dumper.h"

namespace android::display {

// Immutable state of a virtual display, captured on creation.
class VirtualDisplaySnapshot {
public:
    VirtualDisplaySnapshot(GpuVirtualDisplayId gpuId, std::string uniqueId)
          : mIsGpu(true), mUniqueId(std::move(uniqueId)), mVirtualId(gpuId) {}
    VirtualDisplaySnapshot(HalVirtualDisplayId halId, std::string uniqueId)
          : mIsGpu(false), mUniqueId(std::move(uniqueId)), mVirtualId(halId) {}

    VirtualDisplayId displayId() const { return mVirtualId; }
    bool isGpu() const { return mIsGpu; }

    void dump(utils::Dumper& dumper) const {
        using namespace std::string_view_literals;

        dumper.dump("isGpu"sv, mIsGpu ? "true"sv : "false"sv);
        dumper.dump("uniqueId"sv, mUniqueId);
    }

private:
    const bool mIsGpu;
    const std::string mUniqueId;
    const VirtualDisplayId mVirtualId;
};

} // namespace android::display
