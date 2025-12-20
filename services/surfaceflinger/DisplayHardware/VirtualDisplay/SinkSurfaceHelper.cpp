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

// #define LOG_NDEBUG 0
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <android-base/strings.h>
#include <android/data_space.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <gui/BufferItemConsumer.h>
#include <gui/Surface.h>
#include <hardware/gralloc.h>
#include <log/log_main.h>
#include <system/window.h>
#include <ui/DisplayId.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>
#include <utils/Trace.h>

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

#include "SinkSurfaceHelper.h"
#include "VirtualDisplayThreadManager.h"

namespace android {

SinkSurfaceHelper::SinkSurfaceHelper(const sp<Surface>& sink, uid_t creatorUid)
      : mVDThread(VirtualDisplayThreadManager::getInstance().getOrCreateThread(creatorUid)),
        mCreatorUid(creatorUid),
        mSink(sink) {}

std::future<SinkSurfaceHelper::SinkSurfaceData> SinkSurfaceHelper::connectSinkSurface() {
    ATRACE_CALL();

    // Note, this annoying extra shared_ptr is needed since the std::function-based task needs to be
    // copyable.
    auto promise = std::make_shared<std::promise<SinkSurfaceData>>();
    auto future = promise->get_future();

    mVDThread.submitWork(std::bind(&SinkSurfaceHelper::connectSinkSurfaceTask,
                                   sp<SinkSurfaceHelper>::fromExisting(this), promise));

    return future;
}

void SinkSurfaceHelper::abandon() {
    ATRACE_CALL();

    mVDThread.submitWork(
            std::bind(&SinkSurfaceHelper::abandonTask, sp<SinkSurfaceHelper>::fromExisting(this)));
}

bool SinkSurfaceHelper::isFrozen() {
    ATRACE_CALL();
    return mVDThread.isFrozen();
}

std::optional<std::tuple<sp<GraphicBuffer>, sp<Fence>>> SinkSurfaceHelper::getDequeuedBuffer(
        uint32_t width, uint32_t height, uint64_t requiredUsage) {
    ATRACE_CALL();
    std::scoped_lock _l(mDataMutex);

    for (auto& dequeued : mDequeuedBuffers) {
        if (dequeued.inUse) {
            continue;
        }
        auto& buffer = dequeued.buffer;
        if (buffer->width != static_cast<int32_t>(width) ||
            buffer->height != static_cast<int32_t>(height) ||
            ((buffer->usage & requiredUsage) != requiredUsage)) {
            continue;
        }

        dequeued.inUse = true;
        ALOGI("%s: Found available dequeued buffer id=%" PRIu64 " from sink.", __func__,
              buffer->getId());
        return {{dequeued.buffer, dequeued.fence}};
    }

    return std::nullopt;
}

bool SinkSurfaceHelper::returnDequeuedBuffer(const sp<GraphicBuffer>& buffer,
                                             const sp<Fence>& fence) {
    ATRACE_CALL();
    std::scoped_lock _l(mDataMutex);

    for (auto& dequeuedBuffer : mDequeuedBuffers) {
        if (dequeuedBuffer.buffer == buffer) {
            dequeuedBuffer.fence = Fence::merge("VD: freeBuffer", dequeuedBuffer.fence, fence);
            dequeuedBuffer.inUse = false;
            ALOGI("%s: Returned dequeued buffer id=%" PRIu64 " from sink.", __func__,
                  buffer->getId());
            return true;
        }
    }

    return false;
}

void SinkSurfaceHelper::sendBuffer(const sp<GraphicBuffer>& buffer, const sp<Fence>& fence) {
    ATRACE_CALL();

    mVDThread.submitWork(std::bind(&SinkSurfaceHelper::sendBufferTask,
                                   sp<SinkSurfaceHelper>::fromExisting(this), buffer, fence));
}

void SinkSurfaceHelper::setBufferSize(uint32_t width, uint32_t height) {
    ATRACE_CALL();

    mVDThread.submitWork(std::bind(&SinkSurfaceHelper::setBufferSizeTask,
                                   sp<SinkSurfaceHelper>::fromExisting(this), width, height));
}

void SinkSurfaceHelper::onBufferReleased() {
    ATRACE_CALL();

    // The current function _is_ called off the main thread. But, if we run this on the VD
    // thread, we'll be able to catch freezes during the dequeue.
    mVDThread.submitWork(std::bind(&SinkSurfaceHelper::dequeueBufferTask,
                                   sp<SinkSurfaceHelper>::fromExisting(this)));
}

void SinkSurfaceHelper::onRemoteDied() {
    ATRACE_CALL();
    mIsDead = true;
}

void SinkSurfaceHelper::connectSinkSurfaceTask(
        std::shared_ptr<std::promise<SinkSurfaceData>> promise) {
    ATRACE_CALL();

    SinkSurfaceData data;
    // TODO(b/340933138): Should this be EGL?
    mSink->connect(NATIVE_WINDOW_API_CPU, sp<SinkSurfaceHelper>::fromExisting(this));
    mName = mSink->getConsumerName();

    status_t ret = mSink->getConsumerUsage(&data.usage);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to get consumer usage from the sink surface. Status: %d", __func__, ret);
        data.usage = 0;
    }

