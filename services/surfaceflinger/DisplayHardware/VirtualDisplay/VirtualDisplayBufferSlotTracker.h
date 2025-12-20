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

#include <gui/BufferQueue.h>
#include <ui/GraphicBuffer.h>
#include <utils/LruCache.h>

#include <cstdint>
#include <set>

namespace android {

/**
 * Maps GraphicBuffer ids to HWC slots.
 *
 * Note: the maxCapacity is set to BufferQueue::NUM_BUFFER_SLOTS by default. This is the same as the
 * value used in HWC, but should be updated if this value becomes dynamic. It must be the same as
 * the value set on HWC by setClientTargetSlotCount.
 *
 * Not threadsafe.
 */
class VirtualDisplayBufferSlotTracker : public OnEntryRemoved<uint64_t, uint32_t> {
public:
    explicit VirtualDisplayBufferSlotTracker(uint32_t maxCapeacity = BufferQueue::NUM_BUFFER_SLOTS);
    virtual ~VirtualDisplayBufferSlotTracker();

    struct Slot {
        // If true, the full GraphicBuffer must be sent to HWC. This is the case for a buffer that
        // is not currently tracked, or is reusing a slot that previously held a different buffer.
        bool requiresRefresh;
        // The HWC slot assigned to the GraphicBuffer.
        uint32_t slot;
    };

    /**
     * Gets an HWC slot for a given GraphicBuffer.
     *
     * This function manages an LRU cache mapping buffers to HWC slots. If a buffer is already
     * tracked, its existing slot is returned and `requiresRefresh` is false. If the buffer is not
     * tracked, a new or recycled slot is assigned and `requiresRefresh` is true. If the cache is
     * full, the least recently used buffer is evicted to make room.
     */
    Slot getSlot(const sp<GraphicBuffer>& buffer);

private:
    virtual void operator()(uint64_t& key, uint32_t& value) override;

    std::set<uint32_t> mOpenSlots;
    LruCache</*bufferId*/ uint64_t, /*slotId*/ uint32_t> mCache;
};

} // namespace android
