/*
 * Copyright 2025 The Android Open Source Project
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

#include "GraphitePipelineManager.h"

#include <common/trace.h>
#include <ftl/enum.h>
#include <include/core/SkData.h>
#include <include/core/SkRefCnt.h>
#include <include/core/SkSpan.h>
#include <include/effects/SkRuntimeEffect.h>
#include <include/gpu/graphite/GraphiteTypes.h>
#include <include/gpu/graphite/PrecompileContext.h>
#include <include/gpu/graphite/precompile/PaintOptions.h>
#include <include/gpu/graphite/precompile/Precompile.h>
#include <include/gpu/graphite/precompile/PrecompileColorFilter.h>
#include <include/gpu/graphite/precompile/PrecompileRuntimeEffect.h>
#include <include/gpu/graphite/precompile/PrecompileShader.h>
#include <include/gpu/graphite/vk/precompile/VulkanPrecompileShader.h>
#include <include/gpu/vk/VulkanTypes.h>
#include <pthread.h>

#include <type_traits>

#include "PaintOptionsBuilder.h"
#include "skia/filters/RuntimeEffectManager.h"

namespace android::renderengine::skia {

using namespace skgpu::graphite;
using namespace PaintOptionsUtils;
using PrecompileShaders::ImageShaderFlags;

using ::skgpu::graphite::DrawTypeFlags;
using ::skgpu::graphite::PaintOptions;
using ::skgpu::graphite::RenderPassProperties;

struct PrecompileSettings {
    PrecompileSettings(const skgpu::graphite::PaintOptions& paintOptions,
                       skgpu::graphite::DrawTypeFlags drawTypeFlags,
                       const skgpu::graphite::RenderPassProperties& renderPassProps,
                       bool analyticClipping = false)
          : fPaintOptions(paintOptions),
            fDrawTypeFlags(drawTypeFlags),
            fRenderPassProps({&renderPassProps, 1}),
            fAnalyticClipping(analyticClipping) {}

    PrecompileSettings(const skgpu::graphite::PaintOptions& paintOptions,
                       skgpu::graphite::DrawTypeFlags drawTypeFlags,
                       SkSpan<const skgpu::graphite::RenderPassProperties> renderPassProps,
                       bool analyticClipping = false)
          : fPaintOptions(paintOptions),
            fDrawTypeFlags(drawTypeFlags),
            fRenderPassProps(renderPassProps),
            fAnalyticClipping(analyticClipping) {}

    skgpu::graphite::PaintOptions fPaintOptions;
    skgpu::graphite::DrawTypeFlags fDrawTypeFlags = skgpu::graphite::DrawTypeFlags::kNone;
    SkSpan<const skgpu::graphite::RenderPassProperties> fRenderPassProps;
    bool fAnalyticClipping = false;
};

// Used in lieu of SkEnumBitMask to avoid adding casts when copying in precompile cases.
static constexpr DrawTypeFlags operator|(DrawTypeFlags a, DrawTypeFlags b) {
    return static_cast<DrawTypeFlags>(static_cast<std::underlying_type<DrawTypeFlags>::type>(a) |
                                      static_cast<std::underlying_type<DrawTypeFlags>::type>(b));
}

sk_sp<PrecompileShader> vulkan_ycbcr_image_shader(uint64_t format,
                                                  VkSamplerYcbcrModelConversion model,
                                                  VkSamplerYcbcrRange range,
                                                  VkChromaLocation location, bool pqCS = false) {
    SkColorInfo ci{kRGBA_8888_SkColorType, kPremul_SkAlphaType,
                   pqCS ? SkColorSpace::MakeRGB(SkNamedTransferFn::kPQ, SkNamedGamut::kRec2020)
                        : nullptr};

    skgpu::VulkanYcbcrConversionInfo info;

    info.fExternalFormat = format;
    info.fYcbcrModel = model;
    info.fYcbcrRange = range;
    info.fXChromaOffset = location;
    info.fYChromaOffset = location;
    info.fChromaFilter = VK_FILTER_LINEAR;

    return PrecompileShaders::VulkanYCbCrImage(info,
                                               PrecompileShaders::ImageShaderFlags::kExcludeCubic,
                                               {&ci, 1}, {});
}

// Specifies the child shader to be created for a LinearEffect
enum class ChildType {
    kSolidColor,
    kHWTexture,
    kHWTextureYCbCr247,
};

sk_sp<PrecompileShader> create_child_shader(ChildType childType) {
    switch (childType) {
        case ChildType::kSolidColor:
            return PrecompileShaders::Color();
        case ChildType::kHWTexture: {
            SkColorInfo ci{kRGBA_8888_SkColorType, kPremul_SkAlphaType,
                           SkColorSpace::MakeRGB(SkNamedTransferFn::kSRGB,
                                                 SkNamedGamut::kAdobeRGB)};

            return PrecompileShaders::Image(PrecompileShaders::ImageShaderFlags::kExcludeCubic,
                                            {&ci, 1}, {});
        }
        case ChildType::kHWTextureYCbCr247:
            // HardwareImage(3: kEwAAPcAAAAAAAAA)
            return vulkan_ycbcr_image_shader(247, VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020,
                                             VK_SAMPLER_YCBCR_RANGE_ITU_NARROW,
                                             VK_CHROMA_LOCATION_COSITED_EVEN,
                                             /* pqCS= */ true);
    }

    return nullptr;
}

