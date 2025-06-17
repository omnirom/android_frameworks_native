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

namespace android {

status_t TransactionState::writeToParcel(Parcel* parcel) const {
    SAFE_PARCEL(parcel->writeUint64, mId);
    SAFE_PARCEL(parcel->writeUint32, mFlags);
    SAFE_PARCEL(parcel->writeInt64, mDesiredPresentTime);
    SAFE_PARCEL(parcel->writeBool, mIsAutoTimestamp);
    SAFE_PARCEL(parcel->writeParcelable, mFrameTimelineInfo);
    SAFE_PARCEL(parcel->writeStrongBinder, mApplyToken);
    SAFE_PARCEL(parcel->writeBool, mMayContainBuffer);
    SAFE_PARCEL(parcel->writeBool, mLogCallPoints);

    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mDisplayStates.size()));
    for (auto const& displayState : mDisplayStates) {
        displayState.write(*parcel);
    }
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mComposerStates.size()));
    for (auto const& composerState : mComposerStates) {
        composerState.write(*parcel);
    }

    mInputWindowCommands.write(*parcel);
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mUncacheBuffers.size()));
    for (const client_cache_t& uncacheBuffer : mUncacheBuffers) {
        SAFE_PARCEL(parcel->writeStrongBinder, uncacheBuffer.token.promote());
        SAFE_PARCEL(parcel->writeUint64, uncacheBuffer.id);
    }

    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mMergedTransactionIds.size()));
    for (auto mergedTransactionId : mMergedTransactionIds) {
        SAFE_PARCEL(parcel->writeUint64, mergedTransactionId);
    }

    SAFE_PARCEL(parcel->writeBool, mHasListenerCallbacks);
    SAFE_PARCEL(parcel->writeUint32, static_cast<uint32_t>(mListenerCallbacks.size()));
    for (const auto& [listener, callbackIds] : mListenerCallbacks) {
        SAFE_PARCEL(parcel->writeStrongBinder, listener);
        SAFE_PARCEL(parcel->writeParcelableVector, callbackIds);
    }

    return NO_ERROR;
}

status_t TransactionState::readFromParcel(const Parcel* parcel) {
    SAFE_PARCEL(parcel->readUint64, &mId);
    SAFE_PARCEL(parcel->readUint32, &mFlags);
    SAFE_PARCEL(parcel->readInt64, &mDesiredPresentTime);
    SAFE_PARCEL(parcel->readBool, &mIsAutoTimestamp);
    SAFE_PARCEL(parcel->readParcelable, &mFrameTimelineInfo);
    SAFE_PARCEL(parcel->readNullableStrongBinder, &mApplyToken);
    SAFE_PARCEL(parcel->readBool, &mMayContainBuffer);
    SAFE_PARCEL(parcel->readBool, &mLogCallPoints);

    uint32_t count;
    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize())
    mDisplayStates.clear();
    mDisplayStates.reserve(count);
    for (size_t i = 0; i < count; i++) {
        DisplayState displayState;
        if (displayState.read(*parcel) == BAD_VALUE) {
            return BAD_VALUE;
        }
        mDisplayStates.emplace_back(std::move(displayState));
    }

    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize())
    mComposerStates.clear();
    mComposerStates.reserve(count);
    for (size_t i = 0; i < count; i++) {
        ComposerState composerState;
        if (composerState.read(*parcel) == BAD_VALUE) {
            return BAD_VALUE;
        }
        mComposerStates.emplace_back(std::move(composerState));
    }

    if (status_t status = mInputWindowCommands.read(*parcel) != NO_ERROR) {
        return status;
    }

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

    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize())
    mMergedTransactionIds.clear();
    mMergedTransactionIds.resize(count);
    for (size_t i = 0; i < count; i++) {
        SAFE_PARCEL(parcel->readUint64, &mMergedTransactionIds[i]);
    }

    SAFE_PARCEL(parcel->readBool, &mHasListenerCallbacks);
    SAFE_PARCEL_READ_SIZE(parcel->readUint32, &count, parcel->dataSize());
    mListenerCallbacks.clear();
    mListenerCallbacks.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        sp<IBinder> tmpBinder;
        SAFE_PARCEL(parcel->readStrongBinder, &tmpBinder);
        std::vector<CallbackId> callbackIds;
        SAFE_PARCEL(parcel->readParcelableVector, &callbackIds);
        mListenerCallbacks.emplace_back(tmpBinder, callbackIds);
    }

    return NO_ERROR;
}

void TransactionState::merge(TransactionState&& other,
                             const std::function<void(layer_state_t&)>& onBufferOverwrite) {
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
    mMergedTransactionIds.insert(mMergedTransactionIds.begin(), other.mId);

    for (auto const& otherState : other.mComposerStates) {
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
            mComposerStates.push_back(otherState);
        }
    }

    for (auto const& state : other.mDisplayStates) {
        if (auto it = std::find_if(mDisplayStates.begin(), mDisplayStates.end(),
                                   [&state](const auto& displayState) {
                                       return displayState.token == state.token;
                                   });
            it != mDisplayStates.end()) {
            it->merge(state);
        } else {
            mDisplayStates.push_back(state);
        }
    }

    for (const auto& cacheId : other.mUncacheBuffers) {
        mUncacheBuffers.push_back(cacheId);
    }

    mInputWindowCommands.merge(other.mInputWindowCommands);
    // TODO(b/385156191) Consider merging desired present time.
    mFlags |= other.mFlags;
    mMayContainBuffer |= other.mMayContainBuffer;
    mLogCallPoints |= other.mLogCallPoints;

    // mApplyToken is explicitly not merged. Token should be set before applying the transactions to
    // make synchronization decisions a bit simpler.
    mergeFrameTimelineInfo(other.mFrameTimelineInfo);
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
    mComposerStates.clear();
    mDisplayStates.clear();
    mListenerCallbacks.clear();
    mHasListenerCallbacks = false;
    mInputWindowCommands.clear();
    mUncacheBuffers.clear();
    mDesiredPresentTime = 0;
    mIsAutoTimestamp = true;
    mApplyToken = nullptr;
    mFrameTimelineInfo = {};
    mMergedTransactionIds.clear();
    mFlags = 0;
    mMayContainBuffer = false;
    mLogCallPoints = false;
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
    mComposerStates.push_back(s);

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
    mDisplayStates.push_back(s);
    return mDisplayStates.back();
}

}; // namespace android
