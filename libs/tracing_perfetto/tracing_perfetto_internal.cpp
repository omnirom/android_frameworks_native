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

#define FRAMEWORK_CATEGORIES(C)                                  \
  C(always, "always", "Always category")                         \
  C(graphics, "graphics", "Graphics category")                   \
  C(input, "input", "Input category")                            \
  C(view, "view", "View category")                               \
  C(webview, "webview", "WebView category")                      \
  C(windowmanager, "wm", "WindowManager category")               \
  C(activitymanager, "am", "ActivityManager category")           \
  C(syncmanager, "syncmanager", "SyncManager category")          \
  C(audio, "audio", "Audio category")                            \
  C(video, "video", "Video category")                            \
  C(camera, "camera", "Camera category")                         \
  C(hal, "hal", "HAL category")                                  \
  C(app, "app", "App category")                                  \
  C(resources, "res", "Resources category")                      \
  C(dalvik, "dalvik", "Dalvik category")                         \
  C(rs, "rs", "RS category")                                     \
  C(bionic, "bionic", "Bionic category")                         \
  C(power, "power", "Power category")                            \
  C(packagemanager, "packagemanager", "PackageManager category") \
  C(systemserver, "ss", "System Server category")                \
  C(database, "database", "Database category")                   \
  C(network, "network", "Network category")                      \
  C(adb, "adb", "ADB category")                                  \
  C(vibrator, "vibrator", "Vibrator category")                   \
  C(aidl, "aidl", "AIDL category")                               \
  C(nnapi, "nnapi", "NNAPI category")                            \
  C(rro, "rro", "RRO category")                                  \
  C(thermal, "thermal", "Thermal category")

#include <atomic>
#include <mutex>

#include <android_os.h>
#include <android-base/properties.h>
#include <cutils/trace.h>
#include <inttypes.h>

#include "perfetto/public/compiler.h"
#include "perfetto/public/producer.h"
#include "perfetto/public/te_category_macros.h"
#include "perfetto/public/te_macros.h"
#include "perfetto/public/track_event.h"
#include "trace_categories.h"
#include "tracing_perfetto_internal.h"

#ifdef __BIONIC__
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

namespace tracing_perfetto {

namespace internal {

namespace {
PERFETTO_TE_CATEGORIES_DECLARE(FRAMEWORK_CATEGORIES);

PERFETTO_TE_CATEGORIES_DEFINE(FRAMEWORK_CATEGORIES);

static constexpr char kPreferFlagProperty[] = "debug.atrace.prefer_sdk";
static std::atomic<const prop_info*> prefer_property_info = nullptr;
static std::atomic_uint32_t last_prefer_seq_num = 0;
static std::atomic_uint64_t prefer_flags = 0;

static const prop_info* system_property_find(const char* name [[maybe_unused]]) {
  #ifdef __BIONIC__
  return __system_property_find(name);
  #endif

  return nullptr;
}

static uint32_t system_property_serial(const prop_info* pi [[maybe_unused]]) {
  #ifdef __BIONIC__
  return __system_property_serial(pi);
  #endif

  return last_prefer_seq_num;
}

struct PerfettoTeCategory* toCategory(uint64_t inCategory) {
  switch (inCategory) {
    case TRACE_CATEGORY_ALWAYS:
      return &always;
    case TRACE_CATEGORY_GRAPHICS:
      return &graphics;
    case TRACE_CATEGORY_INPUT:
      return &input;
    case TRACE_CATEGORY_VIEW:
      return &view;
    case TRACE_CATEGORY_WEBVIEW:
      return &webview;
    case TRACE_CATEGORY_WINDOW_MANAGER:
      return &windowmanager;
    case TRACE_CATEGORY_ACTIVITY_MANAGER:
      return &activitymanager;
    case TRACE_CATEGORY_SYNC_MANAGER:
      return &syncmanager;
    case TRACE_CATEGORY_AUDIO:
      return &audio;
    case TRACE_CATEGORY_VIDEO:
      return &video;
    case TRACE_CATEGORY_CAMERA:
      return &camera;
    case TRACE_CATEGORY_HAL:
      return &hal;
    case TRACE_CATEGORY_APP:
      return &app;
    case TRACE_CATEGORY_RESOURCES:
      return &resources;
    case TRACE_CATEGORY_DALVIK:
      return &dalvik;
    case TRACE_CATEGORY_RS:
      return &rs;
    case TRACE_CATEGORY_BIONIC:
      return &bionic;
    case TRACE_CATEGORY_POWER:
      return &power;
    case TRACE_CATEGORY_PACKAGE_MANAGER:
      return &packagemanager;
    case TRACE_CATEGORY_SYSTEM_SERVER:
      return &systemserver;
    case TRACE_CATEGORY_DATABASE:
      return &database;
    case TRACE_CATEGORY_NETWORK:
      return &network;
    case TRACE_CATEGORY_ADB:
      return &adb;
    case TRACE_CATEGORY_VIBRATOR:
      return &vibrator;
    case TRACE_CATEGORY_AIDL:
      return &aidl;
    case TRACE_CATEGORY_NNAPI:
      return &nnapi;
    case TRACE_CATEGORY_RRO:
      return &rro;
    case TRACE_CATEGORY_THERMAL:
      return &thermal;
    default:
      return nullptr;
  }
}

}  // namespace

bool isPerfettoCategoryEnabled(PerfettoTeCategory* category) {
  return category != nullptr;
}

/**
 * Updates the cached |prefer_flags|.
 *
 * We cache the prefer_flags because reading it on every trace event is expensive.
 * The cache is invalidated when a sys_prop sequence number changes.
 */
void updatePreferFlags() {
  if (!prefer_property_info.load(std::memory_order_acquire)) {
    auto* new_prefer_property_info = system_property_find(kPreferFlagProperty);
    prefer_flags.store(android::base::GetIntProperty(kPreferFlagProperty, 0),
                       std::memory_order_relaxed);

    if (!new_prefer_property_info) {
      // This should never happen. If it does, we fail gracefully and end up reading the property
      // traced event.
      return;
    }

    last_prefer_seq_num = system_property_serial(new_prefer_property_info);
    prefer_property_info.store(new_prefer_property_info, std::memory_order_release);
  }

  uint32_t prefer_seq_num =  system_property_serial(prefer_property_info);
  if (prefer_seq_num != last_prefer_seq_num.load(std::memory_order_acquire)) {
    prefer_flags.store(android::base::GetIntProperty(kPreferFlagProperty, 0),
                       std::memory_order_relaxed);
    last_prefer_seq_num.store(prefer_seq_num, std::memory_order_release);
  }
}

bool shouldPreferAtrace(PerfettoTeCategory *perfettoCategory, uint64_t atraceCategory) {
  // There are 3 cases:
  // 1. Atrace is not enabled.
  if (!atrace_is_tag_enabled(atraceCategory)) {
    return false;
  }

  // 2. Atrace is enabled but perfetto is not enabled.
  if (!isPerfettoCategoryEnabled(perfettoCategory)) {
    return true;
  }

  // Update prefer_flags before checking it below
  updatePreferFlags();

  // 3. Atrace and perfetto are enabled.
  // Even though this category is enabled for track events, the config mandates that we downgrade
  // it to atrace if the same atrace category is currently enabled. This prevents missing the
  // event from a concurrent session that needs the same category in atrace.
  return (atraceCategory & prefer_flags.load(std::memory_order_relaxed)) == 0;
}

struct PerfettoTeCategory* toPerfettoCategory(uint64_t category) {
  struct PerfettoTeCategory* perfettoCategory = toCategory(category);
  if (perfettoCategory == nullptr) {
    return nullptr;
  }

