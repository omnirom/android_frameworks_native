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

#define LOG_TAG "TransactionState"
#include <gui/LayerState.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/TransactionState.h>
#include <private/gui/ParcelUtils.h>
#include <algorithm>
#include <numeric>

namespace android {

void TransactionListenerCallbacks::clear() {
    *this = TransactionListenerCallbacks();
}

status_t TransactionListenerCallbacks::writeToParcel(Parcel* parcel) const {
    SAFE_PARCEL(parcel->writeBool, mHasListenerCallbacks);
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mFlattenedListenerCallbacks.size()));
    for (const auto& [listener, callbackIds] : mFlattenedListenerCallbacks) {
        SAFE_PARCEL(parcel->writeStrongBinder, listener);
        SAFE_PARCEL(parcel->writeParcelableVector, callbackIds);
    }

    return NO_ERROR;
}

status_t TransactionListenerCallbacks::readFromParcel(const Parcel* parcel) {
    SAFE_PARCEL(parcel->readBool, &mHasListenerCallbacks);
    uint32_t count;
    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize());
    mFlattenedListenerCallbacks.clear();
    mFlattenedListenerCallbacks.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        sp<IBinder> tmpBinder;
        SAFE_PARCEL(parcel->readStrongBinder, &tmpBinder);
        std::vector<CallbackId> callbackIds;
        SAFE_PARCEL(parcel->readParcelableVector, &callbackIds);
        mFlattenedListenerCallbacks.emplace_back(tmpBinder, callbackIds);
    }

    return NO_ERROR;
}

status_t TransactionState::writeToParcel(Parcel* parcel) const {
    SAFE_PARCEL(parcel->writeUint64, mId);
    SAFE_PARCEL(parcel->writeUint32, mFlags);
    SAFE_PARCEL(parcel->writeInt64, mDesiredPresentTime);
    SAFE_PARCEL(parcel->writeBool, mIsAutoTimestamp);

    SAFE_PARCEL(parcel->writeParcelable, mFrameTimelineInfo);

    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mUncacheBuffers.size()));
    for (const client_cache_t& uncacheBuffer : mUncacheBuffers) {
        SAFE_PARCEL(parcel->writeStrongBinder, uncacheBuffer.token.promote());
        SAFE_PARCEL(parcel->writeUint64, uncacheBuffer.id);
    }

    SAFE_PARCEL(parcel->writeUint64Vector, mMergedTransactionIds);
    SAFE_PARCEL(mCallbacks.writeToParcel, parcel);
    SAFE_PARCEL(mInputWindowCommands.write, *parcel);
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mEarlyWakeupInfos.size()));
    for (const auto& e : mEarlyWakeupInfos) {
        e.writeToParcel(parcel);
    }
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mComposerStates.size()));
    for (const auto& s : mComposerStates) {
        SAFE_PARCEL(s.write, *parcel);
    }

    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mDisplayStates.size()));
    for (const auto& d : mDisplayStates) {
        SAFE_PARCEL(d.write, *parcel);
    }

    SAFE_PARCEL(parcel->writeParcelableVector, mBarriers);

    return NO_ERROR;
}

status_t TransactionState::readFromParcel(const Parcel* parcel) {
    SAFE_PARCEL(parcel->readUint64, &mId);
    SAFE_PARCEL(parcel->readUint32, &mFlags);
    SAFE_PARCEL(parcel->readInt64, &mDesiredPresentTime);
    SAFE_PARCEL(parcel->readBool, &mIsAutoTimestamp);

    SAFE_PARCEL(parcel->readParcelable, &mFrameTimelineInfo);

    uint32_t count;
    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize())
    mUncacheBuffers.clear();
    mUncacheBuffers.reserve(count);
    for (size_t i = 0; i < count; i++) {
        client_cache_t client_cache;
        sp<IBinder> tmpBinder;
        SAFE_PARCEL(parcel->readStrongBinder, &tmpBinder);
        client_cache.token = tmpBinder;
        SAFE_PARCEL(parcel->readUint64, &client_cache.id);
        mUncacheBuffers.emplace_back(std::move(client_cache));
    }

    SAFE_PARCEL(parcel->readUint64Vector, &mMergedTransactionIds);
    SAFE_PARCEL(mCallbacks.readFromParcel, parcel);
    SAFE_PARCEL(mInputWindowCommands.read, *parcel);
    count = 0;
    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize());
    std::vector<gui::EarlyWakeupInfo> earlyWakeupInfos;
    earlyWakeupInfos.reserve(count);
    for (size_t i = 0; i < count; i++) {
        gui::EarlyWakeupInfo e;
        e.readFromParcel(parcel);
        earlyWakeupInfos.push_back(std::move(e));
    }
    mEarlyWakeupInfos = std::move(earlyWakeupInfos);

    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize());
    mComposerStates.clear();
    mComposerStates.reserve(count);
    for (size_t i = 0; i < count; i++) {
        ComposerState s;
        SAFE_PARCEL(s.read, *parcel);
        mComposerStates.emplace_back(std::move(s));
    }

    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize());
    mDisplayStates.clear();
    mDisplayStates.reserve(count);
    for (size_t i = 0; i < count; i++) {
        DisplayState d;
        SAFE_PARCEL(d.read, *parcel);
        mDisplayStates.emplace_back(std::move(d));
    }

    mBarriers.clear();
    SAFE_PARCEL(parcel->readParcelableVector, &mBarriers);
    return NO_ERROR;
}

