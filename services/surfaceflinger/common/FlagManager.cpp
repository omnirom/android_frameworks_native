/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <common/FlagManager.h>

#include <SurfaceFlingerProperties.sysprop.h>
#include <android-base/parsebool.h>
#include <android-base/parseint.h>
#include <android-base/properties.h>
#include <android-base/stringprintf.h>
#include <log/log.h>
#include <renderengine/RenderEngine.h>
#include <server_configurable_flags/get_flags.h>
#include <cinttypes>

#include <android_companion_virtualdevice_flags.h>
#include <android_hardware_flags.h>
#include <android_os.h>
#include <com_android_graphics_libgui_flags.h>
#include <com_android_graphics_surfaceflinger_flags.h>
#include <com_android_server_display_feature_flags.h>

namespace android {
using namespace com::android::graphics::surfaceflinger;

static constexpr const char* kExperimentNamespace = "surface_flinger_native_boot";

std::unique_ptr<FlagManager> FlagManager::mInstance;
std::once_flag FlagManager::mOnce;

FlagManager::FlagManager(ConstructorTag) {}
FlagManager::~FlagManager() = default;

namespace {
std::optional<bool> parseBool(const char* str) {
    base::ParseBoolResult parseResult = base::ParseBool(str);
    switch (parseResult) {
        case base::ParseBoolResult::kTrue:
            return std::make_optional(true);
        case base::ParseBoolResult::kFalse:
            return std::make_optional(false);
        case base::ParseBoolResult::kError:
            return std::nullopt;
    }
}

bool getFlagValue(std::function<bool()> getter, std::optional<bool> overrideValue) {
    if (overrideValue.has_value()) {
        return *overrideValue;
    }

    return getter();
}

} // namespace

const FlagManager& FlagManager::getInstance() {
    return getMutableInstance();
}

FlagManager& FlagManager::getMutableInstance() {
    std::call_once(mOnce, [&] {
        LOG_ALWAYS_FATAL_IF(mInstance, "Instance already created");
        mInstance = std::make_unique<FlagManager>(ConstructorTag{});
    });

    return *mInstance;
}

void FlagManager::markBootCompleted() {
    mBootCompleted = true;
}

void FlagManager::setUnitTestMode() {
    mUnitTestMode = true;

    // Also set boot completed as we don't really care about it in unit testing
    mBootCompleted = true;
}

void FlagManager::dumpFlag(std::string& result, bool aconfig, const char* name,
                           std::function<bool()> getter) const {
    if (aconfig || mBootCompleted) {
        base::StringAppendF(&result, "%s: %s\n", name, getter() ? "true" : "false");
    } else {
        base::StringAppendF(&result, "%s: in progress (still booting)\n", name);
    }
}

void FlagManager::dump(std::string& result) const {
#define DUMP_FLAG_INTERNAL(name, aconfig) \
    dumpFlag(result, (aconfig), #name, std::bind(&FlagManager::name, this))
#define DUMP_LEGACY_SERVER_FLAG(name) DUMP_FLAG_INTERNAL(name, false)
#define DUMP_ACONFIG_FLAG(name) DUMP_FLAG_INTERNAL(name, true)
#define DUMP_SYSPROP_FLAG(name) \
    dumpFlag(result, (true), "debug.sf." #name, std::bind(&FlagManager::name, this))

    base::StringAppendF(&result, "FlagManager values: \n");

    /// Sysprop flags ///
    DUMP_SYSPROP_FLAG(disable_sched_fifo_sf);
    DUMP_SYSPROP_FLAG(disable_sched_fifo_sf_binder);
    DUMP_SYSPROP_FLAG(disable_sched_fifo_sf_sched);
    DUMP_SYSPROP_FLAG(disable_sched_fifo_re);
    DUMP_SYSPROP_FLAG(disable_sched_fifo_composer);
    DUMP_SYSPROP_FLAG(disable_sched_fifo_composer_callback);

    /// Legacy server flags ///
    DUMP_LEGACY_SERVER_FLAG(use_adpf_cpu_hint);
    DUMP_LEGACY_SERVER_FLAG(use_skia_tracing);

    /// Trunk stable server (R/W) flags ///
    DUMP_ACONFIG_FLAG(adpf_gpu_sf);
    DUMP_ACONFIG_FLAG(adpf_native_session_manager);
    DUMP_ACONFIG_FLAG(adpf_use_fmq_channel);
    DUMP_ACONFIG_FLAG(correct_virtual_display_power_state);
    DUMP_ACONFIG_FLAG(graphite_renderengine_preview_rollout);
    DUMP_ACONFIG_FLAG(increase_missed_frame_jank_threshold);
    DUMP_ACONFIG_FLAG(monitor_buffer_fences);
    DUMP_ACONFIG_FLAG(refresh_rate_overlay_on_external_display);
    DUMP_ACONFIG_FLAG(vsync_predictor_recovery);

    /// Trunk stable readonly flags ///
    /// IMPORTANT - please keep alphabetize to reduce merge conflicts
    DUMP_ACONFIG_FLAG(add_sf_skipped_frames_to_trace);
    DUMP_ACONFIG_FLAG(adpf_fmq_sf);
    DUMP_ACONFIG_FLAG(allow_n_vsyncs_in_targeter);
    DUMP_ACONFIG_FLAG(arr_setframerate_gte_enum);
    DUMP_ACONFIG_FLAG(begone_bright_hlg);
    DUMP_ACONFIG_FLAG(cache_when_source_crop_layer_only_moved);
    DUMP_ACONFIG_FLAG(commit_not_composited);
    DUMP_ACONFIG_FLAG(connected_display_hdr);
    DUMP_ACONFIG_FLAG(correct_dpi_with_display_size);
    DUMP_ACONFIG_FLAG(deprecate_frame_tracker);
    DUMP_ACONFIG_FLAG(deprecate_vsync_sf);
    DUMP_ACONFIG_FLAG(detached_mirror);
    DUMP_ACONFIG_FLAG(display_config_error_hal);
    DUMP_ACONFIG_FLAG(display_protected);
    DUMP_ACONFIG_FLAG(dont_skip_on_early_ro);
    DUMP_ACONFIG_FLAG(enable_fro_dependent_features);
    DUMP_ACONFIG_FLAG(enable_layer_command_batching);
    DUMP_ACONFIG_FLAG(enable_small_area_detection);
    DUMP_ACONFIG_FLAG(filter_frames_before_trace_starts);
    DUMP_ACONFIG_FLAG(flush_buffer_slots_to_uncache);
    DUMP_ACONFIG_FLAG(force_compile_graphite_renderengine);
    DUMP_ACONFIG_FLAG(fp16_client_target);
    DUMP_ACONFIG_FLAG(frame_rate_category_mrr);
    DUMP_ACONFIG_FLAG(game_default_frame_rate);
    DUMP_ACONFIG_FLAG(graphite_renderengine);
    DUMP_ACONFIG_FLAG(hdcp_level_hal);
    DUMP_ACONFIG_FLAG(hdcp_negotiation);
    DUMP_ACONFIG_FLAG(idle_screen_refresh_rate_timeout);
    DUMP_ACONFIG_FLAG(latch_unsignaled_with_auto_refresh_changed);
    DUMP_ACONFIG_FLAG(local_tonemap_screenshots);
    DUMP_ACONFIG_FLAG(misc1);
    DUMP_ACONFIG_FLAG(no_vsyncs_on_screen_off);
    DUMP_ACONFIG_FLAG(override_trusted_overlay);
    DUMP_ACONFIG_FLAG(protected_if_client);
    DUMP_ACONFIG_FLAG(reject_dupe_layerstacks);
    DUMP_ACONFIG_FLAG(renderable_buffer_usage);
    DUMP_ACONFIG_FLAG(restore_blur_step);
    DUMP_ACONFIG_FLAG(skip_invisible_windows_in_input);
    DUMP_ACONFIG_FLAG(stable_edid_ids);
    DUMP_ACONFIG_FLAG(synced_resolution_switch);
    DUMP_ACONFIG_FLAG(trace_frame_rate_override);
    DUMP_ACONFIG_FLAG(true_hdr_screenshots);
    DUMP_ACONFIG_FLAG(use_known_refresh_rate_for_fps_consistency);
    DUMP_ACONFIG_FLAG(vrr_bugfix_24q4);
    DUMP_ACONFIG_FLAG(vrr_bugfix_dropped_frame);
    DUMP_ACONFIG_FLAG(vrr_config);
    DUMP_ACONFIG_FLAG(vulkan_renderengine);
    DUMP_ACONFIG_FLAG(window_blur_kawase2);
    /// IMPORTANT - please keep alphabetize to reduce merge conflicts

#undef DUMP_ACONFIG_FLAG
#undef DUMP_LEGACY_SERVER_FLAG
#undef DUMP_FLAG_INTERVAL
}

std::optional<bool> FlagManager::getBoolProperty(const char* property) const {
    return parseBool(base::GetProperty(property, "").c_str());
}

bool FlagManager::getServerConfigurableFlag(const char* experimentFlagName) const {
    const auto value = server_configurable_flags::GetServerConfigurableFlag(kExperimentNamespace,
                                                                            experimentFlagName, "");
    const auto res = parseBool(value.c_str());
    return res.has_value() && res.value();
}
#define FLAG_MANAGER_SYSPROP_FLAG(name, defaultVal)                                      \
    bool FlagManager::name() const {                                                     \
        static const bool kFlagValue =                                                   \
                base::GetBoolProperty("debug.sf." #name, /* default value*/ defaultVal); \
        return kFlagValue;                                                               \
    }

#define FLAG_MANAGER_LEGACY_SERVER_FLAG(name, syspropOverride, serverFlagName)              \
    bool FlagManager::name() const {                                                        \
        LOG_ALWAYS_FATAL_IF(!mBootCompleted,                                                \
                            "Can't read %s before boot completed as it is server writable", \
                            __func__);                                                      \
        const auto debugOverride = getBoolProperty(syspropOverride);                        \
        if (debugOverride.has_value()) return debugOverride.value();                        \
        return getServerConfigurableFlag(serverFlagName);                                   \
    }

#define FLAG_MANAGER_ACONFIG_INTERNAL(name, syspropOverride, owner)                            \
    bool FlagManager::name() const {                                                           \
        static const std::optional<bool> debugOverride = getBoolProperty(syspropOverride);     \
        static const bool value = getFlagValue([] { return owner ::name(); }, debugOverride);  \
        if (mUnitTestMode) {                                                                   \
            /*                                                                                 \
             * When testing, we don't want to rely on the cached `value` or the debugOverride. \
             */                                                                                \
            return owner ::name();                                                             \
        }                                                                                      \
        return value;                                                                          \
    }

#define FLAG_MANAGER_ACONFIG_FLAG(name, syspropOverride) \
    FLAG_MANAGER_ACONFIG_INTERNAL(name, syspropOverride, flags)

#define FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(name, syspropOverride, owner) \
    FLAG_MANAGER_ACONFIG_INTERNAL(name, syspropOverride, owner)

/// Debug sysprop flags - default value is always false ///
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_sf, /* default */ false)
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_sf_binder, /* default */ false)
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_sf_sched, /* default */ false)
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_re, /* default */ false)
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_composer, /* default */ false)
FLAG_MANAGER_SYSPROP_FLAG(disable_sched_fifo_composer_callback, /* default */ false)

