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

#include <gtest/gtest.h>
#include <ui/GraphicBuffer.h>

#include "DisplayHardware/VirtualDisplay/VirtualDisplayBufferSlotTracker.h"

namespace android {
namespace {


TEST(VirtualDisplayBufferSlotTrackerTest, GetSlot_NewBuffer) {
    VirtualDisplayBufferSlotTracker tracker(3);
    sp<GraphicBuffer> buffer = sp<GraphicBuffer>::make();

    auto [requiresRefresh, slot] = tracker.getSlot(buffer);

    EXPECT_TRUE(requiresRefresh);
    EXPECT_EQ(0u, slot);
}

TEST(VirtualDisplayBufferSlotTrackerTest, GetSlot_ExistingBuffer) {
    VirtualDisplayBufferSlotTracker tracker(3);
    sp<GraphicBuffer> buffer = sp<GraphicBuffer>::make();

    auto [requiresRefresh1, slot1] = tracker.getSlot(buffer);
    EXPECT_TRUE(requiresRefresh1);
    EXPECT_EQ(0u, slot1);

    auto [requiresRefresh2, slot2] = tracker.getSlot(buffer);
    EXPECT_FALSE(requiresRefresh2);
    EXPECT_EQ(slot1, slot2);
}

TEST(VirtualDisplayBufferSlotTrackerTest, GetSlot_CacheFullEvictsLru) {
    VirtualDisplayBufferSlotTracker tracker(3);
    sp<GraphicBuffer> b1 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b2 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b3 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b4 = sp<GraphicBuffer>::make();

    // Fill the cache
    auto [r1, s1] = tracker.getSlot(b1);
    EXPECT_TRUE(r1);
    EXPECT_EQ(0u, s1);
    auto [r2, s2] = tracker.getSlot(b2);
    EXPECT_TRUE(r2);
    EXPECT_EQ(1u, s2);
    auto [r3, s3] = tracker.getSlot(b3);
    EXPECT_TRUE(r3);
    EXPECT_EQ(2u, s3);

    // Cache is now full. Adding b4 should evict b1 (the LRU element).
    // The slot for b1 (slot 0) should be reused for b4.
    auto [r4, s4] = tracker.getSlot(b4);
    EXPECT_TRUE(r4);
    EXPECT_EQ(0u, s4);

    // Now b1 is not in the cache. Getting it should evict b2.
    // The slot for b2 (slot 1) should be reused for b1.
    auto [r1_again, s1_again] = tracker.getSlot(b1);
    EXPECT_TRUE(r1_again);
    EXPECT_EQ(1u, s1_again);
}

TEST(VirtualDisplayBufferSlotTrackerTest, GetSlot_LruUpdatedOnAccess) {
    VirtualDisplayBufferSlotTracker tracker(3);
    sp<GraphicBuffer> b1 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b2 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b3 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b4 = sp<GraphicBuffer>::make();

    // Fill the cache. Order of use: b1, b2, b3.
    tracker.getSlot(b1); // slot 0
    tracker.getSlot(b2); // slot 1
    tracker.getSlot(b3); // slot 2

    // Access b1 again to make it the most recently used.
    auto [r1_access, s1_access] = tracker.getSlot(b1);
    EXPECT_FALSE(r1_access);
    EXPECT_EQ(0u, s1_access);

    // LRU order is now b2, b3, b1.
    // Adding b4 should evict b2.
    auto [r4, s4] = tracker.getSlot(b4);
    EXPECT_TRUE(r4);
    EXPECT_EQ(1u, s4); // Slot 1 was b2's slot.
}

TEST(VirtualDisplayBufferSlotTrackerTest, GetSlot_MaxCapacityOne) {
    VirtualDisplayBufferSlotTracker tracker(1);
    sp<GraphicBuffer> b1 = sp<GraphicBuffer>::make();
    sp<GraphicBuffer> b2 = sp<GraphicBuffer>::make();

    auto [r1, s1] = tracker.getSlot(b1);
    EXPECT_TRUE(r1);
    EXPECT_EQ(0u, s1);

    // This should evict b1 and reuse slot 0.
    auto [r2, s2] = tracker.getSlot(b2);
    EXPECT_TRUE(r2);
    EXPECT_EQ(0u, s2);

    // This should evict b2 and reuse slot 0.
    auto [r1_again, s1_again] = tracker.getSlot(b1);
    EXPECT_TRUE(r1_again);
    EXPECT_EQ(0u, s1_again);
}

} // namespace
} // namespace android
