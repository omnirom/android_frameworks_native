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

#pragma once

#include <memory>
#include <vector>

namespace android::surfaceflinger::frontend {

class LayerHierarchy;

namespace caching {

// Represents a set of LayerHierarchies that, combined, can form a new set of LayerHierarchies that
// is visually equivalent to the first set of LayerHierarchies, such that if the new set of
// LayerHierarchies replaced the old set, the overall layer graph would be simpler.
//
// The MergeableHierarchy is conceptually owned by exactly one LayerHierarchy. If that
// LayerHierarchy is destroyed, then the entire MergeableHierarchy must also be destroyed. If any
// other LayerHierarchy composing the MergeableHierarchy is destroyed or mutated, then the
// MergeableHierarchy should also be invalidated.
class MergeableHierarchy {
public:
    // Useful state of a hierarchy
    struct HierarchyState {
        uint32_t layerId;
        const LayerHierarchy* hierarchy;
    };

    // Accumulates LayerHierarchies to construct an MergeableHierarchy.
    class Accumulator {
    public:
        // Add a new LayerHierarchy to the equivalency. True if adding it was successful
        bool add(const LayerHierarchy* hierarchy);

        // True if building an MergeableHierarchy is possible
        bool canBuild() { return !mHierarchies.empty(); }

        // Builds an MergeableHierarchy, and ascribes an owner for it.
        std::unique_ptr<MergeableHierarchy> build(uint32_t owner) {
            return std::make_unique<MergeableHierarchy>(owner, std::move(mHierarchies));
        }

    private:
        std::vector<HierarchyState> mHierarchies;
    };

    MergeableHierarchy(uint32_t owner, std::vector<HierarchyState>&& hierarchies)
          : mHierarchies(std::move(hierarchies)), mId(owner) {}

    uint32_t getId() const { return mId; }

    void dump(std::ostream& out) const;

private:
    std::vector<HierarchyState> mHierarchies;
    const uint32_t mId;
};

} // namespace caching
} // namespace android::surfaceflinger::frontend