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

#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android/gui/ISurfaceComposerClient.h>
#include <ftl/finalizer.h>
#include <ftl/ignore.h>
#include <gui/ITransactionCompletedListener.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceControl.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Size.h>
#include <utils/String8.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>

#include "test_framework/core/BufferId.h"
#include "test_framework/surfaceflinger/SimpleBufferPool.h"
#include "test_framework/surfaceflinger/Surface.h"
#include "test_framework/surfaceflinger/events/BufferReleased.h"
#include "test_framework/surfaceflinger/events/TransactionCommitted.h"
#include "test_framework/surfaceflinger/events/TransactionCompleted.h"
#include "test_framework/surfaceflinger/events/TransactionInitiated.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

struct Surface::Passkey final {};

auto Surface::make(const sp<SurfaceComposerClient>& flinger, const Surface::CreationArgs& args)
        -> base::expected<std::shared_ptr<Surface>, std::string> {
    using namespace std::string_literals;

    auto instance = std::make_shared<Surface>(Passkey{});
    if (instance == nullptr) {
        return base::unexpected("Failed to construct a Surface"s);
    }
    if (auto result = instance->init(flinger, args); !result) {
        return base::unexpected("Failed to init a Surface: " + result.error());
    }
    return std::move(instance);
}

Surface::Surface(Passkey passkey) {
    ftl::ignore(passkey);
}

auto Surface::init(const sp<SurfaceComposerClient>& flinger, const Surface::CreationArgs& args)
        -> base::expected<void, std::string> {
    using namespace std::string_literals;

    constexpr auto kFlags = gui::ISurfaceComposerClient::eOpaque;
    sp<SurfaceControl> surfaceControl =
            flinger->createSurface(String8(args.name.c_str()), args.size.width, args.size.height,
                                   args.pixelFormat, kFlags, args.parent);

    if (surfaceControl == nullptr) {
        return base::unexpected("Failed to create a SF Surface");
    }

    auto bufferPool = SimpleBufferPool::make(args.bufferPoolSize, args.size, args.pixelFormat);
    if (!bufferPool) {
        return base::unexpected(bufferPool.error());
    }

    {
        SurfaceComposerClient::Transaction transaction;
        transaction.show(surfaceControl);
        transaction.setPosition(surfaceControl, 0, 0);
        transaction.setLayerStack(surfaceControl, {args.layerStackId});
        constexpr bool synchronous = true;
        transaction.apply(synchronous);
    }

    mSurfaceControl = std::move(surfaceControl);
    mSurfaceSize = args.size;
    mBufferPool = *std::move(bufferPool);

    mCleanup = ftl::Finalizer([this]() { ensureCallbacksCompletedBeforeShutdown(); });

    return {};
}

auto Surface::editCallbacks() -> Callbacks& {
    return mCallbacks;
}