void TransactionState::merge(TransactionState&& other,
                             const std::function<void(const layer_state_t&)>& onBufferOverwrite) {
    // TODO(b/385156191) Consider merging desired present time.
    mFlags |= other.mFlags;
    while (mMergedTransactionIds.size() + other.mMergedTransactionIds.size() >
                   MAX_MERGE_HISTORY_LENGTH - 1 &&
           mMergedTransactionIds.size() > 0) {
        mMergedTransactionIds.pop_back();
    }
    if (other.mMergedTransactionIds.size() == MAX_MERGE_HISTORY_LENGTH) {
        mMergedTransactionIds.insert(mMergedTransactionIds.begin(),
                                     other.mMergedTransactionIds.begin(),
                                     other.mMergedTransactionIds.end() - 1);
    } else if (other.mMergedTransactionIds.size() > 0u) {
        mMergedTransactionIds.insert(mMergedTransactionIds.begin(),
                                     other.mMergedTransactionIds.begin(),
                                     other.mMergedTransactionIds.end());
    }

    for (auto& cacheId : other.mUncacheBuffers) {
        mUncacheBuffers.emplace_back(std::move(cacheId));
    }

    mergeFrameTimelineInfo(other.mFrameTimelineInfo);

    mInputWindowCommands.merge(other.mInputWindowCommands);

    for (gui::EarlyWakeupInfo& op : other.mEarlyWakeupInfos) {
        mEarlyWakeupInfos.emplace_back(std::move(op));
    }

    for (auto& otherState : other.mComposerStates) {
        if (auto it = std::find_if(mComposerStates.begin(), mComposerStates.end(),
                                   [&otherState](const auto& composerState) {
                                       return composerState.state.surface ==
                                               otherState.state.surface;
                                   });
            it != mComposerStates.end()) {
            if (otherState.state.what & layer_state_t::eBufferChanged) {
                onBufferOverwrite(it->state);
            }
            it->state.merge(otherState.state);
        } else {
            mComposerStates.emplace_back(std::move(otherState));
        }
    }

    for (auto& state : other.mDisplayStates) {
        if (auto it = std::find_if(mDisplayStates.begin(), mDisplayStates.end(),
                                   [&state](const auto& displayState) {
                                       return displayState.token == state.token;
                                   });
            it != mDisplayStates.end()) {
            it->merge(state);
        } else {
            mDisplayStates.emplace_back(std::move(state));
        }
    }

    mMergedTransactionIds.insert(mMergedTransactionIds.begin(), other.mId);

    mBarriers.insert(mBarriers.end(), std::make_move_iterator(other.mBarriers.begin()),
                     std::make_move_iterator(other.mBarriers.end()));
    if (mBarriers.size() > MAX_BARRIERS_LENGTH) {
        int numToRemove = mBarriers.size() - MAX_BARRIERS_LENGTH;
        std::string droppedBarriers =
                std::accumulate(mBarriers.begin(), mBarriers.begin() + numToRemove, std::string(),
                                [](std::string&& s,
                                   const gui::TransactionBarrier& barrier) -> std::string {
                                    s += barrier.toString() + ",";
                                    return s;
                                });
        ALOGE("Dropping %d transaction barriers: %s", numToRemove, droppedBarriers.c_str());
        mBarriers.erase(mBarriers.begin(), mBarriers.begin() + numToRemove);
    }

    other.clear();
}

// copied from FrameTimelineInfo::merge()
void TransactionState::mergeFrameTimelineInfo(const FrameTimelineInfo& other) {
    // When merging vsync Ids we take the oldest valid one
    if (mFrameTimelineInfo.vsyncId != FrameTimelineInfo::INVALID_VSYNC_ID &&
        other.vsyncId != FrameTimelineInfo::INVALID_VSYNC_ID) {
        if (other.vsyncId > mFrameTimelineInfo.vsyncId) {
            mFrameTimelineInfo = other;
        }
    } else if (mFrameTimelineInfo.vsyncId == FrameTimelineInfo::INVALID_VSYNC_ID) {
        mFrameTimelineInfo = other;
    }
}

void TransactionState::clear() {
    mId = 0;
    mFlags = 0;
    mDesiredPresentTime = 0;
    mIsAutoTimestamp = true;
    mCallbacks.clear();
    mUncacheBuffers.clear();
    mFrameTimelineInfo = {};
    mMergedTransactionIds.clear();
    mInputWindowCommands.clear();
    mEarlyWakeupInfos.clear();
    mComposerStates.clear();
    mDisplayStates.clear();
    mBarriers.clear();
}

layer_state_t* TransactionState::getLayerState(const sp<SurfaceControl>& sc) {
    auto handle = sc->getLayerStateHandle();
    if (auto it = std::find_if(mComposerStates.begin(), mComposerStates.end(),
                               [&handle](const auto& composerState) {
                                   return composerState.state.surface == handle;
                               });
        it != mComposerStates.end()) {
        return &it->state;
    }

    // we don't have it, add an initialized layer_state to our list
    ComposerState s;
    s.state.surface = handle;
    s.state.layerId = sc->getLayerId();
    mComposerStates.emplace_back(std::move(s));

    return &mComposerStates.back().state;
}

DisplayState& TransactionState::getDisplayState(const sp<IBinder>& token) {
    if (auto it = std::find_if(mDisplayStates.begin(), mDisplayStates.end(),
                               [token](const auto& display) { return display.token == token; });
        it != mDisplayStates.end()) {
        return *it;
    }

    // If display state doesn't exist, add a new one.
    DisplayState s;
    s.token = token;
    mDisplayStates.emplace_back(std::move(s));
    return mDisplayStates.back();
}

}; // namespace android