/// Legacy server flags ///
FLAG_MANAGER_LEGACY_SERVER_FLAG(test_flag, "", "")
FLAG_MANAGER_LEGACY_SERVER_FLAG(use_adpf_cpu_hint, "debug.sf.enable_adpf_cpu_hint",
                                "AdpfFeature__adpf_cpu_hint")
FLAG_MANAGER_LEGACY_SERVER_FLAG(use_skia_tracing, PROPERTY_SKIA_ATRACE_ENABLED,
                                "SkiaTracingFeature__use_skia_tracing")

/// Trunk stable readonly flags ///
FLAG_MANAGER_ACONFIG_FLAG(adpf_fmq_sf, "")
FLAG_MANAGER_ACONFIG_FLAG(arr_setframerate_gte_enum, "debug.sf.arr_setframerate_gte_enum")
FLAG_MANAGER_ACONFIG_FLAG(enable_small_area_detection, "")
FLAG_MANAGER_ACONFIG_FLAG(stable_edid_ids, "debug.sf.stable_edid_ids")
FLAG_MANAGER_ACONFIG_FLAG(frame_rate_category_mrr, "debug.sf.frame_rate_category_mrr")
FLAG_MANAGER_ACONFIG_FLAG(misc1, "")
FLAG_MANAGER_ACONFIG_FLAG(vrr_config, "debug.sf.enable_vrr_config")
FLAG_MANAGER_ACONFIG_FLAG(hdcp_level_hal, "")
FLAG_MANAGER_ACONFIG_FLAG(hdcp_negotiation, "debug.sf.hdcp_negotiation");
FLAG_MANAGER_ACONFIG_FLAG(add_sf_skipped_frames_to_trace, "")
FLAG_MANAGER_ACONFIG_FLAG(use_known_refresh_rate_for_fps_consistency, "")
FLAG_MANAGER_ACONFIG_FLAG(cache_when_source_crop_layer_only_moved,
                          "debug.sf.cache_source_crop_only_moved")
