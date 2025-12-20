/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "PowerHalControllerBenchmarks"

#include <aidl/android/hardware/power/Boost.h>
#include <aidl/android/hardware/power/Mode.h>
#include <benchmark/benchmark.h>
#include <chrono>
#include <powermanager/PowerHalController.h>
#include <powermanager/PowerHintSessionWrapper.h>
#include <testUtil.h>

using aidl::android::hardware::power::Boost;
using aidl::android::hardware::power::Mode;
using aidl::android::hardware::power::WorkDuration;
using android::power::HalResult;
using android::power::PowerHalController;
using android::power::PowerHintSessionWrapper;

using namespace android;
using namespace std::chrono_literals;

// Values from Boost.aidl and Mode.aidl.
static constexpr int64_t FIRST_BOOST = static_cast<int64_t>(*ndk::enum_range<Boost>().begin());
static constexpr int64_t LAST_BOOST = static_cast<int64_t>(*(ndk::enum_range<Boost>().end()-1));
static constexpr int64_t FIRST_MODE = static_cast<int64_t>(*ndk::enum_range<Mode>().begin());
static constexpr int64_t LAST_MODE = static_cast<int64_t>(*(ndk::enum_range<Mode>().end()-1));

class DurationWrapper : public WorkDuration {
public:
    DurationWrapper(int64_t dur, int64_t time) {
        durationNanos = dur;
        timeStampNanos = time;
    }
};

static const std::vector<WorkDuration> DURATIONS = {
        DurationWrapper(1L, 1L),
        DurationWrapper(1000L, 2L),
        DurationWrapper(1000000L, 3L),
        DurationWrapper(1000000000L, 4L),
};

// Delay between oneway method calls to avoid overflowing the binder buffers.
static constexpr std::chrono::microseconds ONEWAY_API_DELAY = 100us;

template <typename T, class... Args0, class... Args1>
static void runBenchmark(benchmark::State& state, HalResult<T> (PowerHalController::*fn)(Args0...),
                         Args1&&... args1) {
    PowerHalController initController;
    HalResult<T> result = (initController.*fn)(std::forward<Args1>(args1)...);
    if (result.isFailed()) {
        state.SkipWithError(result.errorMessage());
        return;
    } else if (result.isUnsupported()) {
        ALOGV("Power HAL does not support this operation, skipping test...");
        state.SkipWithMessage("operation unsupported");
        return;
    }

    for (auto _ : state) {
        PowerHalController controller; // new controller to avoid caching
        HalResult<T> ret = (controller.*fn)(std::forward<Args1>(args1)...);
        if (ret.isFailed()) {
            state.SkipWithError(ret.errorMessage());
            break;
        }
        state.PauseTiming();
        testDelaySpin(
                std::chrono::duration_cast<std::chrono::duration<float>>(ONEWAY_API_DELAY).count());
        state.ResumeTiming();
    }
}

template <typename T, class... Args0, class... Args1>
static void runCachedBenchmark(benchmark::State& state,
                               HalResult<T> (PowerHalController::*fn)(Args0...), Args1&&... args1) {
    PowerHalController controller;
    // First call out of test, to cache HAL service and isSupported result.
    HalResult<T> result = (controller.*fn)(std::forward<Args1>(args1)...);
    if (result.isFailed()) {
        state.SkipWithError(result.errorMessage());
        return;
    } else if (result.isUnsupported()) {
        ALOGV("Power HAL does not support this operation, skipping test...");
        state.SkipWithMessage("operation unsupported");
        return;
    }

    for (auto _ : state) {
        HalResult<T> ret = (controller.*fn)(std::forward<Args1>(args1)...);
        if (ret.isFailed()) {
            state.SkipWithError(ret.errorMessage());
            break;
        }
    }
}

template <class T, class... Args0, class... Args1>
static void runSessionBenchmark(benchmark::State& state,
                                HalResult<T> (PowerHintSessionWrapper::*fn)(Args0...),
                                Args1&&... args1) {
    PowerHalController controller;

    // do not use tid from the benchmark process, use 1 for init
    std::vector<int32_t> threadIds{1};
    int64_t durationNanos = 16666666L;

    HalResult<std::shared_ptr<PowerHintSessionWrapper>> result =
        controller.createHintSession(1, 0, threadIds, durationNanos);

    if (result.isFailed()) {
        state.SkipWithError(result.errorMessage());
        return;
    } else if (result.isUnsupported()) {
        ALOGV("Power HAL doesn't support session, skipping test...");
        state.SkipWithMessage("operation unsupported");
        return;
    }

    std::shared_ptr<PowerHintSessionWrapper> session = result.value();
    HalResult<T> ret = (*session.*fn)(std::forward<Args1>(args1)...);
    if (ret.isUnsupported()) {
        ALOGV("Power HAL session does not support this operation, skipping test...");
        state.SkipWithMessage("operation unsupported");
        return;
    }

    for (auto _ : state) {
        ret = (*session.*fn)(std::forward<Args1>(args1)...);
        if (!ret.isOk()) {
            state.SkipWithError(ret.errorMessage());
            break;
        }
        state.PauseTiming();
        testDelaySpin(
                std::chrono::duration_cast<std::chrono::duration<float>>(ONEWAY_API_DELAY).count());
        state.ResumeTiming();
    }
    session->close();
}

