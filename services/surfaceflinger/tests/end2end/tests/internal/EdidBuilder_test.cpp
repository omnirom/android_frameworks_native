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
#include <iterator>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <ui/Size.h>

#include "test_framework/core/EdidBuilder.h"

namespace android::surfaceflinger::tests::end2end {
namespace {

using namespace std::string_view_literals;

using EdidBuilder = end2end::test_framework::core::EdidBuilder;

TEST(EdidBuilderSynthesizeTiming, for1920x1080x60Hz) {
    using Timing = EdidBuilder::DigitalSeparateDetailedTimingDescriptor::Timing;

    const auto result = Timing::synthesize({1920, 1080}, 60);

    EXPECT_EQ(result.pixelClockMhz, 162'000'000);

    EXPECT_EQ(result.horizontal.addressable, 1920);
    EXPECT_EQ(result.horizontal.border, 0);
    EXPECT_EQ(result.horizontal.frontPorch, 88);
    EXPECT_EQ(result.horizontal.syncPulse, 44);
    EXPECT_EQ(result.horizontal.backPorch, 348);
    EXPECT_EQ(result.horizontal.syncPolarity, EdidBuilder::SyncPolarity::Positive);

    EXPECT_EQ(result.vertical.addressable, 1080);
    EXPECT_EQ(result.vertical.border, 0);
    EXPECT_EQ(result.vertical.frontPorch, 4);
    EXPECT_EQ(result.vertical.syncPulse, 5);
    EXPECT_EQ(result.vertical.backPorch, 36);
    EXPECT_EQ(result.vertical.syncPolarity, EdidBuilder::SyncPolarity::Positive);
}

TEST(EdidBuilderSynthesizeTiming, for1920x1080x144Hz) {
    using Timing = EdidBuilder::DigitalSeparateDetailedTimingDescriptor::Timing;

    const auto result = Timing::synthesize({1920, 1080}, 144);

    EXPECT_EQ(result.pixelClockMhz, 388'800'000);

    EXPECT_EQ(result.horizontal.addressable, 1920);
    EXPECT_EQ(result.horizontal.border, 0);
    EXPECT_EQ(result.horizontal.frontPorch, 88);
    EXPECT_EQ(result.horizontal.syncPulse, 44);
    EXPECT_EQ(result.horizontal.backPorch, 348);
    EXPECT_EQ(result.horizontal.syncPolarity, EdidBuilder::SyncPolarity::Positive);

    EXPECT_EQ(result.vertical.addressable, 1080);
    EXPECT_EQ(result.vertical.border, 0);
    EXPECT_EQ(result.vertical.frontPorch, 4);
    EXPECT_EQ(result.vertical.syncPulse, 5);
    EXPECT_EQ(result.vertical.backPorch, 36);
    EXPECT_EQ(result.vertical.syncPolarity, EdidBuilder::SyncPolarity::Positive);
}

TEST(EdidBuilderSynthesizeTiming, for3840x2160x60Hz) {
    using Timing = EdidBuilder::DigitalSeparateDetailedTimingDescriptor::Timing;

    const auto result = Timing::synthesize({3840, 2160}, 60);

    EXPECT_EQ(result.pixelClockMhz, 580'800'000);

    EXPECT_EQ(result.horizontal.addressable, 3840);
    EXPECT_EQ(result.horizontal.border, 0);
    EXPECT_EQ(result.horizontal.frontPorch, 88);
    EXPECT_EQ(result.horizontal.syncPulse, 44);
    EXPECT_EQ(result.horizontal.backPorch, 428);
    EXPECT_EQ(result.horizontal.syncPolarity, EdidBuilder::SyncPolarity::Positive);

    EXPECT_EQ(result.vertical.addressable, 2160);
    EXPECT_EQ(result.vertical.border, 0);
    EXPECT_EQ(result.vertical.frontPorch, 4);
    EXPECT_EQ(result.vertical.syncPulse, 5);
    EXPECT_EQ(result.vertical.backPorch, 31);
    EXPECT_EQ(result.vertical.syncPolarity, EdidBuilder::SyncPolarity::Positive);
}

TEST(EdidBuilder, DefaultPrebuiltMatchesDefaultWhenBuilt) {
    const auto prebuilt = EdidBuilder::kDefaultEdid;
    const auto built = EdidBuilder().set(EdidBuilder::Version1r4Required{}).build();

    const auto result = std::ranges::mismatch(prebuilt, built);
    const auto prebuiltIndex = std::distance(prebuilt.begin(), result.in1);
    const auto builtIndex = std::distance(built.begin(), result.in2);

    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
    EXPECT_EQ(prebuilt, built) << fmt::format(
            "Difference at prebuilt index {} with byte 0x{:x} vs built index {} with byte 0x{:x} ",
            prebuiltIndex, prebuiltIndex < prebuilt.size() ? prebuilt[prebuiltIndex] : 0,
            builtIndex, builtIndex < built.size() ? built[builtIndex] : 0);
    // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)
}

}  // namespace
}  // namespace android::surfaceflinger::tests::end2end
