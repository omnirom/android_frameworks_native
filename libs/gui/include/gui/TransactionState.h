/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <android/gui/FrameTimelineInfo.h>
#include <binder/Parcelable.h>
#include <gui/LayerState.h>

namespace android {

// Class to store all the transaction data and the parcelling logic
class TransactionState {
public:
    explicit TransactionState() = default;
    TransactionState(TransactionState&& other) = default;
    TransactionState& operator=(TransactionState&& other) = default;
    status_t writeToParcel(Parcel* parcel) const;
    status_t readFromParcel(const Parcel* parcel);
    layer_state_t* getLayerState(const sp<SurfaceControl>& sc);
    DisplayState& getDisplayState(const sp<IBinder>& token);

    // Returns the current id of the transaction.
    // The id is updated every time the transaction is applied.
    uint64_t getId() const { return mId; }
    std::vector<uint64_t> getMergedTransactionIds() const { return mMergedTransactionIds; }
    void enableDebugLogCallPoints() { mLogCallPoints = true; }
    void merge(TransactionState&& other,
               const std::function<void(layer_state_t&)>& onBufferOverwrite);

    // copied from FrameTimelineInfo::merge()
    void mergeFrameTimelineInfo(const FrameTimelineInfo& other);
    void clear();
    bool operator==(const TransactionState& rhs) const = default;
    bool operator!=(const TransactionState& rhs) const = default;

    uint64_t mId = 0;
    std::vector<uint64_t> mMergedTransactionIds;
    uint32_t mFlags = 0;
    // The vsync id provided by Choreographer.getVsyncId and the input event id
    gui::FrameTimelineInfo mFrameTimelineInfo;
    // mDesiredPresentTime is the time in nanoseconds that the client would like the transaction
    // to be presented. When it is not possible to present at exactly that time, it will be
    // presented after the time has passed.
    //
    // If the client didn't pass a desired presentation time, mDesiredPresentTime will be
    // populated to the time setBuffer was called, and mIsAutoTimestamp will be set to true.
    //
    // Desired present times that are more than 1 second in the future may be ignored.
    // When a desired present time has already passed, the transaction will be presented as soon
    // as possible.
    //
    // Transactions from the same process are presented in the same order that they are applied.
    // The desired present time does not affect this ordering.
    int64_t mDesiredPresentTime = 0;
    bool mIsAutoTimestamp = true;
    // If not null, transactions will be queued up using this token otherwise a common token
    // per process will be used.
    sp<IBinder> mApplyToken;
    // Indicates that the Transaction may contain buffers that should be cached. The reason this
    // is only a guess is that buffers can be removed before cache is called. This is only a
    // hint that at some point a buffer was added to this transaction before apply was called.
    bool mMayContainBuffer = false;
    // Prints debug logs when enabled.
    bool mLogCallPoints = false;

    std::vector<DisplayState> mDisplayStates;
    std::vector<ComposerState> mComposerStates;
    InputWindowCommands mInputWindowCommands;
    std::vector<client_cache_t> mUncacheBuffers;
    // Note: mHasListenerCallbacks can be true even if mListenerCallbacks is
    // empty.
    bool mHasListenerCallbacks = false;
    std::vector<ListenerCallbacks> mListenerCallbacks;

private:
    explicit TransactionState(TransactionState const& other) = default;
    friend class TransactionApplicationTest;
    friend class SurfaceComposerClient;
    // We keep track of the last MAX_MERGE_HISTORY_LENGTH merged transaction ids.
    // Ordered most recently merged to least recently merged.
    static constexpr size_t MAX_MERGE_HISTORY_LENGTH = 10u;
};

}; // namespace android
