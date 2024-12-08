/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include <android/gui/DropInputMode.h>
#include <android/gui/ISurfaceComposerClient.h>
#include <ftl/small_map.h>
#include <gui/BufferQueue.h>
#include <gui/LayerState.h>
#include <gui/WindowInfo.h>
#include <layerproto/LayerProtoHeader.h>
#include <math/vec4.h>
#include <sys/types.h>
#include <ui/BlurRegion.h>
#include <ui/DisplayMap.h>
#include <ui/FloatRect.h>
#include <ui/FrameStats.h>
#include <ui/GraphicBuffer.h>
#include <ui/LayerStack.h>
#include <ui/PixelFormat.h>
#include <ui/Region.h>
#include <ui/StretchEffect.h>
#include <ui/Transform.h>
#include <utils/RefBase.h>
#include <utils/Timers.h>

#include <compositionengine/LayerFE.h>
#include <compositionengine/LayerFECompositionState.h>
#include <scheduler/Fps.h>
#include <scheduler/Seamlessness.h>

#include <cstdint>
#include <optional>
#include <vector>

#include "Client.h"
#include "DisplayHardware/HWComposer.h"
#include "FrameTracker.h"
#include "LayerFE.h"
#include "LayerVector.h"
#include "Scheduler/LayerInfo.h"
#include "SurfaceFlinger.h"
#include "TransactionCallbackInvoker.h"

using namespace android::surfaceflinger;

namespace android {

class Client;
class Colorizer;
class DisplayDevice;
class GraphicBuffer;
class SurfaceFlinger;

namespace compositionengine {
class OutputLayer;
struct LayerFECompositionState;
}

namespace frametimeline {
class SurfaceFrame;
} // namespace frametimeline

class Layer : public virtual RefBase {
public:
    // The following constants represent priority of the window. SF uses this information when
    // deciding which window has a priority when deciding about the refresh rate of the screen.
    // Priority 0 is considered the highest priority. -1 means that the priority is unset.
    static constexpr int32_t PRIORITY_UNSET = -1;
    // Windows that are in focus and voted for the preferred mode ID
    static constexpr int32_t PRIORITY_FOCUSED_WITH_MODE = 0;
    // // Windows that are in focus, but have not requested a specific mode ID.
    static constexpr int32_t PRIORITY_FOCUSED_WITHOUT_MODE = 1;
    // Windows that are not in focus, but voted for a specific mode ID.
    static constexpr int32_t PRIORITY_NOT_FOCUSED_WITH_MODE = 2;

    using FrameRate = scheduler::LayerInfo::FrameRate;
    using FrameRateCompatibility = scheduler::FrameRateCompatibility;
    using FrameRateSelectionStrategy = scheduler::LayerInfo::FrameRateSelectionStrategy;

    struct State {
        int32_t sequence; // changes when visible regions can change
        // Crop is expressed in layer space coordinate.
        Rect crop;
        LayerMetadata metadata;

        ui::Dataspace dataspace;

        uint64_t frameNumber;
        uint64_t previousFrameNumber;
        // high watermark framenumber to use to check for barriers to protect ourselves
        // from out of order transactions
        uint64_t barrierFrameNumber;
        ui::Transform transform;

        uint32_t producerId = 0;
        // high watermark producerId to use to check for barriers to protect ourselves
        // from out of order transactions
        uint32_t barrierProducerId = 0;

        uint32_t bufferTransform;
        bool transformToDisplayInverse;
        Region transparentRegionHint;
        std::shared_ptr<renderengine::ExternalTexture> buffer;
        sp<Fence> acquireFence;
        std::shared_ptr<FenceTime> acquireFenceTime;
        sp<NativeHandle> sidebandStream;
        mat4 colorTransform;

        // The deque of callback handles for this frame. The back of the deque contains the most
        // recent callback handle.
        std::deque<sp<CallbackHandle>> callbackHandles;
        nsecs_t desiredPresentTime = 0;
        bool isAutoTimestamp = true;

        // The combined frame rate of parents / children of this layer
        FrameRate frameRateForLayerTree;

