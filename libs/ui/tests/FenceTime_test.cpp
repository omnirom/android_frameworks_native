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
#include <ui/Fence.h>
#include <ui/FenceTime.h>
#include <ui/MockFence.h>

namespace android::ui {
namespace {

using ::testing::Return;

TEST(FenceTimeTest, WasPendingAtViaTestHelperEquals) {
    FenceTime fenceTime(sp<mock::MockFence>::make());
    fenceTime.signalForTest(123);
    EXPECT_TRUE(fenceTime.wasPendingAt(123));
}

TEST(FenceTimeTest, WasPendingAtViaTestHelperBefore) {
    FenceTime fenceTime(sp<mock::MockFence>::make());
    fenceTime.signalForTest(123);
    EXPECT_TRUE(fenceTime.wasPendingAt(100));
}

TEST(FenceTimeTest, WasPendingAtViaTestHelperAfter) {
    FenceTime fenceTime(sp<mock::MockFence>::make());
    fenceTime.signalForTest(123);
    EXPECT_FALSE(fenceTime.wasPendingAt(200));
}

TEST(FenceTimeTest, WasPendingAtViaTestHelperPending) {
    FenceTime fenceTime(sp<mock::MockFence>::make());
    fenceTime.signalForTest(Fence::SIGNAL_TIME_PENDING);
    EXPECT_TRUE(fenceTime.wasPendingAt(123));
}
} // namespace
} // namespace android::ui
