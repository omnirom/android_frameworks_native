/*
 * Copyright 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *-
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <SkBitmap.h>
#include <SkImage.h>
#include <SkRuntimeEffect.h>

#include <aidl/android/hardware/graphics/composer3/LutProperties.h>
#include <gui/DisplayLuts.h>
#include <ui/GraphicTypes.h>

namespace android {
namespace renderengine {
namespace skia {

class LutShader {
public:
    sk_sp<SkShader> lutShader(sk_sp<SkShader>& input, std::shared_ptr<gui::DisplayLuts> displayLuts,
                              ui::Dataspace srcDataspace, sk_sp<SkColorSpace> outColorSpace);

private:
    sk_sp<SkShader> generateLutShader(sk_sp<SkShader> input, const std::vector<float>& buffers,
                                      const int32_t offset, const int32_t length,
                                      const int32_t dimension, const int32_t size,
                                      const int32_t samplingKey, ui::Dataspace srcDataspace);
    std::unique_ptr<SkRuntimeShaderBuilder> mBuilder;
};

} // namespace skia
} // namespace renderengine
} // namespace android
