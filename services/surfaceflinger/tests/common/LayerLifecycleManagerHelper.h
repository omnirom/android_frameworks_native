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

#include <gui/fake/BufferData.h>
#include <renderengine/mock/FakeExternalTexture.h>
#include <ui/ShadowSettings.h>

#include <Client.h> // temporarily needed for LayerCreationArgs
#include <FrontEnd/LayerCreationArgs.h>
#include <FrontEnd/LayerHierarchy.h>
#include <FrontEnd/LayerLifecycleManager.h>
#include <FrontEnd/LayerSnapshotBuilder.h>
#include <Layer.h> // needed for framerate

namespace android::surfaceflinger::frontend {

class LayerLifecycleManagerHelper {
public:
    LayerLifecycleManagerHelper(LayerLifecycleManager& layerLifecycleManager)
          : mLifecycleManager(layerLifecycleManager) {}
    ~LayerLifecycleManagerHelper() = default;

    static LayerCreationArgs createArgs(uint32_t id, bool canBeRoot, uint32_t parentId,
                                        uint32_t layerIdToMirror) {
        LayerCreationArgs args(std::make_optional(id));
        args.name = "testlayer";
        args.addToRoot = canBeRoot;
        args.parentId = parentId;
        args.layerIdToMirror = layerIdToMirror;
        return args;
    }

    static LayerCreationArgs createDisplayMirrorArgs(uint32_t id,
                                                     ui::LayerStack layerStackToMirror) {
        LayerCreationArgs args(std::make_optional(id));
        args.name = "testlayer";
        args.addToRoot = true;
        args.layerStackToMirror = layerStackToMirror;
        return args;
    }

    static std::unique_ptr<RequestedLayerState> rootLayer(uint32_t id) {
        return std::make_unique<RequestedLayerState>(createArgs(/*id=*/id, /*canBeRoot=*/true,
                                                                /*parent=*/UNASSIGNED_LAYER_ID,
                                                                /*mirror=*/UNASSIGNED_LAYER_ID));
    }

    static std::unique_ptr<RequestedLayerState> childLayer(uint32_t id, uint32_t parentId) {
        return std::make_unique<RequestedLayerState>(createArgs(/*id=*/id, /*canBeRoot=*/false,
                                                                parentId,
                                                                /*mirror=*/UNASSIGNED_LAYER_ID));
    }

    static std::vector<TransactionState> setZTransaction(uint32_t id, int32_t z) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eLayerChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.z = z;
        return transactions;
    }

