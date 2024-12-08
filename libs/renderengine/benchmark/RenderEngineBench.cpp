/*
 * Copyright 2021 The Android Open Source Project
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

#include <RenderEngineBench.h>
#include <android-base/file.h>
#include <benchmark/benchmark.h>
#include <gui/SurfaceComposerClient.h>
#include <log/log.h>
#include <renderengine/ExternalTexture.h>
#include <renderengine/LayerSettings.h>
#include <renderengine/RenderEngine.h>
#include <renderengine/impl/ExternalTexture.h>

#include <mutex>

using namespace android;
using namespace android::renderengine;

// To run tests:
/**
 * mmm frameworks/native/libs/renderengine/benchmark;\
 * adb push $OUT/data/benchmarktest/librenderengine_bench/librenderengine_bench
 *      /data/benchmarktest/librenderengine_bench/librenderengine_bench;\
 * adb shell /data/benchmarktest/librenderengine_bench/librenderengine_bench
 *
 * (64-bit devices: out directory contains benchmarktest64 instead of benchmarktest)
 */

///////////////////////////////////////////////////////////////////////////////
//  Helpers for calling drawLayers
///////////////////////////////////////////////////////////////////////////////

std::pair<uint32_t, uint32_t> getDisplaySize() {
    // These will be retrieved from a ui::Size, which stores int32_t, but they will be passed
    // to GraphicBuffer, which wants uint32_t.
    static uint32_t width, height;
    std::once_flag once;
    std::call_once(once, []() {
        auto surfaceComposerClient = SurfaceComposerClient::getDefault();
        auto ids = SurfaceComposerClient::getPhysicalDisplayIds();
        LOG_ALWAYS_FATAL_IF(ids.empty(), "Failed to get any display!");
        ui::Size resolution = ui::kEmptySize;
        // find the largest display resolution
        for (auto id : ids) {
            auto displayToken = surfaceComposerClient->getPhysicalDisplayToken(id);
            ui::DisplayMode displayMode;
            if (surfaceComposerClient->getActiveDisplayMode(displayToken, &displayMode) < 0) {
                LOG_ALWAYS_FATAL("Failed to get active display mode!");
            }
            auto tw = displayMode.resolution.width;
            auto th = displayMode.resolution.height;
            LOG_ALWAYS_FATAL_IF(tw <= 0 || th <= 0, "Invalid display size!");
            if (resolution.width * resolution.height <
                displayMode.resolution.width * displayMode.resolution.height) {
                resolution = displayMode.resolution;
            }
        }
        width = static_cast<uint32_t>(resolution.width);
        height = static_cast<uint32_t>(resolution.height);
    });
    return std::pair<uint32_t, uint32_t>(width, height);
}

static std::unique_ptr<RenderEngine> createRenderEngine(
        RenderEngine::Threaded threaded, RenderEngine::GraphicsApi graphicsApi,
        RenderEngine::BlurAlgorithm blurAlgorithm = RenderEngine::BlurAlgorithm::KAWASE) {
    auto args = RenderEngineCreationArgs::Builder()
                        .setPixelFormat(static_cast<int>(ui::PixelFormat::RGBA_8888))
                        .setImageCacheSize(1)
                        .setEnableProtectedContext(true)
                        .setPrecacheToneMapperShaderOnly(false)
                        .setBlurAlgorithm(blurAlgorithm)
                        .setContextPriority(RenderEngine::ContextPriority::REALTIME)
                        .setThreaded(threaded)
                        .setGraphicsApi(graphicsApi)
                        .build();
    return RenderEngine::create(args);
}

static std::shared_ptr<ExternalTexture> allocateBuffer(RenderEngine& re, uint32_t width,
                                                       uint32_t height,
                                                       uint64_t extraUsageFlags = 0,
                                                       std::string name = "output") {
    return std::make_shared<
            impl::ExternalTexture>(sp<GraphicBuffer>::make(width, height,
                                                           HAL_PIXEL_FORMAT_RGBA_8888, 1u,
                                                           GRALLOC_USAGE_HW_RENDER |
                                                                   GRALLOC_USAGE_HW_TEXTURE |
                                                                   extraUsageFlags,
                                                           std::move(name)),
                                   re,
                                   impl::ExternalTexture::Usage::READABLE |
                                           impl::ExternalTexture::Usage::WRITEABLE);
}

