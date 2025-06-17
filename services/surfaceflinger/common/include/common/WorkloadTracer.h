
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
namespace android::WorkloadTracer {

static constexpr int32_t COMPOSITION_TRACE_COOKIE = 1;
static constexpr int32_t POST_COMPOSITION_TRACE_COOKIE = 2;
static constexpr size_t COMPOSITION_SUMMARY_SIZE = 64;
static constexpr const char* TRACK_NAME = "CriticalWorkload";

} // namespace android::WorkloadTracer