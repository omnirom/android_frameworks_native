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

#include <log/log.h>

#include <fstream>
#include <limits>
#include <string>

namespace android {

class BinderObserverConfigTest;

/**
 * Manages the configuration of the binder observer.
 *
 * Applies sharding to determine whether the observer should be enabled for the current process
 * and which AIDL methods should be tracked.
 *
 * The configuration does not change within the lifecycle of the process.
 */
class BinderObserverConfig {
public:
    struct TrackingInfo {
        bool trackSpam;
        bool trackLatency;

        bool operator==(const TrackingInfo&) const = default;
        bool isTracked() const { return trackSpam || trackLatency; }
    };

    static std::unique_ptr<BinderObserverConfig> createConfig() {
        return createConfig(std::make_unique<Environment>());
    }

    // Returns whether binder stats are enabled for the current process. Should not change
    // within the lifecycle of a process.
    bool isEnabled() { return mEnabled; }

    TrackingInfo getTrackingInfo(const std::u16string_view& interfaceDescriptor, uint32_t txnCode) {
        if (!mEnabled) {
            return {
                    .trackSpam = false,
                    .trackLatency = false,
            };
        }

        // If present callMod must be a multiple of spamMod, so use it as a modulo.
        size_t modulo = mSharding.callMod != 0 ? mSharding.callMod : mSharding.spamMod;
        // As above, mod before addition too to ensure we don't overflow.
        size_t token = mAidlOffset % modulo;
        token += txnCode % modulo;
        token += mEnvironment->hashString16(interfaceDescriptor) % modulo;
        return {
                .trackSpam = mSharding.spamMod > 0 && token % mSharding.spamMod == 0,
                .trackLatency = mSharding.callMod > 0 && token % mSharding.callMod == 0,
        };
    }

private:
    friend class BinderObserverConfigTest;

    struct ShardingConfig {
        size_t processMod;
        size_t spamMod;
        size_t callMod;

        bool operator==(const ShardingConfig&) const = default;
    };

    // Contains a 128-bit random number stable for the current session
    // Looks like this: "16e12b27-2a84-4355-84cd-948348d6c998"
    static constexpr const char* kBootIdPath = "/proc/sys/kernel/random/boot_id";

    // The expected length of /proc/sys/kernel/random/boot_id
    static constexpr size_t kBootIdSize = 36;

    // Interacting with the environment abstracted out for testability.
    class Environment {
    public:
        virtual ~Environment() = default;
        virtual std::string readFileLine(const std::string& path);
        virtual uid_t getUid();
        virtual std::string getProcessName();
        virtual ShardingConfig getSystemServerSharding();
        virtual ShardingConfig getOtherProcessesSharding();

        virtual size_t hashString8(const std::string& content) {
            return std::hash<std::string>{}(content);
        }

        virtual size_t hashString16(const std::u16string_view& content) {
            return std::hash<std::u16string_view>{}(content);
        }
    };

    BinderObserverConfig(std::unique_ptr<BinderObserverConfig::Environment>&& environment,
                         bool enabled, BinderObserverConfig::ShardingConfig sharding,
                         size_t aidlOffset)
          : mEnvironment(std::move(environment)),
            mEnabled(enabled),
            mSharding(sharding),
            mAidlOffset(aidlOffset) {
        if (mEnabled) {
            LOG_ALWAYS_FATAL_IF(mSharding.spamMod == 0, "Enabled but nothing to monitor.");

            // Call sharding must always be a multiple of spam sharding (or 0, which is also OK)
            LOG_ALWAYS_FATAL_IF(mSharding.callMod % mSharding.spamMod != 0,
                                "Call reporting must be a subset of spam reporting.");
        }
    }

    static std::pair<size_t, size_t> getBootStableTokens(Environment& environment);

    static std::unique_ptr<BinderObserverConfig> createConfig(
            std::unique_ptr<BinderObserverConfig::Environment>&& environment);

    const std::unique_ptr<Environment> mEnvironment;
    const bool mEnabled;
    const ShardingConfig mSharding;
    const size_t mAidlOffset;
};
} // namespace android
