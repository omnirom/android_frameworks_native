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

#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <android-base/expected.h>
#include <binder/IBinder.h>
#include <ftl/finalizer.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <ui/GraphicBuffer.h>
#include <ui/PixelFormat.h>
#include <ui/Size.h>
#include <utils/StrongPointer.h>

#include "test_framework/surfaceflinger/events/BufferReleased.h"
#include "test_framework/surfaceflinger/events/TransactionCommitted.h"
#include "test_framework/surfaceflinger/events/TransactionCompleted.h"
#include "test_framework/surfaceflinger/events/TransactionInitiated.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class SimpleBufferPool;

// Wrapper around a gui::SurfaceControl
class Surface final : public std::enable_shared_from_this<Surface> {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    using Timestamp = std::chrono::steady_clock::time_point;

    // The creation arguments for a surface, with reasonable default values.
    struct CreationArgs final {
        std::string name;
        ui::Size size{1, 1};
        PixelFormat pixelFormat = PIXEL_FORMAT_RGBA_8888;
        sp<IBinder> parent = nullptr;
        size_t bufferPoolSize = 3;
        uint32_t layerStackId = 0;
    };

    // The callbacks provided from a surface instance.
    struct Callbacks final {
        // Sent when a transaction is initiated for the surface.
        events::TransactionInitiated::AsyncConnector onTransactionInitiated;

        // Sent when a buffer release event was received for the surface.
        events::BufferReleased::AsyncConnector onBufferReleased;

        // Sent when a transaction commit event is received for the surface.
        events::TransactionCommitted::AsyncConnector onTransactionCommitted;

        // Sent when a transaction complete event is received for the surface.
        events::TransactionCompleted::AsyncConnector onTransactionCompleted;
    };

    [[nodiscard]] static auto make(const sp<SurfaceComposerClient>& flinger,
                                   const Surface::CreationArgs& args)
            -> base::expected<std::shared_ptr<Surface>, std::string>;

    explicit Surface(Passkey passkey);

    [[nodiscard]] auto id() const -> uintptr_t { return std::bit_cast<uintptr_t>(this); }

    // Allows the caller to set handlers for the callbacks emitted by this class.
    [[nodiscard]] auto editCallbacks() -> Callbacks&;

    // Commits the next available buffer from the buffer pool. Returns the frame number associated
    // with the commit.
    [[nodiscard]] auto commitNextBuffer() -> uint64_t;

  private:
    [[nodiscard]] auto init(const sp<SurfaceComposerClient>& flinger,
                            const Surface::CreationArgs& args) -> base::expected<void, std::string>;
    void commitBufferInternal(
            uint64_t frameNumber, const sp<GraphicBuffer>& buffer,
            ReleaseBufferCallback releaseCallback,
            TransactionCompletedCallbackTakesContext transactionCommittedCallback,
            TransactionCompletedCallbackTakesContext transactionCompletedCallback);

    void onBufferRelease(const events::BufferReleased& event, const sp<GraphicBuffer>& buffer);
    void onTransactionInitiated(const events::TransactionInitiated& event) const;
    void onTransactionCommitted(const events::TransactionCommitted& event) const;
    void onTransactionCompleted(const events::TransactionCompleted& event) const;
    void ensureCallbacksCompletedBeforeShutdown();

    Callbacks mCallbacks;

    ui::Size mSurfaceSize;
    std::shared_ptr<SimpleBufferPool> mBufferPool;
    sp<SurfaceControl> mSurfaceControl;
    std::atomic_uint64_t mNextFrameNumber{0};

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl mCleanup;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
