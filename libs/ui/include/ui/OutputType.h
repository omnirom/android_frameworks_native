/*
 * Copyright 2025 The Android Open Source Project
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

#ifndef ANDROID_UI_OUTPUT_TYPE_H
#define ANDROID_UI_OUTPUT_TYPE_H

#include <stdint.h>

namespace android::ui {

// Must be kept in sync with composer3/OutputType.aidl
enum class OutputType : int32_t {
    /**
     * Invalid HDR output type
     */
    OUTPUT_TYPE_INVALID = 0,
    /*
     * Display output format will be chosen by the HAL implementation
     * and will not adjust to match the content format
     */
    OUTPUT_TYPE_SYSTEM = 1,
    /**
     * Display supports SDR output type
     */
    OUTPUT_TYPE_SDR = 2,
    /**
     * Display supports HDR10 output type
     */
    OUTPUT_TYPE_HDR10 = 3,
};

} // namespace android::ui

#endif // ANDROID_UI_OUTPUT_TYPE_H
