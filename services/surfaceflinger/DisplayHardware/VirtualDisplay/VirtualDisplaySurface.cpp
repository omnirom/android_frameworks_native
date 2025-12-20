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

#include <android/data_space.h>
#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <ftl/enum.h>
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

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#include "DisplayHardware/HWComposer.h"
#include "compositionengine/DisplaySurface.h"

#include "SinkSurfaceHelper.h"
#include "VirtualDisplayBufferSlotTracker.h"
#include "VirtualDisplaySurface.h"

namespace android {

class VirtualDisplaySurface::RenderConsumerListener
      : public BufferItemConsumer::FrameAvailableListener {
public:
    RenderConsumerListener(const sp<VirtualDisplaySurface>& virtualDisplay)
          : mVirtualDisplay(virtualDisplay) {}

    // BufferItemConsumer::FrameAvailableListener
    virtual void onFrameAvailable(const BufferItem&) override {
        sp<VirtualDisplaySurface> virtualDisplay = mVirtualDisplay.promote();
        if (virtualDisplay) {
            virtualDisplay->onRenderFrameAvailable();
        }
    }

private:
    wp<VirtualDisplaySurface> mVirtualDisplay;
};

VirtualDisplaySurface::VirtualDisplaySurface(HWComposer& hwComposer,
                                             VirtualDisplayIdVariant displayId,
                                             const std::string& name, uid_t creatorUid,
                                             const sp<Surface>& sinkSurface)
      : mHWC(hwComposer),
        mDisplayId(displayId),
        mName(name),
        mSinkHelper(sp<SinkSurfaceHelper>::make(sinkSurface, creatorUid)) {}

VirtualDisplaySurface::~VirtualDisplaySurface() {
    ALOGI("Shutting down VirtualDisplaySurface %s", mName.c_str());

    mSinkHelper->abandon();
    mRendererConsumer->abandon();
    if (mOutputConsumer) {
        mOutputConsumer->abandon();
    }
    if (mOutputSurface) {
        mOutputSurface->disconnect(NATIVE_WINDOW_API_CPU);
    }
}

void VirtualDisplaySurface::onFirstRef() {
    ATRACE_CALL();
    std::scoped_lock _l(mMutex);

    std::tie(mRendererConsumer, mRendererSurface) = BufferItemConsumer::create(getRendererUsage());
    mRendererListener =
            sp<RenderConsumerListener>::make(sp<VirtualDisplaySurface>::fromExisting(this));
    mRendererConsumer->setFrameAvailableListener(mRendererListener);
    mRendererConsumer->setName(String8(mName + "-RenderBQ"));

    mSinkSurfaceDataFuture = mSinkHelper->connectSinkSurface();

    if (mSinkHelper->isFrozen()) {
        ALOGE("%s: Scheduled surface connection on a frozen helper thread.", __func__);
        return;
    }

    // We only want to wait a short time because this should be a relatively fast operation in good
    // conditions, but we're on the main thread.
    constexpr auto timeToWait = std::chrono::milliseconds(1);
    if (mSinkSurfaceDataFuture.wait_for(timeToWait) == std::future_status::ready) {
        prepareSurfacesLocked();
    } else {
        ALOGW("%s: waited for the sink surface to connect for %lldms and it's not ready.",
              mSinkHelper->getName().c_str(), timeToWait.count());
    }
}

void VirtualDisplaySurface::prepareSurfacesLocked() {
    ATRACE_CALL();

    LOG_ALWAYS_FATAL_IF(!mSinkSurfaceDataFuture.valid(), "%s: called without a valid future.",
                        __func__);

    auto data = mSinkSurfaceDataFuture.get();

    mSinkWidth = data.width;
    mSinkHeight = data.height;
    mSinkFormat = data.format;
    mSinkUsage = data.usage;
    mSinkDataSpace = data.dataSpace;

    // Since the renderer can be used for GPU compositing at any point, make sure we are generating
    // buffers we can send over to the app.
    mRendererConsumer->setConsumerUsageBits(getRendererUsage() | mSinkUsage);

    if (isHalDisplay()) {
        std::tie(mOutputConsumer, mOutputSurface) =
                BufferItemConsumer::create(GRALLOC_USAGE_HW_COMPOSER | mSinkUsage);
        mOutputConsumer->setDefaultBufferFormat(mSinkFormat);
        mOutputConsumer->setDefaultBufferSize(mSinkWidth, mSinkHeight);
        mOutputConsumer->setDefaultBufferDataSpace(static_cast<android_dataspace>(mSinkDataSpace));
        mOutputConsumer->setName(String8(mName + "-OutputBQ"));

        mOutputSurfaceListener = sp<StubSurfaceListener>::make();
        status_t ret = mOutputSurface->connect(NATIVE_WINDOW_API_CPU, mOutputSurfaceListener);
        if (ret != NO_ERROR) {
            ALOGE("%s: Unable to set up output surface listener. Status: %d", __func__, ret);
        }
    }

    mIsReady = true;
}

sp<Surface> VirtualDisplaySurface::getCompositionSurface() const {
    return mRendererSurface;
}

status_t VirtualDisplaySurface::beginFrame(bool mustRecompose) {
    ATRACE_CALL();

    std::scoped_lock _l(mMutex);

    ALOGE_IF(mCurrentFrame != std::nullopt,
             "%s: called while another frame is being processed. Overwriting.", __func__);
    mCurrentFrame.reset();

    if (!mIsReady) {
        ALOGI("%s: attempting to connect to still-unconnected sink surface.", __func__);

        if (mSinkSurfaceDataFuture.valid()) {
            prepareSurfacesLocked();
        } else {
            ALOGW("%s: unable to begin frame because the underlying surface is not connected.",
                  __func__);
            return NO_INIT;
        }
    }

    if (mSinkHelper->isFrozen()) {
        ALOGW("%s: mWorkerThread is frozen! Skipping frame.", __func__);
        return WOULD_BLOCK;
    }

    if (mPendingResize) {
        applyResizeLocked(*mPendingResize);
        mPendingResize.reset();
    }

    mCurrentFrame.emplace(FrameInfo());
    mCurrentFrame->mustRecompose = mustRecompose;

    return OK;
}

status_t VirtualDisplaySurface::prepareFrame(CompositionType compositionType) {
    ATRACE_CALL();

    if (isGpuDisplay() &&
        (compositionType == CompositionType::Hwc || compositionType == CompositionType::Mixed)) {
        ALOGE("%s: called with bad composition type %d on GPU display.", __func__, compositionType);
        return BAD_VALUE;
    }

    std::scoped_lock _l(mMutex);

    if (!mCurrentFrame) {
        ALOGE("%s: called without a current frame.", __func__);
        return INVALID_OPERATION;
    }
    FrameInfo& frameInfo = *mCurrentFrame;
    frameInfo.compositionType = compositionType;

    if (compositionType != CompositionType::Gpu) {
        return OK;
    }

    // For the GPU case, we'll be sending the rendered buffer directly back to the sink with no
    // output in the middle, so we'll attach it to the back of that bufferqueue.
    auto maybeBufferFence =
            mSinkHelper->getDequeuedBuffer(mSinkWidth, mSinkHeight, GRALLOC_USAGE_HW_RENDER);
    if (maybeBufferFence) {
        auto& [buffer, fence] = *maybeBufferFence;
        ALOGI("%s: (%s) Reusing buffer %" PRIu64 " from sink.", __func__,
              ftl::enum_string(compositionType).c_str(), buffer->getId());

        mRendererConsumer->attachBuffer(buffer);
        mRendererConsumer->releaseBuffer(buffer, fence);
    } else {
        ALOGI("%s: (%s) No buffer available from sink.", ftl::enum_string(compositionType).c_str(),
              __func__);
    }

    return OK;
}

status_t VirtualDisplaySurface::advanceFrame(float hdrSdrRatio) {
    ATRACE_CALL();

    std::scoped_lock _l(mMutex);

    if (!mCurrentFrame) {
        ALOGE("advanceFrame called without a current frame.");
        return INVALID_OPERATION;
    }
    FrameInfo& frameInfo = *mCurrentFrame;

    if (isGpuDisplay() || frameInfo.compositionType == CompositionType::Gpu) {
        ALOGD("%s: No work to be done for GPU composition.", __func__);
        return OK;
    }

    auto maybeBufferFence =
            mSinkHelper->getDequeuedBuffer(mSinkWidth, mSinkHeight, GRALLOC_USAGE_HW_COMPOSER);
    if (maybeBufferFence) {
        std::tie(frameInfo.outputBuffer, frameInfo.outputFence) = *maybeBufferFence;

        ALOGI("%s: (%s) Reusing output buffer id=%" PRIu64 " from sink.", __func__,
              ftl::enum_string(frameInfo.compositionType).c_str(), frameInfo.outputBuffer->getId());
    } else {
        status_t status =
                mOutputSurface->dequeueBuffer(&frameInfo.outputBuffer, &frameInfo.outputFence);
        if (status != NO_ERROR) {
            ALOGE("%s: Failed to dequeue output buffer. Status: %d", __func__, status);
            return status;
        }
        status = mOutputSurface->detachBuffer(frameInfo.outputBuffer);
        if (status != NO_ERROR) {
            ALOGE("%s: Failed to detach output buffer. Status: %d", __func__, status);
            return status;
        }

        ALOGI("%s: (%s) Dequeuing a fresh buffer from output surface id=%" PRIu64 " from sink.",
              __func__, ftl::enum_string(frameInfo.compositionType).c_str(),
              frameInfo.outputBuffer->getId());
    }

    auto halDisplayId = std::get<HalVirtualDisplayId>(mDisplayId);
    status_t status =
            mHWC.setOutputBuffer(halDisplayId, frameInfo.outputFence, frameInfo.outputBuffer);
    if (status != NO_ERROR) {
        ALOGE("%s: Failed to set output buffer. Status: %d", __func__, status);
        return status;
    }

    if (frameInfo.compositionType == CompositionType::Mixed) {
        ui::Dataspace dataspace = ui::Dataspace::SRGB; // TODO
        sp<GraphicBuffer>& buffer = frameInfo.clientComposedBufferItem.mGraphicBuffer;
        sp<Fence>& fence = frameInfo.clientComposedBufferItem.mFence;

        auto [shouldSendBuffer, slot] = mSlotTracker.getSlot(buffer);
        status =
                mHWC.setClientTarget(halDisplayId, slot, fence,
                                     (shouldSendBuffer ? buffer : nullptr), dataspace, hdrSdrRatio);
        if (status != NO_ERROR) {
            ALOGE("%s: Failed to set client target buffer. Status: %d", __func__, status);
            return status;
        }
    }

    return OK;
}

void VirtualDisplaySurface::onFrameCommitted() {
    ATRACE_CALL();

    std::scoped_lock _l(mMutex);

    if (!mCurrentFrame) {
        ALOGE("onFrameCommitted called without a current frame.");
        return;
    }

    FrameInfo frameInfo = std::move(mCurrentFrame).value();
    mCurrentFrame.reset();

    // GPU composition is done in onRenderFrameAvailable()
    if (isGpuDisplay() || frameInfo.compositionType == CompositionType::Gpu) {
        return;
    }

    auto halDisplayId = std::get<HalVirtualDisplayId>(mDisplayId);
    sp<Fence> presentFence = mHWC.getPresentFence(halDisplayId);

    // Replace the render buffer so that we can use it in the next frame.
    if (frameInfo.compositionType == CompositionType::Mixed) {
        sp<GraphicBuffer>& renderBuffer = frameInfo.clientComposedBufferItem.mGraphicBuffer;
        sp<Fence>& renderAcquireFence = frameInfo.clientComposedBufferItem.mFence;
        sp<Fence> renderFence =
                Fence::merge("VD Render Acquire/Present", renderAcquireFence, presentFence);

        status_t ret = mRendererConsumer->attachBuffer(renderBuffer);
        if (ret != NO_ERROR) {
            ALOGE("%s: Failed to reattach buffer to render consumer. Status: %d", __func__, ret);
        } else {
            ret = mRendererConsumer->releaseBuffer(renderBuffer, renderFence);
            ALOGE_IF(ret != NO_ERROR,
                     "`%s: Failed to release buffer to render consumer. Status: %d", __func__, ret);
        }
    }

    sp<Fence> outputFence =
            Fence::merge("VD Output Acquire/Present", frameInfo.outputFence, presentFence);

    mSinkHelper->sendBuffer(frameInfo.outputBuffer, frameInfo.outputFence);
}

void VirtualDisplaySurface::dumpAsString(String8& result) const {
    std::scoped_lock _l(mMutex);

    std::string displayIdStr = std::visit([](auto&& arg) { return to_string(arg); }, mDisplayId);
    std::string type = isGpuDisplay() ? "GPU" : "HWC";

    result.append("    VirtualDisplaySurface\n");
    result.appendFormat("        type=%s\n", type.c_str());
    result.appendFormat("        mName=%s\n", mName.c_str());
    result.appendFormat("        mDisplayId=%s\n", displayIdStr.c_str());
    result.appendFormat("        mSinkName=%s\n", mSinkHelper->getName().c_str());
    result.appendFormat("        mSinkFormat=%d\n", mSinkFormat);
    result.appendFormat("        mSinkUsage=%" PRIu64 "\n", mSinkUsage);
    result.appendFormat("        mSinkDataSpace=%d\n", mSinkDataSpace);
    result.appendFormat("        mSinkWidth=%d\n", mSinkWidth);
    result.appendFormat("        mSinkHeight=%d\n", mSinkHeight);
}

void VirtualDisplaySurface::resizeBuffers(const ui::Size& newSize) {
    ATRACE_CALL();

    std::scoped_lock _l(mMutex);
    if (mCurrentFrame) {
        mPendingResize = {newSize};
    } else {
        applyResizeLocked(newSize);
    }
}

uint64_t VirtualDisplaySurface::getRendererUsage() const {
    // If we might be rendering to the HAL, this buffer could be used in HWComposer::setClientTarget
    // in Mixed mode.
    return isHalDisplay() ? (GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_COMPOSER)
                          : GRALLOC_USAGE_HW_RENDER;
}

void VirtualDisplaySurface::applyResizeLocked(const ui::Size& newSize) {
    if (newSize.width < 0 || newSize.height < 0) {
        ALOGE("%s: Called with invalid size %dx%d", __func__, newSize.width, newSize.height);
        return;
    }
    if ((uint32_t)newSize.width == mSinkWidth && (uint32_t)newSize.height == mSinkHeight) {
        return;
    }

    mSinkWidth = (uint32_t)newSize.width;
    mSinkHeight = (uint32_t)newSize.height;

    mSinkHelper->setBufferSize(mSinkWidth, mSinkHeight);

    status_t ret = mOutputConsumer->setDefaultBufferSize(mSinkWidth, mSinkHeight);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to set output consumer default buffer size %dx%d", __func__, mSinkWidth,
              mSinkHeight);
    }

    ret = mOutputSurface->setBuffersDimensions(mSinkWidth, mSinkHeight);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to set output surface buffer size %dx%d", __func__, mSinkWidth,
              mSinkHeight);
    }

    ret = mRendererConsumer->setDefaultBufferSize(mSinkWidth, mSinkHeight);
    if (ret != NO_ERROR) {
        ALOGE("%s: Unable to set render default buffer size %dx%d", __func__, mSinkWidth,
              mSinkHeight);
    }
}