static std::shared_ptr<ExternalTexture> copyBuffer(RenderEngine& re,
                                                   std::shared_ptr<ExternalTexture> original,
                                                   uint64_t extraUsageFlags, std::string name) {
    const uint32_t width = original->getBuffer()->getWidth();
    const uint32_t height = original->getBuffer()->getHeight();
    auto texture = allocateBuffer(re, width, height, extraUsageFlags, name);

    const Rect displayRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
    DisplaySettings display{
            .physicalDisplay = displayRect,
            .clip = displayRect,
            .maxLuminance = 500,
    };

    const FloatRect layerRect(0, 0, width, height);
    LayerSettings layer{
            .geometry =
                    Geometry{
                            .boundaries = layerRect,
                    },
            .source =
                    PixelSource{
                            .buffer =
                                    Buffer{
                                            .buffer = original,
                                    },
                    },
            .alpha = half(1.0f),
    };
    auto layers = std::vector<LayerSettings>{layer};

    sp<Fence> waitFence = re.drawLayers(display, layers, texture, base::unique_fd()).get().value();
    waitFence->waitForever(LOG_TAG);
    return texture;
}

/**
 * Helper for timing calls to drawLayers.
 *
 * Caller needs to create RenderEngine and the LayerSettings, and this takes
 * care of setting up the display, starting and stopping the timer, calling
 * drawLayers, and saving (if --save is used).
 *
 * This times both the CPU and GPU work initiated by drawLayers. All work done
 * outside of the for loop is excluded from the timing measurements.
 */
static void benchDrawLayers(RenderEngine& re, const std::vector<LayerSettings>& layers,
                            benchmark::State& benchState, const char* saveFileName) {
    auto [width, height] = getDisplaySize();
    auto outputBuffer = allocateBuffer(re, width, height);

    const Rect displayRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
    DisplaySettings display{
            .physicalDisplay = displayRect,
            .clip = displayRect,
            .maxLuminance = 500,
    };

    // This loop starts and stops the timer.
    for (auto _ : benchState) {
        sp<Fence> waitFence =
                re.drawLayers(display, layers, outputBuffer, base::unique_fd()).get().value();
        waitFence->waitForever(LOG_TAG);
    }

    if (renderenginebench::save() && saveFileName) {
        // Copy to a CPU-accessible buffer so we can encode it.
        outputBuffer = copyBuffer(re, outputBuffer, GRALLOC_USAGE_SW_READ_OFTEN, "to_encode");

        std::string outFile = base::GetExecutableDirectory();
        outFile.append("/");
        outFile.append(saveFileName);
        outFile.append(".jpg");
        renderenginebench::encodeToJpeg(outFile.c_str(), outputBuffer->getBuffer());
    }
}

/**
 * Return a buffer with the image in the provided path, relative to the executable directory
 */
static std::shared_ptr<ExternalTexture> createTexture(RenderEngine& re, const char* relPathImg) {
    // Initially use cpu access so we can decode into it with AImageDecoder.
    auto [width, height] = getDisplaySize();
    auto srcBuffer =
            allocateBuffer(re, width, height, GRALLOC_USAGE_SW_WRITE_OFTEN, "decoded_source");
    std::string fileName = base::GetExecutableDirectory().append(relPathImg);
    renderenginebench::decode(fileName.c_str(), srcBuffer->getBuffer());
    // Now copy into GPU-only buffer for more realistic timing.
    srcBuffer = copyBuffer(re, srcBuffer, 0, "source");
    return srcBuffer;
}

///////////////////////////////////////////////////////////////////////////////
//  Benchmarks
///////////////////////////////////////////////////////////////////////////////

constexpr char kHomescreenPath[] = "/resources/homescreen.png";

/**
 * Draw a layer with texture and no additional shaders as a baseline to evaluate a shader's impact
 * on performance
 */
