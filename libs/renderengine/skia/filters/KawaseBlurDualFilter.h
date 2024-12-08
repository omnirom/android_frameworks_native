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

#include <SkCanvas.h>
#include <SkImage.h>
#include <SkRuntimeEffect.h>
#include <SkSurface.h>
#include "BlurFilter.h"

namespace android {
namespace renderengine {
namespace skia {

/**
 * This is an implementation of a Kawase blur with dual-filtering passes, as described in here:
 * https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_slides.pdf
 * https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_notes.pdf
 */
class KawaseBlurDualFilter : public BlurFilter {
public:
    explicit KawaseBlurDualFilter();
    virtual ~KawaseBlurDualFilter() {}

    // Execute blur, saving it to a texture
    sk_sp<SkImage> generate(SkiaGpuContext* context, const uint32_t radius,
                            const sk_sp<SkImage> blurInput, const SkRect& blurRect) const override;

private:
    sk_sp<SkRuntimeEffect> mBlurEffect;

    void blurInto(const sk_sp<SkSurface>& drawSurface, const sk_sp<SkImage>& readImage,
                  const float radius, const float alpha) const;

    void blurInto(const sk_sp<SkSurface>& drawSurface, const sk_sp<SkShader> input,
                  const float inverseScale, const float radius, const float alpha) const;
};

} // namespace skia
} // namespace renderengine
} // namespace android
