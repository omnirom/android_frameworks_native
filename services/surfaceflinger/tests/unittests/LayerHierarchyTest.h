/*
 * Copyright 2022 The Android Open Source Project
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <gui/fake/BufferData.h>
#include <renderengine/mock/FakeExternalTexture.h>
#include <ui/ShadowSettings.h>

#include "Client.h" // temporarily needed for LayerCreationArgs
#include "FrontEnd/LayerCreationArgs.h"
#include "FrontEnd/LayerHierarchy.h"
#include "FrontEnd/LayerLifecycleManager.h"
#include "LayerLifecycleManagerHelper.h"

#include "FrontEnd/LayerSnapshotBuilder.h"

namespace android::surfaceflinger::frontend {

class LayerHierarchyTestBase : public testing::Test, public LayerLifecycleManagerHelper {
protected:
    LayerHierarchyTestBase() : LayerLifecycleManagerHelper(mLifecycleManager) {
        // tree with 3 levels of children
        // ROOT
        // ├── 1
        // │   ├── 11
        // │   │   └── 111
        // │   ├── 12
        // │   │   ├── 121
        // │   │   └── 122
        // │   │       └── 1221
        // │   └── 13
        // └── 2

        createRootLayer(1);
        createRootLayer(2);
        createLayer(11, 1);
        createLayer(12, 1);
        createLayer(13, 1);
        createLayer(111, 11);
        createLayer(121, 12);
        createLayer(122, 12);
        createLayer(1221, 122);
    }

    std::vector<uint32_t> getTraversalPath(const LayerHierarchy& hierarchy) const {
        std::vector<uint32_t> layerIds;
        hierarchy.traverse([&layerIds = layerIds](const LayerHierarchy& hierarchy,
                                                  const LayerHierarchy::TraversalPath&) -> bool {
            layerIds.emplace_back(hierarchy.getLayer()->id);
            return true;
        });
        return layerIds;
    }

    std::vector<uint32_t> getTraversalPathInZOrder(const LayerHierarchy& hierarchy) const {
        std::vector<uint32_t> layerIds;
        hierarchy.traverseInZOrder(
                [&layerIds = layerIds](const LayerHierarchy& hierarchy,
                                       const LayerHierarchy::TraversalPath&) -> bool {
                    layerIds.emplace_back(hierarchy.getLayer()->id);
                    return true;
                });
        return layerIds;
    }

    void updateAndVerify(LayerHierarchyBuilder& hierarchyBuilder) {
        hierarchyBuilder.update(mLifecycleManager);
        mLifecycleManager.commitChanges();

        // rebuild layer hierarchy from scratch and verify that it matches the updated state.
        LayerHierarchyBuilder newBuilder;
        newBuilder.update(mLifecycleManager);
        EXPECT_EQ(getTraversalPath(hierarchyBuilder.getHierarchy()),
                  getTraversalPath(newBuilder.getHierarchy()));
        EXPECT_EQ(getTraversalPathInZOrder(hierarchyBuilder.getHierarchy()),
                  getTraversalPathInZOrder(newBuilder.getHierarchy()));
        EXPECT_FALSE(
                mLifecycleManager.getGlobalChanges().test(RequestedLayerState::Changes::Hierarchy));
    }

    LayerLifecycleManager mLifecycleManager;
};

class LayerSnapshotTestBase : public LayerHierarchyTestBase {
protected:
    LayerSnapshotTestBase() : LayerHierarchyTestBase() {}

    void update(LayerSnapshotBuilder& snapshotBuilder) {
        mHierarchyBuilder.update(mLifecycleManager);
        LayerSnapshotBuilder::Args args{.root = mHierarchyBuilder.getHierarchy(),
                                        .layerLifecycleManager = mLifecycleManager,
                                        .includeMetadata = false,
                                        .displays = mFrontEndDisplayInfos,
                                        .displayChanges = mHasDisplayChanges,
                                        .globalShadowSettings = globalShadowSettings,
                                        .supportsBlur = true,
                                        .supportedLayerGenericMetadata = {},
                                        .genericLayerMetadataKeyMap = {}};
        snapshotBuilder.update(args);

        mLifecycleManager.commitChanges();
    }

    LayerHierarchyBuilder mHierarchyBuilder;

    DisplayInfos mFrontEndDisplayInfos;
    bool mHasDisplayChanges = false;

    ShadowSettings globalShadowSettings;
};

} // namespace android::surfaceflinger::frontend
