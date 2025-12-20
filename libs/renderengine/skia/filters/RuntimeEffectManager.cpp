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

#include "RuntimeEffectManager.h"

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <SkRuntimeEffect.h>
#include <SkString.h>
#include <android-base/stringprintf.h>
#include <common/trace.h>
#include <ftl/concat.h>
#include <ftl/enum.h>
#include <log/log.h>
#include <math/mat4.h>
#include <shaders/shaders.h>
#include <ui/DebugUtils.h>
#include <mutex>

#include "skia/ColorSpaces.h"

namespace android {
namespace renderengine {
namespace skia {

using base::StringAppendF;

sk_sp<SkRuntimeEffect> RuntimeEffectManager::getKnownRuntimeEffect(KnownEffectId effectId) {
    auto effectIdValue = static_cast<size_t>(effectId);
    sk_sp<SkRuntimeEffect> runtimeEffect = mKnownEffects[effectIdValue];
    LOG_ALWAYS_FATAL_IF(!runtimeEffect, "RuntimeEffect for ID:%zu has not been created yet",
                        effectIdValue);
    return runtimeEffect;
}

sk_sp<SkRuntimeEffect> RuntimeEffectManager::getOrCreateLinearRuntimeEffect(
        const shaders::LinearEffect& linearEffect) {
    SFTRACE_CALL();
    std::lock_guard lock(mMutex);

    auto effectIter = mLinearEffectMap.find(linearEffect);
    if (effectIter == mLinearEffectMap.end()) {
        sk_sp<SkRuntimeEffect> runtimeEffect = buildLinearRuntimeEffect(linearEffect);
        mLinearEffectMap.insert({linearEffect, runtimeEffect});
        return runtimeEffect;
    } else {
        return effectIter->second;
    }
}

std::string variableNameSafeLinearEffectString(const shaders::LinearEffect& effect) {
    // May only contain alphanumeric characters and underscores. Double underscoress are used to
    // delineate fields, since dataspace names may contain their own single underscores.
    return toString(effect.inputDataspace) + "__" + toString(effect.outputDataspace) + "__" +
            (effect.undoPremultipliedAlpha ? "true" : "false") + "__" +
            toString(effect.fakeOutputDataspace) + "__" +
            (effect.type == shaders::LinearEffect::SkSLType::Shader ? "Shader" : "ColorFilter");
}

// Wrapper around existing toString(ui::Dataspace) that tweaks the result to make it copy/pasteable
// into struct initialization code.
std::string dataspaceEnumString(ui::Dataspace dataspace) {
    const std::string dataspaceStr = toString(dataspace);
    if (dataspaceStr.starts_with("0x")) {
        return "static_cast<ui::Dataspace>(" + dataspaceStr + ")";
    } else {
        return "ui::Dataspace::" + dataspaceStr;
    }
}

void RuntimeEffectManager::createAndStoreKnownEffects() {
    for (int i = 0; i < kEffectCount; i++) {
        SkRuntimeEffect::Options options;
        options.fName = kEffectNames[i];
        const SkString* effectSource = kEffectSources[i];
        if (effectSource) {
            auto [effect, error] = SkRuntimeEffect::MakeForShader(*effectSource, options);
            LOG_ALWAYS_FATAL_IF(!effect, "%s (ID:%d) construction error: %s", kEffectNames[i], i,
                                error.c_str());
            mKnownEffects[i] = effect;
        }
    }

    mKnownEffects[kUNKNOWN__SRGB__false__UNKNOWN__Shader] = getOrCreateLinearRuntimeEffect({
            .inputDataspace = ui::Dataspace::UNKNOWN, // Default
            .outputDataspace = ui::Dataspace::SRGB,   // (deprecated) sRGB sRGB Full range
            .undoPremultipliedAlpha = false,
            .fakeOutputDataspace = ui::Dataspace::UNKNOWN, // Default
            .type = shaders::LinearEffect::SkSLType::Shader,
    });

    mKnownEffects[kBT2020_ITU_PQ__BT2020__false__UNKNOWN__Shader] = getOrCreateLinearRuntimeEffect(
            {.inputDataspace = ui::Dataspace::BT2020_ITU_PQ, // BT2020 SMPTE 2084 Limited range
             .outputDataspace = ui::Dataspace::BT2020,       // BT2020 SMPTE_170M Full range
             .undoPremultipliedAlpha = false,
             .fakeOutputDataspace = ui::Dataspace::UNKNOWN, // Default
             .type = shaders::LinearEffect::SkSLType::Shader});

    mKnownEffects[k0x188a0000__DISPLAY_P3__false__0x90a0000__Shader] =
            getOrCreateLinearRuntimeEffect({
                    .inputDataspace =
                            static_cast<ui::Dataspace>(0x188a0000), // DCI-P3 sRGB Extended range
                    .outputDataspace = ui::Dataspace::DISPLAY_P3,   // DCI-P3 sRGB Full range
                    .undoPremultipliedAlpha = false,
                    .fakeOutputDataspace =
                            static_cast<ui::Dataspace>(0x90a0000), // DCI-P3 gamma 2.2 Full range
                    .type = shaders::LinearEffect::SkSLType::Shader,
            });

    mKnownEffects[kV0_SRGB__V0_SRGB__true__UNKNOWN__Shader] = getOrCreateLinearRuntimeEffect({
            .inputDataspace = ui::Dataspace::V0_SRGB,
            .outputDataspace = ui::Dataspace::V0_SRGB,
            .undoPremultipliedAlpha = true,
            .fakeOutputDataspace = ui::Dataspace::UNKNOWN, // Default
            .type = shaders::LinearEffect::SkSLType::Shader,
    });

    mKnownEffects[k0x188a0000__V0_SRGB__true__0x9010000__Shader] = getOrCreateLinearRuntimeEffect({
            .inputDataspace = static_cast<ui::Dataspace>(0x188a0000), // DCI-P3 sRGB Extended range
            .outputDataspace = ui::Dataspace::V0_SRGB,
            .undoPremultipliedAlpha = true,
            .fakeOutputDataspace = static_cast<ui::Dataspace>(0x9010000),
            .type = shaders::LinearEffect::SkSLType::Shader,
    });
}

void RuntimeEffectManager::dump(std::string& result) {
    StringAppendF(&result, "RenderEngine chosen blur algorithm: %s\n",
                  ftl::enum_string(getChosenBlurAlgorithm()).c_str());

    // LinearEffects are ordered (by hash value) when dumped to reduce churn when iterating on the
    // set of precompiled effects.
    std::vector<shaders::LinearEffect> orderedLinearEffects;
    {
        // Note: rework this locking if other guarded state is dumped in the future!
        std::lock_guard lock(mMutex);
        orderedLinearEffects = std::vector<shaders::LinearEffect>(mLinearEffectMap.size());
        for (const auto& [linearEffect, _] : mLinearEffectMap) {
            orderedLinearEffects.push_back(linearEffect);
        }
    }
    std::sort(orderedLinearEffects.begin(), orderedLinearEffects.end(), [](auto& lhs, auto& rhs) {
        auto hasher = shaders::LinearEffectHasher();
        return hasher(lhs) < hasher(rhs);
    });

    StringAppendF(&result, "RenderEngine LinearEffects (RuntimeEffects): %zu\n",
                  orderedLinearEffects.size());
    for (const auto& linearEffect : orderedLinearEffects) {
        // Note: the formatting of this output should remain copy/pasteable C++
        StringAppendF(&result, "// %s\n", variableNameSafeLinearEffectString(linearEffect).c_str());
        StringAppendF(&result, "{\n");
        StringAppendF(&result, "    .inputDataspace = %s, // %s\n",
                      dataspaceEnumString(linearEffect.inputDataspace).c_str(),
                      dataspaceDetails(static_cast<android_dataspace>(linearEffect.inputDataspace))
                              .c_str());
        StringAppendF(&result, "    .outputDataspace = %s, // %s\n",
                      dataspaceEnumString(linearEffect.outputDataspace).c_str(),
                      dataspaceDetails(static_cast<android_dataspace>(linearEffect.outputDataspace))
                              .c_str());
        StringAppendF(&result, "    .undoPremultipliedAlpha = %s,\n",
                      linearEffect.undoPremultipliedAlpha ? "true" : "false");
        StringAppendF(&result, "    .fakeOutputDataspace = %s, // %s\n",
                      dataspaceEnumString(linearEffect.fakeOutputDataspace).c_str(),
                      dataspaceDetails(
                              static_cast<android_dataspace>(linearEffect.fakeOutputDataspace))
                              .c_str());
        StringAppendF(&result, "    .type = shaders::LinearEffect::SkSLType::%s,\n",
                      linearEffect.type == shaders::LinearEffect::SkSLType::Shader ? "Shader"
                                                                                   : "ColorFilter");
        StringAppendF(&result, "},\n");
    }
}

sk_sp<SkRuntimeEffect> RuntimeEffectManager::buildLinearRuntimeEffect(
        const shaders::LinearEffect& linearEffect) {
    SFTRACE_CALL();
    SkString shaderString = SkString(shaders::buildLinearEffectSkSL(linearEffect));

    std::string name = "RE_LinearEffect_" + variableNameSafeLinearEffectString(linearEffect);
    SkRuntimeEffect::Options options;
    options.fName = name;
    auto [shader, error] = SkRuntimeEffect::MakeForShader(shaderString, options);
    if (!shader) {
        LOG_ALWAYS_FATAL("LinearColorFilter construction error: %s", error.c_str());
    }
    return shader;
}

sk_sp<SkShader> RuntimeEffectManager::createLinearEffectShader(
        sk_sp<SkShader> shader, const shaders::LinearEffect& linearEffect,
        sk_sp<SkRuntimeEffect> runtimeEffect, const mat4& colorTransform, float maxDisplayLuminance,
        float currentDisplayLuminanceNits, float maxLuminance, AHardwareBuffer* buffer,
        aidl::android::hardware::graphics::composer3::RenderIntent renderIntent) {
    SFTRACE_CALL();
    SkRuntimeShaderBuilder effectBuilder(runtimeEffect);

    effectBuilder.child("child") = shader;

    const auto uniforms =
            shaders::buildLinearEffectUniforms(linearEffect, colorTransform, maxDisplayLuminance,
                                               currentDisplayLuminanceNits, maxLuminance, buffer,
                                               renderIntent);

    for (const auto& uniform : uniforms) {
        effectBuilder.uniform(uniform.name.c_str()).set(uniform.value.data(), uniform.value.size());
    }

    shader = effectBuilder.makeShader();
    if (shader) {
        // The linear effect SkSL assumes it operates in with a linear gamma in the source gamut.
        // Wrapping it in a working color space shader satisfies this requirement and will
        // automatically convert the linear effect's output (in the linear source space) to
        // linearEffect.outputDataspace (which is assumed to be the dst CS of the SkSurface this
        // shader will be drawn into).
        //
        // The only exception is when there is a custom OETF, in which case the working color space
        // should be the final output data space, since the linear effect will manage converting to
        // the output gamut and apply the custom OETF. Using the final output space disables Skia's
        // color management post OETF.
        sk_sp<SkColorSpace> inputSpace =
                toSkColorSpace(linearEffect.inputDataspace)->makeLinearGamma();
        sk_sp<SkColorSpace> outputSpace = nullptr;
        if ((linearEffect.fakeOutputDataspace & HAL_DATASPACE_TRANSFER_MASK)) {
            outputSpace = toSkColorSpace(linearEffect.fakeOutputDataspace);
        }
        shader = shader->makeWithWorkingColorSpace(inputSpace, outputSpace);
    }
    return shader;
}

} // namespace skia
} // namespace renderengine
} // namespace android
