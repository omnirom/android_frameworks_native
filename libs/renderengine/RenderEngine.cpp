/*
 * Copyright 2013 The Android Open Source Project
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

#include <renderengine/RenderEngine.h>

#include "renderengine/ExternalTexture.h"
#include "skia/GaneshVkRenderEngine.h"
#include "skia/GraphiteVkRenderEngine.h"
#include "skia/SkiaGLRenderEngine.h"
#include "threaded/RenderEngineThreaded.h"
#include "ui/GraphicTypes.h"

#include <com_android_graphics_surfaceflinger_flags.h>
#include <cutils/properties.h>
#include <ftl/enum.h>
#include <log/log.h>

namespace android {
namespace renderengine {

// TODO: b/341728634 - Don't compile Ganesh unless requested once Graphite is the stable default.
std::unique_ptr<RenderEngine> RenderEngine::create(const RenderEngineCreationArgs& args) {
    threaded::CreateInstanceFactory createInstanceFactory;

    ALOGD("%sRenderEngine with Skia%s Backend (%s)",
          args.threaded == Threaded::Yes ? "Threaded " : "",
          ftl::enum_string(args.graphicsApi).c_str(), ftl::enum_string(args.skiaBackend).c_str());

    if (args.skiaBackend == SkiaBackend::Graphite) {
        createInstanceFactory = [args]() {
            return android::renderengine::skia::GraphiteVkRenderEngine::create(args);
        };
    } else { // GANESH
        if (args.graphicsApi == GraphicsApi::Vk) {
            createInstanceFactory = [args]() {
                return android::renderengine::skia::GaneshVkRenderEngine::create(args);
            };
        } else { // GL
            createInstanceFactory = [args]() {
                return android::renderengine::skia::SkiaGLRenderEngine::create(args);
            };
        }
    }

    if (args.threaded != Threaded::Yes) {
        ALOGE("Non-threaded RenderEngine not supported, please update or "
              "remove " PROPERTY_DEBUG_RENDERENGINE_BACKEND " to use a supported backend");
    }

    return renderengine::threaded::RenderEngineThreaded::create(createInstanceFactory);
}

RenderEngine::~RenderEngine() = default;

void RenderEngine::validateInputBufferUsage(const sp<GraphicBuffer>& buffer) {
    LOG_ALWAYS_FATAL_IF(!(buffer->getUsage() & GraphicBuffer::USAGE_HW_TEXTURE),
                        "input buffer not gpu readable");
}

void RenderEngine::validateOutputBufferUsage(const sp<GraphicBuffer>& buffer) {
    LOG_ALWAYS_FATAL_IF(!(buffer->getUsage() & GraphicBuffer::USAGE_HW_RENDER),
                        "output buffer not gpu writeable");
}

ftl::Future<FenceResult> RenderEngine::drawLayers(const DisplaySettings& display,
                                                  const std::vector<LayerSettings>& layers,
                                                  const std::shared_ptr<ExternalTexture>& buffer,
                                                  base::unique_fd&& bufferFence) {
    const auto resultPromise = std::make_shared<std::promise<FenceResult>>();
    std::future<FenceResult> resultFuture = resultPromise->get_future();
    updateProtectedContext(layers, {buffer.get()});
    drawLayersInternal(std::move(resultPromise), display, layers, buffer, std::move(bufferFence));
    return resultFuture;
}

ftl::Future<FenceResult> RenderEngine::tonemapAndDrawGainmap(
        const std::shared_ptr<ExternalTexture>& hdr, base::borrowed_fd&& hdrFence,
        float hdrSdrRatio, ui::Dataspace dataspace, const std::shared_ptr<ExternalTexture>& sdr,
        const std::shared_ptr<ExternalTexture>& gainmap) {
    const auto resultPromise = std::make_shared<std::promise<FenceResult>>();
    std::future<FenceResult> resultFuture = resultPromise->get_future();
    updateProtectedContext({}, {sdr.get(), hdr.get(), gainmap.get()});
    tonemapAndDrawGainmapInternal(std::move(resultPromise), hdr, std::move(hdrFence), hdrSdrRatio,
                                  dataspace, sdr, gainmap);
    return resultFuture;
}

void RenderEngine::updateProtectedContext(const std::vector<LayerSettings>& layers,
                                          vector<const ExternalTexture*> buffers) {
    const bool needsProtectedContext =
            std::any_of(layers.begin(), layers.end(),
                        [](const LayerSettings& layer) {
                            const std::shared_ptr<ExternalTexture>& buffer =
                                    layer.source.buffer.buffer;
                            return buffer && (buffer->getUsage() & GRALLOC_USAGE_PROTECTED);
                        }) ||
            std::any_of(buffers.begin(), buffers.end(), [](const ExternalTexture* buffer) {
                return buffer && (buffer->getUsage() & GRALLOC_USAGE_PROTECTED);
            });
    useProtectedContext(needsProtectedContext);
}

} // namespace renderengine
} // namespace android