auto Surface::commitNextBuffer() -> uint64_t {
    const auto frameNumber = mNextFrameNumber++;
    auto buffer = mBufferPool->dequeue();
    auto bufferId = core::toBufferId(buffer);

    LOG(VERBOSE) << __func__ << " framenumber " << frameNumber << " buffer "
                 << (buffer ? buffer->getId() : 0) << " " << toString(bufferId) << " wxh "
                 << mSurfaceSize.width << "x" << mSurfaceSize.height;

    commitBufferInternal(
            frameNumber, buffer,
            /* buffer release */
            [weak = weak_from_this(), buffer, bufferId, frameNumber](
                    const ReleaseCallbackId& callbackId, const sp<Fence>& releaseFence,
                    std::optional<uint32_t> currentMaxAcquiredBufferCount) {
                ftl::ignore(callbackId, releaseFence, currentMaxAcquiredBufferCount);
                if (auto self = weak.lock()) {
                    self->onBufferRelease(
                            events::BufferReleased{
                                    .surfaceId = self->id(),
                                    .frameNumber = frameNumber,
                                    .bufferId = bufferId,
                            },
                            buffer);
                } else {
                    LOG(DEBUG) << "Target surface for buffer release callback is dead";
                }
            },
            /* committed */
            [weak = weak_from_this(), frameNumber, bufferId](
                    void* context, nsecs_t latchTime, const sp<Fence>& presentFence,
                    const std::vector<SurfaceControlStats>& stats) {
                ftl::ignore(context, presentFence, stats);
                if (auto self = weak.lock()) {
                    self->onTransactionCommitted(events::TransactionCommitted{
                            .surfaceId = self->id(),
                            .frameNumber = frameNumber,
                            .bufferId = bufferId,
                            .latchTime = Timestamp(std::chrono::nanoseconds(latchTime))});
                } else {
                    LOG(DEBUG) << "Target surface for transaction committed callback is dead.";
                }
            },
            /* completed */
            [weak = weak_from_this(), frameNumber, bufferId](
                    void* context, nsecs_t latchTime, const sp<Fence>& presentFence,
                    const std::vector<SurfaceControlStats>& stats) {
                ftl::ignore(context, presentFence, stats);
                if (auto self = weak.lock()) {
                    const auto timestamp = Timestamp(std::chrono::nanoseconds(latchTime));
                    self->onTransactionCompleted(events::TransactionCompleted{
                            .surfaceId = self->id(),
                            .frameNumber = frameNumber,
                            .bufferId = bufferId,
                            .latchTime = Timestamp(std::chrono::nanoseconds(latchTime))});
                } else {
                    LOG(DEBUG) << "Target surface for transaction completed callback is dead.";
                }
            });

    onTransactionInitiated(events::TransactionInitiated{
            .surfaceId = id(),
            .frameNumber = frameNumber,
            .bufferId = bufferId,
    });

    return frameNumber;
}

void Surface::commitBufferInternal(
        uint64_t frameNumber, const sp<GraphicBuffer>& buffer,
        ReleaseBufferCallback releaseCallback,
        TransactionCompletedCallbackTakesContext transactionCommittedCallback,
        TransactionCompletedCallbackTakesContext transactionCompletedCallback) {
    SurfaceComposerClient::Transaction transaction;

    transaction.setSurfaceDamageRegion(mSurfaceControl,
                                       Region(Rect(0, 0, mSurfaceSize.width, mSurfaceSize.height)));

    const std::optional<sp<Fence>> fence{};
    constexpr int producerId = 0;
    transaction.setBuffer(mSurfaceControl, buffer, fence, frameNumber, producerId,
                          std::move(releaseCallback));

    transaction.addTransactionCommittedCallback(std::move(transactionCommittedCallback), nullptr);

    // Note: transaction.setBuffer() internally and unconditionally sets a no-op transaction
    // completion callback. We must set ours AFTER that call, not before.
    transaction.addTransactionCompletedCallback(std::move(transactionCompletedCallback), nullptr);

    transaction.apply();
}

void Surface::onBufferRelease(const events::BufferReleased& event,
                              const sp<GraphicBuffer>& buffer) {
    mBufferPool->enqueue(buffer);

    LOG(VERBOSE) << __func__ << " " << toString(event);

    mCallbacks.onBufferReleased(event);
}

void Surface::onTransactionInitiated(const events::TransactionInitiated& event) const {
    LOG(VERBOSE) << __func__ << " " << toString(event);

    mCallbacks.onTransactionInitiated(event);
}

void Surface::onTransactionCommitted(const events::TransactionCommitted& event) const {
    LOG(VERBOSE) << __func__ << " " << toString(event);

    mCallbacks.onTransactionCommitted(event);
}

void Surface::onTransactionCompleted(const events::TransactionCompleted& event) const {
    LOG(VERBOSE) << __func__ << " " << toString(event);

    mCallbacks.onTransactionCompleted(event);
}