        // The vsync info that was used to start the transaction
        FrameTimelineInfo frameTimelineInfo;

        // When the transaction was posted
        nsecs_t postTime;
        sp<ITransactionCompletedListener> releaseBufferListener;
        // SurfaceFrame that tracks the timeline of Transactions that contain a Buffer. Only one
        // such SurfaceFrame exists because only one buffer can be presented on the layer per vsync.
        // If multiple buffers are queued, the prior ones will be dropped, along with the
        // SurfaceFrame that's tracking them.
        std::shared_ptr<frametimeline::SurfaceFrame> bufferSurfaceFrameTX;
        // A map of token(frametimelineVsyncId) to the SurfaceFrame that's tracking a transaction
        // that contains the token. Only one SurfaceFrame exisits for transactions that share the
        // same token, unless they are presented in different vsyncs.
        std::unordered_map<int64_t, std::shared_ptr<frametimeline::SurfaceFrame>>
                bufferlessSurfaceFramesTX;
        // An arbitrary threshold for the number of BufferlessSurfaceFrames in the state. Used to
        // trigger a warning if the number of SurfaceFrames crosses the threshold.
        static constexpr uint32_t kStateSurfaceFramesThreshold = 25;
        Rect bufferCrop;
        Rect destinationFrame;
        sp<IBinder> releaseBufferEndpoint;
        bool autoRefresh = false;
        float currentHdrSdrRatio = 1.f;
        float desiredHdrSdrRatio = -1.f;
        int64_t latchedVsyncId = 0;
        bool useVsyncIdForRefreshRateSelection = false;
    };

    explicit Layer(const surfaceflinger::LayerCreationArgs& args);
    virtual ~Layer();

    static bool isLayerFocusedBasedOnPriority(int32_t priority);
    static void miniDumpHeader(std::string& result);

    // This second set of geometry attributes are controlled by
    // setGeometryAppliesWithResize, and their default mode is to be
    // immediate. If setGeometryAppliesWithResize is specified
    // while a resize is pending, then update of these attributes will
    // be delayed until the resize completes.

    // Buffer space
    bool setCrop(const Rect& crop);

    bool setTransform(uint32_t /*transform*/);
    bool setTransformToDisplayInverse(bool /*transformToDisplayInverse*/);
    bool setBuffer(std::shared_ptr<renderengine::ExternalTexture>& /* buffer */,
                   const BufferData& /* bufferData */, nsecs_t /* postTime */,
                   nsecs_t /*desiredPresentTime*/, bool /*isAutoTimestamp*/,
                   const FrameTimelineInfo& /*info*/, gui::GameMode gameMode);
    void setDesiredPresentTime(nsecs_t /*desiredPresentTime*/, bool /*isAutoTimestamp*/);
    bool setDataspace(ui::Dataspace /*dataspace*/);
    bool setExtendedRangeBrightness(float currentBufferRatio, float desiredRatio);
    bool setDesiredHdrHeadroom(float desiredRatio);
    bool setSidebandStream(const sp<NativeHandle>& /*sidebandStream*/,
                           const FrameTimelineInfo& /* info*/, nsecs_t /* postTime */,
                           gui::GameMode gameMode);
    bool setTransactionCompletedListeners(const std::vector<sp<CallbackHandle>>& /*handles*/,
                                          bool willPresent);

    sp<LayerFE> getCompositionEngineLayerFE(const frontend::LayerHierarchy::TraversalPath&);

    // If we have received a new buffer this frame, we will pass its surface
    // damage down to hardware composer. Otherwise, we must send a region with
    // one empty rect.
    Region getVisibleRegion(const DisplayDevice*) const;
    void updateLastLatchTime(nsecs_t latchtime);

    Rect getCrop(const Layer::State& s) const { return s.crop; }

    // from graphics API
    static ui::Dataspace translateDataspace(ui::Dataspace dataspace);
    uint64_t mPreviousFrameNumber = 0;

