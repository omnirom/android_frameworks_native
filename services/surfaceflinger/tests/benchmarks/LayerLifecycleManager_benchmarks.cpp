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

#include <memory>
#include <optional>

#include <benchmark/benchmark.h>

#include <Client.h> // temporarily needed for LayerCreationArgs
#include <FrontEnd/LayerCreationArgs.h>
#include <FrontEnd/LayerLifecycleManager.h>
#include <LayerLifecycleManagerHelper.h>

namespace android::surfaceflinger {

namespace {

using namespace android::surfaceflinger::frontend;

static void addRemoveLayers(benchmark::State& state) {
    LayerLifecycleManager lifecycleManager;
    for (auto _ : state) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        layers.emplace_back(LayerLifecycleManagerHelper::rootLayer(1));
        layers.emplace_back(LayerLifecycleManagerHelper::rootLayer(2));
        layers.emplace_back(LayerLifecycleManagerHelper::rootLayer(3));
        lifecycleManager.addLayers(std::move(layers));
        lifecycleManager.onHandlesDestroyed({{1, "1"}, {2, "2"}, {3, "3"}});
        lifecycleManager.commitChanges();
    }
}
BENCHMARK(addRemoveLayers);

static void updateClientStates(benchmark::State& state) {
    LayerLifecycleManager lifecycleManager;
    std::vector<std::unique_ptr<RequestedLayerState>> layers;
    layers.emplace_back(LayerLifecycleManagerHelper::rootLayer(1));
    lifecycleManager.addLayers(std::move(layers));
    lifecycleManager.commitChanges();
    std::vector<QueuedTransactionState> transactions;
    transactions.emplace_back();
    transactions.back().states.push_back({});
    auto& transactionState = transactions.back().states.front();
    transactionState.state.what = layer_state_t::eColorChanged;
    transactionState.state.color.rgb = {0.f, 0.f, 0.f};
    transactionState.layerId = 1;
    lifecycleManager.applyTransactions(transactions);
    lifecycleManager.commitChanges();
    int i = 0;
    for (auto s : state) {
        if (i++ % 100 == 0) i = 0;
        transactionState.state.color.b = static_cast<float>(i / 100.f);
        lifecycleManager.applyTransactions(transactions);
        lifecycleManager.commitChanges();
    }
}
BENCHMARK(updateClientStates);

static void updateClientStatesNoChanges(benchmark::State& state) {
    LayerLifecycleManager lifecycleManager;
    std::vector<std::unique_ptr<RequestedLayerState>> layers;
    layers.emplace_back(LayerLifecycleManagerHelper::rootLayer(1));
    lifecycleManager.addLayers(std::move(layers));
    std::vector<QueuedTransactionState> transactions;
    transactions.emplace_back();
    transactions.back().states.push_back({});
    auto& transactionState = transactions.back().states.front();
    transactionState.state.what = layer_state_t::eColorChanged;
    transactionState.state.color.rgb = {0.f, 0.f, 0.f};
    transactionState.layerId = 1;
    lifecycleManager.applyTransactions(transactions);
    lifecycleManager.commitChanges();
    for (auto _ : state) {
        lifecycleManager.applyTransactions(transactions);
        lifecycleManager.commitChanges();
    }
}
BENCHMARK(updateClientStatesNoChanges);

static void propagateManyHiddenChildren(benchmark::State& state) {
    LayerLifecycleManager lifecycleManager;
    LayerLifecycleManagerHelper helper(lifecycleManager);

    helper.createRootLayer(0);
    for (uint32_t i = 1; i < 50; ++i) {
        helper.createLayer(i, i - 1);
    }

    helper.hideLayer(0);

    LayerHierarchyBuilder hierarchyBuilder;
    DisplayInfo info;
    info.info.logicalHeight = 100;
    info.info.logicalWidth = 100;
    DisplayInfos displayInfos;
    displayInfos.emplace_or_replace(ui::LayerStack::fromValue(1), info);
    ShadowSettings globalShadowSettings;

    LayerSnapshotBuilder snapshotBuilder;

    int i = 1;
    for (auto _ : state) {
        i++;
        helper.setAlpha(0, (1 + (i % 255)) / 255.0f);

        if (lifecycleManager.getGlobalChanges().test(RequestedLayerState::Changes::Hierarchy)) {
            hierarchyBuilder.update(lifecycleManager);
        }
        LayerSnapshotBuilder::Args args{.root = hierarchyBuilder.getHierarchy(),
                                        .layerLifecycleManager = lifecycleManager,
                                        .displays = displayInfos,
                                        .globalShadowSettings = globalShadowSettings,
                                        .supportedLayerGenericMetadata = {},
                                        .genericLayerMetadataKeyMap = {}};
        snapshotBuilder.update(args);
        lifecycleManager.commitChanges();
    }
}
BENCHMARK(propagateManyHiddenChildren);

} // namespace
} // namespace android::surfaceflinger