void Surface::ensureCallbacksCompletedBeforeShutdown() {
    // If we just shut down the test without this special cleanup, there is a chance for a runtime
    // crash on shutdown in the SurfaceComposerClient buffer cache singleton, from callbacks to it
    // being invoked after being destroyed.
    //
    // There is also a TransactionCompletedListener singleton which can be destroyed after the
    // buffer cache singleton. If there are outstanding transaction completion callbacks, and those
    // have a reference to a GraphicBuffer, when those are destroyed, the GraphicBuffer destructor
    // tries to clear itself from the buffer cache.
    //
    // To work around this, we intentionally submit for some final transactions on the surface to
    // ensure all prior callbacks are complete, and for those use lambdas that do not capture
    // GraphicBuffers.

    auto buffer = sp<GraphicBuffer>::make(GraphicBufferAllocator::AllocationRequest{
            .importBuffer = false,
            .width = 1,
            .height = 1,
            .format = PIXEL_FORMAT_RGBA_8888,
            .layerCount = 1,
            .usage = GraphicBuffer::USAGE_HW_COMPOSER,
            .requestorName = "test",
            .extras = {},
    });

    std::promise<void> firstTransactionPromise;
    auto firstTransactionComplete = firstTransactionPromise.get_future();

    commitBufferInternal(
            mNextFrameNumber++, buffer,
            [](const ReleaseCallbackId& callbackId, const sp<Fence>& releaseFence,
               std::optional<uint32_t> currentMaxAcquiredBufferCount) {
                LOG(VERBOSE) << "first cleanup transaction buffer released";
                ftl::ignore(callbackId, releaseFence, currentMaxAcquiredBufferCount);
                // Note: We don't signal a promise as this doesn't actually seem to be invoked by
                // the next transaction!
            },
            [](void* context, nsecs_t latchTime, const sp<Fence>& presentFence,
               const std::vector<SurfaceControlStats>& stats) {
                ftl::ignore(context, latchTime, presentFence, stats);
                LOG(VERBOSE) << "first cleanup transaction committed";
            },
            [&firstTransactionPromise](void* context, nsecs_t latchTime,
                                       const sp<Fence>& presentFence,
                                       const std::vector<SurfaceControlStats>& stats) {
                ftl::ignore(context, latchTime, presentFence, stats);
                LOG(VERBOSE) << "first cleanup transaction complete";
                firstTransactionPromise.set_value();
            });

    // Now commit a custom second transaction to unset the buffer.
    SurfaceComposerClient::Transaction transaction;
    transaction.unsetBuffer(mSurfaceControl);

    std::promise<void> hideSurfaceTransactionPromise;
    auto hideSurfaceTransactionComplete = hideSurfaceTransactionPromise.get_future();
    transaction.addTransactionCompletedCallback(
            [&hideSurfaceTransactionPromise](void* context, nsecs_t latchTime,
                                             const sp<Fence>& presentFence,
                                             const std::vector<SurfaceControlStats>& stats) {
                ftl::ignore(context, latchTime, presentFence, stats);
                LOG(VERBOSE) << "second cleanup transaction complete";
                hideSurfaceTransactionPromise.set_value();
            },
            nullptr);

    constexpr bool synchronous = true;
    transaction.apply(synchronous);

    // Wait for a more than reasonable amount of time for everything to complete.
    // If we timeout
    constexpr auto kWaitTimeout = std::chrono::milliseconds(1000);
    LOG(VERBOSE) << "waiting for cleanup transactions to complete...";
    LOG_IF(FATAL, firstTransactionComplete.wait_for(kWaitTimeout) != std::future_status::ready)
            << "timed out waiting for the first transaction to complete. Fatal since the lambda "
               "holds a reference to a local!";
    LOG_IF(FATAL,
           hideSurfaceTransactionComplete.wait_for(kWaitTimeout) != std::future_status::ready)
            << "timed out waiting for the second transaction to complete. Fatal since the lambda "
               "holds a reference to a local!";
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
