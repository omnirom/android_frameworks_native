/*
 * Copyright 2019 The Android Open Source Project
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

#include "PhaseOffsets.h"

#include <cutils/properties.h>

#include "SurfaceFlingerProperties.h"

namespace android {
using namespace android::sysprop;

namespace scheduler {

using RefreshRateType = RefreshRateConfigs::RefreshRateType;
PhaseOffsets::~PhaseOffsets() = default;

namespace impl {
PhaseOffsets::PhaseOffsets() {
    int64_t vsyncPhaseOffsetNs = vsync_event_phase_offset_ns(1000000);

    int64_t sfVsyncPhaseOffsetNs = vsync_sf_event_phase_offset_ns(1000000);

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.sf.early_phase_offset_ns", value, "-1");
    const int earlySfOffsetNs = atoi(value);

    property_get("debug.sf.early_gl_phase_offset_ns", value, "-1");
    const int earlyGlSfOffsetNs = atoi(value);

    property_get("debug.sf.early_app_phase_offset_ns", value, "-1");
    const int earlyAppOffsetNs = atoi(value);

    property_get("debug.sf.early_gl_app_phase_offset_ns", value, "-1");
    const int earlyGlAppOffsetNs = atoi(value);

    // Phase Offsets for HIGH1 or HIGH2 Refresh Rate type.
    property_get("debug.sf.high_fps_early_phase_offset_ns", value, "-1");
    const int highFpsEarlySfOffsetNs = atoi(value);

    property_get("debug.sf.high_fps_early_gl_phase_offset_ns", value, "-1");
    const int highFpsEarlyGlSfOffsetNs = atoi(value);

    property_get("debug.sf.high_fps_early_app_phase_offset_ns", value, "-1");
    const int highFpsEarlyAppOffsetNs = atoi(value);

    property_get("debug.sf.high_fps_early_gl_app_phase_offset_ns", value, "-1");
    const int highFpsEarlyGlAppOffsetNs = atoi(value);

    // TODO(b/122905996): Define these in device.mk.
    property_get("debug.sf.high_fps_late_app_phase_offset_ns", value, "2000000");
    const int highFpsLateAppOffsetNs = atoi(value);

    property_get("debug.sf.high_fps_late_sf_phase_offset_ns", value, "1000000");
    const int highFpsLateSfOffsetNs = atoi(value);

    // Phase Offsets for PERFORMANCE Refresh Rate type.
    property_get("debug.sf.perf_fps_early_phase_offset_ns", value, "-1");
    const int perfFpsEarlySfOffsetNs = atoi(value);

    property_get("debug.sf.perf_fps_early_gl_phase_offset_ns", value, "-1");
    const int perfFpsEarlyGlSfOffsetNs = atoi(value);

    property_get("debug.sf.perf_fps_late_sf_phase_offset_ns", value, "-1");
    const int perfFpsLateSfOffsetNs = atoi(value);

    property_get("debug.sf.perf_fps_early_app_phase_offset_ns", value, "-1");
    const int perfFpsEarlyAppOffsetNs = atoi(value);

    property_get("debug.sf.perf_fps_early_gl_app_phase_offset_ns", value, "-1");
    const int perfFpsEarlyGlAppOffsetNs = atoi(value);

    property_get("debug.sf.perf_fps_late_app_phase_offset_ns", value, "-1");
    const int perfFpsLateAppOffsetNs = atoi(value);

    // Below defines the threshold when an offset is considered to be negative, i.e. targeting
    // for the N+2 vsync instead of N+1. This means that:
    // For offset < threshold, SF wake up (vsync_duration - offset) before HW vsync.
    // For offset >= threshold, SF wake up (2 * vsync_duration - offset) before HW vsync.
    property_get("debug.sf.phase_offset_threshold_for_next_vsync_ns", value, "-1");
    const int phaseOffsetThresholdForNextVsyncNs = atoi(value);

    Offsets defaultOffsets;
    Offsets highFpsOffsets;
    Offsets perfFpsOffsets;

    defaultOffsets.early = {RefreshRateType::DEFAULT,
                            earlySfOffsetNs != -1 ? earlySfOffsetNs : sfVsyncPhaseOffsetNs,
                            earlyAppOffsetNs != -1 ? earlyAppOffsetNs : vsyncPhaseOffsetNs};
    defaultOffsets.earlyGl = {RefreshRateType::DEFAULT,
                              earlyGlSfOffsetNs != -1 ? earlyGlSfOffsetNs : sfVsyncPhaseOffsetNs,
                              earlyGlAppOffsetNs != -1 ? earlyGlAppOffsetNs : vsyncPhaseOffsetNs};
    defaultOffsets.late = {RefreshRateType::DEFAULT, sfVsyncPhaseOffsetNs, vsyncPhaseOffsetNs};

    highFpsOffsets.early = {RefreshRateType::HIGH1,
                            highFpsEarlySfOffsetNs != -1 ? highFpsEarlySfOffsetNs
                                                         : highFpsLateSfOffsetNs,
                            highFpsEarlyAppOffsetNs != -1 ? highFpsEarlyAppOffsetNs
                                                          : highFpsLateAppOffsetNs};
    highFpsOffsets.earlyGl = {RefreshRateType::HIGH1,
                              highFpsEarlyGlSfOffsetNs != -1 ? highFpsEarlyGlSfOffsetNs
                                                             : highFpsLateSfOffsetNs,
                              highFpsEarlyGlAppOffsetNs != -1 ? highFpsEarlyGlAppOffsetNs
                                                              : highFpsLateAppOffsetNs};
    highFpsOffsets.late = {RefreshRateType::HIGH1, highFpsLateSfOffsetNs,
                           highFpsLateAppOffsetNs};

    // If a perf_fps property is not configured, it defaults to corresponding high_fps prop value.
    perfFpsOffsets.early = {RefreshRateType::PERFORMANCE,
                            perfFpsEarlySfOffsetNs != -1 ? perfFpsEarlySfOffsetNs
                                                         : highFpsOffsets.early.sf,
                            perfFpsEarlyAppOffsetNs != -1 ? perfFpsEarlyAppOffsetNs
                                                          : highFpsOffsets.early.app};
    perfFpsOffsets.earlyGl = {RefreshRateType::PERFORMANCE,
                              perfFpsEarlyGlSfOffsetNs != -1 ? perfFpsEarlyGlSfOffsetNs
                                                             : highFpsOffsets.earlyGl.sf,
                              perfFpsEarlyGlAppOffsetNs != -1 ? perfFpsEarlyGlAppOffsetNs
                                                              : highFpsOffsets.earlyGl.app};
    perfFpsOffsets.late = {RefreshRateType::PERFORMANCE,
                           perfFpsLateSfOffsetNs != -1 ? perfFpsLateSfOffsetNs
                                                       : highFpsOffsets.late.sf,
                           perfFpsLateAppOffsetNs != -1 ? perfFpsLateAppOffsetNs
                                                        : highFpsOffsets.late.app};

    mOffsets.insert({RefreshRateType::POWER_SAVING, defaultOffsets});
    mOffsets.insert({RefreshRateType::LOW0, defaultOffsets});
    mOffsets.insert({RefreshRateType::LOW1, defaultOffsets});
    mOffsets.insert({RefreshRateType::LOW2, defaultOffsets});
    mOffsets.insert({RefreshRateType::DEFAULT, defaultOffsets});
    mOffsets.insert({RefreshRateType::PERFORMANCE, perfFpsOffsets});
    mOffsets.insert({RefreshRateType::HIGH1, highFpsOffsets});
    mOffsets.insert({RefreshRateType::HIGH2, highFpsOffsets});

    mOffsetThresholdForNextVsync = phaseOffsetThresholdForNextVsyncNs != -1
            ? phaseOffsetThresholdForNextVsyncNs
            : std::numeric_limits<nsecs_t>::max();
}

PhaseOffsets::Offsets PhaseOffsets::getOffsetsForRefreshRate(
        android::scheduler::RefreshRateConfigs::RefreshRateType refreshRateType) const {
    bool isDefault = (refreshRateType == RefreshRateConfigs::RefreshRateType::DEFAULT);
    return isDefault ? mOffsets.at(mDefaultPhaseOffsetType) : mOffsets.at(refreshRateType);
}

void PhaseOffsets::dump(std::string& result) const {
    const auto [early, earlyGl, late] = getCurrentOffsets();
    base::StringAppendF(&result,
                        "         app phase: %9" PRId64 " ns\t         SF phase: %9" PRId64 " ns\n"
                        "   early app phase: %9" PRId64 " ns\t   early SF phase: %9" PRId64 " ns\n"
                        "GL early app phase: %9" PRId64 " ns\tGL early SF phase: %9" PRId64 " ns\n",
                        late.app, late.sf, early.app, early.sf, earlyGl.app, earlyGl.sf);
}

nsecs_t PhaseOffsets::getCurrentAppOffset() {
    return getCurrentOffsets().late.app;
}

nsecs_t PhaseOffsets::getCurrentSfOffset() {
    return getCurrentOffsets().late.sf;
}

} // namespace impl
} // namespace scheduler
} // namespace android
