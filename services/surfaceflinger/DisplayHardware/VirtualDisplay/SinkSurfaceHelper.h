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

#include <android/data_space.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <gui/BufferItemConsumer.h>
#include <gui/Surface.h>
#include <hardware/gralloc.h>
#include <system/window.h>
#include <ui/DisplayId.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>

#include <atomic>
#include <cstdint>
#include <future>
#include <mutex>
#include <optional>

#include "DisplayHardware/VirtualDisplay/VirtualDisplayThread.h"

namespace android {

/**
 * SinkSurfaceHelper owns the sink surface for a VirtualDisplaySurface.
 *
 * The consumer for a sink surface are owned by the app. IPC operations involving this surface can
 * feeze.
 *
 * This class synchronizes access to the surface using a queue of tasks that run on a separate
 * thread. That way, no core SF threads can be blocked by a broken app.
 *
 * As buffers are released by the app (and provided to us via SurfaceListener::onBufferReleased,
 * we'll dequeue them and hold on to them so they can be used by the Virtual Display. Use
 * getDequeuedBuffer to get a buffer, sendBuffer to send it, and returnDequeuedBuffer to mark it as
 * get-able again.
 *
 * sendBuffer can also take non-dequeued buffers, it'll just attach the buffer and queue it.
 */
class SinkSurfaceHelper : public SurfaceListener {
public:
    static constexpr size_t kMaxDequeuedBuffers = 4;

    SinkSurfaceHelper(const sp<Surface>& sink, uid_t creatorUid);

    struct SinkSurfaceData {
        uint32_t width = 0;
        uint32_t height = 0;
        PixelFormat format = PIXEL_FORMAT_UNKNOWN;
        uint64_t usage = 0;
        ADataSpace dataSpace = ADATASPACE_UNKNOWN;
    };
    std::future<SinkSurfaceData> connectSinkSurface();
    void abandon();

    bool isFrozen();

    const std::string& getName() const { return mName; }

    /**
     * Get a buffer that was previously dequeued by the app. If no buffers are available, this
     * will return std::nullopt.
     *
     * The caller is responsible for returning the buffer using returnDequeuedBuffer() or
     * sendBuffer().
     */
    std::optional<std::tuple<sp<GraphicBuffer>, sp<Fence>>> getDequeuedBuffer(
            uint32_t width, uint32_t height, uint64_t requiredUsage);

    /**
     * Return a buffer retrieved via getDequeuedBuffer to the helper, so it can be used later.
     */
    bool returnDequeuedBuffer(const sp<GraphicBuffer>& buffer, const sp<Fence>& fence);

    /**
     * Schedule a buffer to be sent to the sink.
     *
     * If the buffer was previously dequeued (and retrieved by getDequeuedBuffer), the buffer will
     * be sent right away. Otherwise, the buffer will be attached to the underlying BufferQueue.
     */
    void sendBuffer(const sp<GraphicBuffer>& buffer, const sp<Fence>& fence);

    /**
     * Set the size of the sink surface.
     */
    void setBufferSize(uint32_t width, uint32_t height);

    // SurfaceListener:
    virtual bool needsReleaseNotify() override { return true; }
    virtual bool needsDeathNotify() override { return true; }

    virtual void onBufferReleased() override;
    virtual void onRemoteDied() override;
    virtual void onBuffersDiscarded(const std::vector<sp<GraphicBuffer>>&) override {}
    virtual void onBufferDetached(int) override {}

private:
    void connectSinkSurfaceTask(std::shared_ptr<std::promise<SinkSurfaceData>> promise);
    void sendBufferTask(sp<GraphicBuffer> buffer, sp<Fence> fence);
    void freeBufferTask(sp<GraphicBuffer> buffer, sp<Fence> fence);
    void dequeueBufferTask();
    void setBufferSizeTask(uint32_t width, uint32_t height);
    void abandonTask();

    void cancelBuffers(std::vector<std::tuple<sp<GraphicBuffer>, sp<Fence>>> buffers);

    VirtualDisplayThread::Client mVDThread;
    const uid_t mCreatorUid;

    // This should only be accessed on mVDThread.
    //
    // DO NOT access this with mDataMutex, or you'll potentially lock up the rest of SF.
    sp<Surface> mSink;

    std::string mName;
    std::atomic_bool mIsDead = false;

    struct DequeuedSinkBuffer {
        sp<GraphicBuffer> buffer;
        sp<Fence> fence;
        bool inUse = false;
    };

    std::mutex mDataMutex;
    std::vector<DequeuedSinkBuffer> mDequeuedBuffers GUARDED_BY(mDataMutex);
};

} // namespace android