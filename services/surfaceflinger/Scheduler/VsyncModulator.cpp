/*
 * Copyright 2019 The Android Open Source Project
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

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "VsyncModulator.h"

#include <android/gui/EarlyWakeupInfo.h>
#include <common/FlagManager.h>
#include <common/trace.h>
#include <log/log.h>

#include <chrono>
#include <cinttypes>
#include <mutex>

using namespace std::chrono_literals;

namespace android::scheduler {

using base::StringAppendF;

const std::chrono::nanoseconds VsyncModulator::MIN_EARLY_TRANSACTION_TIME = 1ms;

VsyncModulator::VsyncModulator(const VsyncConfigSet& config, Now now)
      : mVsyncConfigSet(config),
        mNow(now) {}

VsyncConfig VsyncModulator::setVsyncConfigSet(const VsyncConfigSet& config) {
    std::lock_guard<std::mutex> lock(mMutex);
    mVsyncConfigSet = config;
    return updateVsyncConfigLocked();
}

VsyncModulator::VsyncConfigOpt VsyncModulator::setTransactionSchedule(
        TransactionSchedule schedule, std::vector<gui::EarlyWakeupInfo> earlyWakeupInfos) {
    std::lock_guard<std::mutex> lock(mMutex);

    for (auto& info : earlyWakeupInfos) {
        sp<IBinder> token = info.token;
        std::string trace = info.trace;

        switch (schedule) {
            case Schedule::EarlyStart:
                if (token) {
                    SFTRACE_FORMAT_INSTANT("%s: EarlyStart requested by %s with token %p", __func__,
                                           trace.c_str(), token.get());
                    mEarlyWakeupRequests[token] =
                            std::make_unique<gui::EarlyWakeupInfo>(std::move(info));
                    token->linkToDeath(sp<DeathRecipient>::fromExisting(this));
                } else {
                    ALOGW("%s: EarlyStart requested without a valid token", __func__);
                }
                break;
            case Schedule::EarlyEnd: {
                if (token && mEarlyWakeupRequests.erase(token)) {
                    SFTRACE_FORMAT_INSTANT("%s: EarlyEnd requested by %s with token %p", __func__,
                                           trace.c_str(), token.get());
                    token->unlinkToDeath(sp<DeathRecipient>::fromExisting(this));
                } else {
                    ALOGW("%s: Unexpected EarlyEnd %s", __func__, info.toString().c_str());
                }
                break;
            }
            case Schedule::Late:
                // This is a speculative fix because we think we are leaking tokens.
                // If earlyWakeupInfos is not empty, we were not in Schedule late
                // because we got both eEarlyWakeupStart and eEarlyWakeupEnd in the
                // same transaction. Clean up state for any earlyWakeupEnd operations.
                if (!info.isStartRequest) {
                    ALOGW("%s: Clearing request by %s with token %p", __func__, trace.c_str(),
                          token.get());
                    mEarlyWakeupRequests.erase(token);
                }
                break;
        }
    }

    SFTRACE_INT("EarlyWakeupRequests", static_cast<int>(mEarlyWakeupRequests.size()));

    if (mEarlyWakeupRequests.empty() && schedule == Schedule::EarlyEnd) {
        mEarlyTransactionFrames = MIN_EARLY_TRANSACTION_FRAMES;
        mEarlyTransactionStartTime = mNow();
    }

    // An early transaction stays an early transaction.
    if (schedule == mTransactionSchedule || mTransactionSchedule == Schedule::EarlyEnd) {
        return std::nullopt;
    }
    mTransactionSchedule = schedule;
    return updateVsyncConfigLocked();
}

VsyncModulator::VsyncConfigOpt VsyncModulator::onTransactionCommit() {
    mLastTransactionCommitTime = mNow();
    if (mTransactionSchedule == Schedule::Late) return std::nullopt;
    mTransactionSchedule = Schedule::Late;
    return updateVsyncConfig();
}

VsyncModulator::VsyncConfigOpt VsyncModulator::onRefreshRateChangeInitiated() {
    if (mRefreshRateChangePending) return std::nullopt;
    mRefreshRateChangePending = true;
    return updateVsyncConfig();
}

VsyncModulator::VsyncConfigOpt VsyncModulator::onRefreshRateChangeCompleted() {
    if (!mRefreshRateChangePending) return std::nullopt;
    mRefreshRateChangePending = false;
    return updateVsyncConfig();
}

VsyncModulator::VsyncConfigOpt VsyncModulator::onDisplayRefresh(bool usedGpuComposition) {
    bool updateOffsetsNeeded = false;

    if (mEarlyTransactionStartTime.load() + MIN_EARLY_TRANSACTION_TIME <=
        mLastTransactionCommitTime.load()) {
        if (mEarlyTransactionFrames > 0) {
            mEarlyTransactionFrames--;
            updateOffsetsNeeded = true;
        }
    }
    if (usedGpuComposition) {
        mEarlyGpuFrames = MIN_EARLY_GPU_FRAMES;
        updateOffsetsNeeded = true;
    } else if (mEarlyGpuFrames > 0) {
        mEarlyGpuFrames--;
        updateOffsetsNeeded = true;
    }

    if (!updateOffsetsNeeded) return std::nullopt;
    return updateVsyncConfig();
}

VsyncConfig VsyncModulator::getVsyncConfig() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mVsyncConfig;
}

auto VsyncModulator::getNextVsyncConfigType() const -> VsyncConfigType {
    // Early offsets are used if we're in the middle of a refresh rate
    // change, or if we recently begin a transaction.
    if (!mEarlyWakeupRequests.empty() || mTransactionSchedule == Schedule::EarlyEnd ||
        mEarlyTransactionFrames > 0 || mRefreshRateChangePending) {
        return VsyncConfigType::Early;
    } else if (mEarlyGpuFrames > 0) {
        return VsyncConfigType::EarlyGpu;
    } else {
        return VsyncConfigType::Late;
    }
}

const VsyncConfig& VsyncModulator::getNextVsyncConfig() const {
    switch (getNextVsyncConfigType()) {
        case VsyncConfigType::Early:
            return mVsyncConfigSet.early;
        case VsyncConfigType::EarlyGpu:
            return mVsyncConfigSet.earlyGpu;
        case VsyncConfigType::Late:
            return mVsyncConfigSet.late;
    }
}

VsyncConfig VsyncModulator::updateVsyncConfig() {
    std::lock_guard<std::mutex> lock(mMutex);
    return updateVsyncConfigLocked();
}

VsyncConfig VsyncModulator::updateVsyncConfigLocked() {
    const VsyncConfig& offsets = getNextVsyncConfig();
    mVsyncConfig = offsets;

    // Trace config type
    SFTRACE_INT("Vsync-Early",  &mVsyncConfig == &mVsyncConfigSet.early);
    SFTRACE_INT("Vsync-EarlyGpu", &mVsyncConfig == &mVsyncConfigSet.earlyGpu);
    SFTRACE_INT("Vsync-Late", &mVsyncConfig == &mVsyncConfigSet.late);

    // Trace early vsync conditions
    SFTRACE_INT("EarlyTransactionFrames", mEarlyTransactionFrames);
    SFTRACE_INT("RefreshRateChangePending", mRefreshRateChangePending);

    // Trace early gpu conditions
    SFTRACE_INT("EarlyGpuFrames", mEarlyGpuFrames);

    return offsets;
}

void VsyncModulator::binderDied(const wp<IBinder>& who) {
    std::lock_guard<std::mutex> lock(mMutex);
    ALOGW("binder died");
    mEarlyWakeupRequests.erase(who);
    static_cast<void>(updateVsyncConfigLocked());
}

bool VsyncModulator::isVsyncConfigEarly() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return getNextVsyncConfigType() != VsyncConfigType::Late;
}

void VsyncModulator::dump(std::string& result) const {
    std::lock_guard<std::mutex> lock(mMutex);
    StringAppendF(&result, " Early Wakeup Requests (count=%zu):\n", mEarlyWakeupRequests.size());
    for (const auto& pair : mEarlyWakeupRequests) {
        StringAppendF(&result, "   %s\n", pair.second->toString().c_str());
    }
}

} // namespace android::scheduler