skgpu::graphite::PaintOptions LinearEffect(RuntimeEffectManager& effectManager,
                                           KnownEffectId linearEffect, ChildType childType,
                                           SkBlendMode blendMode, bool paintColorIsOpaque = true,
                                           bool matrixColorFilter = false, bool dither = false) {
    PaintOptions paintOptions;
    sk_sp<PrecompileShader> child = create_child_shader(childType);
    paintOptions.setShaders(
            {PrecompileRuntimeEffects::MakePrecompileShader(effectManager
                                                                    .mKnownEffects[linearEffect],
                                                            {{std::move(child)}})});
    if (matrixColorFilter) {
        paintOptions.setColorFilters({PrecompileColorFilters::Matrix()});
    }
    paintOptions.setBlendModes({blendMode});
    paintOptions.setPaintColorIsOpaque(paintColorIsOpaque);
    paintOptions.setDither(dither);

    return paintOptions;
}

// =======================================
//         PaintOptions
// =======================================
// NOTE: keep in sync with upstream external/skia/tests/graphite/precompile/AndroidPaintOptions.cpp
// clang-format off

// TODO(b/426601394): Update this to take an SkColorInfo for the input image.
// The other MouriMap* precompile paint options should use a linear SkColorInfo
// derived from this same input image.
skgpu::graphite::PaintOptions MouriMapCrosstalkAndChunk16x16Passthrough(
        RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> crosstalk = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kMouriMap_CrossTalkAndChunk16x16Effect],
            {{std::move(img)}});

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(crosstalk) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions MouriMapCrosstalkAndChunk16x16Premul(
        RuntimeEffectManager& effectManager) {
    // This usage of kUnpremul is non-obvious. It acts to short circuit the identity-colorspace
    // optimization for runtime effects. In this case, the Pipeline requires a
    // ColorSpaceTransformPremul instead of the (optimized) Passthrough.
    SkColorInfo ci { kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> crosstalk = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kMouriMap_CrossTalkAndChunk16x16Effect],
            {{std::move(img)}});

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(crosstalk) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions MouriMapChunk8x8Effect(RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_F16_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGBLinear() };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> chunk8x8 = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kMouriMap_Chunk8x8Effect],
            {{std::move(img)}});

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(chunk8x8) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions MouriMapBlur(RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_F16_SkColorType, kPremul_SkAlphaType, SkColorSpace::MakeSRGBLinear() };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> blur = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kMouriMap_BlurEffect],
            {{std::move(img)}});

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(blur) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions MouriMapToneMap(RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> input = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                             { &ci, 1 },
                                                             {});

    SkColorInfo luxCI { kRGBA_F16_SkColorType,
                        kPremul_SkAlphaType,
                        SkColorSpace::MakeSRGBLinear() };
    sk_sp<PrecompileShader> lux = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                            { &luxCI, 1 },
                                                            {});

    sk_sp<PrecompileShader> toneMap = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kMouriMap_TonemapEffect],
            {{std::move(input)}, {std::move(lux)}});
    sk_sp<PrecompileShader> inLinear =
            toneMap->makeWithWorkingColorSpace(luxCI.refColorSpace());

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(inLinear) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}