FLAG_MANAGER_ACONFIG_FLAG(enable_fro_dependent_features, "")
FLAG_MANAGER_ACONFIG_FLAG(display_protected, "")
FLAG_MANAGER_ACONFIG_FLAG(fp16_client_target, "debug.sf.fp16_client_target")
FLAG_MANAGER_ACONFIG_FLAG(game_default_frame_rate, "")
FLAG_MANAGER_ACONFIG_FLAG(enable_layer_command_batching, "debug.sf.enable_layer_command_batching")
FLAG_MANAGER_ACONFIG_FLAG(vulkan_renderengine, "debug.renderengine.vulkan")
FLAG_MANAGER_ACONFIG_FLAG(renderable_buffer_usage, "")
FLAG_MANAGER_ACONFIG_FLAG(restore_blur_step, "debug.renderengine.restore_blur_step")
FLAG_MANAGER_ACONFIG_FLAG(dont_skip_on_early_ro, "")
FLAG_MANAGER_ACONFIG_FLAG(no_vsyncs_on_screen_off, "debug.sf.no_vsyncs_on_screen_off")
FLAG_MANAGER_ACONFIG_FLAG(protected_if_client, "")
FLAG_MANAGER_ACONFIG_FLAG(vrr_bugfix_24q4, "");
FLAG_MANAGER_ACONFIG_FLAG(vrr_bugfix_dropped_frame, "")
FLAG_MANAGER_ACONFIG_FLAG(graphite_renderengine, "debug.renderengine.graphite")
FLAG_MANAGER_ACONFIG_FLAG(filter_frames_before_trace_starts, "")
FLAG_MANAGER_ACONFIG_FLAG(latch_unsignaled_with_auto_refresh_changed, "");
FLAG_MANAGER_ACONFIG_FLAG(deprecate_vsync_sf, "");
FLAG_MANAGER_ACONFIG_FLAG(allow_n_vsyncs_in_targeter, "");
FLAG_MANAGER_ACONFIG_FLAG(detached_mirror, "");
FLAG_MANAGER_ACONFIG_FLAG(commit_not_composited, "");
FLAG_MANAGER_ACONFIG_FLAG(correct_dpi_with_display_size, "");
FLAG_MANAGER_ACONFIG_FLAG(local_tonemap_screenshots, "debug.sf.local_tonemap_screenshots");
FLAG_MANAGER_ACONFIG_FLAG(override_trusted_overlay, "");
FLAG_MANAGER_ACONFIG_FLAG(flush_buffer_slots_to_uncache, "");
FLAG_MANAGER_ACONFIG_FLAG(force_compile_graphite_renderengine, "");
FLAG_MANAGER_ACONFIG_FLAG(true_hdr_screenshots, "debug.sf.true_hdr_screenshots");
FLAG_MANAGER_ACONFIG_FLAG(display_config_error_hal, "");
FLAG_MANAGER_ACONFIG_FLAG(connected_display_hdr, "debug.sf.connected_display_hdr");
FLAG_MANAGER_ACONFIG_FLAG(deprecate_frame_tracker, "");
FLAG_MANAGER_ACONFIG_FLAG(skip_invisible_windows_in_input, "");
FLAG_MANAGER_ACONFIG_FLAG(begone_bright_hlg, "debug.sf.begone_bright_hlg");
FLAG_MANAGER_ACONFIG_FLAG(window_blur_kawase2, "");
FLAG_MANAGER_ACONFIG_FLAG(reject_dupe_layerstacks, "");
FLAG_MANAGER_ACONFIG_FLAG(synced_resolution_switch, "");

