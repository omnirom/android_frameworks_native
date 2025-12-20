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

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <numeric>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <android-base/logging.h>
#include <ftl/enum.h>
#include <ui/Size.h>

#include "test_framework/core/EdidBuilder.h"

namespace android::surfaceflinger::tests::end2end::test_framework::core {

namespace {

constexpr unsigned kDescriptor1Offset = 54;
constexpr unsigned kDescriptorSize = 18;
constexpr unsigned kDescriptorEndOffset = 126;

// The base EDID binary format is full of values where the bits are packed non-contiguously.
//
// This function helps select specific contiguous bits from an input integer value, and optionally
// shifts them so they can be combined with other bits.
//
// `kInMask` specifies the bits of interest from the integer value.
// `kOutMask` specifies the output mask, helping document where in the output byte those bits go.
//
template <uint32_t kInMask, uint32_t kOutMask = std::numeric_limits<uint8_t>::max()>
constexpr auto select(uint32_t value) -> uint32_t {
    static_assert(kInMask > 0);   // Must not be zero bits
    static_assert(kOutMask > 0);  // Must not be zero bits
    // The number of bits in each mask must be the same
    static_assert(std::popcount(kInMask) == std::popcount(kOutMask));
    // The bits in each mask must be contiguous
    static_assert(std::popcount(kInMask) == std::countr_one(kInMask >> std::countr_zero(kInMask)));
    static_assert(std::popcount(kOutMask) ==
                  std::countr_one(kOutMask >> std::countr_zero(kOutMask)));

    // Determine how much to shift kInMask to have it match kOutMask. May be negative.
    static constexpr int kShift = std::countr_zero(kInMask) - std::countr_zero(kOutMask);

    // Mask and shift.
    if constexpr (kShift < 0) {
        return (value & kInMask) << (-kShift);
    } else {
        return (value & kInMask) >> kShift;
    }
}

class Writer final {
  public:
    explicit Writer(EdidBuilder::EdidBytes& buffer, size_t begin,
                    size_t end = EdidBuilder::kEdidByteCount)
        : destination(&buffer), offset(begin), end(end) {
        CHECK(begin < EdidBuilder::kEdidByteCount);
        CHECK(end <= EdidBuilder::kEdidByteCount);
    }

    void operator()(uint32_t value) {
        CHECK(offset < EdidBuilder::kEdidByteCount);
        CHECK(value >= 0 && value <= std::numeric_limits<uint8_t>::max());

        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
        (*destination)[offset] = static_cast<uint8_t>(value);
        // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
        offset += 1;
    }

    void strncpy(std::string_view::const_iterator source, size_t size) {
        CHECK(offset < end);
        CHECK(offset + size <= end);
        std::copy_n(source, size, destination->begin() + offset);
        offset += size;
    }

    void copy(std::span<const uint8_t> source) {
        CHECK(offset < end);
        CHECK(offset + source.size() <= end);
        std::copy_n(source.begin(), source.size(), destination->begin() + offset);
        offset += source.size();
    }

    void fill(uint8_t value, size_t size) {
        CHECK(offset < end);
        CHECK(offset + size <= end);
        std::fill_n(destination->begin() + offset, size, value);
        offset += size;
    }

