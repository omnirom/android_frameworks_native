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

#pragma once

#include <algorithm>
#include <numeric>

#include "android-base/stringprintf.h"

#include "DisplayHardware/HWComposer.h"
#include "Scheduler/SchedulerUtils.h"

namespace android {
namespace scheduler {

/**
 * This class is used to encapsulate configuration for refresh rates. It holds information
 * about available refresh rates on the device, and the mapping between the numbers and human
 * readable names.
 */
class RefreshRateConfigs {
    static const int DEFAULT_FPS = 60;

public:
    // Enum to indicate which vsync rate to run at. Power saving is intended to be the lowest
    // (eg. when the screen is in AOD mode or off), default is the old 60Hz, and performance
    // is the new 90Hz. Eventually we want to have a way for vendors to map these in the configs.
    enum class RefreshRateType {POWER_SAVING, LOW0, LOW1, LOW2, DEFAULT, PERFORMANCE, HIGH1, HIGH2};

    struct RefreshRate {
        // This config ID corresponds to the position of the config in the vector that is stored
        // on the device.
        int configId;
        // Human readable name of the refresh rate.
        std::string name;
        // Refresh rate in frames per second, rounded to the nearest integer.
        uint32_t fps = 0;
        // config Id (returned from HWC2::Display::Config::getId())
        hwc2_config_t id;
    };

    // TODO(b/122916473): Get this information from configs prepared by vendors, instead of
    // baking them in.
    const std::map<RefreshRateType, std::shared_ptr<RefreshRate>>& getRefreshRates() const {
        return mRefreshRates;
    }
    std::shared_ptr<RefreshRate> getRefreshRate(RefreshRateType type) const {
        const auto& refreshRate = mRefreshRates.find(type);
        if (refreshRate != mRefreshRates.end()) {
            return refreshRate->second;
        }
        return nullptr;
    }

    std::shared_ptr<RefreshRate> getRefreshRate(uint32_t fps) const {
        for (const auto& [type, refreshRate] : mRefreshRates) {
            if (refreshRate->fps == fps) {
                return refreshRate;
            }
        }
        return nullptr;
    }

    std::shared_ptr<RefreshRate> getRefreshRate(int configId) const {
        for (const auto& [type, refreshRate] : mRefreshRates) {
            if (refreshRate->configId == configId) {
                return refreshRate;
            }
        }
        return nullptr;
    }

    RefreshRateType getRefreshRateType(hwc2_config_t id) const {
        for (const auto& [type, refreshRate] : mRefreshRates) {
            if (refreshRate->id == id) {
                return type;
            }
        }

        return RefreshRateType::DEFAULT;
    }

    RefreshRateType getDefaultRefreshRateType() const {
        const auto& refreshRate = mRefreshRates.find(RefreshRateType::DEFAULT);
        if (refreshRate != mRefreshRates.end()) {
            uint32_t fps = refreshRate->second->fps;
            if (fps <= DEFAULT_FPS) {
                return RefreshRateType::DEFAULT;
            } else if (fps < (2 * DEFAULT_FPS)) {
                return RefreshRateType::PERFORMANCE;
            } else if (fps >= (2 * DEFAULT_FPS)) {
                return RefreshRateType::HIGH1;
            }
        }

        return RefreshRateType::DEFAULT;
    }

