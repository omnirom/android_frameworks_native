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

#include <optional>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>

#include <LocklessQueue.h>

namespace android::surfaceflinger {

namespace {

static void pushPop(benchmark::State& state) {
    using ItemT = std::vector<int64_t>;
    LocklessQueue<ItemT> queue;
    ItemT item(size_t(state.range(0)), 42);
    for (auto _ : state) {
        queue.push(std::move(item));
        item = std::move(*queue.pop());
        benchmark::DoNotOptimize(item);
    }
}
BENCHMARK(pushPop)->Range(1, 1048576);

} // namespace
} // namespace android::surfaceflinger
