/*
 * Copyright 2024 The Android Open Source Project
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

#include <android-base/result.h>
#include <android/configuration.h>
#include <ftl/enum.h>
#include <ui/FloatRect.h>
#include <ui/LogicalDisplayId.h>
#include <ui/Transform.h>

#include <cinttypes>
#include <unordered_map>
#include <vector>

namespace android {

/**
 * The edge of the current display, where adjacent display is attached to.
 */
enum class DisplayTopologyPosition : int32_t {
    LEFT = 0,
    TOP = 1,
    RIGHT = 2,
    BOTTOM = 3,

    ftl_last = BOTTOM
};

/**
 * Directed edge in the graph of adjacent displays.
 */
struct DisplayTopologyAdjacentDisplay {
    ui::LogicalDisplayId displayId = ui::LogicalDisplayId::INVALID;
    // Position of the adjacent display, relative to the source display.
    DisplayTopologyPosition position;
    // The offset in DP of the adjacent display, relative to the source display.
    float offsetDp;

    std::string dump() const;
};

/**
 * Directed Graph representation of Display Topology.
 */
struct DisplayTopologyGraph {
    struct Properties {
        std::vector<DisplayTopologyAdjacentDisplay> adjacentDisplays;
        int32_t density;
        FloatRect boundsInGlobalDp;
    };

    ui::LogicalDisplayId primaryDisplayId = ui::LogicalDisplayId::INVALID;
    std::unordered_map<ui::LogicalDisplayId, Properties> graph;

    ui::Transform localPxToGlobalDpTransform(ui::LogicalDisplayId displayId) const;
    ui::Transform globalDpToLocalPxTransform(ui::LogicalDisplayId displayId) const;

    DisplayTopologyGraph() = default;
    std::string dump() const;

    // Builds the topology graph from components.
    // Returns error if a valid graph cannot be built from the supplied components.
    static base::Result<const DisplayTopologyGraph> create(
            ui::LogicalDisplayId primaryDisplay,
            std::unordered_map<ui::LogicalDisplayId, Properties>&& topologyGraph);

private:
    DisplayTopologyGraph(ui::LogicalDisplayId primaryDisplay,
                         std::unordered_map<ui::LogicalDisplayId, Properties>&& topologyGraph);
};

} // namespace android
