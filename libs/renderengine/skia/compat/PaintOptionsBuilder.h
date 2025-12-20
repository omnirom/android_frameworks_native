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

// NOTE: keep in sync with upstream external/skia/tests/graphite/precompile/PaintOptionsBuilder.h

#ifndef PaintOptionsBuilder_DEFINED
#define PaintOptionsBuilder_DEFINED

#include <include/gpu/graphite/precompile/PaintOptions.h>

namespace android::renderengine::skia {

namespace PaintOptionsUtils {

enum ImgColorInfo {
    kAlpha,
    kAlphaSRGB,
    kPremul,
    kSRGB,
};

enum ImgTileModeOptions {
    kNone,
    kClamp,
    kRepeat,
};

// This enum directly maps to YUVImageShaderFlags but, crucially, is more compact.
enum YUVSamplingOptions {
    kNoCubic,     // YUVImageShaderFlags::kExcludeCubic
    kHWAndShader, // YUVImageShaderFlags::kNoCubicNoNonSwizzledHW
};

enum LinearGradientOptions {
    kSmall,
    kComplex, // idiosyncratic case - c.f. Builder::linearGrad
};

// This is a minimal builder object that allows for compact construction of the most common
// PaintOptions combinations - eliminating a lot of boilerplate.
class Builder {
public:
    Builder() {}

    // Shaders
    Builder& hwImg(ImgColorInfo ci, ImgTileModeOptions tmOptions = kNone);
    Builder& yuv(YUVSamplingOptions options);
    Builder& linearGrad(LinearGradientOptions options);
    Builder& blend();

    // ColorFilters
    Builder& matrixCF();
    Builder& porterDuffCF();

    // Blendmodes
    Builder& clear() { return this->addBlendMode(SkBlendMode::kClear); }
    Builder& dstIn() { return this->addBlendMode(SkBlendMode::kDstIn); }
    Builder& src() { return this->addBlendMode(SkBlendMode::kSrc); }
    Builder& srcOver() { return this->addBlendMode(SkBlendMode::kSrcOver); }

    // Misc settings
    Builder& transparent() {
        fPaintOptions.setPaintColorIsOpaque(false);
        return *this;
    }
    Builder& dither() {
        fPaintOptions.setDither(true);
        return *this;
    }

    operator skgpu::graphite::PaintOptions() const { return fPaintOptions; }

private:
    skgpu::graphite::PaintOptions fPaintOptions;

    Builder& addBlendMode(SkBlendMode bm) {
        fPaintOptions.addBlendMode(bm);
        return *this;
    }
};

} // namespace PaintOptionsUtils

} // namespace android::renderengine::skia
#endif // PaintOptionsBuilder_DEFINED