template <class... Args>
void BM_homescreen(benchmark::State& benchState, Args&&... args) {
    auto args_tuple = std::make_tuple(std::move(args)...);
    auto re = createRenderEngine(static_cast<RenderEngine::Threaded>(std::get<0>(args_tuple)),
                                 static_cast<RenderEngine::GraphicsApi>(std::get<1>(args_tuple)));

    auto [width, height] = getDisplaySize();
    auto srcBuffer = createTexture(*re, kHomescreenPath);

    const FloatRect layerRect(0, 0, width, height);
    LayerSettings layer{
            .geometry =
                    Geometry{
                            .boundaries = layerRect,
                    },
            .source =
                    PixelSource{
                            .buffer =
                                    Buffer{
                                            .buffer = srcBuffer,
                                    },
                    },
            .alpha = half(1.0f),
    };
    auto layers = std::vector<LayerSettings>{layer};
    benchDrawLayers(*re, layers, benchState, "homescreen");
}

template <class... Args>
void BM_homescreen_blur(benchmark::State& benchState, Args&&... args) {
    auto args_tuple = std::make_tuple(std::move(args)...);
    auto re = createRenderEngine(static_cast<RenderEngine::Threaded>(std::get<0>(args_tuple)),
                                 static_cast<RenderEngine::GraphicsApi>(std::get<1>(args_tuple)));

    auto [width, height] = getDisplaySize();
    auto srcBuffer = createTexture(*re, kHomescreenPath);

    const FloatRect layerRect(0, 0, width, height);
    LayerSettings layer{
            .geometry =
                    Geometry{
                            .boundaries = layerRect,
                    },
            .source =
                    PixelSource{
                            .buffer =
                                    Buffer{
                                            .buffer = srcBuffer,
                                    },
                    },
            .alpha = half(1.0f),
    };
    LayerSettings blurLayer{
            .geometry =
                    Geometry{
                            .boundaries = layerRect,
                    },
            .alpha = half(1.0f),
            .skipContentDraw = true,
            .backgroundBlurRadius = 60,
    };

    auto layers = std::vector<LayerSettings>{layer, blurLayer};
    benchDrawLayers(*re, layers, benchState, "homescreen_blurred");
}

template <class... Args>
void BM_homescreen_edgeExtension(benchmark::State& benchState, Args&&... args) {
    auto args_tuple = std::make_tuple(std::move(args)...);
    auto re = createRenderEngine(static_cast<RenderEngine::Threaded>(std::get<0>(args_tuple)),
                                 static_cast<RenderEngine::GraphicsApi>(std::get<1>(args_tuple)));

    auto [width, height] = getDisplaySize();
    auto srcBuffer = createTexture(*re, kHomescreenPath);

    LayerSettings layer{
            .geometry =
                    Geometry{
                            .boundaries = FloatRect(0, 0, width, height),
                    },
            .source =
                    PixelSource{
                            .buffer =
                                    Buffer{
                                            .buffer = srcBuffer,
                                            // Part of the screen is not covered by the texture but
                                            // will be filled in by the shader
                                            .textureTransform =
                                                    mat4(mat3(),
                                                         vec3(width * 0.3f, height * 0.3f, 0.0f)),
                                    },
                    },
            .alpha = half(1.0f),
            .edgeExtensionEffect =
                    EdgeExtensionEffect(/* left */ true,
                                        /* right  */ false, /* top */ true, /* bottom */ false),
    };
    auto layers = std::vector<LayerSettings>{layer};
    benchDrawLayers(*re, layers, benchState, "homescreen_edge_extension");
}

BENCHMARK_CAPTURE(BM_homescreen_blur, gaussian, RenderEngine::Threaded::YES,
                  RenderEngine::GraphicsApi::GL, RenderEngine::BlurAlgorithm::GAUSSIAN);

BENCHMARK_CAPTURE(BM_homescreen_blur, kawase, RenderEngine::Threaded::YES,
                  RenderEngine::GraphicsApi::GL, RenderEngine::BlurAlgorithm::KAWASE);

BENCHMARK_CAPTURE(BM_homescreen_blur, kawase_dual_filter, RenderEngine::Threaded::YES,
                  RenderEngine::GraphicsApi::GL, RenderEngine::BlurAlgorithm::KAWASE_DUAL_FILTER);

BENCHMARK_CAPTURE(BM_homescreen, SkiaGLThreaded, RenderEngine::Threaded::YES,
                  RenderEngine::GraphicsApi::GL);

BENCHMARK_CAPTURE(BM_homescreen_edgeExtension, SkiaGLThreaded, RenderEngine::Threaded::YES,
                  RenderEngine::GraphicsApi::GL);
