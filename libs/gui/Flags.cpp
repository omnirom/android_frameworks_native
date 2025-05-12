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

namespace android {
namespace flagtools {
sp<SurfaceType> surfaceToSurfaceType(const sp<Surface>& surface) {
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
    return surface;
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
} // namespace android