/// Trunk stable server (R/W) flags ///
FLAG_MANAGER_ACONFIG_FLAG(refresh_rate_overlay_on_external_display, "")
FLAG_MANAGER_ACONFIG_FLAG(adpf_gpu_sf, "")
FLAG_MANAGER_ACONFIG_FLAG(adpf_native_session_manager, "");
FLAG_MANAGER_ACONFIG_FLAG(graphite_renderengine_preview_rollout, "");
FLAG_MANAGER_ACONFIG_FLAG(increase_missed_frame_jank_threshold, "");
FLAG_MANAGER_ACONFIG_FLAG(monitor_buffer_fences, "");
FLAG_MANAGER_ACONFIG_FLAG(vsync_predictor_recovery, "");

/// Trunk stable server (R/W) flags from outside SurfaceFlinger ///
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(adpf_use_fmq_channel, "", android::os)
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(correct_virtual_display_power_state, "",
                                   android::companion::virtualdevice::flags)

/// Trunk stable readonly flags from outside SurfaceFlinger ///
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(idle_screen_refresh_rate_timeout, "",
                                   com::android::server::display::feature::flags)
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(adpf_use_fmq_channel_fixed, "", android::os)
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(trace_frame_rate_override, "",
                                   com::android::graphics::libgui::flags);
FLAG_MANAGER_ACONFIG_FLAG_IMPORTED(luts_api, "",
                                   android::hardware::flags);
} // namespace android