static void BM_PowerHalControllerBenchmarks_init(benchmark::State& state) {
    for (auto _ : state) {
        PowerHalController controller;
        controller.init();
    }
}

static void BM_PowerHalControllerBenchmarks_initCached(benchmark::State& state) {
    PowerHalController controller;
    // First connection out of test.
    controller.init();

    while (state.KeepRunning()) {
        controller.init();
    }
}

static void BM_PowerHalControllerBenchmarks_setBoost(benchmark::State& state) {
    Boost boost = static_cast<Boost>(state.range(0));
    runBenchmark(state, &PowerHalController::setBoost, boost, 1);
}

static void BM_PowerHalControllerBenchmarks_setBoostCached(benchmark::State& state) {
    Boost boost = static_cast<Boost>(state.range(0));
    runCachedBenchmark(state, &PowerHalController::setBoost, boost, 1);
}

static void BM_PowerHalControllerBenchmarks_setMode(benchmark::State& state) {
    Mode mode = static_cast<Mode>(state.range(0));
    runBenchmark(state, &PowerHalController::setMode, mode, false);
}

static void BM_PowerHalControllerBenchmarks_setModeCached(benchmark::State& state) {
    Mode mode = static_cast<Mode>(state.range(0));
    runCachedBenchmark(state, &PowerHalController::setMode, mode, false);
}

static void BM_PowerHalControllerBenchmarks_createHintSession(benchmark::State& state) {
    std::vector<int32_t> threadIds{static_cast<int32_t>(state.range(0))};
    int64_t durationNanos = 16666666L;
    int32_t tgid = 999;
    int32_t uid = 1001;
    PowerHalController controller;

    HalResult<std::shared_ptr<PowerHintSessionWrapper>> result =
        controller.createHintSession(tgid, uid, threadIds, durationNanos);
    if (result.isFailed()) {
        state.SkipWithError(result.errorMessage());
        return;
    } else if (result.isUnsupported()) {
        ALOGV("Power HAL doesn't support session, skipping test...");
        state.SkipWithMessage("operation unsupported");
        return;
    } else {
        result.value()->close();
    }

    for (auto _ : state) {
        result = controller.createHintSession(tgid, uid, threadIds, durationNanos);
        if (!result.isOk()) {
            state.SkipWithError(result.errorMessage());
            break;
        }
        state.PauseTiming();
        result.value()->close();
        state.ResumeTiming();
    }
}

static void BM_PowerHalControllerBenchmarks_updateTargetWorkDuration(benchmark::State& state) {
    int64_t duration = 1000;
    runSessionBenchmark(state, &PowerHintSessionWrapper::updateTargetWorkDuration, duration);
}

static void BM_PowerHalControllerBenchmarks_reportActualWorkDuration(benchmark::State& state) {
    runSessionBenchmark(state, &PowerHintSessionWrapper::reportActualWorkDuration, DURATIONS);
}

BENCHMARK(BM_PowerHalControllerBenchmarks_init);
BENCHMARK(BM_PowerHalControllerBenchmarks_initCached);
BENCHMARK(BM_PowerHalControllerBenchmarks_setBoost)->DenseRange(FIRST_BOOST, LAST_BOOST, 1);
BENCHMARK(BM_PowerHalControllerBenchmarks_setBoostCached)->DenseRange(FIRST_BOOST, LAST_BOOST, 1);
BENCHMARK(BM_PowerHalControllerBenchmarks_setMode)->DenseRange(FIRST_MODE, LAST_MODE, 1);
BENCHMARK(BM_PowerHalControllerBenchmarks_setModeCached)->DenseRange(FIRST_MODE, LAST_MODE, 1);
BENCHMARK(BM_PowerHalControllerBenchmarks_createHintSession)->Arg(1);
BENCHMARK(BM_PowerHalControllerBenchmarks_updateTargetWorkDuration);
BENCHMARK(BM_PowerHalControllerBenchmarks_reportActualWorkDuration);
