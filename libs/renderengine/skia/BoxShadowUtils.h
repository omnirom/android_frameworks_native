#pragma once

#include <android/gui/BoxShadowSettings.h>
#include <include/core/SkCanvas.h>
#include "filters/RuntimeEffectManager.h"

namespace android::renderengine::skia {

class BoxShadowUtils {
public:
    explicit BoxShadowUtils(RuntimeEffectManager& manager);
    void drawBoxShadows(SkCanvas* canvas, const SkRect& rect, float cornerRadius,
                        const android::gui::BoxShadowSettings& settings, bool shouldDrawFpkRect);

private:
    RuntimeEffectManager& mManager;
};

} // namespace android::renderengine::skia