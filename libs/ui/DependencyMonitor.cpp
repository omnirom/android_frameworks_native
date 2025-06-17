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
#undef LOG_TAG
#define LOG_TAG "DependencyMonitor"

#include <ui/DependencyMonitor.h>
#include <ui/Fence.h>
#include <utils/Timers.h>

#include <inttypes.h>

namespace android {

void DependencyMonitor::addIngress(FenceTimePtr fence, std::string annotation) {
    std::lock_guard lock(mMutex);
    resolveLocked();
    if (mDependencies.isFull() && !mDependencies.front().updateSignalTimes(true)) {
        ALOGD("%s: Clobbering unresolved dependencies -- make me bigger!", mToken.c_str());
    }

    auto& entry = mDependencies.next();
    entry.reset(mToken.c_str());
    ALOGV("%" PRId64 "/%s: addIngress at CPU time %" PRId64 " (%s)", mDependencies.back().id,
          mToken.c_str(), systemTime(), annotation.c_str());

    mDependencies.back().ingress = {std::move(fence), std::move(annotation)};
}

void DependencyMonitor::addAccessCompletion(FenceTimePtr fence, std::string annotation) {
    std::lock_guard lock(mMutex);
    if (mDependencies.size() == 0) {
        return;
    }
    ALOGV("%" PRId64 "/%s: addAccessCompletion at CPU time %" PRId64 " (%s)",
          mDependencies.back().id, mToken.c_str(), systemTime(), annotation.c_str());
    mDependencies.back().accessCompletions.emplace_back(std::move(fence), std::move(annotation));
}

void DependencyMonitor::addEgress(FenceTimePtr fence, std::string annotation) {
    std::lock_guard lock(mMutex);
    if (mDependencies.size() == 0) {
        return;
    }
    ALOGV("%" PRId64 "/%s: addEgress at CPU time %" PRId64 " (%s)", mDependencies.back().id,
          mToken.c_str(), systemTime(), annotation.c_str());
    mDependencies.back().egress = {std::move(fence), std::move(annotation)};
}

void DependencyMonitor::resolveLocked() {
    if (mDependencies.size() == 0) {
        return;
    }

    for (size_t i = mDependencies.size(); i > 0; i--) {
        auto& dependencyBlock = mDependencies[i - 1];

        if (dependencyBlock.validated) {
            continue;
        }

        if (!dependencyBlock.updateSignalTimes(false)) {
            break;
        }

        dependencyBlock.validated = true;
        dependencyBlock.checkUnsafeAccess();
    }
}

bool DependencyMonitor::DependencyBlock::updateSignalTimes(bool excludeIngress) {
    if (egress.fence->getSignalTime() == Fence::SIGNAL_TIME_PENDING) {
        return false;
    }

    if (!excludeIngress && ingress.fence->getSignalTime() == Fence::SIGNAL_TIME_PENDING) {
        return false;
    }

    for (auto& accessCompletion : accessCompletions) {
        if (accessCompletion.fence->getSignalTime() == Fence::SIGNAL_TIME_PENDING) {
            return false;
        }
    }

    return true;
}

void DependencyMonitor::DependencyBlock::checkUnsafeAccess() const {
    const nsecs_t egressTime = egress.fence->getCachedSignalTime();
    const nsecs_t ingressTime = ingress.fence->getCachedSignalTime();

    ALOGV_IF(egressTime != Fence::SIGNAL_TIME_INVALID,
             "%" PRId64 "/%s: Egress time: %" PRId64 " (%s)", token, id, egressTime,
             egress.annotation.c_str());
    ALOGV_IF(Fence::isValidTimestamp(egressTime) && Fence::isValidTimestamp(ingressTime) &&
                     egressTime < ingressTime,
             "%" PRId64 "/%s: Detected egress before ingress!: %" PRId64 " (%s) < %" PRId64 " (%s)",
             id, token, egressTime, egress.annotation, ingressTime, ingress.annotation.c_str());

    for (auto& accessCompletion : accessCompletions) {
        const nsecs_t accessCompletionTime = accessCompletion.fence->getCachedSignalTime();
        if (!Fence::isValidTimestamp(accessCompletionTime)) {
            ALOGI("%" PRId64 "/%s: Detected invalid access completion! <%s>", id, token,
                  accessCompletion.annotation.c_str());
            continue;
        } else {
            ALOGV("%" PRId64 "/%s: Access completion time: %" PRId64 " <%s>", id, token,
                  accessCompletionTime, accessCompletion.annotation.c_str());
        }

        ALOGI_IF(Fence::isValidTimestamp(egressTime) && accessCompletionTime > egressTime,
                 "%" PRId64 "/%s: Detected access completion after egress!: %" PRId64
                 " (%s) > %" PRId64 " (%s)",
                 id, token, accessCompletionTime, accessCompletion.annotation.c_str(), egressTime,
                 egress.annotation.c_str());

        ALOGI_IF(Fence::isValidTimestamp(ingressTime) && accessCompletionTime < ingressTime,
                 "%" PRId64 "/%s: Detected access completion prior to ingress!: %" PRId64
                 " (%s) < %" PRId64 " (%s)",
                 id, token, accessCompletionTime, accessCompletion.annotation.c_str(), ingressTime,
                 ingress.annotation.c_str());
    }

    ALOGV_IF(ingressTime != Fence::SIGNAL_TIME_INVALID,
             "%" PRId64 "/%s: Ingress time: %" PRId64 " (%s)", id, token, ingressTime,
             ingress.annotation.c_str());
}

} // namespace android