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
#include "MouriMap.h"
#include <SkCanvas.h>
#include <SkColorType.h>
#include <SkPaint.h>
#include <SkTileMode.h>

#include "RuntimeEffectManager.h"

namespace android {
namespace renderengine {
namespace skia {

// NOTE: These shaders are intended to operate on linear color values, but to
// avoid mapping to/from linear Srgb using the SkSL intrinsics, color space
// management is coordinated with the SkColorSpace of the output surfaces and
// with `SkShader::withWorkingColorSpace`.

// This shader must output to a surface with a linear color space, and relies on
// Skia's automatic colorspace conversion to map from the input `bitmap`'s
// color space to said linear color space.
const SkString kEffectSource_MouriMap_CrossTalkAndChunk16x16Effect(R"(
    uniform shader bitmap;
    uniform float inputMultiplier;
    vec4 main(vec2 xy) {
        float maximum = 0.0;
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                float3 linear = bitmap.eval((xy - 0.5) * 16 + 0.5 + vec2(x, y)).rgb * inputMultiplier;
                float maxRGB = max(linear.r, max(linear.g, linear.b));
                maximum = max(maximum, log2(max(maxRGB, 1.0)));
            }
        }
        return float4(float3(maximum), 1.0);
    }
)");
// This shader assumes `bitmap` is already in a linear color space and does not
// perform any color space conversion.
const SkString kEffectSource_MouriMap_Chunk8x8Effect(R"(
    uniform shader bitmap;
    vec4 main(vec2 xy) {
        float maximum = 0.0;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                maximum = max(maximum, bitmap.eval((xy - 0.5) * 8 + 0.5 + vec2(x, y)).r);
            }
        }
        return float4(float3(maximum), 1.0);
    }
)");
// This shader assumes `bitmap` is already in a linear color space and does not
// perform any color space conversion.
const SkString kEffectSource_MouriMap_BlurEffect(R"(
    uniform shader bitmap;
    vec4 main(vec2 xy) {
        float C[5];
        C[0] = 1.0 / 16.0;
        C[1] = 4.0 / 16.0;
        C[2] = 6.0 / 16.0;
        C[3] = 4.0 / 16.0;
        C[4] = 1.0 / 16.0;
        float result = 0.0;
        for (int y = -2; y <= 2; y++) {
            for (int x = -2; x <= 2; x++) {
                result += C[y + 2] * C[x + 2] * bitmap.eval(xy + vec2(x, y)).r;
            }
        }
        return float4(float3(exp2(result)), 1.0);
    }
)");
// This shader assumes `image` is in the original color space and `lux` is in a
// linear color space. The shader must be wrapped with `SkShader::makeWithWorkingColorSpace`
// to automatically convert `image` into the linear color space (`lux`'s conversion
// is a no-op). `makeWithWorkingColorSpace` will also automatically convert back to
// the output color space, avoiding the need to call to/fromLinearSrgb per pixel.
const SkString kEffectSource_MouriMap_TonemapEffect(R"(
    uniform shader image;
    uniform shader lux;
    uniform float scaleFactor;
    uniform float inputMultiplier;
    uniform float targetHdrSdrRatio;
    vec4 main(vec2 xy) {
        float localMax = lux.eval(xy * scaleFactor).r;
        float4 rgba = image.eval(xy);
        float3 linear = rgba.rgb * inputMultiplier;

        if (localMax <= targetHdrSdrRatio) {
            return float4(linear, rgba.a);
        }

        float maxRGB = max(linear.r, max(linear.g, linear.b));
        localMax = max(localMax, maxRGB);
        float gain = (1 + maxRGB * (targetHdrSdrRatio / (localMax * localMax)))
                / (1 + maxRGB / targetHdrSdrRatio);
        return float4(linear * gain, rgba.a);
    }
)");

namespace {

// Draws the given runtime shader on a GPU surface and returns the result as an SkImage.
sk_sp<SkImage> makeImage(SkSurface* surface, const SkRuntimeShaderBuilder& builder) {
    sk_sp<SkShader> shader = builder.makeShader(nullptr);
    LOG_ALWAYS_FATAL_IF(!shader, "%s, Failed to make shader!", __func__);
    SkPaint paint;
    paint.setShader(std::move(shader));
    paint.setBlendMode(SkBlendMode::kSrc);
    surface->getCanvas()->drawPaint(paint);
    return surface->makeTemporaryImage();
}

} // namespace
MouriMap::MouriMap(RuntimeEffectManager& effectManager)
      : mCrosstalkAndChunk16x16(
                effectManager.mKnownEffects[kMouriMap_CrossTalkAndChunk16x16Effect]),
        mChunk8x8(effectManager.mKnownEffects[kMouriMap_Chunk8x8Effect]),
        mBlur(effectManager.mKnownEffects[kMouriMap_BlurEffect]),
        mTonemap(effectManager.mKnownEffects[kMouriMap_TonemapEffect]) {}

sk_sp<SkShader> MouriMap::mouriMap(SkiaGpuContext* context, sk_sp<SkShader> input,
                                   float inputMultiplier, float targetHdrSdrRatio) {
    auto downchunked = downchunk(context, input, inputMultiplier);
    auto localLux = blur(context, downchunked.get());
    return tonemap(input, localLux.get(), inputMultiplier, targetHdrSdrRatio);
}

