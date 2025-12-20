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

#include <vector>
#include "BinderStatsUtils.h"

class BinderStatsPusherTest_GetBootstrapService_Test;

namespace android {
/**
 * Processes and pushes binder transaction statistics to the StatsBootstrapAtomService.
 *
 * This class is responsible for aggregating collected BinderCallData
 * such as binder spam which are then reported as atoms.
 * It manages the interaction with the StatsBootstrapAtomService, including
 * handling boot completion and service availability checks.
 *
 * This class is not Thread-safe.
 */
class BinderStatsPusher {
public:
    // Pushes binder transaction data to the StatsBootstrapAtomService.
    void pushLocked(const std::vector<BinderCallData>& data, const int64_t nowSec);

private:
    friend ::BinderStatsPusherTest_GetBootstrapService_Test;
    sp<os::IStatsBootstrapAtomService> getBootstrapAtomServiceLocked(const int64_t nowSec);

    // timeout for checking the service.
    static const int32_t kCheckServiceTimeoutSec = 5;
    static const int32_t kLatencyCountFirstWatermark = 10;
    static const int32_t kLatencyCountSecondWatermark = 50;
    static const int32_t kSpamFirstWatermark = 125;
    static const int32_t kSpamSecondWatermark = 250;
    // Time window (in seconds) for aggregating per-second call counts.
    // Data for a specific second (startTimeSec) is processed for spam detection
    // and to update latency-related 'secondsWithAtLeastXCalls' counts
    // once 'nowSec - startTimeSec >= kAggregationWindowSec'.
    static const int64_t kAggregationWindowSec = 5;

    // KeyEqual function for Binder Spam aggregation of BinderCallData
    struct SpamStatsKeyEqual {
        bool operator()(const BinderCallData& lhs, const BinderCallData& rhs) const {
            return lhs.transactionCode == rhs.transactionCode && lhs.senderUid == rhs.senderUid &&
                    lhs.interfaceDescriptor == rhs.interfaceDescriptor;
        }
    };
    // Custom Hash for Binder Spam aggregation of BinderCallData in std::unordered_map.
    struct SpamStatsKeyHash {
        size_t operator()(const BinderCallData& bcd) const {
            size_t h = std::hash<uid_t>{}(bcd.senderUid); // uid_t is usually uint32_t or uint64_t
            h = std::__hash_combine(h, std::hash<std::u16string_view>{}(bcd.interfaceDescriptor));
            h = std::__hash_combine(h, std::hash<uint32_t>{}(bcd.transactionCode));
            return h;
        }
    };

    // Metrics aggregated for reporting.
    struct AidlTargetMetrics {
        AidlTargetMetrics() = default;
        uint32_t totalCalls = 0;
        uint32_t callsWithLatency = 0;
        int64_t durationSumMicros = 0;
    };

    using StatsBufferMap =
            std::unordered_map<BinderCallData,
                               std::unordered_map<int64_t /*startTimeSec*/, AidlTargetMetrics>,
                               SpamStatsKeyHash,   // Hash
                               SpamStatsKeyEqual>; // KeyEqual

    // If last service check was a success.
    bool mLastServiceCheckSucceeded = true;
    // time of last check
    int64_t mServiceCheckTimeSec = -kCheckServiceTimeoutSec - 1;
    // Aggregates binder transaction data into BinderSpamReport objects.
    void aggregateStatsLocked(const std::vector<BinderCallData>& data,
                              const sp<os::IStatsBootstrapAtomService>& service,
                              const int64_t nowSec);

    // The stats which are not sent to StatsBootStrap
    StatsBufferMap mStatsBuffer;
};

} // namespace android