  bool enabled = PERFETTO_UNLIKELY(PERFETTO_ATOMIC_LOAD_EXPLICIT(
       (*perfettoCategory).enabled, PERFETTO_MEMORY_ORDER_RELAXED));
  return enabled ? perfettoCategory : nullptr;
}

void registerWithPerfetto(bool test) {
  if (!android::os::perfetto_sdk_tracing()) {
    return;
  }

  static std::once_flag registration;
  std::call_once(registration, [test]() {
    struct PerfettoProducerInitArgs args = PERFETTO_PRODUCER_INIT_ARGS_INIT();
    args.backends = test ? PERFETTO_BACKEND_IN_PROCESS : PERFETTO_BACKEND_SYSTEM;
    PerfettoProducerInit(args);
    PerfettoTeInit();
    PERFETTO_TE_REGISTER_CATEGORIES(FRAMEWORK_CATEGORIES);
  });
}

void perfettoTraceBegin(const struct PerfettoTeCategory& category, const char* name) {
  PERFETTO_TE(category, PERFETTO_TE_SLICE_BEGIN(name));
}

void perfettoTraceEnd(const struct PerfettoTeCategory& category) {
  PERFETTO_TE(category, PERFETTO_TE_SLICE_END());
}

void perfettoTraceAsyncBeginForTrack(const struct PerfettoTeCategory& category, const char* name,
                                       const char* trackName, uint64_t cookie) {
  PERFETTO_TE(
      category, PERFETTO_TE_SLICE_BEGIN(name),
      PERFETTO_TE_NAMED_TRACK(trackName, cookie, PerfettoTeProcessTrackUuid()));
}

void perfettoTraceAsyncEndForTrack(const struct PerfettoTeCategory& category,
                                     const char* trackName, uint64_t cookie) {
  PERFETTO_TE(
      category, PERFETTO_TE_SLICE_END(),
      PERFETTO_TE_NAMED_TRACK(trackName, cookie, PerfettoTeProcessTrackUuid()));
}

void perfettoTraceAsyncBegin(const struct PerfettoTeCategory& category, const char* name,
                               uint64_t cookie) {
  perfettoTraceAsyncBeginForTrack(category, name, name, cookie);
}

void perfettoTraceAsyncEnd(const struct PerfettoTeCategory& category, const char* name,
                             uint64_t cookie) {
  perfettoTraceAsyncEndForTrack(category, name, cookie);
}

void perfettoTraceInstant(const struct PerfettoTeCategory& category, const char* name) {
  PERFETTO_TE(category, PERFETTO_TE_INSTANT(name));
}

void perfettoTraceInstantForTrack(const struct PerfettoTeCategory& category,
                                    const char* trackName, const char* name) {
  PERFETTO_TE(
      category, PERFETTO_TE_INSTANT(name),
      PERFETTO_TE_NAMED_TRACK(trackName, 1, PerfettoTeProcessTrackUuid()));
}

void perfettoTraceCounter(const struct PerfettoTeCategory& category,
                            [[maybe_unused]] const char* name, int64_t value) {
  PERFETTO_TE(category, PERFETTO_TE_COUNTER(),
              PERFETTO_TE_INT_COUNTER(value));
}
}  // namespace internal

}  // namespace tracing_perfetto
