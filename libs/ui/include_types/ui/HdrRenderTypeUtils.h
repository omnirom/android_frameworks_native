/*
 * Copyright 2021 The Android Open Source Project
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

#include <ui/GraphicTypes.h>

namespace android {

enum class HdrRenderType {
    SDR,         // just render to SDR
    DISPLAY_HDR, // HDR by extended brightness
    GENERIC_HDR  // tonemapped HDR
};

/***
 * A helper function to classify how we treat the result based on params.
 *
 * @param dataspace the dataspace
 * @param pixelFormat optional, in case there is no source buffer.
 * @param hdrSdrRatio default is 1.f, render engine side doesn't take care of it.
 * @return HdrRenderType
 */
inline HdrRenderType getHdrRenderType(ui::Dataspace dataspace,
                                      std::optional<ui::PixelFormat> pixelFormat,
                                      float hdrSdrRatio = 1.f, bool hasHdrMetadata = false) {
    const auto transfer = dataspace & HAL_DATASPACE_TRANSFER_MASK;
    const auto range = dataspace & HAL_DATASPACE_RANGE_MASK;

    if (transfer == HAL_DATASPACE_TRANSFER_ST2084 || transfer == HAL_DATASPACE_TRANSFER_HLG) {
        return HdrRenderType::GENERIC_HDR;
    }

    static const auto BT2020_LINEAR_EXT = static_cast<ui::Dataspace>(HAL_DATASPACE_STANDARD_BT2020 |
                                                                     HAL_DATASPACE_TRANSFER_LINEAR |
                                                                     HAL_DATASPACE_RANGE_EXTENDED);

    if ((dataspace == BT2020_LINEAR_EXT || dataspace == ui::Dataspace::V0_SCRGB) &&
        pixelFormat.has_value() && pixelFormat.value() == ui::PixelFormat::RGBA_FP16 &&
        hasHdrMetadata) {
        return HdrRenderType::GENERIC_HDR;
    }

    // Extended range layer with an hdr/sdr ratio of > 1.01f can "self-promote" to HDR.
    if (range == HAL_DATASPACE_RANGE_EXTENDED && hdrSdrRatio > 1.01f) {
        return HdrRenderType::DISPLAY_HDR;
    }

    return HdrRenderType::SDR;
}

/**
 * Returns the maximum headroom allowed for this content under "idealized"
 * display conditions (low surround luminance, high-enough display brightness).
 *
 * TODO: take into account hdr metadata, but square it with the fact that some
 * HLG content has CTA.861-3 metadata
 */
inline float getIdealizedMaxHeadroom(ui::Dataspace dataspace) {
    const auto transfer = dataspace & HAL_DATASPACE_TRANSFER_MASK;

    switch (transfer) {
        case HAL_DATASPACE_TRANSFER_ST2084:
            return 10000.0f / 203.0f;
        case HAL_DATASPACE_TRANSFER_HLG:
            return 1000.0f / 203.0f;
        default:
            return 1.0f;
    }
}

} // namespace android
