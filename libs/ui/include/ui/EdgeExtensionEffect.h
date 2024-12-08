/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
 * Stores the edge that will be extended
 */
namespace android {

enum CanonicalDirections { NONE = 0, LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

inline std::string to_string(CanonicalDirections direction) {
    switch (direction) {
        case LEFT:
            return "LEFT";
        case RIGHT:
            return "RIGHT";
        case TOP:
            return "TOP";
        case BOTTOM:
            return "BOTTOM";
        case NONE:
            return "NONE";
    }
}

struct EdgeExtensionEffect {
    EdgeExtensionEffect(bool left, bool right, bool top, bool bottom) {
        extensionEdges = NONE;
        if (left) {
            extensionEdges |= LEFT;
        }
        if (right) {
            extensionEdges |= RIGHT;
        }
        if (top) {
            extensionEdges |= TOP;
        }
        if (bottom) {
            extensionEdges |= BOTTOM;
        }
    }

    EdgeExtensionEffect() { EdgeExtensionEffect(false, false, false, false); }

    bool extendsEdge(CanonicalDirections edge) const { return extensionEdges & edge; }

    bool hasEffect() const { return extensionEdges != NONE; };

    void reset() { extensionEdges = NONE; }

    bool operator==(const EdgeExtensionEffect& other) const {
        return extensionEdges == other.extensionEdges;
    }

    bool operator!=(const EdgeExtensionEffect& other) const { return !(*this == other); }

private:
    int extensionEdges;
};

inline std::string to_string(const EdgeExtensionEffect& effect) {
    std::string s = "EdgeExtensionEffect={edges=[";
    if (effect.hasEffect()) {
        for (CanonicalDirections edge : {LEFT, RIGHT, TOP, BOTTOM}) {
            if (effect.extendsEdge(edge)) {
                s += to_string(edge) + ", ";
            }
        }
    } else {
        s += to_string(NONE);
    }
    s += "]}";
    return s;
}

inline std::ostream& operator<<(std::ostream& os, const EdgeExtensionEffect effect) {
    os << to_string(effect);
    return os;
}
} // namespace android
