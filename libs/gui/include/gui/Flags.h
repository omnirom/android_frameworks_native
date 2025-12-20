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

#include <com_android_graphics_libgui_flags.h>
#include <utils/StrongPointer.h>

struct ANativeWindow;

namespace android {

class IGraphicBufferProducer;
class Surface;
namespace view {
class Surface;
}

#define WB_LIBCAMERASERVICE_WITH_DEPENDENCIES \
    (COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_LIBCAMERASERVICE))

// Camera
#if WB_LIBCAMERASERVICE_WITH_DEPENDENCIES
typedef android::Surface SurfaceType;
typedef android::view::Surface ParcelableSurfaceType;
#else
typedef android::IGraphicBufferProducer SurfaceType;
typedef android::sp<android::IGraphicBufferProducer> ParcelableSurfaceType;
#endif

namespace flagtools {
sp<SurfaceType> surfaceToSurfaceType(const sp<Surface>& surface);
ParcelableSurfaceType surfaceToParcelableSurfaceType(const sp<Surface>& surface);
ParcelableSurfaceType toParcelableSurfaceType(const view::Surface& surface);
sp<IGraphicBufferProducer> surfaceTypeToIGBP(const sp<SurfaceType>& surface);
bool isSurfaceTypeValid(const sp<SurfaceType>& surface);
ParcelableSurfaceType convertSurfaceTypeToParcelable(sp<SurfaceType> surface);
sp<SurfaceType> convertParcelableSurfaceTypeToSurface(const ParcelableSurfaceType& surface);
} // namespace flagtools

// Media
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_MEDIA_MIGRATION)
typedef android::Surface MediaSurfaceType;
typedef android::view::Surface MediaParcelableSurfaceType;
#else
typedef android::IGraphicBufferProducer MediaSurfaceType;
typedef android::sp<android::IGraphicBufferProducer> MediaParcelableSurfaceType;
#endif

namespace mediaflagtools {
sp<MediaSurfaceType> nativeWindowToSurfaceType(ANativeWindow* anw);
sp<MediaSurfaceType> igbpToSurfaceType(const sp<IGraphicBufferProducer>& igbp);
sp<IGraphicBufferProducer> surfaceTypeToIGBP(const sp<MediaSurfaceType>& mst);
sp<SurfaceType> mediaSurfaceToCameraSurfaceType(const sp<MediaSurfaceType>& mst,
                                                bool controlledByApp = false);
sp<Surface> surfaceTypeToSurface(const sp<MediaSurfaceType>& mst, bool controlledByApp = false);
sp<MediaSurfaceType> surfaceToSurfaceType(const sp<Surface>& surface);
} // namespace mediaflagtools
} // namespace android