sk_sp<SkImage> MouriMap::downchunk(SkiaGpuContext* context, sk_sp<SkShader> input,
                                   float inputMultiplier) const {
    SkMatrix matrix;
    SkImage* image = input->isAImage(&matrix, (SkTileMode*)nullptr);
    SkRuntimeShaderBuilder crosstalkAndChunk16x16Builder(mCrosstalkAndChunk16x16);
    crosstalkAndChunk16x16Builder.child("bitmap") = input;
    crosstalkAndChunk16x16Builder.uniform("inputMultiplier") = inputMultiplier;
    // TODO: fp16 might be overkill. Most practical surfaces use 8-bit RGB for HDR UI and 10-bit YUV
    // for HDR video. These downsample operations compute log2(max(linear RGB, 1.0)). So we don't
    // care about LDR precision since they all resolve to LDR-max. For appropriately mastered HDR
    // content that follows BT. 2408, 25% of the bit range for HLG and 42% of the bit range for PQ
    // are reserved for HDR. This means that we can fit the entire HDR range for 10-bit HLG inside
    // of 8 bits. We can also fit about half of the range for PQ, but most content does not fill the
    // entire 10k nit range for PQ. Furthermore, we blur all of this later on anyways, so we might
    // not need to be so precise. So, it's possible that we could use A8 or R8 instead. If we want
    // to be really conservative we can try to use R16 or even RGBA1010102 to fake an R10 surface,
    // which would cut write bandwidth significantly.

    sk_sp<SkColorSpace> linearCS = image->imageInfo().colorSpace()->makeLinearGamma();

    static constexpr auto kFirstDownscaleAmount = 16;
    sk_sp<SkSurface> firstDownsampledSurface = context->createRenderTarget(
            image->imageInfo()
                    .makeWH(std::max(1, image->width() / kFirstDownscaleAmount),
                            std::max(1, image->height() / kFirstDownscaleAmount))
                    .makeColorSpace(linearCS)
                    .makeColorType(kRGBA_F16_SkColorType));
    LOG_ALWAYS_FATAL_IF(!firstDownsampledSurface, "%s: Failed to create surface!", __func__);
    auto firstDownsampledImage =
            makeImage(firstDownsampledSurface.get(), crosstalkAndChunk16x16Builder);

    // Since `secondDownsampledSurface` has the same color space as `firstDownsampledImage`, this
    // is equivalent to using makeRawShader(), but preserves the color space on the objects.
    SkRuntimeShaderBuilder chunk8x8Builder(mChunk8x8);
    chunk8x8Builder.child("bitmap") =
            firstDownsampledImage->makeShader(SkTileMode::kClamp, SkTileMode::kClamp,
                                              SkSamplingOptions());
    static constexpr auto kSecondDownscaleAmount = 8;
    sk_sp<SkSurface> secondDownsampledSurface = context->createRenderTarget(
            firstDownsampledImage->imageInfo()
                    .makeWH(std::max(1, firstDownsampledImage->width() / kSecondDownscaleAmount),
                            std::max(1, firstDownsampledImage->height() / kSecondDownscaleAmount)));
    LOG_ALWAYS_FATAL_IF(!secondDownsampledSurface, "%s: Failed to create surface!", __func__);
    return makeImage(secondDownsampledSurface.get(), chunk8x8Builder);
}
sk_sp<SkImage> MouriMap::blur(SkiaGpuContext* context, SkImage* input) const {
    // Since the `blurSurface` has the same color space as `input`, this will perform no color
    // space conversion (and it is assumed that `input` is already in a linear color space).
    SkRuntimeShaderBuilder blurBuilder(mBlur);
    blurBuilder.child("bitmap") =
            input->makeShader(SkTileMode::kClamp, SkTileMode::kClamp, SkSamplingOptions());
    sk_sp<SkSurface> blurSurface = context->createRenderTarget(input->imageInfo());
    LOG_ALWAYS_FATAL_IF(!blurSurface, "%s: Failed to create surface!", __func__);
    return makeImage(blurSurface.get(), blurBuilder);
}
sk_sp<SkShader> MouriMap::tonemap(sk_sp<SkShader> input, SkImage* localLux, float inputMultiplier,
                                  float targetHdrSdrRatio) const {
    static constexpr float kScaleFactor = 1.0f / 128.0f;
    // The tonemap shader will work in `localLux`'s color space, so that child shader will not
    // actually perform any color space conversion. Skia will automatically convert `input`
    // into this same linear color space.
    SkRuntimeShaderBuilder tonemapBuilder(mTonemap);
    tonemapBuilder.child("image") = input;
    tonemapBuilder.child("lux") =
            localLux->makeShader(SkTileMode::kClamp, SkTileMode::kClamp,
                                 SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone));
    tonemapBuilder.uniform("scaleFactor") = kScaleFactor;
    tonemapBuilder.uniform("inputMultiplier") = inputMultiplier;
    tonemapBuilder.uniform("targetHdrSdrRatio") = targetHdrSdrRatio;

    sk_sp<SkShader> tonemapShader = tonemapBuilder.makeShader();
    // Now wrap the shader in a `makeWithWorkingColorSpace` call with lux's color space so `input`
    // is automatically converted at the start, and the output is automatically converted from
    // lux's space to the final destination color space.
    return tonemapShader->makeWithWorkingColorSpace(localLux->imageInfo().refColorSpace());
}
} // namespace skia
} // namespace renderengine
} // namespace android