    void onCompositionPresented(const DisplayDevice*,
                                const std::shared_ptr<FenceTime>& /*glDoneFence*/,
                                const std::shared_ptr<FenceTime>& /*presentFence*/,
                                const CompositorTiming&, gui::GameMode gameMode);

    // If a buffer was replaced this frame, release the former buffer
    void releasePendingBuffer(nsecs_t /*dequeueReadyTime*/);

    /*
     * latchBuffer - called each time the screen is redrawn and returns whether
     * the visible regions need to be recomputed (this is a fairly heavy
     * operation, so this should be set only if needed). Typically this is used
     * to figure out if the content or size of a surface has changed.
     */
    bool latchBufferImpl(bool& /*recomputeVisibleRegions*/, nsecs_t /*latchTime*/,
                         bool bgColorOnly);

    sp<GraphicBuffer> getBuffer() const;
    /**
     * Returns active buffer size in the correct orientation. Buffer size is determined by undoing
     * any buffer transformations. Returns Rect::INVALID_RECT if the layer has no buffer or the
     * layer does not have a display frame and its parent is not bounded.
     */
    Rect getBufferSize(const Layer::State&) const;

    FrameRate getFrameRateForLayerTree() const;

    bool getTransformToDisplayInverse() const;

    // Implements RefBase.
    void onFirstRef() override;

    struct BufferInfo {
        nsecs_t mDesiredPresentTime;
        std::shared_ptr<FenceTime> mFenceTime;
        sp<Fence> mFence;
        uint32_t mTransform{0};
        ui::Dataspace mDataspace{ui::Dataspace::UNKNOWN};
        Rect mCrop;
        PixelFormat mPixelFormat{PIXEL_FORMAT_NONE};
        bool mTransformToDisplayInverse{false};
        std::shared_ptr<renderengine::ExternalTexture> mBuffer;
        uint64_t mFrameNumber;
        sp<IBinder> mReleaseBufferEndpoint;
        bool mFrameLatencyNeeded{false};
        float mDesiredHdrSdrRatio = -1.f;
    };

    BufferInfo mBufferInfo;
    std::shared_ptr<gui::BufferReleaseChannel::ProducerEndpoint> mBufferReleaseChannel;

    bool fenceHasSignaled() const;
    void onPreComposition(nsecs_t refreshStartTime);
    void onLayerDisplayed(ftl::SharedFuture<FenceResult>, ui::LayerStack layerStack,
                          std::function<FenceResult(FenceResult)>&& continuation = nullptr);

    // Tracks mLastClientCompositionFence and gets the callback handle for this layer.
    sp<CallbackHandle> findCallbackHandle();

    // Adds the future release fence to a list of fences that are used to release the
    // last presented buffer. Also keeps track of the layerstack in a list of previous
    // layerstacks that have been presented.
    void prepareReleaseCallbacks(ftl::Future<FenceResult>, ui::LayerStack layerStack);

    void setWasClientComposed(const sp<Fence>& fence) {
        mLastClientCompositionFence = fence;
        mClearClientCompositionFenceOnLayerDisplayed = false;
    }

    const char* getDebugName() const;

    static bool computeTrustedPresentationState(const FloatRect& bounds,
                                                const FloatRect& sourceBounds,
                                                const Region& coveredRegion,
                                                const FloatRect& screenBounds, float,
                                                const ui::Transform&,
                                                const TrustedPresentationThresholds&);
    void updateTrustedPresentationState(const DisplayDevice* display,
                                        const frontend::LayerSnapshot* snapshot, int64_t time_in_ms,
                                        bool leaveState);

    inline bool hasTrustedPresentationListener() {
        return mTrustedPresentationListener.callbackInterface != nullptr;
    }

    // Sets the masked bits.
    void setTransactionFlags(uint32_t mask);

    int32_t getSequence() const { return sequence; }

    // For tracing.
    // TODO: Replace with raw buffer id from buffer metadata when that becomes available.
    // GraphicBuffer::getId() does not provide a reliable global identifier. Since the traces
    // creates its tracks by buffer id and has no way of associating a buffer back to the process
    // that created it, the current implementation is only sufficient for cases where a buffer is
    // only used within a single layer.
    uint64_t getCurrentBufferId() const { return getBuffer() ? getBuffer()->getId() : 0; }

