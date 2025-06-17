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

#include <ui/FatVector.h>
#include <ui/FenceTime.h>
#include <ui/RingBuffer.h>

namespace android {

// Debugging class for that tries to add userspace logging for fence depencencies.
// The model that a DependencyMonitor tries to follow is, for each access of some resource:
// 1. There is a single ingress fence, that guards whether a resource is now safe to read from
// another system.
// 2. There are multiple access fences, that are fired when a resource is read.
// 3. There is a single egress fence, that is fired when a resource is released and sent to another
// system.
//
// Note that there can be repeated ingress and egress of a resource, but the assumption is that
// there is exactly one egress for every ingress, unless the resource is destroyed rather than
// released.
//
// The DependencyMonitor will log if there is an anomaly in the fences tracked for some resource.
// This includes:
// * If (2) happens before (1)
// * If (2) happens after (3)
//
// Note that this class has no knowledge of the "other system". I.e., if the other system ignores
// the fence reported in (3), but still takes a long time to write to the resource and produce (1),
// then nothing will be logged. That other system must have its own DependencyMonitor. Conversely,
// this class has imperfect knowledge of the system it is monitoring. For example, this class does
// not know the precise start times of reading from a resource, the exact time that a read might
// occur from a hardware unit is not known to userspace.
//
// In other words, this class logs specific classes of fence violations, but is not sensitive to
// *all* violations. One property of this is that unless the system tracked by a DependencyMonitor
// is feeding in literally incorrect fences, then there is no chance of a false positive.
//
// This class is thread safe.
class DependencyMonitor {
public:
    // Sets a debug token identifying the resource this monitor is tracking.
    void setToken(std::string token) { mToken = std::move(token); }

    // Adds a fence that is fired when the resource ready to be ingested by the system using the
    // DependencyMonitor.
    void addIngress(FenceTimePtr fence, std::string annotation);
    // Adds a fence that is fired when the resource is accessed.
    void addAccessCompletion(FenceTimePtr fence, std::string annotation);
    // Adds a fence that is fired when the resource is released to another system.
    void addEgress(FenceTimePtr fence, std::string annotation);

private:
    struct AnnotatedFenceTime {
        FenceTimePtr fence;
        std::string annotation;
    };

    struct DependencyBlock {
        int64_t id = -1;
        AnnotatedFenceTime ingress = {FenceTime::NO_FENCE, ""};
        FatVector<AnnotatedFenceTime> accessCompletions;
        AnnotatedFenceTime egress = {FenceTime::NO_FENCE, ""};
        bool validated = false;
        const char* token = nullptr;

        void reset(const char* newToken) {
            static std::atomic<int64_t> counter = 0;
            id = counter++;
            ingress = {FenceTime::NO_FENCE, ""};
            accessCompletions.clear();
            egress = {FenceTime::NO_FENCE, ""};
            validated = false;
            token = newToken;
        }

        // Returns true if all fences in this block have valid signal times.
        bool updateSignalTimes(bool excludeIngress);

        void checkUnsafeAccess() const;
    };

    void resolveLocked() REQUIRES(mMutex);

    std::string mToken;
    std::mutex mMutex;
    ui::RingBuffer<DependencyBlock, 10> mDependencies GUARDED_BY(mMutex);
};

} // namespace android