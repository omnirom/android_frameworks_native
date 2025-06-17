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
#include <input/DisplayTopologyGraph.h>

#include <string>
#include <string_view>
#include <tuple>

namespace android {

namespace {

constexpr ui::LogicalDisplayId DISPLAY_ID_1{1};
constexpr ui::LogicalDisplayId DISPLAY_ID_2{2};
constexpr int DENSITY_MEDIUM = 160;

} // namespace

using DisplayTopologyGraphTestFixtureParam =
        std::tuple<std::string_view /*name*/, DisplayTopologyGraph, bool /*isValid*/>;

class DisplayTopologyGraphTestFixture
      : public testing::Test,
        public testing::WithParamInterface<DisplayTopologyGraphTestFixtureParam> {};

TEST_P(DisplayTopologyGraphTestFixture, DisplayTopologyGraphTest) {
    const auto& [_, displayTopology, isValid] = GetParam();
    EXPECT_EQ(isValid, displayTopology.isValid());
}

INSTANTIATE_TEST_SUITE_P(
        DisplayTopologyGraphTest, DisplayTopologyGraphTestFixture,
        testing::Values(
                std::make_tuple(
                        "InvalidPrimaryDisplay",
                        DisplayTopologyGraph{.primaryDisplayId = ui::LogicalDisplayId::INVALID,
                                             .graph = {},
                                             .displaysDensity = {}},
                        false),
                std::make_tuple("PrimaryDisplayNotInGraph",
                                DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                                     .graph = {},
                                                     .displaysDensity = {}},
                                false),
                std::make_tuple("DisplayDensityMissing",
                                DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                                     .graph = {{DISPLAY_ID_1, {}}},
                                                     .displaysDensity = {}},
                                false),
                std::make_tuple("ValidSingleDisplayTopology",
                                DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                                     .graph = {{DISPLAY_ID_1, {}}},
                                                     .displaysDensity = {{DISPLAY_ID_1,
                                                                          DENSITY_MEDIUM}}},
                                true),
                std::make_tuple(
                        "MissingReverseEdge",
                        DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                             .graph = {{DISPLAY_ID_1,
                                                        {{DISPLAY_ID_2,
                                                          DisplayTopologyPosition::TOP, 0}}}},
                                             .displaysDensity = {{DISPLAY_ID_1, DENSITY_MEDIUM},
                                                                 {DISPLAY_ID_2, DENSITY_MEDIUM}}},
                        false),
                std::make_tuple(
                        "IncorrectReverseEdgeDirection",
                        DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                             .graph = {{DISPLAY_ID_1,
                                                        {{DISPLAY_ID_2,
                                                          DisplayTopologyPosition::TOP, 0}}},
                                                       {DISPLAY_ID_2,
                                                        {{DISPLAY_ID_1,
                                                          DisplayTopologyPosition::TOP, 0}}}},
                                             .displaysDensity = {{DISPLAY_ID_1, DENSITY_MEDIUM},
                                                                 {DISPLAY_ID_2, DENSITY_MEDIUM}}},
                        false),
                std::make_tuple(
                        "IncorrectReverseEdgeOffset",
                        DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                             .graph = {{DISPLAY_ID_1,
                                                        {{DISPLAY_ID_2,
                                                          DisplayTopologyPosition::TOP, 10}}},
                                                       {DISPLAY_ID_2,
                                                        {{DISPLAY_ID_1,
                                                          DisplayTopologyPosition::BOTTOM, 20}}}},
                                             .displaysDensity = {{DISPLAY_ID_1, DENSITY_MEDIUM},
                                                                 {DISPLAY_ID_2, DENSITY_MEDIUM}}},
                        false),
                std::make_tuple(
                        "ValidMultiDisplayTopology",
                        DisplayTopologyGraph{.primaryDisplayId = DISPLAY_ID_1,
                                             .graph = {{DISPLAY_ID_1,
                                                        {{DISPLAY_ID_2,
                                                          DisplayTopologyPosition::TOP, 10}}},
                                                       {DISPLAY_ID_2,
                                                        {{DISPLAY_ID_1,
                                                          DisplayTopologyPosition::BOTTOM, -10}}}},
                                             .displaysDensity = {{DISPLAY_ID_1, DENSITY_MEDIUM},
                                                                 {DISPLAY_ID_2, DENSITY_MEDIUM}}},
                        true)),
        [](const testing::TestParamInfo<DisplayTopologyGraphTestFixtureParam>& p) {
            return std::string{std::get<0>(p.param)};
        });

} // namespace android
