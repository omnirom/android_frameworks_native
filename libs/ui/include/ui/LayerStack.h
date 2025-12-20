/*
 * Copyright 2021 The Android Open Source Project
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

#include <cstdint>

#include <ftl/cast.h>
#include <ftl/string.h>
#include <log/log.h>

namespace android::ui {

// A LayerStack identifies a Z-ordered group of layers. A layer can only be associated to a single
// LayerStack, and a LayerStack should be unique to each display in the composition target.
struct LayerStack {
    uint32_t id = UINT32_MAX;

    template <typename T>
    static constexpr LayerStack fromValue(T v) {
        if (ftl::cast_safety<uint32_t>(v) == ftl::CastSafety::kSafe) {
            return {static_cast<uint32_t>(v)};
        }

        ALOGW("Invalid layer stack %s", ftl::to_string(v).c_str());
        return {};
    }
};

// An unassigned LayerStack can indicate that a layer is offscreen and will not be
// rendered onto a display. Multiple displays are allowed to have unassigned LayerStacks.
constexpr LayerStack UNASSIGNED_LAYER_STACK;
constexpr LayerStack DEFAULT_LAYER_STACK{0u};

inline bool operator==(LayerStack lhs, LayerStack rhs) {
    return lhs.id == rhs.id;
}

inline bool operator!=(LayerStack lhs, LayerStack rhs) {
    return !(lhs == rhs);
}

inline bool operator>(LayerStack lhs, LayerStack rhs) {
    return lhs.id > rhs.id;
}

inline bool operator<(LayerStack lhs, LayerStack rhs) {
    return lhs.id < rhs.id;
}

} // namespace android::ui

namespace std {
template <>
struct hash<android::ui::LayerStack> {
    size_t operator()(const android::ui::LayerStack& layerStack) const {
        return hash<uint32_t>()(layerStack.id);
    }
};
} // namespace std
