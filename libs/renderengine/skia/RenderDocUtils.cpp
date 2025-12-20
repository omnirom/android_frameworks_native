/*
 * Copyright (C) 2025 The Android Open Source Project
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

#define RE_ENABLE_RENDERDOC 0

#if RE_ENABLE_RENDERDOC
#include "RenderDocUtils.h"
#include <dlfcn.h>
#include <log/log_main.h>
#include "../../../../../external/angle/third_party/renderdoc/src/renderdoc_app.h"

__attribute__((visibility("default"), used)) //
extern "C" const char* _android_override_gles_debug_layers =
        "/data/local/debug/gles/libVkLayer_GLES_RenderDoc.so";

// This path must have write permission
constexpr char kPathTemplate[] = "/data/local/tmp/sf_rdoc_capture";

// Only supports GLES
constexpr char kLayerPath[] = "/data/local/debug/gles/libVkLayer_GLES_RenderDoc.so";

bool RenderDocUtils::ensureLoaded() {
    if (mApi) {
        // Already loaded;
        return true;
    }

    void* mod = dlopen(kLayerPath, RTLD_NOW | RTLD_NOLOAD);
    if (!mod) {
        ALOGE("[RDOC] Failed to load RenderDoc library %s", kLayerPath);
        return false;
    }
    ALOGI("[RDOC] Loaded RenderDoc library from %s", kLayerPath);

    pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
    if (!RENDERDOC_GetAPI) {
        ALOGE("[RDOC] Failed to load RenderDoc GetAPI function.");
        return false;
    }
    ALOGI("[RDOC] Found RenderDoc GetAPI function.");

    RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&mApi));
    if (!mApi) {
        ALOGE("[RDOC] RenderDoc library loaded but failed to find correct API version.");
        return false;
    }
    ALOGI("[RDOC] Loaded RenderDoc API.");
    mApi->SetCaptureFilePathTemplate(kPathTemplate);
    mApi->SetCaptureOptionU32(eRENDERDOC_Option_CaptureAllCmdLists, 1);

    return true;
}

void RenderDocUtils::startFrameCapture() {
    if (!ensureLoaded()) {
        return;
    }

    ALOGI("[RDOC] Starting RenderDoc frame capture");
    mApi->StartFrameCapture(NULL, NULL);
}

void RenderDocUtils::endFrameCapture() {
    if (!mApi) {
        return;
    }
    ALOGI("[RDOC] Ending RenderDoc frame capture");
    mApi->EndFrameCapture(NULL, NULL);
}

#else // RE_ENABLE_RENDERDOC
#include <log/log_main.h>
#include "RenderDocUtils.h"

void RenderDocUtils::startFrameCapture() {
    ALOGE("[RDOC] RenderDoc not enabled. RenderDoc support must be enabled at compile time.");
}
void RenderDocUtils::endFrameCapture() {}
bool RenderDocUtils::ensureLoaded() {
    return false;
}

#endif // RE_ENABLE_RENDERDOC
