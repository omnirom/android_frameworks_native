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
#include "FrontEnd/Caching/MergeableHierarchy.h"
#include "ftl/small_map.h"

namespace android::surfaceflinger::frontend::caching {

// Manages the lifecycle of MergeableHierarchies constructed from the layer graph
class MergeableHierarchyManager {
public:
    // Adds a new MergeableHierarchy to be tracked by the manager
    void add(std::unique_ptr<MergeableHierarchy>&& mergeableHierarchy) {
        mMergeableHierarchies.emplace_back(std::move(mergeableHierarchy));
    }

    // Removes an MergeableHierarchy, if it exists, based on its owner ID
    void remove(uint32_t id) {
        std::erase_if(mMergeableHierarchies, [id](const auto& mergeableHierarchy) {
            return mergeableHierarchy->getId() == id;
        });
    }

    // Dumps all tracked MergeableHiearchies to a string
    std::string dump() const {
        std::ostringstream os;
        dump(os);
        return os.str();
    }

    // Dumps all tracked MergeableHiearchies to the supplied ostream
    void dump(std::ostream& out) const {
        out << "\nMergeable Hierarchies\n";
        for (const auto& mergeableHierarchy : mMergeableHierarchies) {
            out << "  ";
            mergeableHierarchy->dump(out);
            out << "\n";
        }
    }

private:
    // TODO: use a better data structure for this. Conceptually we want a set of sets
    // so that destroying a LayerHierarchy won't cause a linear time search.
    std::vector<std::unique_ptr<caching::MergeableHierarchy>> mMergeableHierarchies;
};

} // namespace android::surfaceflinger::frontend::caching