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

#include "GainmapFactory.h"

#include <log/log.h>

#include "RuntimeEffectManager.h"

namespace android {
namespace renderengine {
namespace skia {

// Please refer to https://developer.android.com/media/platform/hdr-image-format#gain_map-generation
// This shader assumes that it will be used in a linear gamma colorspace context, e.g. all values
// sampled from `sdr` and `hdr` are linear and it is outputting an encoded ratio of luminances.
const SkString kEffectSource_GainmapEffect(R"(
    uniform shader sdr;
    uniform shader hdr;
    uniform float mapMaxLog2;

    const float mapMinLog2 = 0.0;
    const float mapGamma = 1.0;
    const float offsetSdr = 0.015625;
    const float offsetHdr = 0.015625;

    float luminance(vec3 linearColor) {
        return 0.2126 * linearColor.r + 0.7152 * linearColor.g + 0.0722 * linearColor.b;
    }

    vec4 main(vec2 xy) {
        float sdrY = luminance(sdr.eval(xy).rgb);
        float hdrY = luminance(hdr.eval(xy).rgb);
        float pixelGain = (hdrY + offsetHdr) / (sdrY + offsetSdr);
        float logRecovery = (log2(pixelGain) - mapMinLog2) / (mapMaxLog2 - mapMinLog2);
        return vec4(pow(clamp(logRecovery, 0.0, 1.0), mapGamma));
    }
)");


GainmapFactory::GainmapFactory(RuntimeEffectManager& effectManager) {
    mEffect = effectManager.mKnownEffects[kGainmapEffect];
}

sk_sp<SkShader> GainmapFactory::createSkShader(const sk_sp<SkShader>& sdr,
                                               const sk_sp<SkShader>& hdr, float hdrSdrRatio) {
    SkRuntimeShaderBuilder shaderBuilder(mEffect);
    shaderBuilder.child("sdr") = sdr;
    shaderBuilder.child("hdr") = hdr;
    shaderBuilder.uniform("mapMaxLog2") = std::log2(hdrSdrRatio);
    return shaderBuilder.makeShader();
}

} // namespace skia
} // namespace renderengine
} // namespace android
