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

#include <feature_override/FeatureOverrideParser.h>

#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <android-base/macros.h>
#include <graphicsenv/FeatureOverrides.h>
#include <log/log.h>
#include <vkjson.h>

#include "feature_config.pb.h"

namespace {

void resetFeatureOverrides(android::FeatureOverrides &featureOverrides) {
    featureOverrides.mGlobalFeatures.clear();
    featureOverrides.mPackageFeatures.clear();
}

bool
gpuVendorIdMatches(const VkJsonInstance &vkJsonInstance,
               const uint32_t &configVendorId) {
    if (vkJsonInstance.devices.empty()) {
        return false;
    }

    // Always match the TEST Vendor ID
    if (configVendorId == feature_override::GpuVendorID::VENDOR_ID_TEST) {
        return true;
    }

    // Always assume one GPU device.
    uint32_t vendorID = vkJsonInstance.devices.front().properties.vendorID;

    return vendorID == configVendorId;
}

bool
conditionsMet(const VkJsonInstance &vkJsonInstance,
              const android::FeatureConfig &featureConfig) {
    bool gpuVendorIdMatch = false;

    if (featureConfig.mGpuVendorIDs.empty()) {
        gpuVendorIdMatch = true;
    } else {
        for (const auto &gpuVendorID: featureConfig.mGpuVendorIDs) {
            if (gpuVendorIdMatches(vkJsonInstance, gpuVendorID)) {
                gpuVendorIdMatch = true;
                break;
            }
        }
    }

    return gpuVendorIdMatch;
}

void initFeatureConfig(android::FeatureConfig &featureConfig,
                       const feature_override::FeatureConfig &featureConfigProto) {
    featureConfig.mFeatureName = featureConfigProto.feature_name();
    featureConfig.mEnabled = featureConfigProto.enabled();
    for (const auto &gpuVendorIdProto: featureConfigProto.gpu_vendor_ids()) {
        featureConfig.mGpuVendorIDs.emplace_back(static_cast<uint32_t>(gpuVendorIdProto));
    }
}

feature_override::FeatureOverrideProtos readFeatureConfigProtos(const std::string &configFilePath) {
    feature_override::FeatureOverrideProtos overridesProtos;

    std::ifstream protobufBinaryFile(configFilePath.c_str());
    if (protobufBinaryFile.fail()) {
        ALOGE("Failed to open feature config file: `%s`.", configFilePath.c_str());
        return overridesProtos;
    }

    std::stringstream buffer;
    buffer << protobufBinaryFile.rdbuf();
    std::string serializedConfig = buffer.str();
    std::vector<uint8_t> serialized(
            reinterpret_cast<const uint8_t *>(serializedConfig.data()),
            reinterpret_cast<const uint8_t *>(serializedConfig.data()) +
            serializedConfig.size());

    if (!overridesProtos.ParseFromArray(serialized.data(),
                                        static_cast<int>(serialized.size()))) {
        ALOGE("Failed to parse GpuConfig protobuf data.");
    }

    return overridesProtos;
}

} // namespace

namespace android {

std::string FeatureOverrideParser::getFeatureOverrideFilePath() const {
    const std::string kConfigFilePath = "/system/etc/angle/feature_config_vk.binarypb";

    return kConfigFilePath;
}

bool FeatureOverrideParser::shouldReloadFeatureOverrides() const {
    std::string configFilePath = getFeatureOverrideFilePath();

    std::ifstream configFile(configFilePath);
    if (!configFile.good()) {
        return false;
    }

    struct stat fileStat{};
    if (stat(configFilePath.c_str(), &fileStat) != 0) {
        ALOGE("Error getting file information for '%s': %s", configFilePath.c_str(),
              strerror(errno));
        // stat'ing the file failed, so return false since reading it will also likely fail.
        return false;
    }

    return fileStat.st_mtime > mLastProtobufReadTime;
}

void FeatureOverrideParser::forceFileRead() {
    mLastProtobufReadTime = 0;
}

void FeatureOverrideParser::parseFeatureOverrides() {
    const feature_override::FeatureOverrideProtos overridesProtos = readFeatureConfigProtos(
            getFeatureOverrideFilePath());

    // Clear out the stale values before adding the newly parsed data.
    resetFeatureOverrides(mFeatureOverrides);

    if (overridesProtos.global_features().empty() &&
        overridesProtos.package_features().empty()) {
        // No overrides to parse.
        return;
    }

    const VkJsonInstance vkJsonInstance = VkJsonGetInstance();

    // Global feature overrides.
    for (const auto &featureConfigProto: overridesProtos.global_features()) {
        FeatureConfig featureConfig;
        initFeatureConfig(featureConfig, featureConfigProto);

        if (conditionsMet(vkJsonInstance, featureConfig)) {
            mFeatureOverrides.mGlobalFeatures.emplace_back(featureConfig);
        }
    }

    // App-specific feature overrides.
    for (auto const &pkgConfigProto: overridesProtos.package_features()) {
        const std::string &packageName = pkgConfigProto.package_name();

        if (mFeatureOverrides.mPackageFeatures.count(packageName)) {
            ALOGE("Package already has feature overrides! Skipping.");
            continue;
        }

        std::vector<FeatureConfig> featureConfigs;
        for (const auto &featureConfigProto: pkgConfigProto.feature_configs()) {
            FeatureConfig featureConfig;
            initFeatureConfig(featureConfig, featureConfigProto);

            if (conditionsMet(vkJsonInstance, featureConfig)) {
                featureConfigs.emplace_back(featureConfig);
            }
        }

        mFeatureOverrides.mPackageFeatures[packageName] = featureConfigs;
    }

    mLastProtobufReadTime = std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());
}

FeatureOverrides FeatureOverrideParser::getFeatureOverrides() {
    if (shouldReloadFeatureOverrides()) {
        parseFeatureOverrides();
    }

    return mFeatureOverrides;
}

} // namespace android