  private:
    EdidBuilder::EdidBytes* destination;
    size_t offset;
    size_t end;
};

// Ref "3.4.1 ID Manufacturer Name: 2 Bytes" (address 08h - 09h)
constexpr auto encodeManufacturerId(std::array<char, 3> value) -> uint16_t {
    CHECK(value[0] >= 'A' && value[0] <= 'Z');
    CHECK(value[1] >= 'A' && value[1] <= 'Z');
    CHECK(value[2] >= 'A' && value[2] <= 'Z');

    // Encode the three characters as three five bit values, where A=1 and Z=26.
    // Each five bit value is packed into the bottom fifteen bits of a sixteen bit value.

    // NOLINTBEGIN(*-magic-numbers)
    uint16_t encoded = 0;
    encoded |= select<0b11111, 0b11111'00000'00000>(value[0]);  // 1st char, top 5 bits
    encoded |= select<0b11111, 0b00000'11111'00000>(value[1]);  // 2nd char, middle 5 bits
    encoded |= select<0b11111, 0b00000'00000'11111>(value[2]);  // 3rd char, bottom 5 bits
    // NOLINTEND(*-magic-numbers)

    return encoded;
}

// Ref "3.6.1 Video Input Definition: 1 Byte" (address 14h)
constexpr auto encodeDigitalVideoInputBitmap(EdidBuilder::ColorBitDepth depth,
                                             EdidBuilder::DigitalInterface interface) -> uint8_t {
    // Note: This is for the digital interface interpretation of the byte, indicated by the top bit
    // (bit 7) of the byte being 1.
    constexpr uint8_t kVideoInputDigitalBit = 0x80;

    // NOLINTBEGIN(*-magic-numbers)
    uint8_t encoded = kVideoInputDigitalBit;
    encoded |= select<0b0111, 0b0111'0000>(ftl::to_underlying(depth));
    encoded |= select<0b1111, 0b0000'1111>(ftl::to_underlying(interface));
    // NOLINTEND(*-magic-numbers)

    return encoded;
}

// Ref "3.6.3 Display Transfer Characteristics (GAMMA): 1 Byte" (address 17h)
constexpr auto encodeGamma(float gamma) -> uint8_t {
    constexpr float kGammaMin = 1;
    constexpr float kGammaMax = 3.5390625;
    constexpr float kGammaScale = 100;

    CHECK(gamma >= kGammaMin && gamma <= kGammaMax);
    return static_cast<uint8_t>((gamma - kGammaMin) * kGammaScale);
}

// Ref "3.6.4 Feature Support: 1 Byte" (address 18h)
constexpr auto encodeFeatures(const EdidBuilder::DigitalDisplayFeatureSupport& support) -> uint8_t {
    uint8_t encoded = 0;
    // NOLINTBEGIN(*-magic-numbers)
    encoded |= select<0x1, 0b1000'0000>(ftl::to_underlying(support.standby));
    encoded |= select<0x1, 0b0100'0000>(ftl::to_underlying(support.suspend));
    encoded |= select<0x1, 0b0010'0000>(ftl::to_underlying(support.activeOff));
    encoded |= select<0x3, 0b0001'1000>(ftl::to_underlying(support.digitalFormats));
    encoded |= select<0x1, 0b0000'0100>(ftl::to_underlying(support.srgb));
    encoded |= select<0x1, 0b0000'0010>(ftl::to_underlying(support.preferredIsNative));
    encoded |= select<0x1, 0b0000'0001>(ftl::to_underlying(support.continuousFrequency));
    // NOLINTEND(*-magic-numbers)

    return encoded;
}

// Ref "3.4 Vendor & Product ID: 10 Bytes (address 08h - 11h)
constexpr void setVendor(EdidBuilder::EdidBytes& edid, const EdidBuilder::Vendor& values) {
    constexpr unsigned kBlockBegin = 0x08;
    constexpr unsigned kBlockEnd = 0x12;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    // NOLINTBEGIN(*-magic-numbers)

    const uint16_t encodedManufacturerId = encodeManufacturerId(values.manufacturerId);
    // Store the 15 bit encoded id with most significant byte first (unlike most multibyte
    // fields here)
    write(select<0xff00>(encodedManufacturerId));
    write(select<0x00ff>(encodedManufacturerId));

    // Store the 16 bit product code, lsb first.
    write(select<0x00ff>(values.manufacturerProductCode));
    write(select<0xff00>(values.manufacturerProductCode));

    // Store the 32 bit serial number, lsb first.
    write(select<0x000000ff>(values.manufacturerSerialNumber));
    write(select<0x0000ff00>(values.manufacturerSerialNumber));
    write(select<0x00ff0000>(values.manufacturerSerialNumber));
    write(select<0xff000000>(values.manufacturerSerialNumber));

    // Store the 8 bit manufactured week
    CHECK((values.manufacturedWeek <= 54) || (values.manufacturedWeek == 255));
    write(values.manufacturedWeek);

    // Store the encoded 8 bit year
    constexpr int32_t kBaseYear = 1990;
    const auto encodedYear = values.manufacturedYear - kBaseYear;
    write(encodedYear);

    // NOLINTEND(*-magic-numbers)
}

// Ref "3.5 EDID Structure Version & Revision: 2 Bytes" (address 12h - 13h)
constexpr void setVersion(EdidBuilder::EdidBytes& edid, const EdidBuilder::EdidVersion& values) {
    constexpr unsigned kBlockBegin = 0x12;
    constexpr unsigned kBlockEnd = 0x14;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    write(values.version);
    write(values.revision);
}

// Ref "3.6 Basic Display Parameters and Features: 5 Bytes" (address 14h - 18h)
constexpr void setBasic(EdidBuilder::EdidBytes& edid,
                        const EdidBuilder::BasicDigitalDisplayParametersAndFeatures& values) {
    constexpr unsigned kBlockBegin = 0x14;
    constexpr unsigned kBlockEnd = 0x18;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    // NOLINTBEGIN(*-magic-numbers)

    // Store the encoded 8 bit video input bitmap
    write(encodeDigitalVideoInputBitmap(values.depth, values.interface));

    // Store the screen size in centimeters (8 bits each)
    write(values.horizontalSizeCm);
    write(values.verticalSizeCm);

    // Store the encoded 8 bit gamma
    write(encodeGamma(values.gamma));

    // Store the encoded 8 bit feature flags
    write(encodeFeatures(values.features));

    // NOLINTEND(*-magic-numbers)
}

// Ref "3.10.2 Detailed Timing Descriptor: 18 bytes" (any optional display descriptor slot)
constexpr void setDTD(EdidBuilder::EdidBytes& edid,
                      EdidBuilder::DigitalSeparateDetailedTimingDescriptor descriptor) {
    constexpr uint32_t kPixelClockDivisor = 10'000;  // 10 Khz
    constexpr uint32_t kPixelClockMin = 0;
    constexpr uint32_t kPixelClockMax = 655'350'000;  // 655.35 Mhz

    constexpr size_t kMax12BitValue = 4095;
    constexpr size_t kMax10BitValue = 1023;
    constexpr size_t kMax6BitValue = 63;

    // clang-format off
    //
    // Interpretation of timings, according to EDID 1.4. Ref "3.12 Note Regarding Borders"
    //
    // |-------------Active signal------------|----------------Blanking---------------|
    // |-Border-|-Addressable Pixels-|-Border-|-Front Porch-|-Sync Pulse-|-Back Porch-|
    //
    // clang-format on

    const uint16_t horizontalActive =
            descriptor.timing.horizontal.addressable + (descriptor.timing.horizontal.border * 2);
    const uint16_t horizontalBlanking = descriptor.timing.horizontal.frontPorch +
                                        descriptor.timing.horizontal.syncPulse +
                                        descriptor.timing.horizontal.backPorch;
    const uint16_t verticalActive =
            descriptor.timing.vertical.addressable + (descriptor.timing.vertical.border * 2);
    const uint16_t verticalBlanking = descriptor.timing.vertical.frontPorch +
                                      descriptor.timing.vertical.syncPulse +
                                      descriptor.timing.vertical.backPorch;

    CHECK(descriptor.timing.pixelClockMhz >= kPixelClockMin &&
          descriptor.timing.pixelClockMhz <= kPixelClockMax);
    CHECK(horizontalActive <= kMax12BitValue);
    CHECK(horizontalBlanking <= kMax12BitValue);
    CHECK(verticalActive <= kMax12BitValue);
    CHECK(verticalBlanking <= kMax12BitValue);
    CHECK(descriptor.timing.horizontal.frontPorch <= kMax12BitValue);
    CHECK(descriptor.timing.horizontal.syncPulse <= kMax12BitValue);
    CHECK(descriptor.timing.vertical.frontPorch <= kMax6BitValue);
    CHECK(descriptor.timing.vertical.syncPulse <= kMax6BitValue);

    auto write = Writer(edid, kDescriptor1Offset, kDescriptor1Offset + kDescriptorSize);

    // NOLINTBEGIN(*-magic-numbers)

    // The first two bytes contain the pixel clock, in units of 10Khz, little endian order.
    write(select<0x00ff>(descriptor.timing.pixelClockMhz / kPixelClockDivisor));
    write(select<0xff00>(descriptor.timing.pixelClockMhz / kPixelClockDivisor));

    // The next byte contains the bottom 8 bits of the horizontal active pixels.
    write(select<0x0ff>(horizontalActive));
    // The next byte contains the bottom 8 bits of the horizontal blanking pixels.
    write(select<0x0ff>(horizontalBlanking));
    // The next byte contains the top 4 bits of the horizontal active pixels in its top 4 bits,
    // and the top 4 bits of the horizontal blanking pixels in the bottom 4 bits.
    write(select<0xf00, 0xf0>(horizontalActive) | select<0xf00, 0x0f>(horizontalBlanking));

    // The next byte contains the bottom 8 bits of the vertical active lines.
    write(select<0x0ff>(verticalActive));
    // The next byte contains the bottom 8 bits of the vertical blanking lines.
    write(select<0x0ff>(verticalBlanking));
    // The next byte contains the top 4 bits of the vertical active lines in its top 4 bits,
    // and the top 4 bits of the vertical blanking lines in the bottom 4 bits.
    write(select<0xf00, 0xf0>(verticalActive) | select<0xf00, 0x0f>(verticalBlanking));

    // The next byte contains the bottom 8 bits of the horizontal front porch.
    write(select<0x0ff>(descriptor.timing.horizontal.frontPorch));
    // The next byte contains the bottom 8 bits of the horizontal sync pulse.
    write(select<0x0ff>(descriptor.timing.horizontal.syncPulse));
    // The next byte contains the bottom 4 bits of the vertical front porch in the top 4 bits,
    // and the bottom 4 bits of the vertical sync pulse in the bottom 4 bits.
    write(select<0x0f, 0xf0>(descriptor.timing.vertical.frontPorch) |
          select<0x0f, 0x0f>(descriptor.timing.vertical.syncPulse));
    // The next byte contains:
    // - the top 2 bits of the horizontal front porch in the top 2 bits
    // - the top 2 bits of the horizontal sync pulse in the next 2 bits
    // - the top 2 bits of the vertical front porch in the next 2 bits
    // - the top 2 bits of the vertical sync pulse in the final 2 bits
    write(select<0b11'0000'0000, 0b1100'0000>(descriptor.timing.horizontal.frontPorch) |
          select<0b11'0000'0000, 0b0011'0000>(descriptor.timing.horizontal.syncPulse) |
          select<0b00'0011'0000, 0b0000'1100>(descriptor.timing.vertical.frontPorch) |
          select<0b00'0011'0000, 0b0000'0011>(descriptor.timing.vertical.syncPulse));

    // The next byte contains the bottom 8 bits of the horizontal image size in mm.
    write(select<0x0ff>(descriptor.horizontalImageSizeMm));
    // The next byte contains the bottom 8 bits of the vertical image size in mm.
    write(select<0x0ff>(descriptor.verticalImageSizeMm));
    // The next byte contains:
    // - the top 4 bits of the horizontal image size in the top 4 bits
    // - the top 4 bits of the vertical image size in the final 4 bits
    write((select<0xf00, 0xf0>(descriptor.horizontalImageSizeMm) |
           select<0xf00, 0x0f>(descriptor.verticalImageSizeMm)));

    // The next byte contains the horizontal border size in pixels.
    write(descriptor.timing.horizontal.border);
    // The next byte contains the vertical border size in lines.
    write(descriptor.timing.vertical.border);

    constexpr int32_t kDigitalSync = 1;
    constexpr int32_t kDigitalSeparateSync = 1;

    // The final byte contains a bunch of feature bits, some of which are interpreted
    // differently depending on how bits 4 and 3 are set. We just configure digital/separate
    // sync for simplicity.
    uint32_t featureBitmap = 0;
    featureBitmap |= select<0x1, 0x80>(ftl::to_underlying(descriptor.signalInterlace));
    featureBitmap |= select<0x6, 0x60>(ftl::to_underlying(descriptor.stereoMode));
    featureBitmap |= select<0x1, 0x10>(kDigitalSync);
    featureBitmap |= select<0x1, 0x08>(kDigitalSeparateSync);
    featureBitmap |= select<0x1, 0x04>(ftl::to_underlying(descriptor.timing.vertical.syncPolarity));
    featureBitmap |=
            select<0x1, 0x02>(ftl::to_underlying(descriptor.timing.horizontal.syncPolarity));
    featureBitmap |= select<0x1, 0x01>(ftl::to_underlying(descriptor.stereoMode));
    write(featureBitmap);

    // NOLINTEND(*-magic-numbers)
};

// Ref "3.3 Header: 8 Bytes" (address 00h - 07h)
constexpr void setHeader(EdidBuilder::EdidBytes& edid) {
    constexpr auto kHeaderBegin = 0;
    constexpr auto kHeaderEnd = 8;
    auto write = Writer(edid, kHeaderBegin, kHeaderEnd);

    // NOLINTBEGIN(*-magic-numbers)
    // The header is a fixed pattern of eight bytes.
    write(0);
    write.fill(0xff, 6);
    write(0);
    // NOLINTEND(*-magic-numbers)
}

// Ref "3.7 Display x, y Chromaticity Coordinates: 10 Bytes" (address 19h - 22h)
constexpr void setChromaticityDefaults(EdidBuilder::EdidBytes& edid) {
    constexpr auto kBlockBegin = 25;
    constexpr auto kBlockEnd = 35;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    constexpr auto encodeChromaticity = [](double value) -> int {
        constexpr auto kNumEncodeBits = 10;
        return (static_cast<int>((1 << (kNumEncodeBits + 1)) * value) + 1) >> 1;
    };

    // NOLINTBEGIN(*-magic-numbers)

    constexpr auto packed8BitsFrom4xBottom2Bits = [](uint32_t value0, uint32_t value1,
                                                     uint32_t value2, uint32_t value3) -> uint32_t {
        return (select<0b11, 0b1100'0000>(value0) | select<0b11, 0b0011'0000>(value1) |
                select<0b11, 0b0000'1100>(value2) | select<0b11, 0b0000'0011>(value3));
    };

    constexpr auto packed8BitsFromTop8Bits = [](uint32_t value) -> uint32_t {
        return select<0b11'1111'1100>(value);
    };

    // NOLINTEND(*-magic-numbers)

    // These chromaticity points are taken from the VESA EDID Structure Version 1, Revision 4
    // standard description, from the first example EDID in Appendix A.

    constexpr auto kChromaticityRedX = 0.627;
    constexpr auto kChromaticityRedY = 0.341;
    constexpr auto kChromaticityGreenX = 0.292;
    constexpr auto kChromaticityGreenY = 0.605;
    constexpr auto kChromaticityBlueX = 0.149;
    constexpr auto kChromaticityBlueY = 0.072;
    constexpr auto kChromaticityWhiteX = 0.283;
    constexpr auto kChromaticityWhiteY = 0.297;

    constexpr int encodedRX = encodeChromaticity(kChromaticityRedX);
    constexpr int encodedRY = encodeChromaticity(kChromaticityRedY);
    constexpr int encodedGX = encodeChromaticity(kChromaticityGreenX);
    constexpr int encodedGY = encodeChromaticity(kChromaticityGreenY);
    constexpr int encodedBX = encodeChromaticity(kChromaticityBlueX);
    constexpr int encodedBY = encodeChromaticity(kChromaticityBlueY);
    constexpr int encodedWX = encodeChromaticity(kChromaticityWhiteX);
    constexpr int encodedWY = encodeChromaticity(kChromaticityWhiteY);

    const int lowBitsRG = packed8BitsFrom4xBottom2Bits(encodedRX, encodedRY, encodedGX, encodedGY);
    const int lowBitsBW = packed8BitsFrom4xBottom2Bits(encodedBX, encodedBY, encodedWX, encodedWY);

    write(lowBitsRG);
    write(lowBitsBW);
    write(packed8BitsFromTop8Bits(encodedRX));
    write(packed8BitsFromTop8Bits(encodedRY));
    write(packed8BitsFromTop8Bits(encodedGX));
    write(packed8BitsFromTop8Bits(encodedGY));
    write(packed8BitsFromTop8Bits(encodedBX));
    write(packed8BitsFromTop8Bits(encodedBY));
    write(packed8BitsFromTop8Bits(encodedWX));
    write(packed8BitsFromTop8Bits(encodedWY));
}

// Ref "3.8 Established Timings I & II: 3 bytes" (address 23h - 25h)
constexpr void setEstablishedTimingsDefaults(EdidBuilder::EdidBytes& edid) {
    constexpr auto kBlockBegin = 35;
    constexpr auto kBlockEnd = 38;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    // NOLINTBEGIN(*-magic-numbers)

    // These three bytes are bitmasks that indicate support for specific, established display
    // modes as defined by the EDID standard.
    // We fill with zero to indicate no support for these modes for simplicity.
    write.fill(0, kBlockEnd - kBlockBegin);

    // NOLINTEND(*-magic-numbers)
}

// Ref "3.9 Standard Timings: 16 Bytes" (address 26h - 35h)
constexpr void setStandardTimingsDefaults(EdidBuilder::EdidBytes& edid) {
    constexpr auto kBlockBegin = 38;
    constexpr auto kBlockEnd = 53;
    auto write = Writer(edid, kBlockBegin, kBlockEnd);

    // NOLINTBEGIN(*-magic-numbers)

    // These 16 bytes are eight two-byte compactly encoded entries for modes that are supported.
    // See the EDID specification for details. While we might be able to indicate that an
    // injected fake display has a mode that can be represented through this table, we instead
    // just fill it with (1, 1) entries to indicate that those entries are not used.
    write.fill(1, kBlockEnd - kBlockBegin);

    // NOLINTEND(*-magic-numbers)
}

// Ref "3.10 18 Byte Descriptors - 72 Bytes" (addresses 36h - 7dh, four 18 byte slots)
constexpr void setUnusedDescriptors(EdidBuilder::EdidBytes& edid, size_t nextDescriptorOffset) {
    constexpr uint8_t kTagUnused = 0x10;

    while (nextDescriptorOffset < kDescriptorEndOffset) {
        auto write = Writer(edid, nextDescriptorOffset, nextDescriptorOffset + kDescriptorSize);
        nextDescriptorOffset += kDescriptorSize;

        // NOLINTBEGIN(*-magic-numbers)
        write.fill(0, 3);
        write(kTagUnused);
        write.fill(0, 14);
        // NOLINTEND(*-magic-numbers)
    }
}

// Ref "3.11 EXTENSION Flag and Checksum" {address 7eh)
constexpr void setNoExtensionBlocks(EdidBuilder::EdidBytes& edid) {
    // Sets the extension flag byte to indicate there are no extension blocks after base EDID block.
    constexpr unsigned kExtensionFlagOffset = 126;
    edid[kExtensionFlagOffset] = 0;
}

// Ref "3.11 EXTENSION Flag and Checksum" {address 7fh)
constexpr void updateChecksum(EdidBuilder::EdidBytes& edid) {
    // Sets the checksum byte so that the sum of all 128 bytes is zero.
    constexpr unsigned kChecksumOffset = 127;
    const int sum = std::accumulate(edid.begin(), edid.end() - 1, 0);
    edid[kChecksumOffset] = static_cast<uint8_t>(-sum);
}

}  // namespace

// Note: This value is exposed for use as a public default, as well as being used as a starting
// value for the builder.
const EdidBuilder::EdidBytes EdidBuilder::kDefaultEdid = {
        // clang-format off
    // https://en.wikipedia.org/wiki/Extended_Display_Identification_Data
    /*  0 -  3 */ 0x00, 0xff, 0xff, 0xff,   // Fixed Header pattern 1
    /*  4 -  7 */ 0xff, 0xff, 0xff, 0x00,   // Fixed Header pattern 2

    /* Vendor block */
    /*  8 -  9 */ 0x1c, 0xec,               // Manufacturer ID (3x 5-bit chars) (default: "GGL")
    /* 10 - 11 */ 0x01, 0x00,               // Manufacturer product code (default: 1)
    /* 12 - 15 */ 0x01, 0x00, 0x00, 0x00,   // Serial Number (default: 1)
    /* 16 - 17 */ 0xff, 0x23,               // Manufactured Week, Year-1990 (default: No week, 2025)

    /* Version block */
    /* 18 - 19 */ 0x01, 0x04,               // EDID Version 1.4

    /* Video input definition block */
    /*      20 */ 0xb0,                     // Video params bitmap (default: digital, 10 bit color depth, undefined interface)
    /* 21 - 22 */ 0x40, 0x24,               // Screen size in cm (default: 64 cm x 36 cm)
    /*      23 */ 0x82,                     // Gamma (default: 1.30)
    /*      24 */ 0x02,                     // Supported features bitmap (default: None with format RGB4444)

    /* Chromaticity block */
    /* 25 - 26 */ 0x9c, 0x68,               // Bottom two bits for each of Rx/y, Gx/y, Bx/y, Wx/y
    /* 27 - 28 */ 0xa0, 0x57,               // Rx, Ry top 8 bits
    /* 29 - 30 */ 0x4a, 0x9b,               // Gx, Ry top 8 bits
    /* 31 - 32 */ 0x26, 0x12,               // Bx, By top 8 bits
    /* 33 - 34 */ 0x48, 0x4c,               // Wx, Wy top 8 bits

    /* Standard timings block */
    /* 35 - 37 */ 0x00, 0x00, 0x00,         // Standard mode support bitmap (default: None)
    /* 38 - 39 */ 0x01, 0x01,               // Standard timing 1 (default: Unused)
    /* 40 - 41 */ 0x01, 0x01,               // Standard timing 1 (default: Unused)
    /* 42 - 43 */ 0x01, 0x01,               // Standard timing 2 (default: Unused)
    /* 44 - 45 */ 0x01, 0x01,               // Standard timing 3 (default: Unused)
    /* 46 - 47 */ 0x01, 0x01,               // Standard timing 4 (default: Unused)
    /* 48 - 49 */ 0x01, 0x01,               // Standard timing 5 (default: Unused)
    /* 50 - 41 */ 0x01, 0x01,               // Standard timing 6 (default: Unused)
    /* 52 - 53 */ 0x01, 0x01,               // Standard timing 7 (default: Unused)

    /* 54 - 71 : Detailed Timing Descriptor (required with EDID 1.4) */
    /* These values are defaults for a 1920x1080 60Hz display */
    /* + 0 -  1 */ 0x02, 0x3a,              // Pixel clock
    /* + 2 -  4 */ 0x80, 0x18, 0x71,        // Horizontal timing
    /* + 5 -  7 */ 0x38, 0x2d, 0x40,        // Vertical timing
    /* + 8 - 11 */ 0x58, 0x2c, 0x45, 0x00,  // H/V front porch, pulse info
    /* +12 - 14 */ 0x80, 0x68, 0x21,        // H/V image size mm
    /* +15 - 16 */ 0x00, 0x00,              // H/V border pixels
    /* +     17 */ 0x1e,                    // Features bitmap

    /* 72 - 89 : Descriptor 2 */
    /* + 0 -  3 */ 0x00, 0x00, 0x00, 0x10,  // Not Used Descriptor
    /* +      4 */ 0x00,                    // Reserved
    /* + 5 -  8 */ 0x00, 0x00, 0x00, 0x00,
    /* + 9 - 12 */ 0x00, 0x00, 0x00, 0x00,
    /* +13 - 16 */ 0x00, 0x00, 0x00, 0x00,
    /* +     17 */ 0x00,

    /* 90 - 107 : Descriptor 3 */
    /* + 0 -  3 */ 0x00, 0x00, 0x00, 0x10,  // Not Used Descriptor
    /* +      4 */ 0x00,                    // Reserved
    /* + 5 -  8 */ 0x00, 0x00, 0x00, 0x00,
    /* + 9 - 12 */ 0x00, 0x00, 0x00, 0x00,
    /* +13 - 16 */ 0x00, 0x00, 0x00, 0x00,
    /* +     17 */ 0x00,

    /* 108 - 125 : Descriptor 4 */
    /* + 0 -  3 */ 0x00, 0x00, 0x00, 0x10,  // Not Used Descriptor
    /* +      4 */ 0x00,                    // Reserved
    /* + 5 -  8 */ 0x00, 0x00, 0x00, 0x00,
    /* + 9 - 12 */ 0x00, 0x00, 0x00, 0x00,
    /* +13 - 16 */ 0x00, 0x00, 0x00, 0x00,
    /* +     17 */ 0x00,

    /*      126 */ 0x00,                    // Number of extensions (zero)
    /*      127 */ 0x77,                    // Checksum (Sum of bytes 0-127 should be zero!)
        // clang-format on
};

EdidBuilder::EdidBuilder()
    : edid(kDefaultEdid), nextDescriptorOffset(kDescriptor1Offset + kDescriptorSize) {}

// NOLINTBEGIN(readability-convert-member-functions-to-static)
// clang-tidy incorrectly shows this error for C++23 "deducing this" methods.

auto EdidBuilder::set(this EdidBuilder&& self, const Version1r4Required& values) -> EdidBuilder&& {
    setVersion(self.edid, {.version = 1, .revision = 4});
    setVendor(self.edid, values.vendor);
    setBasic(self.edid, values.parametersAndFeatures);
    setDTD(self.edid, values.preferred);

    return std::move(self);
}

// Ref "3.10.3.4 Display Product Name (ASCII) String Descriptor Definition (tag #FCh)"
// Note: The string value is truncated if more than 13 characters are given.
auto EdidBuilder::addDisplayProductNameStringDescriptor(this EdidBuilder&& self,
                                                        std::string_view value) -> EdidBuilder&& {
    const size_t descriptorOffset = self.nextDescriptorOffset;
    CHECK(descriptorOffset < kDescriptorEndOffset);
    self.nextDescriptorOffset += kDescriptorSize;

    auto write = Writer(self.edid, descriptorOffset, descriptorOffset + kDescriptorSize);

    constexpr uint8_t kTag = 0xfc;
    constexpr size_t kMaxStringLength = kMaxProductNameStringLength;
    constexpr uint8_t kStringTerminator = 0x0a;
    constexpr uint8_t kPaddingByte = 0x20;

    write(0);
    write(0);
    write(0);
    write(kTag);
    write(0);

    write.strncpy(value.begin(), std::min(value.size(), kMaxStringLength));
    if (value.size() < kMaxStringLength) {
        write(kStringTerminator);
        if (value.size() < kMaxStringLength - 1) {
            // Pad out the rest of the descriptor with ASCII spaces (0x20) characters.
            write.fill(kPaddingByte, kMaxStringLength - 1 - value.size());
        }
    }

    return std::move(self);
};

auto EdidBuilder::build(this EdidBuilder&& self) -> EdidBuilder::EdidBytes {
    setHeader(self.edid);
    setChromaticityDefaults(self.edid);
    setEstablishedTimingsDefaults(self.edid);
    setStandardTimingsDefaults(self.edid);
    setUnusedDescriptors(self.edid, self.nextDescriptorOffset);
    setNoExtensionBlocks(self.edid);

    // Must be last.
    updateChecksum(self.edid);

    const auto result = self.edid;
    std::ignore = std::move(self);  // NOLINT(performance-move-const-arg)
    return result;
}

// NOLINTEND(readability-convert-member-functions-to-static)

auto EdidBuilder::DigitalSeparateDetailedTimingDescriptor::Timing::synthesize(
        ui::Size displayed, int refreshRate) -> Timing {
    // This function synthesizes a valid DTD for a given pixel size and refresh rate, for
    // use with injecting fake/simulated displays with vkms.
    //
    // IMPORTANT: This is not part of the EDID standard, and further is not intended to be
    // used to generate the timing values for a real display.
    //
    // How this works:
    //
    // We want a pixelClock that is an even multiple of 1000.
    //
    // The pixelClock value is the product of total horizontal pixels (displayed + blanking), the
    // total vertical lines (displayed + blanking), and the refresh rate (assumed here to be a
    // simple integer, so 59.994Hz for TV's is not possible with this function)
    //
    // One way of doing that is to make sure the horizontal total pixels is a multiple of 400, and
    // the vertical total pixels is a multiple of 25, which means that the product of those two
    // values alone will be a multiple of 1000.
    //
    // That means we must choose (nonzero) values for the horizontal and vertical blanking to meet
    // that constraint. We also use those values as the minimum amount of blanking.

    const int horizontalAddressable = displayed.width;
    const int verticalAddressable = displayed.height;

    constexpr auto kHorizontalMultiple = 400;
    constexpr auto kVerticalMultiple = 25;

    const auto horizontalBlanking =
            (2 * kHorizontalMultiple) - (horizontalAddressable % kHorizontalMultiple);
    const auto verticalBlanking =
            (2 * kVerticalMultiple) - (verticalAddressable % kVerticalMultiple);
    CHECK_GE(horizontalBlanking, kHorizontalMultiple);
    CHECK_GE(verticalBlanking, kVerticalMultiple);

    const auto horizontalTotal = horizontalAddressable + horizontalBlanking;
    const auto verticalTotal = verticalAddressable + verticalBlanking;
    CHECK(horizontalTotal % kHorizontalMultiple == 0) << horizontalTotal;
    CHECK(verticalTotal % kVerticalMultiple == 0) << verticalTotal;

    const int pixelClockMhz = horizontalTotal * verticalTotal * refreshRate;
    CHECK(pixelClockMhz % 1000 == 0) << pixelClockMhz;

    // With a real world device there are probably some additional constraints around these values.
    // Here we just use some of the values for a standard 1920x1080x60Hz signal, adjusting the back
    // porch values to fill the total blank times.
    constexpr auto kHorizontalFrontPorch = 88;
    constexpr auto kHorizontalSyncPulse = 44;
    constexpr auto kVerticalFrontPorch = 4;
    constexpr auto kVerticalSyncPulse = 5;
    const int horizontalBackPorch =
            horizontalBlanking - (kHorizontalFrontPorch + kHorizontalSyncPulse);
    const int verticalBackPorch = verticalBlanking - (kVerticalFrontPorch + kVerticalSyncPulse);
    CHECK_GE(horizontalBackPorch, kHorizontalFrontPorch);
    CHECK_GE(verticalBackPorch, kVerticalFrontPorch);

    return {.pixelClockMhz = static_cast<uint32_t>(pixelClockMhz),
            .horizontal =
                    {
                            .addressable = static_cast<uint16_t>(horizontalAddressable),
                            .border = 0,
                            .frontPorch = kHorizontalFrontPorch,
                            .syncPulse = kHorizontalSyncPulse,
                            .backPorch = static_cast<uint16_t>(horizontalBackPorch),
                            .syncPolarity = SyncPolarity::Positive,
                    },
            .vertical = {
                    .addressable = static_cast<uint16_t>(verticalAddressable),
                    .border = 0,
                    .frontPorch = kVerticalFrontPorch,
                    .syncPulse = kVerticalSyncPulse,
                    .backPorch = static_cast<uint16_t>(verticalBackPorch),
                    .syncPolarity = SyncPolarity::Positive,
            }};
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core