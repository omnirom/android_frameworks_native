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

#pragma once

#include <include/gpu/graphite/PrecompileContext.h>

namespace android::renderengine::skia {

using namespace skgpu;

class RuntimeEffectManager;

/**
 * Handles precompiling the underlying Skia rendering pipelines needed for RenderEngine's drawing
 * operations.
 *
 * The precompilation settings that RenderEngine gives to Skia that control which pipelines get
 * precompiled are maintained in upstream Skia, where iteration and testing is easier. They are then
 * copied here, where slight modifications may be needed (e.g. feeding in the effectManager).
 *
 * Note: this class may handle other minor Skia pipeline management tasks in the future, such as
 * providing a callback for Skia to tell RenderEngine the label of a pipeline that was missed during
 * precompilation.
 */
class GraphitePipelineManager {
public:
    // Should be invoked on a thread dedicated to precompilation, as execution may take several
    // seconds. Drawing work may occur in parallel, as precompilation and drawing operations are
    // thread-safe in Graphite.
    static void PrecompilePipelines(std::unique_ptr<graphite::PrecompileContext> precompileContext,
                                    RuntimeEffectManager& effectManager);
};

} // namespace android::renderengine::skia