    void writeCompositionStateToProto(perfetto::protos::LayerProto* layerProto,
                                      ui::LayerStack layerStack);

    inline const State& getDrawingState() const { return mDrawingState; }
    inline State& getDrawingState() { return mDrawingState; }

    void miniDump(std::string& result, const frontend::LayerSnapshot&, const DisplayDevice&) const;
    void dumpFrameStats(std::string& result) const;
    void clearFrameStats();
    void logFrameStats();
    void getFrameStats(FrameStats* outStats) const;
    void onDisconnect();

    bool onHandleDestroyed() { return mHandleAlive = false; }

    /**
     * Returns the cropped buffer size or the layer crop if the layer has no buffer. Return
     * INVALID_RECT if the layer has no buffer and no crop.
     * A layer with an invalid buffer size and no crop is considered to be boundless. The layer
     * bounds are constrained by its parent bounds.
     */
    Rect getCroppedBufferSize(const Layer::State& s) const;

    void setFrameTimelineVsyncForBufferTransaction(const FrameTimelineInfo& info, nsecs_t postTime,
                                                   gui::GameMode gameMode);
    void setFrameTimelineVsyncForBufferlessTransaction(const FrameTimelineInfo& info,
                                                       nsecs_t postTime, gui::GameMode gameMode);

    void addSurfaceFrameDroppedForBuffer(std::shared_ptr<frametimeline::SurfaceFrame>& surfaceFrame,
                                         nsecs_t dropTime);
    void addSurfaceFramePresentedForBuffer(
            std::shared_ptr<frametimeline::SurfaceFrame>& surfaceFrame, nsecs_t acquireFenceTime,
            nsecs_t currentLatchTime);

    std::shared_ptr<frametimeline::SurfaceFrame> createSurfaceFrameForTransaction(
            const FrameTimelineInfo& info, nsecs_t postTime, gui::GameMode gameMode);
    std::shared_ptr<frametimeline::SurfaceFrame> createSurfaceFrameForBuffer(
            const FrameTimelineInfo& info, nsecs_t queueTime, std::string debugName,
            gui::GameMode gameMode);
    void setFrameTimelineVsyncForSkippedFrames(const FrameTimelineInfo& info, nsecs_t postTime,
                                               std::string debugName, gui::GameMode gameMode);

    bool setTrustedPresentationInfo(TrustedPresentationThresholds const& thresholds,
                                    TrustedPresentationListener const& listener);
    void setBufferReleaseChannel(
            const std::shared_ptr<gui::BufferReleaseChannel::ProducerEndpoint>& channel);

    // Creates a new handle each time, so we only expect
    // this to be called once.
    sp<IBinder> getHandle();
    const std::string& getName() const { return mName; }

    virtual uid_t getOwnerUid() const { return mOwnerUid; }

    // Used to check if mUsedVsyncIdForRefreshRateSelection should be expired when it stop updating.
    nsecs_t mMaxTimeForUseVsyncId = 0;
    // True when DrawState.useVsyncIdForRefreshRateSelection previously set to true during updating
    // buffer.
    bool mUsedVsyncIdForRefreshRateSelection{false};

    // Layer serial number.  This gives layers an explicit ordering, so we
    // have a stable sort order when their layer stack and Z-order are
    // the same.
    const int32_t sequence;

    // See mPendingBufferTransactions
    void decrementPendingBufferCount();
    std::atomic<int32_t>* getPendingBufferCounter() { return &mPendingBufferTransactions; }
    std::string getPendingBufferCounterName() { return mBlastTransactionName; }
    void callReleaseBufferCallback(const sp<ITransactionCompletedListener>& listener,
                                   const sp<GraphicBuffer>& buffer, uint64_t framenumber,
                                   const sp<Fence>& releaseFence);
    bool setFrameRateForLayerTree(FrameRate, const scheduler::LayerProps&, nsecs_t now);
    void recordLayerHistoryBufferUpdate(const scheduler::LayerProps&, nsecs_t now);
    void recordLayerHistoryAnimationTx(const scheduler::LayerProps&, nsecs_t now);
    bool hasBuffer() const { return mBufferInfo.mBuffer != nullptr; }
    void setTransformHint(std::optional<ui::Transform::RotationFlags> transformHint) {
        mTransformHint = transformHint;
    }
    void commitTransaction();
    // Keeps track of the previously presented layer stacks. This is used to get
    // the release fences from the correct displays when we release the last buffer
    // from the layer.
    std::vector<ui::LayerStack> mPreviouslyPresentedLayerStacks;