skgpu::graphite::PaintOptions KawaseBlurLowSrcSrcOver(RuntimeEffectManager& effectManager) {
    sk_sp<SkRuntimeEffect> lowSampleBlurEffect = effectManager.mKnownEffects[kKawaseBlurDualFilter_LowSampleBlurEffect];

    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> kawase = PrecompileRuntimeEffects::MakePrecompileShader(
            std::move(lowSampleBlurEffect),
            { { img } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(kawase) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc, SkBlendMode::kSrcOver });
    return paintOptions;
}

skgpu::graphite::PaintOptions KawaseBlurHighSrc(RuntimeEffectManager& effectManager) {
    sk_sp<SkRuntimeEffect> highSampleBlurEffect = effectManager.mKnownEffects[kKawaseBlurDualFilter_HighSampleBlurEffect];

    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> kawase = PrecompileRuntimeEffects::MakePrecompileShader(
            std::move(highSampleBlurEffect),
            { { img } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(kawase) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions BlurFilterMix(RuntimeEffectManager& effectManager) {
    sk_sp<SkRuntimeEffect> mixEffect = effectManager.mKnownEffects[
            kBlurFilter_MixEffect];

    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> mix = PrecompileRuntimeEffects::MakePrecompileShader(
            std::move(mixEffect),
            { { img }, { img } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(mix) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

PaintOptions ImagePremulYCbCr238Srcover(bool narrow) {
    PaintOptions paintOptions;

    // HardwareImage(3: kHoAAO4AAAAAAAAA)
    paintOptions.setShaders({ vulkan_ycbcr_image_shader(238,
                                                        VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
                                                        narrow ? VK_SAMPLER_YCBCR_RANGE_ITU_NARROW
                                                               : VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
                                                        VK_CHROMA_LOCATION_MIDPOINT) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    return paintOptions;
}

PaintOptions TransparentPaintImagePremulYCbCr238Srcover() {
    PaintOptions paintOptions;

    // HardwareImage(3: kHoAAO4AAAAAAAAA)
    paintOptions.setShaders({ vulkan_ycbcr_image_shader(238,
                                                        VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
                                                        VK_SAMPLER_YCBCR_RANGE_ITU_NARROW,
                                                        VK_CHROMA_LOCATION_MIDPOINT) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    paintOptions.setPaintColorIsOpaque(false);
    return paintOptions;
}

PaintOptions ImagePremulYCbCr240Srcover() {
    PaintOptions paintOptions;

    // HardwareImage(3: kHIAAPAAAAAAAAAA)
    paintOptions.setShaders({ vulkan_ycbcr_image_shader(240,
                                                        VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
                                                        VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
                                                        VK_CHROMA_LOCATION_MIDPOINT) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    return paintOptions;
}

PaintOptions TransparentPaintImagePremulYCbCr240Srcover() {
    PaintOptions paintOptions;

    // HardwareImage(3: kHIAAPAAAAAAAAAA)
    paintOptions.setShaders({ vulkan_ycbcr_image_shader(240,
                                                        VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709,
                                                        VK_SAMPLER_YCBCR_RANGE_ITU_FULL,
                                                        VK_CHROMA_LOCATION_MIDPOINT) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    paintOptions.setPaintColorIsOpaque(false);
    return paintOptions;
}

skgpu::graphite::PaintOptions MouriMapCrosstalkAndChunk16x16YCbCr247(
        RuntimeEffectManager& effectManager) {
    PaintOptions paintOptions;

    // HardwareImage(3: kEwAAPcAAAAAAAAA)
    sk_sp<PrecompileShader> img = vulkan_ycbcr_image_shader(
            247,
            VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020,
            VK_SAMPLER_YCBCR_RANGE_ITU_NARROW,
            VK_CHROMA_LOCATION_COSITED_EVEN,
            /*pqCS=*/true);

    sk_sp<PrecompileShader> crosstalk = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.getKnownRuntimeEffect(
                    kMouriMap_CrossTalkAndChunk16x16Effect),
            { { std::move(img) } });

    paintOptions.setShaders({ std::move(crosstalk) });
    paintOptions.setBlendModes({ SkBlendMode::kSrc });
    return paintOptions;
}

skgpu::graphite::PaintOptions EdgeExtensionPassthroughSrcover(RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };

    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> edgeEffect = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kEdgeExtensionEffect],
            { { std::move(img) } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(edgeEffect) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    return paintOptions;
}

skgpu::graphite::PaintOptions EdgeExtensionPremulSrcover(RuntimeEffectManager& effectManager) {
    // This usage of kUnpremul is non-obvious. It acts to short circuit the identity-colorspace
    // optimization for runtime effects. In this case, the Pipeline requires a
    // ColorSpaceTransformPremul instead of the (optimized) Passthrough.
    SkColorInfo ci { kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, nullptr };

    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> edgeEffect = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kEdgeExtensionEffect],
            { { std::move(img) } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(edgeEffect) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    return paintOptions;
}



skgpu::graphite::PaintOptions TransparentPaintEdgeExtensionPassthroughMatrixCFDitherSrcover(
        RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> edgeEffect = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kEdgeExtensionEffect],
            { { std::move(img) } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(edgeEffect) });
    paintOptions.setColorFilters({ PrecompileColorFilters::Matrix() });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    paintOptions.setPaintColorIsOpaque(false);
    paintOptions.setDither(true);

    return paintOptions;
}

skgpu::graphite::PaintOptions TransparentPaintEdgeExtensionPassthroughSrcover(
        RuntimeEffectManager& effectManager) {
    SkColorInfo ci { kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr };
    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> edgeEffect = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kEdgeExtensionEffect],
            { { std::move(img) } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(edgeEffect) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    paintOptions.setPaintColorIsOpaque(false);

    return paintOptions;
}

skgpu::graphite::PaintOptions TransparentPaintEdgeExtensionPremulSrcover(
        RuntimeEffectManager& effectManager) {
    // This usage of kUnpremul is non-obvious. It acts to short circuit the identity-colorspace
    // optimization for runtime effects. In this case, the Pipeline requires a
    // ColorSpaceTransformPremul instead of the (optimized) Passthrough.
    SkColorInfo ci { kRGBA_8888_SkColorType, kUnpremul_SkAlphaType, nullptr };

    sk_sp<PrecompileShader> img = PrecompileShaders::Image(ImageShaderFlags::kExcludeCubic,
                                                           { &ci, 1 },
                                                           {});

    sk_sp<PrecompileShader> edgeEffect = PrecompileRuntimeEffects::MakePrecompileShader(
            effectManager.mKnownEffects[kEdgeExtensionEffect],
            { { std::move(img) } });

    PaintOptions paintOptions;
    paintOptions.setShaders({ std::move(edgeEffect) });
    paintOptions.setBlendModes({ SkBlendMode::kSrcOver });
    paintOptions.setPaintColorIsOpaque(false);

    return paintOptions;
}

// clang-format on

// =======================================
//         RenderPassProperties
// =======================================
// NOTE: keep in sync with upstream external/skia/tests/graphite/precompile/AndroidPaintOptions.cpp
// clang-format off

// Single sampled R w/ just depth
const skgpu::graphite::RenderPassProperties kR_1_D {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kAlpha_8_SkColorType,
        /* fDstCS= */ nullptr,
        /* fRequiresMSAA= */ false
};

// Single sampled RGBA w/ just depth
const skgpu::graphite::RenderPassProperties kRGBA_1_D {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kRGBA_8888_SkColorType,
        /* fDstCS= */ nullptr,
        /* fRequiresMSAA= */ false
};

// The same as kRGBA_1_D but w/ an SRGB colorSpace
const skgpu::graphite::RenderPassProperties kRGBA_1_D_SRGB {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        /* fRequiresMSAA= */ false
};

// MSAA RGBA w/ depth and stencil
const skgpu::graphite::RenderPassProperties kRGBA_4_DS {
        skgpu::graphite::DepthStencilFlags::kDepthStencil,
        kRGBA_8888_SkColorType,
        /* fDstCS= */ nullptr,
        /* fRequiresMSAA= */ true
};

// The same as kRGBA_4_DS but w/ an SRGB colorSpace
const skgpu::graphite::RenderPassProperties kRGBA_4_DS_SRGB {
        skgpu::graphite::DepthStencilFlags::kDepthStencil,
        kRGBA_8888_SkColorType,
        SkColorSpace::MakeSRGB(),
        /* fRequiresMSAA= */ true
};

// Single sampled RGBA16F w/ just depth
const skgpu::graphite::RenderPassProperties kRGBA16F_1_D {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kRGBA_F16_SkColorType,
        /* fDstCS= */ nullptr,
        /* fRequiresMSAA= */ false
};

// The same as kRGBA16F_1_D but w/ an SRGB colorSpace
const skgpu::graphite::RenderPassProperties kRGBA16F_1_D_SRGB {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kRGBA_F16_SkColorType,
        SkColorSpace::MakeSRGB(),
        /* fRequiresMSAA= */ false
};

// The same as kRGBA16F_1_D but w/ a linear SRGB colorSpace
const skgpu::graphite::RenderPassProperties kRGBA16F_1_D_Linear {
        skgpu::graphite::DepthStencilFlags::kDepth,
        kRGBA_F16_SkColorType,
        SkColorSpace::MakeSRGBLinear(),
        /* fRequiresMSAA= */ false
};

// clang-format on

const RenderPassProperties kCombo_RGBA_1D_4DS[2] = {kRGBA_1_D, kRGBA_4_DS};
const RenderPassProperties kCombo_RGBA_1D_4DS_SRGB[2] = {kRGBA_1_D_SRGB, kRGBA_4_DS_SRGB};
const RenderPassProperties kCombo_RGBA_1D_SRGB_w16F[2] = {kRGBA_1_D_SRGB, kRGBA16F_1_D_SRGB};

// =======================================
//            DrawTypeFlags
// =======================================
// NOTE: keep in sync with upstream external/skia/tests/graphite/precompile/AndroidPaintOptions.cpp
// clang-format off

constexpr bool kWithAnalyticClip = true;

constexpr DrawTypeFlags kRRectAndNonAARect =
        static_cast<DrawTypeFlags>(DrawTypeFlags::kAnalyticRRect |
                                   DrawTypeFlags::kNonAAFillRect);

// clang-format on

std::vector<PrecompileSettings> chooseBlurPrecompileSettings(RuntimeEffectManager& effectManager) {
    std::vector<PrecompileSettings> settingsList;
    // Note: reorder these cases to match BlurAlgorithm's definition order as precompilation support
    // for each algorithm is added. Each case should be added explicitly to ensure awareness of
    // potential precompilation gaps when new blurring algorithms are added.
    switch (effectManager.getChosenBlurAlgorithm()) {
        case RenderEngine::BlurAlgorithm::None:
            break;
        case RenderEngine::BlurAlgorithm::KawaseDualFilter:
            settingsList.push_back({KawaseBlurLowSrcSrcOver(effectManager),
                                    DrawTypeFlags::kNonAAFillRect, kRGBA_1_D});
            settingsList.push_back(
                    {KawaseBlurHighSrc(effectManager), DrawTypeFlags::kNonAAFillRect, kRGBA_1_D});
            break;
        case RenderEngine::BlurAlgorithm::Gaussian:
        case RenderEngine::BlurAlgorithm::Kawase:
        case RenderEngine::BlurAlgorithm::KawaseDualFilterV2:
            ALOGW("Pipeline precompilation for %s is not yet supported",
                  ftl::enum_string_full(effectManager.getChosenBlurAlgorithm()).c_str());
            break;
    }

    // Mix effect is used regardless of blurring algorithm (excluding BlurAlgorithm::None).
    if (!settingsList.empty()) {
        settingsList.push_back({BlurFilterMix(effectManager), kRRectAndNonAARect, kRGBA_1_D});
    }
    return settingsList;
}

void precompilePipelineCases(graphite::PrecompileContext* precompileContext,
                             SkSpan<const PrecompileSettings> cases) {
    SFTRACE_FORMAT("Precompiling %zu cases", cases.size());

    for (size_t i = 0; i < cases.size(); i++) {
        const PrecompileSettings& settings = cases[i];
        Precompile(precompileContext, settings.fPaintOptions, settings.fDrawTypeFlags,
                   settings.fRenderPassProps);

        if (settings.fAnalyticClipping) {
            DrawTypeFlags newFlags = settings.fDrawTypeFlags | DrawTypeFlags::kAnalyticClip;

            Precompile(precompileContext, settings.fPaintOptions,
                       static_cast<DrawTypeFlags>(newFlags), settings.fRenderPassProps);
        }
    }
}

void GraphitePipelineManager::PrecompilePipelines(
        std::unique_ptr<graphite::PrecompileContext> precompileContext,
        RuntimeEffectManager& effectManager) {
    pthread_setname_np(pthread_self(), "Precompile"); // Limited to 15 characters
    SFTRACE_CALL();

    // =======================================
    //            Combinations
    // =======================================
    // NOTE: keep in sync with upstream
    // external/skia/tests/graphite/precompile/AndroidPaintOptions.cpp
    // clang-format off

    const PrecompileSettings precompileCases[] = {
        { Builder().hwImg(kPremul).srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D },

        { Builder().hwImg(kPremul).srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().hwImg(kPremul).srcOver(),
          kRRectAndNonAARect,
          kRGBA_4_DS },

        { Builder().hwImg(kSRGB).srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_SRGB },

        { Builder().hwImg(kSRGB).srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB,
          kWithAnalyticClip },

        { Builder().transparent().hwImg(kPremul).srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().transparent().hwImg(kPremul).srcOver(),
          kRRectAndNonAARect,
          kRGBA_4_DS },

        { Builder().transparent().hwImg(kSRGB).srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB },

        { Builder().src().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().srcOver(),
          kRRectAndNonAARect,
          kRGBA_4_DS },

        { Builder().hwImg(kPremul).matrixCF().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().transparent().hwImg(kPremul).matrixCF().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().transparent().hwImg(kPremul).matrixCF().dither().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D },

        { Builder().transparent().hwImg(kPremul).matrixCF().dither().srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_4_DS },

        { Builder().hwImg(kPremul).matrixCF().dither().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().hwImg(kPremul).matrixCF().dither().srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_4_DS },

        { Builder().transparent().hwImg(kSRGB).matrixCF().dither().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB },

        { Builder().hwImg(kSRGB).matrixCF().dither().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB,
          kWithAnalyticClip },

        { Builder().hwImg(kSRGB).matrixCF().dither().srcOver(),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_4_DS_SRGB },

        { Builder().hwImg(kAlphaSRGB).matrixCF().srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kCombo_RGBA_1D_4DS_SRGB },

        { Builder().hwImg(kPremul).src(),
          kRRectAndNonAARect,
          kRGBA_1_D },

        { Builder().hwImg(kPremul).src(),
          DrawTypeFlags::kPerEdgeAAQuad,
          kRGBA_1_D },

        { Builder().hwImg(kPremul).src(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_4_DS },

        // TODO(b/426601394): Group these paint option settings into a function that accepts an
        // input image color space so that the intermediate linear color spaces adapt correctly.
        { MouriMapBlur(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_Linear },

        { MouriMapToneMap(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D_SRGB },

        { MouriMapCrosstalkAndChunk16x16Passthrough(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_Linear },

        { MouriMapChunk8x8Effect(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_Linear },

        // These two are solid colors drawn w/ a LinearEffect
        { LinearEffect(effectManager,
                       kUNKNOWN__SRGB__false__UNKNOWN__Shader,
                       ChildType::kSolidColor,
                       SkBlendMode::kSrcOver),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_SRGB },

        { LinearEffect(effectManager, kBT2020_ITU_PQ__BT2020__false__UNKNOWN__Shader,
                       ChildType::kSolidColor,
                       SkBlendMode::kSrc),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, kUNKNOWN__SRGB__false__UNKNOWN__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver),
          DrawTypeFlags::kNonAAFillRect,
          kCombo_RGBA_1D_SRGB_w16F },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver),
          DrawTypeFlags::kAnalyticRRect,
          kCombo_RGBA_1D_4DS_SRGB },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D_SRGB,
          kWithAnalyticClip },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ false),
          DrawTypeFlags::kAnalyticRRect,
          kCombo_RGBA_1D_4DS_SRGB },

        // The next 3 have a RE_LinearEffect and a MatrixFilter along w/ different ancillary
        // additions
        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ true),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ false,
                       /* matrixColorFilter= */ true),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ true,
                       /* dither= */ true),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, kV0_SRGB__V0_SRGB__true__UNKNOWN__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ false,
                       /* dither= */ false),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_SRGB },

        { LinearEffect(effectManager, kV0_SRGB__V0_SRGB__true__UNKNOWN__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ false,
                       /* dither= */ false),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__V0_SRGB__true__0x9010000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ true,
                       /* dither= */ true),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__V0_SRGB__true__0x9010000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ false,
                       /* dither= */ false),
          DrawTypeFlags::kAnalyticRRect,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ false),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_1_D_SRGB },

        { LinearEffect(effectManager, k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader,
                       ChildType::kHWTexture,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ true),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_1_D_SRGB },

        { Builder().srcOver(),
          DrawTypeFlags::kNonSimpleShape,
          kRGBA_4_DS },

        { Builder().hwImg(kPremul).srcOver(),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_4_DS },

        { Builder().src().srcOver(),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_4_DS },

        { {}, // ignored
          DrawTypeFlags::kDropShadows,
          kRGBA_1_D },

        { {}, // ignored
          DrawTypeFlags::kDropShadows,
          kRGBA_4_DS },

        { EdgeExtensionPremulSrcover(effectManager),
          kRRectAndNonAARect,
          kRGBA_1_D },

        { TransparentPaintEdgeExtensionPassthroughMatrixCFDitherSrcover(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D },

        { TransparentPaintEdgeExtensionPassthroughSrcover(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D },

        { TransparentPaintEdgeExtensionPremulSrcover(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D },

        { EdgeExtensionPassthroughSrcover(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { Builder().hwImg(kAlpha, kClamp).src(),
          DrawTypeFlags::kNonAAFillRect,
          kR_1_D },

        { Builder().hwImg(kSRGB).matrixCF().srcOver(),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB },

        { MouriMapCrosstalkAndChunk16x16Premul(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_SRGB },

        { Builder().blend().srcOver(),
          DrawTypeFlags::kAnalyticRRect,
          kCombo_RGBA_1D_4DS },

        { Builder().blend().srcOver(),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_1_D },

        { Builder().transparent().blend().srcOver(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D },

        // 238 Full range (kHIAAO4AAAAAAAAA) block ----------------

        { ImagePremulYCbCr238Srcover(/* narrow= */ false),
          DrawTypeFlags::kNonAAFillRect,
          kCombo_RGBA_1D_4DS_SRGB,
          kWithAnalyticClip },

        // 238 Narrow range (kHoAAO4AAAAAAAAA) block ----------------

        { ImagePremulYCbCr238Srcover(/* narrow= */ true),
          kRRectAndNonAARect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { TransparentPaintImagePremulYCbCr238Srcover(),
          kRRectAndNonAARect,
          kCombo_RGBA_1D_4DS },

        { TransparentPaintImagePremulYCbCr238Srcover(),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_4_DS },

        { ImagePremulYCbCr238Srcover(/* narrow= */ true),
          kRRectAndNonAARect,
          kRGBA_4_DS },

        // Note: this didn't get folded into the above since the RRect draw isn't appearing w/ a
        // clip
        { ImagePremulYCbCr238Srcover(/* narrow= */ true),
          DrawTypeFlags::kNonAAFillRect | DrawTypeFlags::kAnalyticClip,
          kRGBA_4_DS },

        // 240 (kHIAAPAAAAAAAAAA) block ----------------

        { ImagePremulYCbCr240Srcover(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_1_D,
          kWithAnalyticClip },

        { ImagePremulYCbCr240Srcover(),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_4_DS },

        { TransparentPaintImagePremulYCbCr240Srcover(),
          DrawTypeFlags::kNonAAFillRect,
          kCombo_RGBA_1D_4DS },

        // 247 (kEwAAPcAAAAAAAAA) block ----------------

        { MouriMapCrosstalkAndChunk16x16YCbCr247(effectManager),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA16F_1_D_SRGB },

        // The next 2 have the same PaintOptions but different destination surfaces

        { LinearEffect(effectManager, kBT2020_ITU_PQ__BT2020__false__UNKNOWN__Shader,
                       ChildType::kHWTextureYCbCr247,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ false,
                       /* dither= */ true),
          kRRectAndNonAARect,
          kRGBA_1_D_SRGB,
          kWithAnalyticClip },

        { LinearEffect(effectManager, kBT2020_ITU_PQ__BT2020__false__UNKNOWN__Shader,
                       ChildType::kHWTextureYCbCr247,
                       SkBlendMode::kSrcOver,
                       /* paintColorIsOpaque= */ true,
                       /* matrixColorFilter= */ false,
                       /* dither= */ true),
          DrawTypeFlags::kNonAAFillRect,
          kRGBA_4_DS_SRGB },
    };

    // clang-format on

    ALOGD("Precompiling blur pipeline cases for %s",
          ftl::enum_string_full(effectManager.getChosenBlurAlgorithm()).c_str());
    precompilePipelineCases(precompileContext.get(), chooseBlurPrecompileSettings(effectManager));

    ALOGD("Precompiling general pipeline cases");
    precompilePipelineCases(precompileContext.get(), {precompileCases});

    ALOGD("Pipeline precompilation finished");
}

} // namespace android::renderengine::skia