    void populate(const std::vector<std::shared_ptr<const HWC2::Display::Config>>& configs) {
        mRefreshRates.clear();

        // This is the rate that HWC encapsulates right now when the device is in DOZE mode.
        mRefreshRates.emplace(RefreshRateType::POWER_SAVING,
                              std::make_shared<RefreshRate>(
                                      RefreshRate{SCREEN_OFF_CONFIG_ID, "ScreenOff", 0,
                                                  HWC2_SCREEN_OFF_CONFIG_ID}));

        if (configs.size() < 1) {
            ALOGE("Device does not have valid configs. Config size is 0.");
            return;
        }

        // Populate mRefreshRates with configs having the same resolution as active config.
        // Resolution change or SF::setActiveConfig will re-populate the mRefreshRates map.
        int32_t activeWidth = configs.at(mActiveConfig)->getWidth();
        int32_t activeHeight = configs.at(mActiveConfig)->getHeight();
        bool hasSmartPanel = configs.at(mActiveConfig)->hasSmartPanel();

        // Create a map between config index and vsync period. This is all the info we need
        // from the configs.
        std::vector<std::pair<int, nsecs_t>> configIdToVsyncPeriod;
        for (int i = 0; i < configs.size(); ++i) {
            if ((configs.at(i)->getWidth() != activeWidth) ||
                (configs.at(i)->getHeight() != activeHeight) ||
                (configs.at(i)->hasSmartPanel() != hasSmartPanel)) {
                continue;
            }
            configIdToVsyncPeriod.emplace_back(i, configs.at(i)->getVsyncPeriod());
        }

        // Sort the configs based on Refresh rate.
        std::sort(configIdToVsyncPeriod.begin(), configIdToVsyncPeriod.end(),
                  [](const std::pair<int, nsecs_t>& a, const std::pair<int, nsecs_t>& b) {
                      return a.second > b.second;
                  });

        int maxRefreshType = (int)RefreshRateType::HIGH2;
        int lowRefreshType = (int)RefreshRateType::LOW0;
        int defaultType = (int)RefreshRateType::DEFAULT;
        int type = (int)RefreshRateType::DEFAULT;

        // When the configs are sorted by refresh rate. For configs with refresh rate lower than
        // DEFAULT_FPS, they are supported with LOW0, LOW1 and LOW2 refresh rate types. For the
        // configs with refresh rate higher than DEFAULT_FPS, they are supported with PERFORMANCE,
        // HIGH1 and HIGH2 refresh rate types.

        for (int j = 0; j < configIdToVsyncPeriod.size(); j++) {
            nsecs_t vsyncPeriod = configIdToVsyncPeriod[j].second;
            if (vsyncPeriod == 0) {
                continue;
            }

            const float fps = 1e9 / vsyncPeriod;
            uint32_t refreshRate = static_cast<uint32_t>(fps);
            const int configId = configIdToVsyncPeriod[j].first;
            hwc2_config_t hwcConfigId = configs.at(configId)->getId();

            if ((refreshRate < DEFAULT_FPS) && (lowRefreshType < defaultType)) {
                // Populate Low Refresh Rate Configs
                mRefreshRates.emplace(static_cast<RefreshRateType>(lowRefreshType),
                                      std::make_shared<RefreshRate>(
                                          RefreshRate{configId, base::StringPrintf("%2.ffps", fps),
                                                      refreshRate, hwcConfigId}));
                lowRefreshType++;
            } else if ((refreshRate >= DEFAULT_FPS) && (type <= maxRefreshType)) {
                // Populate Default or Perf Refresh Rate Configs
                mRefreshRates.emplace(static_cast<RefreshRateType>(type),
                                      std::make_shared<RefreshRate>(
                                          RefreshRate{configId, base::StringPrintf("%2.ffps", fps),
                                                      refreshRate, hwcConfigId}));
                type++;
            }
        }
    }

    void setActiveConfig(int config) { mActiveConfig = config; }

    RefreshRateType getMaxPerfRefreshRateType() const { return mMaxPerfRefreshRateType; }

    // Update the allowed Display Config(s) based on Smart Panel attribute.
    void getAllowedConfigs(const std::vector<std::shared_ptr<const HWC2::Display::Config>>& configs,
                           std::vector<int32_t> *allowedConfigs) {
        bool isSmart = configs.at(mActiveConfig)->hasSmartPanel();

        for (int i = 0; i < allowedConfigs->size(); i++) {
            int32_t configId = allowedConfigs->at(i);
            if (configs.at(configId)->hasSmartPanel() == isSmart) {
                continue;
            }

            // Get the corresponding Refresh Rate config.
            nsecs_t vsyncPeriod = configs.at(configId)->getVsyncPeriod();
            if (vsyncPeriod == 0) {
                continue;
            }

            float fps = 1e9 / vsyncPeriod;
            uint32_t refreshRate = static_cast<uint32_t>(fps);
            auto refreshRateConfig = getRefreshRate(refreshRate);
            if (refreshRateConfig != nullptr) {
                allowedConfigs->at(i) = refreshRateConfig->configId;
            }
        }

        // Set the Max Allowed Perf RefreshRateType for Content Detection.
        mMaxPerfRefreshRateType = RefreshRateType::PERFORMANCE;
        uint32_t maxAllowedPerfRefreshRate = DEFAULT_FPS;

        for (int j = 0; j < allowedConfigs->size(); j++) {
            auto refreshRateConfig = getRefreshRate(allowedConfigs->at(j));
            if (refreshRateConfig != nullptr) {
                if (refreshRateConfig->fps > maxAllowedPerfRefreshRate) {
                    maxAllowedPerfRefreshRate = refreshRateConfig->fps;
                    mMaxPerfRefreshRateType = getRefreshRateType(refreshRateConfig->id);
                }
            }
        }
    }

private:
    std::map<RefreshRateType, std::shared_ptr<RefreshRate>> mRefreshRates;
    int mActiveConfig = 0;
    RefreshRateType mMaxPerfRefreshRateType = RefreshRateType::PERFORMANCE;
};

} // namespace scheduler
} // namespace android