    struct FenceAndContinuation {
        ftl::SharedFuture<FenceResult> future;
        std::function<FenceResult(FenceResult)> continuation;

        ftl::SharedFuture<FenceResult> chain() const {
            if (continuation) {
                return ftl::Future(future).then(continuation).share();
            } else {
                return future;
            }
        }
    };
    std::vector<FenceAndContinuation> mPreviousReleaseFenceAndContinuations;

    // Release fences for buffers that have not yet received a release
    // callback. A release callback may not be given when capturing
    // screenshots asynchronously. There may be no buffer update for the
    // layer, but the layer will still be composited on the screen in every
    // frame. Kepping track of these fences ensures that they are not dropped
    // and can be dispatched to the client at a later time. Older fences are
    // dropped when a layer stack receives a new fence.
    // TODO(b/300533018): Track fence per multi-instance RenderEngine
    ftl::SmallMap<ui::LayerStack, ftl::Future<FenceResult>, ui::kDisplayCapacity>
            mAdditionalPreviousReleaseFences;

    // Exposed so SurfaceFlinger can assert that it's held
    const sp<SurfaceFlinger> mFlinger;

    // Check if the damage region is a small dirty.
    void setIsSmallDirty(frontend::LayerSnapshot* snapshot);

protected:
    // For unit tests
    friend class TestableSurfaceFlinger;
    friend class FpsReporterTest;
    friend class RefreshRateSelectionTest;
    friend class SetFrameRateTest;
    friend class TransactionFrameTracerTest;
    friend class TransactionSurfaceFrameTest;

    void gatherBufferInfo();

    compositionengine::OutputLayer* findOutputLayerForDisplay(const DisplayDevice*) const;
    compositionengine::OutputLayer* findOutputLayerForDisplay(
            const DisplayDevice*, const frontend::LayerHierarchy::TraversalPath& path) const;

    const std::string mName;
    const std::string mTransactionName{"TX - " + mName};

    // These are only accessed by the main thread.
    State mDrawingState;

    TrustedPresentationThresholds mTrustedPresentationThresholds;
    TrustedPresentationListener mTrustedPresentationListener;
    bool mLastComputedTrustedPresentationState = false;
    bool mLastReportedTrustedPresentationState = false;
    int64_t mEnteredTrustedPresentationStateTime = -1;

    uint32_t mTransactionFlags{0};

    // Timestamp history for UIAutomation. Thread safe.
    FrameTracker mFrameTracker;

    // main thread
    sp<NativeHandle> mSidebandStream;

    // We encode unset as -1.
    std::atomic<uint64_t> mCurrentFrameNumber{0};

    // protected by mLock
    mutable Mutex mLock;

    // This layer can be a cursor on some displays.
    bool mPotentialCursor{false};

    // Window types from WindowManager.LayoutParams
    const gui::WindowInfo::Type mWindowType;

    // The owner of the layer. If created from a non system process, it will be the calling uid.
    // If created from a system process, the value can be passed in.
    uid_t mOwnerUid;

    // The owner pid of the layer. If created from a non system process, it will be the calling pid.
    // If created from a system process, the value can be passed in.
    pid_t mOwnerPid;

    int32_t mOwnerAppId;

    // Keeps track of the time SF latched the last buffer from this layer.
    // Used in buffer stuffing analysis in FrameTimeline.
    nsecs_t mLastLatchTime = 0;

