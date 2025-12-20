/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <common/FlagManager.h>
#include <ui/LayerStack.h>

namespace android {

// A LayerFilter determines if a layer is included for output to a display.
struct LayerFilter {
    ui::LayerStack layerStack;

    // True if the layer is only output to internal displays, i.e. excluded from screenshots, screen
    // recordings, and mirroring to virtual or external displays. Used for display cutout overlays.
    bool toInternalDisplay = false;

    // When true for Output LayerFilters, this indicates the Output respects the skipScreenshot
    // flag (i.e. the Output is used to take a screenshot). When true for Layer LayerFilters, it
    // means the layer has requested to be skipped in screenshots.
    bool skipScreenshot = false;

    // Returns true if the input filter can be output to this filter.
    bool includes(LayerFilter other) const {
        // The layer stacks must match.
        if (other.layerStack == ui::UNASSIGNED_LAYER_STACK || other.layerStack != layerStack) {
            return false;
        }

        if (FlagManager::getInstance().connected_displays_cursor()) {
            return !(skipScreenshot && other.skipScreenshot);
        }

        // The output must be to an internal display if the input filter has that constraint.
        return !other.toInternalDisplay || toInternalDisplay;
    }
};

inline bool operator==(LayerFilter lhs, LayerFilter rhs) {
    return lhs.layerStack == rhs.layerStack && lhs.toInternalDisplay == rhs.toInternalDisplay;
}

} // namespace android
