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

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include <ui/Size.h>

namespace android::surfaceflinger::tests::end2end::test_framework::core {

/**
 * Builds a 128 byte binary EDID 1.4 blob from a more human readable set of arguments.
 *
 * Note: Definitions here are based on the "E-EDID A.2_revised_2020.pdf", dated December 31, 2020,
 * as obtained from the download link for this and a few other public standards at
 * https://vesa.org/vesa-standards/.
 *
 * The introduction page for the revised version just notes some minor changes from the original
 * "Release A, Revision 2" document from September 25, 2006 (which can be found from other
 * unofficial sources as "ESA-EEDID-A2.pdf"), namely:
 *
 *  - Rewording the language used in one sentence in section 2.2.4 about VESA maintaining the list
 *    of extension block tag numbers.
 *  - Updating contact information.
 */
struct EdidBuilder final {
    static constexpr size_t kEdidByteCount = 128;

    static constexpr size_t kMaxProductNameStringLength = 13;

    using EdidBytes = std::array<uint8_t, kEdidByteCount>;

    // A prebuilt EDID useful as a default value.
    static const EdidBytes kDefaultEdid;

  public:
    // Ref: "3.4 Vendor and Product Id: 10 Bytes"
    struct Vendor final {
        static constexpr uint8_t kYearIsModelYear = 255;
        static constexpr int16_t kDefaultYear = 2025;

        std::array<char, 3> manufacturerId = {'G', 'G', 'L'};
        uint16_t manufacturerProductCode = 1;
        uint32_t manufacturerSerialNumber = 1;
        // 0 = unused, 1...54 = week of the year, 255 = Year is model year.
        uint8_t manufacturedWeek = kYearIsModelYear;
        // 1990...2245
        int16_t manufacturedYear = kDefaultYear;
    };

    enum class ColorBitDepth : uint8_t {
        Undefined = 0,
        Six = 1,
        Eight = 2,
        Ten = 3,
        Twelve = 4,
        Fourteen = 5,
        Sixteen = 6,
    };

    enum class DigitalInterface : uint8_t {
        Undefined = 0,
        DVI = 1,
        HDMIa = 2,
        HDMIb = 3,
        MDDI = 4,
        DisplayPort = 5,
    };

    enum class Supported : uint8_t {
        No = 0,
        Yes = 1,
    };

    enum class DigitalSupportedColorEncodingFormats : uint8_t {
        RGB444 = 0,
        RGB444PlusYCrCb444 = 1,
        RGB444PlusYCrCb422 = 2,
        RGB444PlusYCrCb444PlusYCrCb422 = 3,
    };

    // Ref: "3.5 EDID Struct Version & Revision: 2 Bytes"
    struct EdidVersion final {
        uint8_t version;
        uint8_t revision;
    };

    // Ref: "3.6.4 Feature Support: 1 Byte",
    // Note: Variant of the structure for a digital display
    struct DigitalDisplayFeatureSupport final {
        Supported standby = Supported::No;
        Supported suspend = Supported::No;
        Supported activeOff = Supported::No;
        DigitalSupportedColorEncodingFormats digitalFormats =
                DigitalSupportedColorEncodingFormats::RGB444;
        Supported srgb = Supported::No;
        Supported preferredIsNative = Supported::Yes;
        Supported continuousFrequency = Supported::No;
    };

    // Ref: "3.6 Basic Display Parameters and Features: 5 Bytes"
    // Note: Variant of the structure for a digital display
    struct BasicDigitalDisplayParametersAndFeatures final {
        static constexpr auto kDefaultGamma = 2.3F;
        static constexpr auto kDefaultHorizontalSizeCm = 64;
        static constexpr auto kDefaultVerticalSizeCm = 36;

        ColorBitDepth depth = ColorBitDepth::Ten;
        DigitalInterface interface = DigitalInterface::Undefined;
        uint8_t horizontalSizeCm = kDefaultHorizontalSizeCm;
        uint8_t verticalSizeCm = kDefaultVerticalSizeCm;
        float gamma = kDefaultGamma;
        DigitalDisplayFeatureSupport features;
    };

