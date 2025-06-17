/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <benchmark/benchmark.h>
#include <cstddef>
#include <optional>
#include <vector>
#include "binder/Parcel.h"
#include "gui/SurfaceComposerClient.h"
#include "gui/SurfaceControl.h"
#include "log/log_main.h"

namespace android {
namespace {
using android::hardware::graphics::common::V1_1::BufferUsage;

std::vector<sp<SurfaceControl>> createSurfaceControl(const char* name, size_t num) {
    sp<SurfaceComposerClient> client = sp<SurfaceComposerClient>::make();
    LOG_FATAL_IF(client->initCheck() != OK, "Could not init SurfaceComposerClient");
    std::vector<sp<SurfaceControl>> surfaceControls;
    for (size_t i = 0; i < num; i++) {
        surfaceControls.push_back(
                client->createSurface(String8(name), 0, 0, PIXEL_FORMAT_RGBA_8888,
                                      ISurfaceComposerClient::eFXSurfaceBufferState));
    }
    return surfaceControls;
}

void applyTransaction(benchmark::State& state) {
    std::vector<sp<SurfaceControl>> surfaceControls = createSurfaceControl(__func__, 5 /* num */);
    for (auto _ : state) {
        SurfaceComposerClient::Transaction t;
        for (auto& sc : surfaceControls) {
            t.setCrop(sc, FloatRect{1, 2, 3, 4});
            t.setAutoRefresh(sc, true);
            t.hide(sc);
            t.setAlpha(sc, 0.5);
            t.setCornerRadius(sc, 0.8);
        }
        Parcel p;
        t.writeToParcel(&p);
        t.clear();
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(applyTransaction);

// Mimic a buffer transaction with callbacks
void applyBufferTransaction(benchmark::State& state) {
    std::vector<sp<SurfaceControl>> surfaceControls = createSurfaceControl(__func__, 5 /* num */);
    std::vector<sp<GraphicBuffer>> buffers;
    for (size_t i = 0; i < surfaceControls.size(); i++) {
        int64_t usageFlags = BufferUsage::CPU_READ_OFTEN | BufferUsage::CPU_WRITE_OFTEN |
                BufferUsage::COMPOSER_OVERLAY | BufferUsage::GPU_TEXTURE;
        buffers.emplace_back(
                sp<GraphicBuffer>::make(5, 5, PIXEL_FORMAT_RGBA_8888, 1, usageFlags, "test"));
    }

    for (auto _ : state) {
        SurfaceComposerClient::Transaction t;
        int i = 0;
        for (auto& sc : surfaceControls) {
            std::function<void(const ReleaseCallbackId&, const sp<Fence>& /*releaseFence*/,
                               std::optional<uint32_t> currentMaxAcquiredBufferCount)>
                    releaseBufferCallback;
            t.setBuffer(sc, buffers[i], std::nullopt, std::nullopt, 5, releaseBufferCallback);
        }
        Parcel p;
        // proxy for applying the transaction
        t.writeToParcel(&p);
        t.clear();
        benchmark::DoNotOptimize(t);
    }
}
BENCHMARK(applyBufferTransaction);

void mergeTransaction(benchmark::State& state) {
    std::vector<sp<SurfaceControl>> surfaceControls = createSurfaceControl(__func__, 5 /* num */);
    for (auto _ : state) {
        SurfaceComposerClient::Transaction t1;
        for (auto& sc : surfaceControls) {
            t1.setCrop(sc, FloatRect{1, 2, 3, 4});
            t1.setAutoRefresh(sc, true);
            t1.hide(sc);
            t1.setAlpha(sc, 0.5);
            t1.setCornerRadius(sc, 0.8);
        }

        SurfaceComposerClient::Transaction t2;
        for (auto& sc : surfaceControls) {
            t2.hide(sc);
            t2.setAlpha(sc, 0.5);
            t2.setCornerRadius(sc, 0.8);
            t2.setBackgroundBlurRadius(sc, 5);
        }
        t1.merge(std::move(t2));
        benchmark::DoNotOptimize(t1);
    }
}
BENCHMARK(mergeTransaction);

void readTransactionFromParcel(benchmark::State& state) {
    std::vector<sp<SurfaceControl>> surfaceControls = createSurfaceControl(__func__, 5 /* num */);
    SurfaceComposerClient::Transaction t;
    for (auto& sc : surfaceControls) {
        t.setCrop(sc, FloatRect{1, 2, 3, 4});
        t.setAutoRefresh(sc, true);
        t.hide(sc);
        t.setAlpha(sc, 0.5);
        t.setCornerRadius(sc, 0.8);
    }
    Parcel p;
    t.writeToParcel(&p);
    t.clear();

    for (auto _ : state) {
        SurfaceComposerClient::Transaction t2;
        t2.readFromParcel(&p);
        p.setDataPosition(0);
        benchmark::DoNotOptimize(t2);
    }
}
BENCHMARK(readTransactionFromParcel);

} // namespace
} // namespace android
