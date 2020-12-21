/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include <compositionengine/impl/HwcBufferCache.h>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include <gui/BufferQueue.h>
#include <ui/GraphicBuffer.h>
#include <cstdlib>
#include <cutils/properties.h>
#include <QtiGrallocDefs.h>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"

constexpr int MAX_VIDEO_WIDTH = 5760;
constexpr int MAX_VIDEO_HEIGHT = 2160;
constexpr int MAX_NUM_SLOTS_FOR_WIDE_VIDEOS = 4;

namespace android::compositionengine::impl {

HwcBufferCache::HwcBufferCache() {
    std::fill(std::begin(mBuffers), std::end(mBuffers), wp<GraphicBuffer>(nullptr));
    char value[PROPERTY_VALUE_MAX];
    property_get("vendor.display.reduce_slots_for_wide_video", value, "1");
    mReduceSlotsForWideVideo = atoi(value);
}

//TODO: Move to common location
static bool formatIsYuv(const PixelFormat format) {
    switch (format) {
        case HAL_PIXEL_FORMAT_YCBCR_422_SP:
        case HAL_PIXEL_FORMAT_YCRCB_420_SP:
        case HAL_PIXEL_FORMAT_YCBCR_422_I:
        case HAL_PIXEL_FORMAT_YCBCR_420_888:
        case HAL_PIXEL_FORMAT_Y8:
        case HAL_PIXEL_FORMAT_Y16:
        case HAL_PIXEL_FORMAT_YV12:
        case HAL_PIXEL_FORMAT_YCBCR_P010:
        case HAL_PIXEL_FORMAT_NV12_ENCODEABLE:
        case HAL_PIXEL_FORMAT_NV21_ENCODEABLE:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO:
        case HAL_PIXEL_FORMAT_YCrCb_422_SP:
        case HAL_PIXEL_FORMAT_YCbCr_444_SP:
        case HAL_PIXEL_FORMAT_YCrCb_444_SP:
        case HAL_PIXEL_FORMAT_YCrCb_422_I:
        case HAL_PIXEL_FORMAT_NV21_ZSL:
        case HAL_PIXEL_FORMAT_YCrCb_420_SP_VENUS:
        case HAL_PIXEL_FORMAT_NV12_HEIF:
        case HAL_PIXEL_FORMAT_YCbCr_420_P010_UBWC:
        case HAL_PIXEL_FORMAT_YCbCr_420_P010_VENUS:
        case HAL_PIXEL_FORMAT_CbYCrY_422_I:
        case HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT:
        case HAL_PIXEL_FORMAT_YCbCr_422_I_10BIT_COMPRESSED:
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_VENUS_UBWC:
        case HAL_PIXEL_FORMAT_YCbCr_420_TP10_UBWC:
            return true;
         default:
            return false;
    }
}

void HwcBufferCache::getHwcBuffer(int slot, const sp<GraphicBuffer>& buffer, uint32_t* outSlot,
                                  sp<GraphicBuffer>* outBuffer) {
    // default is 0
    wp<GraphicBuffer> weakCopy(buffer);
    uint32_t width = 0;
    uint32_t height = 0;
    PixelFormat format = PIXEL_FORMAT_NONE;
    if (buffer) {
        width = buffer->getWidth();
        height = buffer->getHeight();
        format = buffer->getPixelFormat();
    }
    bool widevideo = false;
    uint32_t numSlots = BufferQueue::NUM_BUFFER_SLOTS;

    // Workaround to reduce slots for 8k buffers
    if ((width * height > MAX_VIDEO_WIDTH * MAX_VIDEO_HEIGHT) && mReduceSlotsForWideVideo &&
        formatIsYuv(format)) {
        numSlots = MAX_NUM_SLOTS_FOR_WIDE_VIDEOS;
        widevideo = true;
    }
    if (slot == BufferQueue::INVALID_BUFFER_SLOT || slot < 0 ||
        slot >= numSlots) {
        *outSlot = 0;
        if (widevideo && slot >= numSlots) {
            *outSlot = mNextSlot % numSlots;
            mNextSlot = *outSlot + 1;
        }
    } else {
        *outSlot = static_cast<uint32_t>(slot);
    }

    auto& currentBuffer = mBuffers[*outSlot];
    if (currentBuffer == weakCopy) {
        // already cached in HWC, skip sending the buffer
        *outBuffer = nullptr;
    } else {
        *outBuffer = buffer;

        // update cache
        currentBuffer = buffer;
    }
}

} // namespace android::compositionengine::impl