    enum class SignalInterlace : uint8_t {
        NotInterlaced = 0,
        Interlaced = 1,
    };

    enum class StereoMode : uint8_t {
        NoStereo = 0b000,     // Low bit is a don't care.
        NoStereoAlt = 0b001,  //
        FieldSequentialRight = 0b010,
        FieldSequentialLeft = 0b100,
        Interleaved2WayRight = 0b011,
        Interleaved2WayLeft = 0b101,
        Interleaved4Way = 0b110,
        SideBySide = 0b111,
    };

    enum class SyncPolarity : uint8_t {
        Negative = 0,
        Positive = 1,
    };

    // Ref: "3.10.2 Detailed Timing Descriptor: 18 bytes"
    // Note: Variant of the structure for a digital display
    struct DigitalSeparateDetailedTimingDescriptor final {
        struct Timing {
            struct InnerTiming {
                uint16_t addressable = 0;
                uint8_t border = 0;
                uint16_t frontPorch = 0;
                uint16_t syncPulse = 0;
                uint16_t backPorch = 0;
                SyncPolarity syncPolarity = SyncPolarity::Positive;
            };

            uint32_t pixelClockMhz = 0;
            InnerTiming horizontal;
            InnerTiming vertical;

            // This function synthesizes a valid DTD for a given pixel size and refresh rate, for
            // use with injecting fake/simulated displays with vkms.
            //
            // IMPORTANT: This is not part of the EDID standard, and further is not intended to be
            // used to generate the timing values for a real display.
            static auto synthesize(ui::Size displayed, int refreshRate) -> Timing;
        };

        // These timing values are the standard timings for a 1920x1080 panel running at 60Hz.
        static constexpr Timing k1920x1080x60HzStandard = {
                .pixelClockMhz = 148'500'000,  // 148.5 Mhz
                .horizontal =
                        {
                                .addressable = 1920,
                                .border = 0,
                                .frontPorch = 88,
                                .syncPulse = 44,
                                .backPorch = 148,
                                .syncPolarity = SyncPolarity::Positive,
                        },
                .vertical =
                        {
                                .addressable = 1080,
                                .border = 0,
                                .frontPorch = 4,
                                .syncPulse = 5,
                                .backPorch = 36,
                                .syncPolarity = SyncPolarity::Positive,
                        },
        };

        static constexpr uint16_t kDefaultHorizontalImageSizeMm = 640;
        static constexpr uint16_t kDefaultVerticalImageSizeMm = 360;

        Timing timing = k1920x1080x60HzStandard;
        uint16_t horizontalImageSizeMm = kDefaultHorizontalImageSizeMm;
        uint16_t verticalImageSizeMm = kDefaultVerticalImageSizeMm;
        SignalInterlace signalInterlace = SignalInterlace::NotInterlaced;
        StereoMode stereoMode = StereoMode::NoStereo;
    };

    // The required portion of the base 1.4 EDID, lacking only the three optional descriptors.
    struct Version1r4Required {
        Vendor vendor;
        BasicDigitalDisplayParametersAndFeatures parametersAndFeatures;
        DigitalSeparateDetailedTimingDescriptor preferred;
    };

    EdidBuilder();

    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    // clang-tidy incorrectly shows this error for C++23 "deducing this" methods.
    auto set(this EdidBuilder&& self, const Version1r4Required& values) -> EdidBuilder&&;

    // Note: The string value is truncated if more than kMaxProductNameStringLength characters are
    // given.
    auto addDisplayProductNameStringDescriptor(this EdidBuilder&& self,
                                               std::string_view value) -> EdidBuilder&&;

    auto build(this EdidBuilder&& self) -> EdidBytes;
    // NOLINTEND(readability-convert-member-functions-to-static)

  private:
    EdidBytes edid;

    // Descriptor1 must be a Detailed Timing Descriptor. Any additional descriptors start after.
    size_t nextDescriptorOffset;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
