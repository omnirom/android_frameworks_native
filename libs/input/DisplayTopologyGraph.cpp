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

#define LOG_TAG "DisplayTopologyValidator"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <ftl/enum.h>
#include <input/DisplayTopologyGraph.h>
#include <input/PrintTools.h>
#include <ui/LogicalDisplayId.h>

#include <algorithm>

#define INDENT "  "

namespace android {

namespace {

DisplayTopologyPosition getOppositePosition(DisplayTopologyPosition position) {
    switch (position) {
        case DisplayTopologyPosition::LEFT:
            return DisplayTopologyPosition::RIGHT;
        case DisplayTopologyPosition::TOP:
            return DisplayTopologyPosition::BOTTOM;
        case DisplayTopologyPosition::RIGHT:
            return DisplayTopologyPosition::LEFT;
        case DisplayTopologyPosition::BOTTOM:
            return DisplayTopologyPosition::TOP;
    }
}

bool validatePrimaryDisplay(const android::DisplayTopologyGraph& displayTopologyGraph) {
    return displayTopologyGraph.primaryDisplayId != ui::LogicalDisplayId::INVALID &&
            displayTopologyGraph.graph.contains(displayTopologyGraph.primaryDisplayId);
}

bool validateTopologyGraph(const android::DisplayTopologyGraph& displayTopologyGraph) {
    for (const auto& [sourceDisplay, adjacentDisplays] : displayTopologyGraph.graph) {
        for (const DisplayTopologyAdjacentDisplay& adjacentDisplay : adjacentDisplays) {
            const auto adjacentGraphIt = displayTopologyGraph.graph.find(adjacentDisplay.displayId);
            if (adjacentGraphIt == displayTopologyGraph.graph.end()) {
                LOG(ERROR) << "Missing adjacent display in topology graph: "
                           << adjacentDisplay.displayId << " for source " << sourceDisplay;
                return false;
            }
            const auto reverseEdgeIt =
                    std::find_if(adjacentGraphIt->second.begin(), adjacentGraphIt->second.end(),
                                 [sourceDisplay](const DisplayTopologyAdjacentDisplay&
                                                         reverseAdjacentDisplay) {
                                     return sourceDisplay == reverseAdjacentDisplay.displayId;
                                 });
            if (reverseEdgeIt == adjacentGraphIt->second.end()) {
                LOG(ERROR) << "Missing reverse edge in topology graph for: " << sourceDisplay
                           << " -> " << adjacentDisplay.displayId;
                return false;
            }
            DisplayTopologyPosition expectedPosition =
                    getOppositePosition(adjacentDisplay.position);
            if (reverseEdgeIt->position != expectedPosition) {
                LOG(ERROR) << "Unexpected reverse edge for: " << sourceDisplay << " -> "
                           << adjacentDisplay.displayId
                           << " expected position: " << ftl::enum_string(expectedPosition)
                           << " actual " << ftl::enum_string(reverseEdgeIt->position);
                return false;
            }
            if (reverseEdgeIt->offsetDp != -adjacentDisplay.offsetDp) {
                LOG(ERROR) << "Unexpected reverse edge offset: " << sourceDisplay << " -> "
                           << adjacentDisplay.displayId
                           << " expected offset: " << -adjacentDisplay.offsetDp << " actual "
                           << reverseEdgeIt->offsetDp;
                return false;
            }
        }
    }
    return true;
}

bool validateDensities(const android::DisplayTopologyGraph& displayTopologyGraph) {
    for (const auto& [sourceDisplay, adjacentDisplays] : displayTopologyGraph.graph) {
        if (!displayTopologyGraph.displaysDensity.contains(sourceDisplay)) {
            LOG(ERROR) << "Missing density value in topology graph for display: " << sourceDisplay;
            return false;
        }
    }
    return true;
}

std::string logicalDisplayIdToString(const ui::LogicalDisplayId& displayId) {
    return base::StringPrintf("displayId(%d)", displayId.val());
}

std::string adjacentDisplayToString(const DisplayTopologyAdjacentDisplay& adjacentDisplay) {
    return adjacentDisplay.dump();
}

std::string adjacentDisplayVectorToString(
        const std::vector<DisplayTopologyAdjacentDisplay>& adjacentDisplays) {
    return dumpVector(adjacentDisplays, adjacentDisplayToString);
}

} // namespace

std::string DisplayTopologyAdjacentDisplay::dump() const {
    std::string dump;
    dump += base::StringPrintf("DisplayTopologyAdjacentDisplay: {displayId: %d, position: %s, "
                               "offsetDp: %f}",
                               displayId.val(), ftl::enum_string(position).c_str(), offsetDp);
    return dump;
}

bool DisplayTopologyGraph::isValid() const {
    return validatePrimaryDisplay(*this) && validateTopologyGraph(*this) &&
            validateDensities(*this);
}

std::string DisplayTopologyGraph::dump() const {
    std::string dump;
    dump += base::StringPrintf("PrimaryDisplayId: %d\n", primaryDisplayId.val());
    dump += base::StringPrintf("TopologyGraph:\n");
    dump += addLinePrefix(dumpMap(graph, logicalDisplayIdToString, adjacentDisplayVectorToString),
                          INDENT);
    dump += "\n";
    dump += base::StringPrintf("DisplaysDensity:\n");
    dump += addLinePrefix(dumpMap(displaysDensity, logicalDisplayIdToString), INDENT);
    dump += "\n";
    return dump;
}

} // namespace android
