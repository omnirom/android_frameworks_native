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

#include <bit>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <ftl/ignore.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/PixelFormat.h>
#include <ui/Size.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/GuardedSharedState.h"
#include "test_framework/surfaceflinger/SimpleBufferPool.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

struct SimpleBufferPool::Passkey final {};

auto SimpleBufferPool::make(size_t count, ui::Size size, PixelFormat format)
        -> base::expected<std::shared_ptr<SimpleBufferPool>, std::string> {
    using namespace std::string_literals;

    auto instance = std::make_unique<SimpleBufferPool>(Passkey{});
    if (instance == nullptr) {
        return base::unexpected("Failed to construct a SimpleBufferPool"s);
    }
    if (auto result = instance->init(count, size, format); !result) {
        return base::unexpected("Failed to init a SimpleBufferPool: " + result.error());
    }
    return std::move(instance);
}

SimpleBufferPool::SimpleBufferPool(Passkey passkey) {
    ftl::ignore(passkey);
}

auto SimpleBufferPool::init(size_t count, ui::Size size,
                            PixelFormat format) -> base::expected<void, std::string> {
    return mState.withExclusiveLock(
            [count, size, format](auto& state) -> base::expected<void, std::string> {
                for (size_t i = 0; i < count; i++) {
                    constexpr auto kLayers = 1;
                    constexpr auto kUsage = GraphicBuffer::USAGE_HW_COMPOSER;
                    const GraphicBufferAllocator::AllocationRequest request{
                            .importBuffer = false,
                            .width = std::bit_cast<uint32_t>(size.width),
                            .height = std::bit_cast<uint32_t>(size.height),
                            .format = format,
                            .layerCount = kLayers,
                            .usage = kUsage,
                            .requestorName = "test",
                            .extras = {},
                    };
                    state.pool.push_back(sp<GraphicBuffer>::make(request));
                    CHECK(state.pool.back() != nullptr);
                }
                return {};
            });
}

auto SimpleBufferPool::dequeue() -> sp<GraphicBuffer> {
    return mState.withExclusiveLock([](auto& state) -> sp<GraphicBuffer> {
        if (state.pool.empty()) {
            return nullptr;
        }
        auto buffer = state.pool.front();
        CHECK(buffer != nullptr);
        state.pool.pop_front();
        return buffer;
    });
};

void SimpleBufferPool::enqueue(sp<GraphicBuffer> buffer) {
    CHECK(buffer != nullptr);
    mState.withExclusiveLock([buffer = std::move(buffer)](auto& state) mutable {
        state.pool.push_back(std::move(buffer));
    });
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
