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
#include <compositionengine/DisplaySurface.h>
#include <gui/BufferItem.h>
#include <gui/Surface.h>
#include <system/graphics.h>
#include <ui/DisplayId.h>
#include <ui/PixelFormat.h>
#include <ui/Size.h>
#include <utils/Errors.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <variant>

#include "SinkSurfaceHelper.h"
#include "VirtualDisplayBufferSlotTracker.h"

namespace android {

class BufferItemConsumer;
class Fence;
class GraphicBuffer;
class HWComposer;
class Surface;

/**
 * A VirtualDisplaySurface2 provides a "surface" for compositing. Compositing to
 * this surface ultimately sends buffers to an application over the sinkSurface
 * provided at construction time.
 *
 * In addition to the lifecycle management for every frame provided by the implementation of
 * compositionengine::DisplaySurface, the compositor needs a Surface/ANativeWindow to actually
 * render to. This is provided by VirtualDisplaySurface2::getCompositionSurface.
 *
 * Virtual Display composition can happen in several different types (see CompositionType):
 *   - Hwc: All layers are composited in the HardwareComposer itself.
 *   - Gpu: All layers are composited on the GPU in CompositionEngine/RenderEngine.
 *   - Mixed: Some layers are composited on the GPU and are combined with others composited in
 *   HardwareComposer.
 *
 * Hwc compositing is pretty straightforward. An output buffer has to be set in HWC for rendering in
 * advanceFrame, which will be sent to the application with the present fence in onFrameCommitted.
 *
 * Gpu compositing is a little more complicated: CompositionEngine dequeues a buffer from the
 * composition surface, does all the composition on the GPU, and queues it. Then we take the buffer
 * and its fence out of the composition surface (in a SurfaceListener::onFrameAvailable callback)
 * and send it to the application.
 *
 * Mixed compositing is the most complicated. Both composition paths are taken, some number of
 * layers are composed on the GPU and provided to us by the composition surface. We set that in a
 * special field in HWC during advanceFrame. The output buffer and present fence are sent to the app
 * after the frame's committed to HWC.
 *
 * To manage buffers, we do a certain amount of "buffer juggling" to promote recycling and reuse.
 * There are three main BQs at play:
 *   - Sink BQ: A surface that is provided by the application at creation time. The purpose of a
 *   virtual display is do compositing and send buffers to the sink.
 *   - Render BQ: A surface that is provided to composition engine for GPU rendering.
 *   VirtualDisplaySurface2 owns the consumer. In Gpu, buffers are taken out of this queue and sent
 *   to the application.
 *   - Output BQ: A surface that is used to provide buffers for HWC outputs. VirtualDisplaySurface2
 *   owns both the surface and consumer. In Mixed and Hwc modes, buffers are taken out of this
 *   queue and sent to the application.
 *
 * We try to reuse buffers dequeued from the Sink BQ as often as possible to avoid having to send
 * new buffers over IPC to the application too frequently.
 */
class VirtualDisplaySurface : public compositionengine::DisplaySurface {
public:
    VirtualDisplaySurface(HWComposer& hwComposer, VirtualDisplayIdVariant displayId,
                          const std::string& name, uid_t creatorUid,
                          const sp<Surface>& sinkSurface);
    virtual ~VirtualDisplaySurface() override;

    void onFirstRef() override;

    sp<Surface> getCompositionSurface() const;

    // compositionengine::DisplaySurface
    virtual status_t beginFrame(bool mustRecompose) override;
    virtual status_t prepareFrame(CompositionType) override;
    virtual status_t advanceFrame(float hdrSdrRatio) override;
    virtual void onFrameCommitted() override;
    virtual void dumpAsString(String8& result) const override;
    virtual void resizeBuffers(const ui::Size&) override;
    virtual const sp<Fence>& getClientTargetAcquireFence() const override;
    virtual bool supportsCompositionStrategyPrediction() const override { return false; }

private:
    class RenderConsumerListener;

    struct FrameInfo {
        FrameInfo() = default;
        FrameInfo(FrameInfo&&) = default;
        FrameInfo& operator=(FrameInfo&&) = default;

        // This object is move-only, non-copyable.
        FrameInfo(const FrameInfo&) = delete;
        FrameInfo& operator=(const FrameInfo&) = delete;

        // What type of composition is being done in this frame, set by beginFrame.
        CompositionType compositionType = CompositionType::Unknown;
        // Whether we must recompose no matter what, set by beginFrame.
        bool mustRecompose = false;
        BufferItem clientComposedBufferItem = {};
        sp<GraphicBuffer> outputBuffer = nullptr;
        sp<Fence> outputFence = nullptr;
    };

    bool isGpuDisplay() const { return std::holds_alternative<GpuVirtualDisplayId>(mDisplayId); };
    bool isHalDisplay() const { return std::holds_alternative<HalVirtualDisplayId>(mDisplayId); };

    uint64_t getRendererUsage() const;

    void applyResizeLocked(const ui::Size& size) REQUIRES(mMutex);

    void prepareSurfacesLocked() REQUIRES(mMutex);

    void onRenderFrameAvailable();

    mutable std::mutex mMutex;
    HWComposer& mHWC;
    const VirtualDisplayIdVariant mDisplayId;
    const std::string mName;

    sp<SinkSurfaceHelper> mSinkHelper;
    std::future<SinkSurfaceHelper::SinkSurfaceData> mSinkSurfaceDataFuture;
    bool mIsReady = false;
    uint64_t mSinkUsage;
    PixelFormat mSinkFormat;
    ADataSpace mSinkDataSpace;
    uint32_t mSinkWidth;
    uint32_t mSinkHeight;
    std::optional<ui::Size> mPendingResize;
    VirtualDisplayBufferSlotTracker mSlotTracker;

    sp<BufferItemConsumer> mRendererConsumer;
    sp<RenderConsumerListener> mRendererListener;
    sp<Surface> mRendererSurface;

    sp<BufferItemConsumer> mOutputConsumer = nullptr;
    sp<Surface> mOutputSurface = nullptr;
    sp<SurfaceListener> mOutputSurfaceListener = nullptr;

    std::optional<FrameInfo> mCurrentFrame;
};

} // namespace android