    sp<Fence> mLastClientCompositionFence;
    bool mClearClientCompositionFenceOnLayerDisplayed = false;
private:
    // Range of uids allocated for a user.
    // This value is taken from android.os.UserHandle#PER_USER_RANGE.
    static constexpr int32_t PER_USER_RANGE = 100000;

    friend class SlotGenerationTest;
    friend class TransactionFrameTracerTest;
    friend class TransactionSurfaceFrameTest;

    bool getSidebandStreamChanged() const { return mSidebandStreamChanged; }

    std::atomic<bool> mSidebandStreamChanged{false};

    aidl::android::hardware::graphics::composer3::Composition getCompositionType(
            const DisplayDevice&) const;
    aidl::android::hardware::graphics::composer3::Composition getCompositionType(
            const compositionengine::OutputLayer*) const;

    inline void tracePendingBufferCount(int32_t pendingBuffers);

    // Latch sideband stream and returns true if the dirty region should be updated.
    bool latchSidebandStream(bool& recomputeVisibleRegions);

    void updateTexImage(nsecs_t latchTime, bool bgColorOnly = false);

    // Crop that applies to the buffer
    Rect computeBufferCrop(const State& s);

    void callReleaseBufferCallback(const sp<ITransactionCompletedListener>& listener,
                                   const sp<GraphicBuffer>& buffer, uint64_t framenumber,
                                   const sp<Fence>& releaseFence,
                                   uint32_t currentMaxAcquiredBufferCount);

    bool hasBufferOrSidebandStream() const {
        return ((mSidebandStream != nullptr) || (mBufferInfo.mBuffer != nullptr));
    }

    bool hasBufferOrSidebandStreamInDrawing() const {
        return ((mDrawingState.sidebandStream != nullptr) || (mDrawingState.buffer != nullptr));
    }

    bool mGetHandleCalled = false;

    // The inherited shadow radius after taking into account the layer hierarchy. This is the
    // final shadow radius for this layer. If a shadow is specified for a layer, then effective
    // shadow radius is the set shadow radius, otherwise its the parent's shadow radius.
    float mEffectiveShadowRadius = 0.f;

    // Game mode for the layer. Set by WindowManagerShell and recorded by SurfaceFlingerStats.
    gui::GameMode mGameMode = gui::GameMode::Unsupported;

    bool mIsAtRoot = false;

    uint32_t mLayerCreationFlags;

    void releasePreviousBuffer();
    void resetDrawingStateBufferInfo();

    // Transform hint provided to the producer. This must be accessed holding
    // the mStateLock.
    std::optional<ui::Transform::RotationFlags> mTransformHint = std::nullopt;

    ReleaseCallbackId mPreviousReleaseCallbackId = ReleaseCallbackId::INVALID_ID;
    sp<IBinder> mPreviousReleaseBufferEndpoint;

    bool mReleasePreviousBuffer = false;

    // Stores the last set acquire fence signal time used to populate the callback handle's acquire
    // time.
    std::variant<nsecs_t, sp<Fence>> mCallbackHandleAcquireTimeOrFence = -1;

    const std::string mBlastTransactionName{"BufferTX - " + mName};
    // This integer is incremented everytime a buffer arrives at the server for this layer,
    // and decremented when a buffer is dropped or latched. When changed the integer is exported
    // to systrace with SFTRACE_INT and mBlastTransactionName. This way when debugging perf it is
    // possible to see when a buffer arrived at the server, and in which frame it latched.
    //
    // You can understand the trace this way:
    //     - If the integer increases, a buffer arrived at the server.
    //     - If the integer decreases in latchBuffer, that buffer was latched
    //     - If the integer decreases in setBuffer, a buffer was dropped
    std::atomic<int32_t> mPendingBufferTransactions{0};

    // Contains requested position and matrix updates. This will be applied if the client does
    // not specify a destination frame.
    ui::Transform mRequestedTransform;

    std::vector<std::pair<frontend::LayerHierarchy::TraversalPath, sp<LayerFE>>> mLayerFEs;
    bool mHandleAlive = false;
};

std::ostream& operator<<(std::ostream& stream, const Layer::FrameRate& rate);

} // namespace android
