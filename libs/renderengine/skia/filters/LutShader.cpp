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
#include "LutShader.h"

#include <SkM44.h>
#include <SkTileMode.h>
#include <common/trace.h>
#include <cutils/ashmem.h>
#include <math/half.h>
#include <sys/mman.h>
#include <ui/ColorSpace.h>

#include "include/core/SkColorSpace.h"

using aidl::android::hardware::graphics::composer3::LutProperties;

namespace android {
namespace renderengine {
namespace skia {

static const SkString kShader = SkString(R"(
    uniform shader image;
    uniform shader lut;
    uniform int size;
    uniform int key;
    uniform int dimension;
    uniform vec3 luminanceCoefficients; // for CIE_Y
    // for hlg/pq transfer function, we need normalize it to [0.0, 1.0]
    // we use `normalizeScalar` to do so
    uniform float normalizeScalar;

    vec4 main(vec2 xy) {
        float4 rgba = image.eval(xy);
        float3 linear = toLinearSrgb(rgba.rgb) * normalizeScalar;
        if (dimension == 1) {
            // RGB
            if (key == 0) {
                float indexR = linear.r * float(size - 1);
                float indexG = linear.g * float(size - 1);
                float indexB = linear.b * float(size - 1);
                float gainR = lut.eval(vec2(indexR, 0.0) + 0.5).r;
                float gainG = lut.eval(vec2(indexG, 0.0) + 0.5).r;
                float gainB = lut.eval(vec2(indexB, 0.0) + 0.5).r;
                linear = float3(linear.r * gainR, linear.g * gainG, linear.b * gainB);
            // MAX_RGB
            } else if (key == 1) {
                float maxRGB = max(linear.r, max(linear.g, linear.b));
                float index = maxRGB * float(size - 1);
                float gain = lut.eval(vec2(index, 0.0) + 0.5).r;
                linear = linear * gain;
            // CIE_Y
            } else if (key == 2) {
                float y = dot(linear, luminanceCoefficients) / 3.0;
                float index = y * float(size - 1);
                float gain = lut.eval(vec2(index, 0.0) + 0.5).r;
                linear = linear * gain;
            }
        } else if (dimension == 3) {
            if (key == 0) {
                float tx = linear.r * float(size - 1);
                float ty = linear.g * float(size - 1);
                float tz = linear.b * float(size - 1);

                // calculate lower and upper bounds for each dimension
                int x = int(tx);
                int y = int(ty);
                int z = int(tz);

                int i000 = x + y * size + z * size * size;
                int i100 = i000 + 1;
                int i010 = i000 + size;
                int i110 = i000 + size + 1;
                int i001 = i000 + size * size;
                int i101 = i000 + size * size + 1;
                int i011 = i000 + size * size + size;
                int i111 = i000 + size * size + size + 1;

                // get 1d normalized indices
                float c000 = float(i000) / float(size * size * size);
                float c100 = float(i100) / float(size * size * size);
                float c010 = float(i010) / float(size * size * size);
                float c110 = float(i110) / float(size * size * size);
                float c001 = float(i001) / float(size * size * size);
                float c101 = float(i101) / float(size * size * size);
                float c011 = float(i011) / float(size * size * size);
                float c111 = float(i111) / float(size * size * size);

                //TODO(b/377984618): support Tetrahedral interpolation
                // perform trilinear interpolation
                float3 c00 = mix(lut.eval(vec2(c000, 0.0) + 0.5).rgb,
                                 lut.eval(vec2(c100, 0.0) + 0.5).rgb, linear.r);
                float3 c01 = mix(lut.eval(vec2(c001, 0.0) + 0.5).rgb,
                                 lut.eval(vec2(c101, 0.0) + 0.5).rgb, linear.r);
                float3 c10 = mix(lut.eval(vec2(c010, 0.0) + 0.5).rgb,
                                 lut.eval(vec2(c110, 0.0) + 0.5).rgb, linear.r);
                float3 c11 = mix(lut.eval(vec2(c011, 0.0) + 0.5).rgb,
                                 lut.eval(vec2(c111, 0.0) + 0.5).rgb, linear.r);

                float3 c0 = mix(c00, c10, linear.g);
                float3 c1 = mix(c01, c11, linear.g);

                linear = mix(c0, c1, linear.b);
            }
        }
        return float4(fromLinearSrgb(linear), rgba.a);
    })");

// same as shader::toColorSpace function
// TODO: put this function in a general place
static ColorSpace toColorSpace(ui::Dataspace dataspace) {
    switch (dataspace & HAL_DATASPACE_STANDARD_MASK) {
        case HAL_DATASPACE_STANDARD_BT709:
            return ColorSpace::sRGB();
        case HAL_DATASPACE_STANDARD_DCI_P3:
            return ColorSpace::DisplayP3();
        case HAL_DATASPACE_STANDARD_BT2020:
        case HAL_DATASPACE_STANDARD_BT2020_CONSTANT_LUMINANCE:
            return ColorSpace::BT2020();
        case HAL_DATASPACE_STANDARD_ADOBE_RGB:
            return ColorSpace::AdobeRGB();
        case HAL_DATASPACE_STANDARD_BT601_625:
        case HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED:
        case HAL_DATASPACE_STANDARD_BT601_525:
        case HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED:
        case HAL_DATASPACE_STANDARD_BT470M:
        case HAL_DATASPACE_STANDARD_FILM:
        case HAL_DATASPACE_STANDARD_UNSPECIFIED:
        default:
            return ColorSpace::sRGB();
    }
}

sk_sp<SkShader> LutShader::generateLutShader(sk_sp<SkShader> input,
                                             const std::vector<float>& buffers,
                                             const int32_t offset, const int32_t length,
                                             const int32_t dimension, const int32_t size,
                                             const int32_t samplingKey,
                                             ui::Dataspace srcDataspace) {
    SFTRACE_NAME("lut shader");
    std::vector<half> buffer(length * 4); // 4 is for RGBA
    auto d = static_cast<LutProperties::Dimension>(dimension);
    if (d == LutProperties::Dimension::ONE_D) {
        auto it = buffers.begin() + offset;
        std::generate(buffer.begin(), buffer.end(), [it, i = 0]() mutable {
            float val = (i++ % 4 == 0) ? *it++ : 0.0f;
            return half(val);
        });
    } else {
        for (int i = 0; i < length; i++) {
            buffer[i * 4] = half(buffers[offset + i]);
            buffer[i * 4 + 1] = half(buffers[offset + length + i]);
            buffer[i * 4 + 2] = half(buffers[offset + length * 2 + i]);
            buffer[i * 4 + 3] = half(0);
        }
    }
    /**
     * 1D Lut RGB/MAX_RGB
     * (R0, 0, 0, 0)
     * (R1, 0, 0, 0)
     *
     * 1D Lut CIE_Y
     * (Y0, 0, 0, 0)
     * (Y1, 0, 0, 0)
     * ...
     *
     * 3D Lut MAX_RGB
     * (R0, G0, B0, 0)
     * (R1, G1, B1, 0)
     * ...
     */
    SkImageInfo info = SkImageInfo::Make(length /* the number of rgba */, 1, kRGBA_F16_SkColorType,
                                         kPremul_SkAlphaType);
    SkBitmap bitmap;
    bitmap.allocPixels(info);
    if (!bitmap.installPixels(info, buffer.data(), info.minRowBytes())) {
        ALOGW("bitmap.installPixels failed, skip this Lut!");
        return input;
    }

    sk_sp<SkImage> lutImage = SkImages::RasterFromBitmap(bitmap);
    if (!lutImage) {
        ALOGW("Got a nullptr from SkImages::RasterFromBitmap, skip this Lut!");
        return input;
    }

    mBuilder->child("image") = input;
    mBuilder->child("lut") =
            lutImage->makeRawShader(SkTileMode::kClamp, SkTileMode::kClamp,
                                    d == LutProperties::Dimension::ONE_D
                                            ? SkSamplingOptions(SkFilterMode::kLinear)
                                            : SkSamplingOptions());

    float normalizeScalar = 1.0;
    switch (srcDataspace & HAL_DATASPACE_TRANSFER_MASK) {
        case HAL_DATASPACE_TRANSFER_HLG:
            normalizeScalar = 0.203;
            break;
        case HAL_DATASPACE_TRANSFER_ST2084:
            normalizeScalar = 0.0203;
            break;
        default:
            normalizeScalar = 1.0;
    }
    const int uSize = static_cast<int>(size);
    const int uKey = static_cast<int>(samplingKey);
    const int uDimension = static_cast<int>(dimension);
    const float uNormalizeScalar = static_cast<float>(normalizeScalar);

    if (static_cast<LutProperties::SamplingKey>(samplingKey) == LutProperties::SamplingKey::CIE_Y) {
        // Use predefined colorspaces of input dataspace so that we can get D65 illuminant
        mat3 toXYZMatrix(toColorSpace(srcDataspace).getRGBtoXYZ());
        mBuilder->uniform("luminanceCoefficients") =
                SkV3{toXYZMatrix[0][1], toXYZMatrix[1][1], toXYZMatrix[2][1]};
    } else {
        mBuilder->uniform("luminanceCoefficients") = SkV3{1.f, 1.f, 1.f};
    }
    mBuilder->uniform("size") = uSize;
    mBuilder->uniform("key") = uKey;
    mBuilder->uniform("dimension") = uDimension;
    mBuilder->uniform("normalizeScalar") = uNormalizeScalar;
    return mBuilder->makeShader();
}

sk_sp<SkShader> LutShader::lutShader(sk_sp<SkShader>& input,
                                     std::shared_ptr<gui::DisplayLuts> displayLuts,
                                     ui::Dataspace srcDataspace,
                                     sk_sp<SkColorSpace> outColorSpace) {
    if (mBuilder == nullptr) {
        const static SkRuntimeEffect::Result instance = SkRuntimeEffect::MakeForShader(kShader);
        mBuilder = std::make_unique<SkRuntimeShaderBuilder>(instance.effect);
    }

    auto& fd = displayLuts->getLutFileDescriptor();
    if (fd.ok()) {
        // de-gamma the image without changing the primaries
        SkImage* baseImage = input->isAImage((SkMatrix*)nullptr, (SkTileMode*)nullptr);
        sk_sp<SkColorSpace> baseColorSpace = baseImage && baseImage->colorSpace()
                ? baseImage->refColorSpace()
                : SkColorSpace::MakeSRGB();
        sk_sp<SkColorSpace> lutMathColorSpace = baseColorSpace->makeLinearGamma();
        input = input->makeWithWorkingColorSpace(lutMathColorSpace);

        auto& offsets = displayLuts->offsets;
        auto& lutProperties = displayLuts->lutProperties;
        std::vector<float> buffers;
        int fullLength = offsets[lutProperties.size() - 1];
        if (lutProperties[lutProperties.size() - 1].dimension == 1) {
            fullLength += lutProperties[lutProperties.size() - 1].size;
        } else {
            fullLength += (lutProperties[lutProperties.size() - 1].size *
                           lutProperties[lutProperties.size() - 1].size *
                           lutProperties[lutProperties.size() - 1].size * 3);
        }
        size_t bufferSize = fullLength * sizeof(float);

        // decode the shared memory of luts
        float* ptr =
                (float*)mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd.get(), 0);
        if (ptr == MAP_FAILED) {
            LOG_ALWAYS_FATAL("mmap failed");
        }
        buffers = std::vector<float>(ptr, ptr + fullLength);
        munmap(ptr, bufferSize);

        for (size_t i = 0; i < offsets.size(); i++) {
            int bufferSizePerLut = (i == offsets.size() - 1) ? buffers.size() - offsets[i]
                                                             : offsets[i + 1] - offsets[i];
            // divide by 3 for 3d Lut because of 3 (RGB) channels
            if (static_cast<LutProperties::Dimension>(lutProperties[i].dimension) ==
                LutProperties::Dimension::THREE_D) {
                bufferSizePerLut /= 3;
            }
            input = generateLutShader(input, buffers, offsets[i], bufferSizePerLut,
                                      lutProperties[i].dimension, lutProperties[i].size,
                                      lutProperties[i].samplingKey, srcDataspace);
        }

        input = input->makeWithWorkingColorSpace(outColorSpace);
    }
    return input;
}

} // namespace skia
} // namespace renderengine
} // namespace android