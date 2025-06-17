/*
 * Copyright 2018 The Android Open Source Project
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

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <ui/DeviceProductInfo.h>
#include <ui/DisplayId.h>
#include <ui/Size.h>

#define LEGACY_DISPLAY_TYPE_PRIMARY 0
#define LEGACY_DISPLAY_TYPE_EXTERNAL 1

namespace android {

using DisplayIdentificationData = std::vector<uint8_t>;

struct DetailedTimingDescriptor {
    ui::Size pixelSizeCount;
    ui::Size physicalSizeInMm;
};

// These values must match the ones in ScreenPartStatus.aidl file in the composer HAL
enum class ScreenPartStatus : uint8_t {
    /**
     * Device cannot differentiate an original screen from a replaced screen.
     */
    UNSUPPORTED = 0,
    /**
     * Device has the original screen it was manufactured with.
     */
    ORIGINAL = 1,
    /**
     * Device has a replaced screen.
     */
    REPLACED = 2,
};

struct DisplayIdentificationInfo {
    PhysicalDisplayId id;
    std::string name;
    uint8_t port;
    std::optional<DeviceProductInfo> deviceProductInfo;
    std::optional<DetailedTimingDescriptor> preferredDetailedTimingDescriptor;
    ScreenPartStatus screenPartStatus;
};

struct ExtensionBlock {
    uint8_t tag;
    uint8_t revisionNumber;
};

struct HdmiPhysicalAddress {
    // The address describes the path from the display sink in the network of connected HDMI
    // devices. The format of the address is "a.b.c.d". For example, address 2.1.0.0 means we are
    // connected to port 1 of a device which is connected to port 2 of the sink.
    uint8_t a, b, c, d;
};

struct HdmiVendorDataBlock {
    HdmiPhysicalAddress physicalAddress;
};

struct Cea861ExtensionBlock : ExtensionBlock {
    std::optional<HdmiVendorDataBlock> hdmiVendorDataBlock;
};

struct Edid {
    uint16_t manufacturerId;
    uint16_t productId;
    std::optional<uint64_t> hashedBlockZeroSerialNumberOpt;
    std::optional<uint64_t> hashedDescriptorBlockSerialNumberOpt;
    PnpId pnpId;
    uint32_t modelHash;
    // Up to 13 characters of ASCII text terminated by LF and padded with SP.
    std::string_view displayName;
    uint8_t manufactureOrModelYear;
    uint8_t manufactureWeek;
    ui::Size physicalSizeInCm;
    std::optional<Cea861ExtensionBlock> cea861Block;
    std::optional<DetailedTimingDescriptor> preferredDetailedTimingDescriptor;
};

bool isEdid(const DisplayIdentificationData&);
std::optional<Edid> parseEdid(const DisplayIdentificationData&);
std::optional<PnpId> getPnpId(uint16_t manufacturerId);

std::optional<DisplayIdentificationInfo> parseDisplayIdentificationData(
        uint8_t port, const DisplayIdentificationData&);

PhysicalDisplayId getVirtualDisplayId(uint32_t id);

// Generates a consistent, stable, and hashed display ID that is based on the
// display's parsed EDID fields.
PhysicalDisplayId generateEdidDisplayId(const Edid& edid);

} // namespace android
