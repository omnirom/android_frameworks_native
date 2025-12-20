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

#include <ui/GraphicBuffer.h>
#include <utils/LruCache.h>

#include <cstdint>

#include "VirtualDisplayBufferSlotTracker.h"

namespace android {

VirtualDisplayBufferSlotTracker::VirtualDisplayBufferSlotTracker(uint32_t maxCapacity)
      : mCache(maxCapacity) {
    mCache.setOnEntryRemovedListener(this);
    for (uint32_t i = 0; i < maxCapacity; ++i) {
        mOpenSlots.insert(i);
    }
}

VirtualDisplayBufferSlotTracker::~VirtualDisplayBufferSlotTracker() = default;

VirtualDisplayBufferSlotTracker::Slot VirtualDisplayBufferSlotTracker::getSlot(
        const sp<GraphicBuffer>& buffer) {
    uint64_t bufferId = buffer->getId();

    if (mCache.contains(bufferId)) {
        return {false, mCache.get(bufferId)};
    }

    if (mOpenSlots.empty()) {
        mCache.removeOldest();
    }

    uint32_t slot = *mOpenSlots.begin();
    mOpenSlots.erase(slot);
    mCache.put(bufferId, slot);
    return {true, slot};
}

void VirtualDisplayBufferSlotTracker::operator()(uint64_t&, uint32_t& value) {
    // Since the slot is not being cached any longer, make it available for use.
    mOpenSlots.insert(value);
}

} // namespace android