    data.format = ANativeWindow_getFormat(mSink.get());
    if (data.format < 0) {
        ALOGE("%s: Bad format returned from %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), data.format);
        data.format = 0;
    }

    int32_t dataSpace = ANativeWindow_getBuffersDefaultDataSpace(mSink.get());
    if (dataSpace < 0) {
        ALOGE("%s: Bad dataSpace returned from %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), dataSpace);
        dataSpace = 0;
    }
    data.dataSpace = static_cast<ADataSpace>(dataSpace);

    int32_t width = ANativeWindow_getWidth(mSink.get());
    if (width < 0) {
        ALOGE("%s: Bad width returned from %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), width);
        width = 0;
    }
    data.width = static_cast<uint32_t>(width);

    int32_t height = ANativeWindow_getHeight(mSink.get());
    if (height < 0) {
        ALOGE("%s: Bad height returned from %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), height);
        height = 0;
    }
    data.height = static_cast<uint32_t>(height);

    ret = mSink->setAsyncMode(true);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to set async mode for %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), ret);
    }

    ret = mSink->setMaxDequeuedBufferCount(SinkSurfaceHelper::kMaxDequeuedBuffers);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to set max dequeued buffer count for %s. Status: %d", __func__,
              mSink->getConsumerName().c_str(), ret);
    }

    promise->set_value(std::move(data));
}

void SinkSurfaceHelper::sendBufferTask(sp<GraphicBuffer> buffer, sp<Fence> fence) {
    ATRACE_CALL();

    if (mIsDead) {
        return;
    }

    bool requiresAttach = true;
    {
        auto lock = std::lock_guard(mDataMutex);

        for (auto it = mDequeuedBuffers.begin(); it != mDequeuedBuffers.end(); ++it) {
            if (it->buffer == buffer) {
                requiresAttach = false;
                mDequeuedBuffers.erase(it);
                break;
            }
        }
    }

    status_t ret;
    if (requiresAttach) {
        {
            ATRACE_NAME("attachBuffer");
            ret = mSink->attachBuffer(buffer.get());
        }
        if (ret != NO_ERROR) {
            ALOGE("%s: Unable to attach buffer %" PRIu64 "to surface %s. Status %d", __func__,
                  buffer->getId(), mSink->getConsumerName().c_str(), ret);
            return;
        }
    }

    ALOGI("%s: Sending buffer %" PRIu64 " to %s. It %s previously dequeued.", __func__,
          buffer->getId(), mName.c_str(), requiresAttach ? "was" : "wasn't");

    SurfaceQueueBufferOutput output;
    {
        ATRACE_NAME("queueBuffer");
        ret = mSink->queueBuffer(buffer, fence, &output);
        if (ret != NO_ERROR) {
            ALOGE("%s: Unable to queue buffer %" PRIu64 "to surface %s. Status %d", __func__,
                  buffer->getId(), mSink->getConsumerName().c_str(), ret);
            return;
        }
    }

    if (output.bufferReplaced) {
        mVDThread.submitWork(std::bind(&SinkSurfaceHelper::dequeueBufferTask,
                                       sp<SinkSurfaceHelper>::fromExisting(this)));
    }
    ALOGD("%s: Queued buffer %" PRIu64 "to surface %s.", __func__, buffer->getId(),
          mSink->getConsumerName().c_str());
}

