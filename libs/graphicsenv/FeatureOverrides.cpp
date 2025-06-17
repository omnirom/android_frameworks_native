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

#include <cinttypes>

#include <android-base/stringprintf.h>
#include <binder/Parcel.h>
#include <graphicsenv/FeatureOverrides.h>

namespace android {

using base::StringAppendF;

status_t FeatureConfig::writeToParcel(Parcel* parcel) const {
    status_t status;

    status = parcel->writeUtf8AsUtf16(mFeatureName);
    if (status != OK) {
        return status;
    }
    status = parcel->writeBool(mEnabled);
    if (status != OK) {
        return status;
    }
    // Number of GPU vendor IDs.
    status = parcel->writeVectorSize(mGpuVendorIDs);
    if (status != OK) {
        return status;
    }
    // GPU vendor IDs.
    for (const auto& vendorID : mGpuVendorIDs) {
        status = parcel->writeUint32(vendorID);
        if (status != OK) {
            return status;
        }
    }

    return OK;
}

status_t FeatureConfig::readFromParcel(const Parcel* parcel) {
    status_t status;

    status = parcel->readUtf8FromUtf16(&mFeatureName);
    if (status != OK) {
        return status;
    }
    status = parcel->readBool(&mEnabled);
    if (status != OK) {
        return status;
    }
    // Number of GPU vendor IDs.
    int numGpuVendorIDs;
    status = parcel->readInt32(&numGpuVendorIDs);
    if (status != OK) {
        return status;
    }
    // GPU vendor IDs.
    for (int i = 0; i < numGpuVendorIDs; i++) {
        uint32_t gpuVendorIdUint;
        status = parcel->readUint32(&gpuVendorIdUint);
        if (status != OK) {
            return status;
        }
        mGpuVendorIDs.emplace_back(gpuVendorIdUint);
    }

    return OK;
}

std::string FeatureConfig::toString() const {
    std::string result;
    StringAppendF(&result, "Feature: %s\n", mFeatureName.c_str());
    StringAppendF(&result, "      Status: %s\n", mEnabled ? "enabled" : "disabled");
    for (const auto& vendorID : mGpuVendorIDs) {
        // vkjson outputs decimal, so print both formats.
        StringAppendF(&result, "      GPU Vendor ID: 0x%04X (%d)\n", vendorID, vendorID);
    }

    return result;
}

status_t FeatureOverrides::writeToParcel(Parcel* parcel) const {
    status_t status;
    // Number of global feature configs.
    status = parcel->writeVectorSize(mGlobalFeatures);
    if (status != OK) {
        return status;
    }
    // Global feature configs.
    for (const auto& cfg : mGlobalFeatures) {
        status = cfg.writeToParcel(parcel);
        if (status != OK) {
            return status;
        }
    }
    // Number of package feature overrides.
    status = parcel->writeInt32(static_cast<int32_t>(mPackageFeatures.size()));
    if (status != OK) {
        return status;
    }
    for (const auto& feature : mPackageFeatures) {
        // Package name.
        status = parcel->writeUtf8AsUtf16(feature.first);
        if (status != OK) {
            return status;
        }
        // Number of package feature configs.
        status = parcel->writeVectorSize(feature.second);
        if (status != OK) {
            return status;
        }
        // Package feature configs.
        for (const auto& cfg : feature.second) {
            status = cfg.writeToParcel(parcel);
            if (status != OK) {
                return status;
            }
        }
    }

    return OK;
}

status_t FeatureOverrides::readFromParcel(const Parcel* parcel) {
    status_t status;

    // Number of global feature configs.
    status = parcel->resizeOutVector(&mGlobalFeatures);
    if (status != OK) {
        return status;
    }
    // Global feature configs.
    for (FeatureConfig& cfg : mGlobalFeatures) {
        status = cfg.readFromParcel(parcel);
        if (status != OK) {
            return status;
        }
    }

    // Number of package feature overrides.
    int numPkgOverrides;
    status = parcel->readInt32(&numPkgOverrides);
    if (status != OK) {
        return status;
    }
    // Package feature overrides.
    for (int i = 0; i < numPkgOverrides; i++) {
        // Package name.
        std::string name;
        status = parcel->readUtf8FromUtf16(&name);
        if (status != OK) {
            return status;
        }
        std::vector<FeatureConfig> cfgs;
        // Number of package feature configs.
        int numCfgs;
        status = parcel->readInt32(&numCfgs);
        if (status != OK) {
            return status;
        }
        // Package feature configs.
        for (int j = 0; j < numCfgs; j++) {
            FeatureConfig cfg;
            status = cfg.readFromParcel(parcel);
            if (status != OK) {
                return status;
            }
            cfgs.emplace_back(cfg);
        }
        mPackageFeatures[name] = cfgs;
    }

    return OK;
}

std::string FeatureOverrides::toString() const {
    std::string result;
    result.append("Global Features:\n");
    for (auto& cfg : mGlobalFeatures) {
        result.append("  " + cfg.toString());
    }
    result.append("\n");
    result.append("Package Features:\n");
    for (const auto& packageFeature : mPackageFeatures) {
        result.append("  Package:");
        StringAppendF(&result, " %s\n", packageFeature.first.c_str());
        for (auto& cfg : packageFeature.second) {
            result.append("    " + cfg.toString());
        }
    }

    return result;
}

} // namespace android