const sp<Fence>& VirtualDisplaySurface::getClientTargetAcquireFence() const {
    std::scoped_lock _l(mMutex);

    if (!mCurrentFrame) {
        return Fence::NO_FENCE;
    }

    return mCurrentFrame->clientComposedBufferItem.mFence
            ? mCurrentFrame->clientComposedBufferItem.mFence
            : Fence::NO_FENCE;
}

void VirtualDisplaySurface::onRenderFrameAvailable() {
    ATRACE_CALL();

    std::scoped_lock _l(mMutex);

    // No matter what, we always want to acquire the buffer, even if we're not officially doing a
    // frame. The compositor will continue even if the earlier steps fail.
    BufferItem item;
    status_t ret = mRendererConsumer->acquireBuffer(&item,
                                                    /*presentWhen*/ -1,
                                                    /*waitForFence*/ false);
    if (ret != NO_ERROR) {
        ALOGE("%s: Failed to acquire the buffer queued to the render surface. Status: %d", __func__,
              ret);
        return;
    }

    if (!mCurrentFrame) {
        ALOGE("Notified that render frame is available without a pending frame.");
        ret = mRendererConsumer->releaseBuffer(item.mGraphicBuffer, item.mFence);
        if (ret != NO_ERROR) {
            ALOGE("%s: Failed to release the buffer queued to the render surface. Status: %d",
                  __func__, ret);
        }
        return;
    }
    FrameInfo& frameInfo = *mCurrentFrame;

    ret = mRendererConsumer->detachBuffer(item.mGraphicBuffer);
    if (ret != NO_ERROR) {
        ALOGE("%s: Failed to detach the buffer queued to the render surface. Status: %d", __func__,
              ret);
        return;
    }

    if (frameInfo.compositionType != CompositionType::Gpu) {
        ALOGI("%s: HWC composition, storing buffer %" PRIu64 " for later", __func__,
              item.mGraphicBuffer->getId());
        frameInfo.clientComposedBufferItem = std::move(item);
        return;
    }

    sp<GraphicBuffer> buffer = item.mGraphicBuffer;
    sp<Fence> fence = item.mFence;

    ALOGI("%s: Preparing to submit frame to %s. BufferId=%" PRIu64, __func__,
          mSinkHelper->getName().c_str(), buffer->getId());

    mSinkHelper->sendBuffer(buffer, fence);
}

} // namespace android
