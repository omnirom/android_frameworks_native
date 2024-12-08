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

#include <SkRuntimeEffect.h>
#include <SkShader.h>

namespace android {
namespace renderengine {
namespace skia {

/**
 * Generates a shader for computing a gainmap, given an SDR base image and its idealized HDR
 * rendition. The shader follows the procedure in the UltraHDR spec:
 * https://developer.android.com/media/platform/hdr-image-format#gain_map-generation, but makes some
 * simplifying assumptions about metadata typical for RenderEngine's usage.
 */
class GainmapFactory {
public:
    GainmapFactory();
    // Generates the gainmap shader. The hdrSdrRatio is the max_content_boost in the UltraHDR
    // specification.
    sk_sp<SkShader> createSkShader(const sk_sp<SkShader>& sdr, const sk_sp<SkShader>& hdr,
                                   float hdrSdrRatio);

private:
    sk_sp<SkRuntimeEffect> mEffect;
};
} // namespace skia
} // namespace renderengine
} // namespace android