void SinkSurfaceHelper::dequeueBufferTask() {
    ATRACE_CALL();

    if (mIsDead) {
        return;
    }

    {
        std::scoped_lock _l(mDataMutex);

        // Keep a spare buffer always available for attaching in sendBufferTask.
        if (mDequeuedBuffers.size() >= kMaxDequeuedBuffers - 1) {
            ALOGD("%s: Not dequeuing more buffers past the maximum of %zu", __func__,
                  kMaxDequeuedBuffers - 1);
            return;
        }
    }

    ATRACE_NAME("dequeueBuffer");
    sp<GraphicBuffer> buffer;
    sp<Fence> fence;
    status_t ret = mSink->dequeueBuffer(&buffer, &fence);
    if (ret != NO_ERROR) {
        ALOGE("%s: Failed to dequeue buffer from the sink surface. Status: %d", __func__, ret);
        return;
    }

    ALOGI("%s: Dequeued buffer %" PRIu64 " from %s.", __func__, buffer->getId(), mName.c_str());

    {
        auto lock = std::lock_guard(mDataMutex);
        mDequeuedBuffers.push_back({buffer, fence});
    }
}

void SinkSurfaceHelper::setBufferSizeTask(uint32_t width, uint32_t height) {
    ATRACE_CALL();

    if (mIsDead) {
        return;
    }

    std::vector<std::tuple<sp<GraphicBuffer>, sp<Fence>>> buffers;
    {
        std::scoped_lock _l(mDataMutex);

        for (auto it = mDequeuedBuffers.begin(); it != mDequeuedBuffers.end();) {
            if (it->buffer->getWidth() != width || it->buffer->getHeight() != height) {
                buffers.push_back({it->buffer, it->fence});
                it = mDequeuedBuffers.erase(it);
                continue;
            }

            ++it;
        }
    }

    mSink->setBuffersDimensions(width, height);
    cancelBuffers(std::move(buffers));
}

void SinkSurfaceHelper::abandonTask() {
    ATRACE_CALL();
    if (!mIsDead) {
        status_t ret = mSink->disconnect(NATIVE_WINDOW_API_CPU);
        ALOGE_IF(ret != NO_ERROR, "%s: Error disconnecting from sink surface: %d", __func__, ret);
    }
}

void SinkSurfaceHelper::cancelBuffers(
        std::vector<std::tuple<sp<GraphicBuffer>, sp<Fence>>> buffers) {
    ATRACE_CALL();

    if (mIsDead) {
        return;
    }

    std::vector<Surface::BatchBuffer> batchBuffers(buffers.size());
    std::vector<uint64_t> bufferIds(buffers.size());
    for (auto& [buffer, fence] : buffers) {
        auto& batchBuffer = batchBuffers.emplace_back();
        batchBuffer.buffer = buffer.get();
        batchBuffer.fenceFd = fence->dup();
        bufferIds.push_back(buffer->getId());
    }

    ALOGI("%s: Cancelling buffers [%s] from %s.", __func__, base::Join(bufferIds, ", ").c_str(),
          mName.c_str());

    ATRACE_NAME("cancelBuffers");
    status_t res = mSink->cancelBuffers(batchBuffers);
    if (res != NO_ERROR) {
        ALOGE("%s: Unable to cancel buffers. Status: %d", __func__, res);
        return;
    }
}

} // namespace android