    void createRootLayer(uint32_t id) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        layers.emplace_back(std::make_unique<RequestedLayerState>(
                createArgs(/*id=*/id, /*canBeRoot=*/true, /*parent=*/UNASSIGNED_LAYER_ID,
                           /*mirror=*/UNASSIGNED_LAYER_ID)));
        mLifecycleManager.addLayers(std::move(layers));
    }

    void createRootLayerWithUid(uint32_t id, gui::Uid uid) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        auto args = createArgs(/*id=*/id, /*canBeRoot=*/true, /*parent=*/UNASSIGNED_LAYER_ID,
                               /*mirror=*/UNASSIGNED_LAYER_ID);
        args.ownerUid = uid.val();
        layers.emplace_back(std::make_unique<RequestedLayerState>(args));
        mLifecycleManager.addLayers(std::move(layers));
    }

    void createDisplayMirrorLayer(uint32_t id, ui::LayerStack layerStack) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        layers.emplace_back(std::make_unique<RequestedLayerState>(
                createDisplayMirrorArgs(/*id=*/id, layerStack)));
        mLifecycleManager.addLayers(std::move(layers));
    }

    void createLayer(uint32_t id, uint32_t parentId) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        layers.emplace_back(std::make_unique<RequestedLayerState>(
                createArgs(/*id=*/id, /*canBeRoot=*/false, /*parent=*/parentId,
                           /*mirror=*/UNASSIGNED_LAYER_ID)));
        mLifecycleManager.addLayers(std::move(layers));
    }

    std::vector<TransactionState> reparentLayerTransaction(uint32_t id, uint32_t newParentId) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().parentId = newParentId;
        transactions.back().states.front().state.what = layer_state_t::eReparent;
        transactions.back().states.front().relativeParentId = UNASSIGNED_LAYER_ID;
        transactions.back().states.front().layerId = id;
        return transactions;
    }

    void reparentLayer(uint32_t id, uint32_t newParentId) {
        mLifecycleManager.applyTransactions(reparentLayerTransaction(id, newParentId));
    }

    std::vector<TransactionState> relativeLayerTransaction(uint32_t id, uint32_t relativeParentId) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().relativeParentId = relativeParentId;
        transactions.back().states.front().state.what = layer_state_t::eRelativeLayerChanged;
        transactions.back().states.front().layerId = id;
        return transactions;
    }

    void reparentRelativeLayer(uint32_t id, uint32_t relativeParentId) {
        mLifecycleManager.applyTransactions(relativeLayerTransaction(id, relativeParentId));
    }

    void removeRelativeZ(uint32_t id) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().state.what = layer_state_t::eLayerChanged;
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setPosition(uint32_t id, float x, float y) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().state.what = layer_state_t::ePositionChanged;
        transactions.back().states.front().state.x = x;
        transactions.back().states.front().state.y = y;
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void mirrorLayer(uint32_t id, uint32_t parentId, uint32_t layerIdToMirror) {
        std::vector<std::unique_ptr<RequestedLayerState>> layers;
        layers.emplace_back(std::make_unique<RequestedLayerState>(
                createArgs(/*id=*/id, /*canBeRoot=*/false, /*parent=*/parentId,
                           /*mirror=*/layerIdToMirror)));
        mLifecycleManager.addLayers(std::move(layers));
    }

    void updateBackgroundColor(uint32_t id, half alpha) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().state.what = layer_state_t::eBackgroundColorChanged;
        transactions.back().states.front().state.bgColor.a = alpha;
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void destroyLayerHandle(uint32_t id) { mLifecycleManager.onHandlesDestroyed({{id, "test"}}); }

    void setZ(uint32_t id, int32_t z) {
        mLifecycleManager.applyTransactions(setZTransaction(id, z));
    }

    void setCrop(uint32_t id, const Rect& crop) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eCropChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.crop = crop;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFlags(uint32_t id, uint32_t mask, uint32_t flags) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eFlagsChanged;
        transactions.back().states.front().state.flags = flags;
        transactions.back().states.front().state.mask = mask;
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setAlpha(uint32_t id, float alpha) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eAlphaChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.color.a = static_cast<half>(alpha);
        mLifecycleManager.applyTransactions(transactions);
    }

    void hideLayer(uint32_t id) {
        setFlags(id, layer_state_t::eLayerHidden, layer_state_t::eLayerHidden);
    }

    void showLayer(uint32_t id) { setFlags(id, layer_state_t::eLayerHidden, 0); }

    void setColor(uint32_t id, half3 rgb = half3(1._hf, 1._hf, 1._hf)) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().state.what = layer_state_t::eColorChanged;
        transactions.back().states.front().state.color.rgb = rgb;
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setLayerStack(uint32_t id, int32_t layerStack) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eLayerStackChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.layerStack = ui::LayerStack::fromValue(layerStack);
        mLifecycleManager.applyTransactions(transactions);
    }

    void setTouchableRegion(uint32_t id, Region region) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eInputInfoChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.windowInfoHandle =
                sp<gui::WindowInfoHandle>::make();
        auto inputInfo = transactions.back().states.front().state.windowInfoHandle->editInfo();
        inputInfo->touchableRegion = region;
        inputInfo->token = sp<BBinder>::make();
        mLifecycleManager.applyTransactions(transactions);
    }

    void setInputInfo(uint32_t id, std::function<void(gui::WindowInfo&)> configureInput) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eInputInfoChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.windowInfoHandle =
                sp<gui::WindowInfoHandle>::make();
        auto inputInfo = transactions.back().states.front().state.windowInfoHandle->editInfo();
        if (!inputInfo->token) {
            inputInfo->token = sp<BBinder>::make();
        }
        configureInput(*inputInfo);

        mLifecycleManager.applyTransactions(transactions);
    }

    void setTouchableRegionCrop(uint32_t id, Region region, uint32_t touchCropId,
                                bool replaceTouchableRegionWithCrop) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eInputInfoChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.windowInfoHandle =
                sp<gui::WindowInfoHandle>::make();
        auto inputInfo = transactions.back().states.front().state.windowInfoHandle->editInfo();
        inputInfo->touchableRegion = region;
        inputInfo->replaceTouchableRegionWithCrop = replaceTouchableRegionWithCrop;
        transactions.back().states.front().touchCropId = touchCropId;

        inputInfo->token = sp<BBinder>::make();
        mLifecycleManager.applyTransactions(transactions);
    }

    void setBackgroundBlurRadius(uint32_t id, uint32_t backgroundBlurRadius) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eBackgroundBlurRadiusChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.backgroundBlurRadius = backgroundBlurRadius;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFrameRateSelectionPriority(uint32_t id, int32_t priority) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eFrameRateSelectionPriority;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.frameRateSelectionPriority = priority;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFrameRate(uint32_t id, float frameRate, int8_t compatibility,
                      int8_t changeFrameRateStrategy) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eFrameRateChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.frameRate = frameRate;
        transactions.back().states.front().state.frameRateCompatibility = compatibility;
        transactions.back().states.front().state.changeFrameRateStrategy = changeFrameRateStrategy;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFrameRate(uint32_t id, Layer::FrameRate framerate) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eFrameRateChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.frameRate = framerate.vote.rate.getValue();
        transactions.back().states.front().state.frameRateCompatibility = 0;
        transactions.back().states.front().state.changeFrameRateStrategy = 0;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFrameRateCategory(uint32_t id, int8_t frameRateCategory) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eFrameRateCategoryChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.frameRateCategory = frameRateCategory;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setFrameRateSelectionStrategy(uint32_t id, int8_t strategy) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what =
                layer_state_t::eFrameRateSelectionStrategyChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.frameRateSelectionStrategy = strategy;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setDefaultFrameRateCompatibility(uint32_t id, int8_t defaultFrameRateCompatibility) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what =
                layer_state_t::eDefaultFrameRateCompatibilityChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.defaultFrameRateCompatibility =
                defaultFrameRateCompatibility;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setRoundedCorners(uint32_t id, float radius) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eCornerRadiusChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.cornerRadius = radius;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setBuffer(uint32_t id, std::shared_ptr<renderengine::ExternalTexture> texture) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eBufferChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().externalTexture = texture;
        transactions.back().states.front().state.bufferData =
                std::make_shared<fake::BufferData>(texture->getId(), texture->getWidth(),
                                                   texture->getHeight(), texture->getPixelFormat(),
                                                   texture->getUsage());
        mLifecycleManager.applyTransactions(transactions);
    }

    void setBuffer(uint32_t id) {
        static uint64_t sBufferId = 1;
        setBuffer(id,
                  std::make_shared<renderengine::mock::
                                           FakeExternalTexture>(1U /*width*/, 1U /*height*/,
                                                                sBufferId++,
                                                                HAL_PIXEL_FORMAT_RGBA_8888,
                                                                GRALLOC_USAGE_PROTECTED /*usage*/));
    }

    void setFrontBuffer(uint32_t id) {
        static uint64_t sBufferId = 1;
        setBuffer(id,
                  std::make_shared<renderengine::mock::FakeExternalTexture>(
                          1U /*width*/, 1U /*height*/, sBufferId++, HAL_PIXEL_FORMAT_RGBA_8888,
                          GRALLOC_USAGE_PROTECTED | AHARDWAREBUFFER_USAGE_FRONT_BUFFER /*usage*/));
    }

    void setBufferCrop(uint32_t id, const Rect& bufferCrop) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eBufferCropChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.bufferCrop = bufferCrop;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setDamageRegion(uint32_t id, const Region& damageRegion) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eSurfaceDamageRegionChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.surfaceDamageRegion = damageRegion;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setDataspace(uint32_t id, ui::Dataspace dataspace) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eDataspaceChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.dataspace = dataspace;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setMatrix(uint32_t id, float dsdx, float dtdx, float dtdy, float dsdy) {
        layer_state_t::matrix22_t matrix{dsdx, dtdx, dtdy, dsdy};

        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eMatrixChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.matrix = matrix;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setShadowRadius(uint32_t id, float shadowRadius) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eShadowRadiusChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.shadowRadius = shadowRadius;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setTrustedOverlay(uint32_t id, gui::TrustedOverlay trustedOverlay) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eTrustedOverlayChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.trustedOverlay = trustedOverlay;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setDropInputMode(uint32_t id, gui::DropInputMode dropInputMode) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().state.what = layer_state_t::eDropInputModeChanged;
        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.dropInputMode = dropInputMode;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setGameMode(uint32_t id, gui::GameMode gameMode) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});
        transactions.back().states.front().state.what = layer_state_t::eMetadataChanged;
        transactions.back().states.front().state.metadata = LayerMetadata();
        transactions.back().states.front().state.metadata.setInt32(METADATA_GAME_MODE,
                                                                   static_cast<int32_t>(gameMode));
        transactions.back().states.front().layerId = id;
        mLifecycleManager.applyTransactions(transactions);
    }

    void setEdgeExtensionEffect(uint32_t id, int edge) {
        std::vector<TransactionState> transactions;
        transactions.emplace_back();
        transactions.back().states.push_back({});

        transactions.back().states.front().layerId = id;
        transactions.back().states.front().state.what |= layer_state_t::eEdgeExtensionChanged;
        transactions.back().states.front().state.edgeExtensionParameters =
                gui::EdgeExtensionParameters();
        transactions.back().states.front().state.edgeExtensionParameters.extendLeft = edge & LEFT;
        transactions.back().states.front().state.edgeExtensionParameters.extendRight = edge & RIGHT;
        transactions.back().states.front().state.edgeExtensionParameters.extendTop = edge & TOP;
        transactions.back().states.front().state.edgeExtensionParameters.extendBottom =
                edge & BOTTOM;
        mLifecycleManager.applyTransactions(transactions);
    }

private:
    LayerLifecycleManager& mLifecycleManager;
};

} // namespace android::surfaceflinger::frontend
