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

#include <gui/Flags.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <gui/view/Surface.h>
#include <system/window.h>

namespace android {
namespace flagtools {
sp<SurfaceType> surfaceToSurfaceType(const sp<Surface>& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return surface;
#else
    return surface->getIGraphicBufferProducer();
#endif
}

ParcelableSurfaceType surfaceToParcelableSurfaceType(const sp<Surface>& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return view::Surface::fromSurface(surface);
#else
    return surface->getIGraphicBufferProducer();
#endif
}

sp<IGraphicBufferProducer> surfaceTypeToIGBP(const sp<SurfaceType>& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return surface->getIGraphicBufferProducer();
#else
    return surface;
#endif
}

bool isSurfaceTypeValid(const sp<SurfaceType>& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return Surface::isValid(surface);
#else
    return surface != nullptr;
#endif
}

ParcelableSurfaceType toParcelableSurfaceType(const view::Surface& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return surface;
#else
    return surface.graphicBufferProducer;
#endif
}

ParcelableSurfaceType convertSurfaceTypeToParcelable(sp<SurfaceType> surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return view::Surface::fromSurface(surface);
#else
    return surface;
#endif
}

sp<SurfaceType> convertParcelableSurfaceTypeToSurface(const ParcelableSurfaceType& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return surface.toSurface();
#else
    return surface;
#endif
}
} // namespace flagtools
namespace mediaflagtools {

sp<MediaSurfaceType> nativeWindowToSurfaceType(ANativeWindow* anw) {
    if (anw == nullptr) {
        return nullptr;
    }

    sp<Surface> surface = Surface::from(anw);
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
    return surface;
#else
    return surface->getIGraphicBufferProducer();
#endif
}

sp<MediaSurfaceType> igbpToSurfaceType(const sp<IGraphicBufferProducer>& igbp) {
    if (igbp == nullptr) {
        return nullptr;
    }
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
    return sp<Surface>::make(igbp);
#else
    return igbp;
#endif
}

sp<IGraphicBufferProducer> surfaceTypeToIGBP(const sp<MediaSurfaceType>& mst) {
    if (mst == nullptr) {
        return nullptr;
    }
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
    return mst->getIGraphicBufferProducer();
#else
    return mst;
#endif
}

sp<SurfaceType> mediaSurfaceToCameraSurfaceType(const sp<MediaSurfaceType>& mst,
                                                [[maybe_unused]] bool controlledByApp) {
    if (mst == nullptr) {
        return nullptr;
    }
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return mst;
#else
    return mst->getIGraphicBufferProducer();
#endif
#else
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return sp<Surface>::make(mst, controlledByApp);
#else
    return mst;
#endif
#endif
}

sp<Surface> surfaceTypeToSurface(const sp<MediaSurfaceType>& mst,
                                 [[maybe_unused]] bool controlledByApp) {
    if (mst == nullptr) {
        return nullptr;
    }
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
    return mst;
#else
    return sp<Surface>::make(mst, controlledByApp);
#endif
}

sp<MediaSurfaceType> surfaceToSurfaceType(const sp<Surface>& surface) {
    if (surface == nullptr) {
        return nullptr;
    }
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
    return surface;
#else
    return surface->getIGraphicBufferProducer();
#endif
}
} // namespace mediaflagtools
} // namespace android