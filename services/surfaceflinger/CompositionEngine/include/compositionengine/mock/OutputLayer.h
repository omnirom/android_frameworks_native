/*
 * Copyright 2019 The Android Open Source Project
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

#include <optional>

#include <compositionengine/CompositionEngine.h>
#include <compositionengine/LayerFE.h>
#include <compositionengine/Output.h>
#include <compositionengine/OutputLayer.h>
#include <compositionengine/impl/OutputLayerCompositionState.h>
#include <gmock/gmock.h>
#include <cstdint>

namespace android::compositionengine::mock {

class OutputLayer : public compositionengine::OutputLayer {
public:
    OutputLayer();
    virtual ~OutputLayer();

    MOCK_METHOD1(setHwcLayer, void(std::shared_ptr<HWC2::Layer>));

    MOCK_METHOD1(uncacheBuffers, void(const std::vector<uint64_t>&));

    MOCK_CONST_METHOD0(getOutput, const compositionengine::Output&());
    MOCK_CONST_METHOD0(getLayerFE, compositionengine::LayerFE&());

    MOCK_CONST_METHOD0(getState, const impl::OutputLayerCompositionState&());
    MOCK_METHOD0(editState, impl::OutputLayerCompositionState&());

    MOCK_METHOD(void, updateCompositionState,
                (bool, bool, ui::Transform::RotationFlags,
                 (const std::optional<std::vector<std::optional<
                          aidl::android::hardware::graphics::composer3::LutProperties>>>)));
    MOCK_METHOD(void, writeStateToHWC, (bool, bool, uint32_t, bool, bool, bool));
    MOCK_CONST_METHOD0(writeCursorPositionToHWC, void());

    MOCK_CONST_METHOD0(getHwcLayer, HWC2::Layer*());
    MOCK_CONST_METHOD0(requiresClientComposition, bool());
    MOCK_CONST_METHOD0(isHardwareCursor, bool());
    MOCK_METHOD1(applyDeviceCompositionTypeChange,
                 void(aidl::android::hardware::graphics::composer3::Composition));
    MOCK_METHOD0(prepareForDeviceLayerRequests, void());
    MOCK_METHOD1(applyDeviceLayerRequest, void(Hwc2::IComposerClient::LayerRequest request));
    MOCK_CONST_METHOD0(needsFiltering, bool());
    MOCK_CONST_METHOD0(getOverrideCompositionSettings, std::optional<LayerFE::LayerSettings>());
    MOCK_METHOD(void, applyDeviceLayerLut,
                (::android::base::unique_fd,
                 (std::vector<std::pair<
                          int, aidl::android::hardware::graphics::composer3::LutProperties>>)));
    MOCK_METHOD(int64_t, getPictureProfilePriority, (), (const));
    MOCK_METHOD(const PictureProfileHandle&, getPictureProfileHandle, (), (const));
    MOCK_METHOD(void, commitPictureProfileToCompositionState, ());
    MOCK_CONST_METHOD1(dump, void(std::string&));
};

} // namespace android::compositionengine::mock
