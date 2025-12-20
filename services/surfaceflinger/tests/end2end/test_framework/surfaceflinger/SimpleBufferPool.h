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

#include <cstddef>
#include <deque>
#include <memory>
#include <string>

#include <android-base/expected.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Size.h>
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>

#include "test_framework/core/GuardedSharedState.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class SimpleBufferPool final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    [[nodiscard]] static auto make(size_t count, ui::Size size, PixelFormat format)
            -> base::expected<std::shared_ptr<SimpleBufferPool>, std::string>;

    explicit SimpleBufferPool(Passkey passkey);

    auto dequeue() -> sp<GraphicBuffer>;
    void enqueue(sp<GraphicBuffer> buffer);

  private:
    [[nodiscard]] auto init(size_t count, ui::Size size,
                            PixelFormat format) -> base::expected<void, std::string>;

    struct State {
        std::deque<sp<GraphicBuffer>> pool;
    };
    core::GuardedSharedState<State> mState;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
