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

#include <SkImage.h>
#include <SkRect.h>
#include <SkRuntimeEffect.h>
#include <SkShader.h>
#include <renderengine/LayerSettings.h>
#include <ui/EdgeExtensionEffect.h>

namespace android::renderengine::skia {

/**
 * This shader is designed to prolong the texture of a surface whose bounds have been extended over
 * the size of the texture. This shader is similar to the default clamp, but adds a blur effect and
 * samples from close to the edge (compared to on the edge) to avoid weird artifacts when elements
 * (in particular, scrollbars) touch the edge.
 */
class EdgeExtensionShaderFactory {
public:
    EdgeExtensionShaderFactory();

    sk_sp<SkShader> createSkShader(const sk_sp<SkShader>& inputShader, const LayerSettings& layer,
                                   const SkRect& imageBounds) const;

private:
    std::unique_ptr<const SkRuntimeEffect::Result> mResult;
};
} // namespace android::renderengine::skia
