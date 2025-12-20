#include "BoxShadowUtils.h"

#include <common/trace.h>

namespace android::renderengine::skia {

const SkString kEffectSource_BoxShadowEffect(R"(
uniform half4  u_rectBounds;     // l, t, r, b
uniform half u_cornerRadius;

uniform half4  u_keyShadowColor;
uniform half2  u_keyOffset;
uniform half u_keyBlurRadius;
uniform half u_keySpreadRadius;

uniform half4  u_ambientShadowColor;
uniform half2  u_ambientOffset;
uniform half u_ambientBlurRadius;
uniform half u_ambientSpreadRadius;

half sdRoundRect(half2 p, half2 b, half r) {
    half2 q = abs(p) - b + r;
    return min(max(q.x, q.y), half(0.0)) + length(max(q, half2(0.0))) - r;
}

// Accurate approximation of erf
// This can be further approximated and probably
// nooone would notice the reduced visual quality.
half erf(half x) {
    return sign(x)*sqrt(half(1.0)-exp2(half(-1.78776)*x*x));
}

// Gaussian blur in 1D looks good enough.
half shadow(half x, half blurRadius)
{
    half sigma = half(0.57735) * blurRadius + half(0.5);
    return half(0.5)*(half(1.0) - erf(x/(sigma*half(1.414213))));
}

half4 main(float2 fragCoord) {
    half2 rectSize = u_rectBounds.zw - u_rectBounds.xy;
    half2 halfDims = rectSize * half(0.5);
    half2 rectCenter = u_rectBounds.xy + halfDims;

    // Ambient Shadow Calculation
    half2 ambientHalfDims = halfDims + u_ambientSpreadRadius;
    half2 ambientP = fragCoord - rectCenter - u_ambientOffset;
    half ambientDist = sdRoundRect(ambientP, ambientHalfDims, u_cornerRadius + u_ambientSpreadRadius);
    half ambientIntensity = shadow(ambientDist, u_ambientBlurRadius);
    half4 ambientColor = u_ambientShadowColor * ambientIntensity;

    // Key Shadow Calculation
    half2 keyHalfDims = halfDims + u_keySpreadRadius;
    half2 keyP = fragCoord - rectCenter - u_keyOffset;
    half keyDist = sdRoundRect(keyP, keyHalfDims, u_cornerRadius + u_keySpreadRadius);
    half keyIntensity = shadow(keyDist, u_keyBlurRadius);
    half4 keyColor = u_keyShadowColor * keyIntensity;

    // Blend the two shadow colors (standard src-over)
    return keyColor + ambientColor * (half(1.0) - keyColor.a);
}
)");

BoxShadowUtils::BoxShadowUtils(RuntimeEffectManager& manager) : mManager(manager) {}

void BoxShadowUtils::drawBoxShadows(SkCanvas* canvas, const SkRect& rect, float cornerRadius,
                                    const android::gui::BoxShadowSettings& settings,
                                    bool shouldDrawFpkRect) {
    SFTRACE_CALL();

    if (settings.boxShadows.size() == 0) {
        return;
    }

    sk_sp<SkRuntimeEffect> effect = mManager.mKnownEffects[kBoxShadowEffect];

    android::gui::BoxShadowSettings::BoxShadowParams keyParams = settings.boxShadows[0];
    android::gui::BoxShadowSettings::BoxShadowParams ambientParams;
    if (settings.boxShadows.size() > 1) {
        ambientParams = settings.boxShadows[1];
    }

    // Calculate the total bounding box needed to draw both shadows.
    SkRect keyShadowRect = rect.makeOutset(keyParams.spreadRadius, keyParams.spreadRadius)
                                   .makeOffset(keyParams.offsetX, keyParams.offsetY);

    float keyOutset = convertBlurUserRadiusToSigma(keyParams.blurRadius) * 3.0f;
    SkRect keyDrawRect = keyShadowRect.makeOutset(keyOutset, keyOutset);

    SkRect ambientShadowRect =
            rect.makeOutset(ambientParams.spreadRadius, ambientParams.spreadRadius)
                    .makeOffset(ambientParams.offsetX, ambientParams.offsetY);
    float ambientOutset = convertBlurUserRadiusToSigma(ambientParams.blurRadius) * 3.0f;
    SkRect ambientDrawRect = ambientShadowRect.makeOutset(ambientOutset, ambientOutset);

    SkRect unionDrawRect = keyDrawRect;
    unionDrawRect.join(ambientDrawRect);

    // Set up the shader
    SkRuntimeShaderBuilder builder(effect);
    builder.uniform("u_rectBounds") = SkV4{rect.fLeft, rect.fTop, rect.fRight, rect.fBottom};
    builder.uniform("u_cornerRadius") = cornerRadius;

    builder.uniform("u_keyShadowColor") = SkColor4f::FromColor(keyParams.color);
    builder.uniform("u_keyOffset") = SkV2{keyParams.offsetX, keyParams.offsetY};
    builder.uniform("u_keyBlurRadius") = keyParams.blurRadius;
    builder.uniform("u_keySpreadRadius") = keyParams.spreadRadius;

    builder.uniform("u_ambientShadowColor") = SkColor4f::FromColor(ambientParams.color);
    builder.uniform("u_ambientOffset") = SkV2{ambientParams.offsetX, ambientParams.offsetY};
    builder.uniform("u_ambientBlurRadius") = ambientParams.blurRadius;
    builder.uniform("u_ambientSpreadRadius") = ambientParams.spreadRadius;

    SkPaint shadowPaint;
    shadowPaint.setAntiAlias(false);
    shadowPaint.setShader(builder.makeShader());

    // Draw the combined shadow in a single draw call.
    canvas->drawRect(unionDrawRect, shadowPaint);

    if (shouldDrawFpkRect) {
        SFTRACE_NAME("FPKOptimization");
        // This optimization is just for Ganesh and can be removed once graphite is
        // enabled.
        // On a device with ARM Mali-G710 MP7 with 4 chrome windows open this
        // reduces GPU work period from 16ms to 10ms.
        float kSin45Deg = 0.70710678118f;
        SkRect killRect = rect;
        float inset = (1.0f - kSin45Deg) * cornerRadius;
        killRect.inset(inset, inset);
        // Must be pixel aligned to generate glClear
        killRect.fLeft = ceilf(killRect.fLeft);
        killRect.fTop = ceilf(killRect.fTop);
        killRect.fRight = floorf(killRect.fRight);
        killRect.fBottom = floorf(killRect.fBottom);

        SkPaint paint;
        paint.setAntiAlias(false);
        paint.setColor(0);
        paint.setBlendMode(SkBlendMode::kSrc);
        canvas->drawRect(killRect, paint);
    }
}

} // namespace android::renderengine::skia