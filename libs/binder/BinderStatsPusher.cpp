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
#define LOG_TAG "libbinder.BinderStatsPusher"

#include "BinderStatsPusher.h"

#include <android-base/properties.h>
#include <android/os/IStatsBootstrapAtomService.h>
#include <binder/Functional.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <utils/SystemClock.h>
#include "BinderStatsUtils.h"
#include "JvmUtils.h"

namespace android {
// defined in stats/atoms/framework/framework_extension_atoms.proto
constexpr int32_t kBinderSpamAtomId = 1064;
constexpr int32_t kBinderLatencyAtomId = 1090;
[[clang::no_destroy]] static const StaticString16 kStatsBootstrapServiceName(u"statsbootstrap");

sp<os::IStatsBootstrapAtomService> BinderStatsPusher::getBootstrapAtomServiceLocked(
        const int64_t nowSec) {
    // When this is removed, the device does not get past the boot animation
    // TODO(b/299356196): This might result in dropped stats for high usage apps like
    // servicemanager.
    if (!mLastServiceCheckSucceeded && (mServiceCheckTimeSec + kCheckServiceTimeoutSec > nowSec)) {
        return nullptr;
    };
    auto sm = defaultServiceManager();
    if (!sm) {
        LOG_ALWAYS_FATAL("defaultServiceManager() returned nullptr.");
    }
    auto service = interface_cast<os::IStatsBootstrapAtomService>(
            defaultServiceManager()->checkService(kStatsBootstrapServiceName));
    mServiceCheckTimeSec = nowSec;
    if (service == nullptr) {
        mLastServiceCheckSucceeded = false;
    } else {
        mLastServiceCheckSucceeded = true;
    }
    return service;
}

__attribute__((no_sanitize("signed-integer-overflow")))
void BinderStatsPusher::aggregateStatsLocked(const std::vector<BinderCallData>& data,
                                             const sp<os::IStatsBootstrapAtomService>& service,
                                             const int64_t nowSec) {
    for (const auto& datum : data) {
        int64_t startTimeSec = datum.startTimeNanos / 1000'000'000;
        // Check if the buffer period has passed.
        auto [it, inserted] = mStatsBuffer[datum].try_emplace(startTimeSec, AidlTargetMetrics());
        it->second.totalCalls++;
        if (datum.hasLatencyData()) {
            it->second.callsWithLatency++;
            it->second.durationSumMicros += (datum.endTimeNanos - datum.startTimeNanos) / 1000;
        }
    }
    if (!service) return;
    // Ensure that if this is a local binder and this thread isn't attached
    // to the VM then skip pushing. This is required since StatsBootstrap is
    // a Java service and needs a JNI interface to be called from native code.
    bool isProcessSystemServer = IInterface::asBinder(service)->localBinder() != nullptr;
    if (isProcessSystemServer) {
        if (!isThreadAttachedToJVM()) {
            return;
        }
    }
    // Clear calling identity if this is called from system server. This
    // will allow libStatsBootstrap to verify calling uid correctly.
    int64_t callingIdentity;
    if (isProcessSystemServer) {
        callingIdentity = IPCThreadState::self()->clearCallingIdentity();
    }
    auto callingIdentityGuard = binder::impl::make_scope_guard([&] {
        if (isProcessSystemServer) {
            IPCThreadState::self()->restoreCallingIdentity(callingIdentity);
        }
    });

    for (auto outerIt = mStatsBuffer.begin(); outerIt != mStatsBuffer.end();
         /* no increment */) {
        int32_t secondsWithAtLeast125Calls = 0;
        int32_t secondsWithAtLeast250Calls = 0;

        uint64_t callsWithLatency = 0;
        uint64_t durationSumMicros = 0;
        uint32_t secondsWithAtLeast10Calls = 0;
        uint32_t secondsWithAtLeast50Calls = 0;
        for (auto innerIt = outerIt->second.begin(); innerIt != outerIt->second.end();
             /* no increment */) {
            // Check if the buffer period has passed.
            int64_t startTimeSec = innerIt->first;
            if (nowSec - startTimeSec >= kAggregationWindowSec) {
                uint64_t totalCalls = innerIt->second.totalCalls;
                if (totalCalls > kSpamFirstWatermark) {
                    secondsWithAtLeast125Calls++;
                    if (totalCalls > kSpamSecondWatermark) {
                        secondsWithAtLeast250Calls++;
                    }
                }
                if (innerIt->second.callsWithLatency > 0) {
                    callsWithLatency += innerIt->second.callsWithLatency;
                    durationSumMicros += innerIt->second.durationSumMicros;
                    if (innerIt->second.callsWithLatency >= kLatencyCountFirstWatermark) {
                        secondsWithAtLeast10Calls++;
                        if (innerIt->second.callsWithLatency >= kLatencyCountSecondWatermark) {
                            secondsWithAtLeast50Calls++;
                        }
                    }
                }
                // Erase the datum from the buffer so we don't aggregate it again
                innerIt = outerIt->second.erase(innerIt);
            } else {
                ++innerIt;
            }
        }
        if (callsWithLatency > 0) {
            auto datum = outerIt->first;
            auto atom = os::StatsBootstrapAtom();
            atom.atomId = kBinderLatencyAtomId;
            std::vector<os::StatsBootstrapAtomValue> values;
            atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(datum.senderUid)));
            atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(getuid()))); // host uid
            atom.values.push_back(createPrimitiveValue(datum.interfaceDescriptor));
            // TODO (b/299356196): get actual method name.
            atom.values.push_back(
                    createPrimitiveValue(std::to_string(datum.transactionCode))); // aidl method
            atom.values.push_back(
                    createPrimitiveValue(static_cast<int64_t>(callsWithLatency))); // call count
            atom.values.push_back(
                    createPrimitiveValue(static_cast<int64_t>(durationSumMicros))); // dur sum in us
            atom.values.push_back(createPrimitiveValue(static_cast<int32_t>(
                    secondsWithAtLeast10Calls))); // seconds_with_at_least_10_calls
            atom.values.push_back(createPrimitiveValue(static_cast<int32_t>(
                    secondsWithAtLeast50Calls))); // seconds_with_at_least_50_calls

            // TODO(b/414551350): combine multiple binder calls into one.
            service->reportBootstrapAtom(atom);
        }

        if (secondsWithAtLeast125Calls > 0) {
            auto datum = outerIt->first;
            auto atom = os::StatsBootstrapAtom();
            atom.atomId = kBinderSpamAtomId;
            std::vector<os::StatsBootstrapAtomValue> values;
            atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(datum.senderUid)));
            atom.values.push_back(createPrimitiveValue(static_cast<int64_t>(getuid()))); // host uid
            atom.values.push_back(createPrimitiveValue(datum.interfaceDescriptor));
            // TODO (b/299356196): get actual method name.
            atom.values.push_back(
                    createPrimitiveValue(std::to_string(datum.transactionCode))); // aidl method
            atom.values.push_back(
                    createPrimitiveValue(static_cast<int32_t>(secondsWithAtLeast125Calls)));
            atom.values.push_back(
                    createPrimitiveValue(static_cast<int32_t>(secondsWithAtLeast250Calls)));
            // TODO(b/414551350): combine multiple binder calls into one.
            service->reportBootstrapAtom(atom);
        }

        if (outerIt->second.empty()) {
            outerIt = mStatsBuffer.erase(outerIt);
        } else {
            ++outerIt;
        }
    }
}

void BinderStatsPusher::pushLocked(const std::vector<BinderCallData>& data, const int64_t nowSec) {
    auto service = getBootstrapAtomServiceLocked(nowSec);
    aggregateStatsLocked(data, service, nowSec);
}

} // namespace android
