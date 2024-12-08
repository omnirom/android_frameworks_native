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

#include "EdgeExtensionShaderFactory.h"
#include <SkPoint.h>
#include <SkRuntimeEffect.h>
#include <SkStream.h>
#include <SkString.h>
#include <com_android_graphics_libgui_flags.h>
#include "log/log_main.h"

namespace android::renderengine::skia {

static const SkString edgeShader = SkString(R"(
    uniform shader uContentTexture;
    uniform vec2 uImgSize;

    // TODO(b/214232209) oobTolerance is temporary and will be removed when the scrollbar will be
    // hidden during the animation
    const float oobTolerance = 15;
    const int blurRadius = 3;
    const float blurArea = float((2 * blurRadius + 1) * (2 * blurRadius + 1));

    vec4 boxBlur(vec2 p) {
        vec4 sumColors = vec4(0);

        for (int i = -blurRadius; i <= blurRadius; i++) {
            for (int j = -blurRadius; j <= blurRadius; j++) {
                sumColors += uContentTexture.eval(p + vec2(i, j));
            }
        }
        return sumColors / blurArea;
    }

    vec4 main(vec2 coord) {
        vec2 nearestTexturePoint = clamp(coord, vec2(0, 0), uImgSize);
        if (coord == nearestTexturePoint) {
            return uContentTexture.eval(coord);
        } else {
            vec2 samplePoint = nearestTexturePoint + oobTolerance * normalize(
                                    nearestTexturePoint - coord);
            return boxBlur(samplePoint);
        }
    }
)");

EdgeExtensionShaderFactory::EdgeExtensionShaderFactory() {
    if (!com::android::graphics::libgui::flags::edge_extension_shader()) {
        return;
    }
    mResult = std::make_unique<SkRuntimeEffect::Result>(SkRuntimeEffect::MakeForShader(edgeShader));
    LOG_ALWAYS_FATAL_IF(!mResult->errorText.isEmpty(),
                        "EdgeExtensionShaderFactory compilation "
                        "failed with an unexpected error: %s",
                        mResult->errorText.c_str());
}

sk_sp<SkShader> EdgeExtensionShaderFactory::createSkShader(const sk_sp<SkShader>& inputShader,
                                                           const LayerSettings& layer,
                                                           const SkRect& imageBounds) const {
    LOG_ALWAYS_FATAL_IF(mResult == nullptr,
                        "EdgeExtensionShaderFactory did not initialize mResult. "
                        "This means that we unexpectedly applied the edge extension shader");

    SkRuntimeShaderBuilder builder = SkRuntimeShaderBuilder(mResult->effect);

    builder.child("uContentTexture") = inputShader;
    if (imageBounds.isEmpty()) {
        builder.uniform("uImgSize") = SkPoint{layer.geometry.boundaries.getWidth(),
                                              layer.geometry.boundaries.getHeight()};
    } else {
        builder.uniform("uImgSize") = SkPoint{imageBounds.width(), imageBounds.height()};
    }
    return builder.makeShader();
}
} // namespace android::renderengine::skia