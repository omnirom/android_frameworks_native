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

#pragma once

#include <ftl/flags.h>
#include <stdint.h>

namespace android::adpf {
// Additional composition workload that can increase cpu load.
enum class Workload : uint32_t {
    NONE = 0,
    // Layer effects like blur and shadows which forces client composition
    EFFECTS = 1 << 0,

    // Geometry changes which requires HWC to validate and share composition strategy
    VISIBLE_REGION = 1 << 1,

    // Diplay changes which can cause geometry changes
    DISPLAY_CHANGES = 1 << 2,

    // Changes in sf duration which can shorten the deadline for sf to composite the frame
    WAKEUP = 1 << 3,

    // Increases in refresh rates can cause the deadline for sf to composite to be shorter
    REFRESH_RATE_INCREASE = 1 << 4,

    // Screenshot requests increase both the cpu and gpu workload
    SCREENSHOT = 1 << 5
};
} // namespace android::adpf
