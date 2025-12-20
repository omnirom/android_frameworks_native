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

#include <android/configuration.h>
#include <com_android_input_flags.h>
#include <gtest/gtest.h>
#include <input/DisplayTopologyGraph.h>

#include <string>
#include <string_view>
#include <tuple>

#include "ScopedFlagOverride.h"

namespace android {

namespace {

constexpr ui::LogicalDisplayId DISPLAY_ID_1{1};
constexpr ui::LogicalDisplayId DISPLAY_ID_2{2};

} // namespace

using DisplayProperties = DisplayTopologyGraph::Properties;
using AdjacentDisplaysVector = std::vector<DisplayTopologyAdjacentDisplay>;
using DisplayPropertiesMap = std::unordered_map<ui::LogicalDisplayId, DisplayProperties>;

using DisplayTopologyGraphTestFixtureParam =
        std::tuple<std::string_view /*name*/, ui::LogicalDisplayId /*primaryDisplayId*/,
                   DisplayPropertiesMap, bool /*isValid*/>;

class DisplayTopologyGraphTestFixture
      : public testing::Test,
        public testing::WithParamInterface<DisplayTopologyGraphTestFixtureParam> {};

TEST_P(DisplayTopologyGraphTestFixture, DisplayTopologyGraphTest) {
    SCOPED_FLAG_OVERRIDE(enable_display_topology_validation, true);
    auto [_, primaryDisplayId, graph, isValid] = GetParam();
    auto result = DisplayTopologyGraph::create(primaryDisplayId, std::move(graph));
    EXPECT_EQ(isValid, result.ok());
}

INSTANTIATE_TEST_SUITE_P(
        DisplayTopologyGraphTest, DisplayTopologyGraphTestFixture,
        testing::Values(
                std::make_tuple("InvalidPrimaryDisplay",
                                /*primaryDisplayId=*/ui::LogicalDisplayId::INVALID,
                                /*graph=*/DisplayPropertiesMap{}, false),
                std::make_tuple("PrimaryDisplayNotInGraph",
                                /*primaryDisplayId=*/DISPLAY_ID_1,
                                /*graph=*/DisplayPropertiesMap{}, false),
                std::make_tuple(
                        "DisplayDensityInvalid",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays = AdjacentDisplaysVector{},
                                                   .density = -1,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}}},
                        false),
                std::make_tuple(
                        "DisplayBoundsInvalid",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays = AdjacentDisplaysVector{},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 0, 0)}}},
                        false),
                std::make_tuple(
                        "ValidSingleDisplayTopology",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays = AdjacentDisplaysVector{},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}}},
                        true),
                std::make_tuple(
                        "MissingReverseEdge",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::TOP,
                                                                    0}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}},
                                {DISPLAY_ID_2,
                                 DisplayProperties{.adjacentDisplays = AdjacentDisplaysVector{},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 100, 100,
                                                                                 200)}}},
                        false),
                std::make_tuple(
                        "IncorrectReverseEdgeDirection",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::TOP,
                                                                    0}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}},
                                {DISPLAY_ID_2,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_1,
                                                                    DisplayTopologyPosition::TOP,
                                                                    0}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 100, 100,
                                                                                 200)}}},
                        false),
                std::make_tuple(
                        "IncorrectReverseEdgeOffset",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::TOP,
                                                                    10}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}},
                                {DISPLAY_ID_2,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_1,
                                                                    DisplayTopologyPosition::TOP,
                                                                    20}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 100, 100,
                                                                                 200)}}},
                        false),
                std::make_tuple(
                        "ValidMultiDisplayTopology",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::TOP,
                                                                    10}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}},
                                {DISPLAY_ID_2,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_1,
                                                                    DisplayTopologyPosition::BOTTOM,
                                                                    -10}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 100, 100,
                                                                                 200)}}},
                        true),
                std::make_tuple(
                        "ValidMultiDisplayTopologyCornerAdjacency",
                        /*primaryDisplayId=*/DISPLAY_ID_1,
                        /*graph=*/
                        DisplayPropertiesMap{
                                {DISPLAY_ID_1,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::RIGHT,
                                                                    0},
                                                                   {DISPLAY_ID_2,
                                                                    DisplayTopologyPosition::BOTTOM,
                                                                    0}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(0, 0, 100, 100)}},
                                {DISPLAY_ID_2,
                                 DisplayProperties{.adjacentDisplays =
                                                           AdjacentDisplaysVector{
                                                                   {DISPLAY_ID_1,
                                                                    DisplayTopologyPosition::LEFT,
                                                                    0},
                                                                   {DISPLAY_ID_1,
                                                                    DisplayTopologyPosition::TOP,
                                                                    0}},
                                                   .density = ACONFIGURATION_DENSITY_MEDIUM,
                                                   .boundsInGlobalDp = FloatRect(100, 100, 200,
                                                                                 200)}}},
                        true)),
        [](const testing::TestParamInfo<DisplayTopologyGraphTestFixtureParam>& p) {
            return std::string{std::get<0>(p.param)};
        });

} // namespace android
