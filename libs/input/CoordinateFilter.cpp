/**
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

#define LOG_TAG "CoordinateFilter"

#include <input/CoordinateFilter.h>

namespace android {

CoordinateFilter::CoordinateFilter(float minCutoffFreq, float beta)
      : mXFilter{minCutoffFreq, beta}, mYFilter{minCutoffFreq, beta} {}

void CoordinateFilter::filter(std::chrono::nanoseconds timestamp, PointerCoords& coords) {
    coords.setAxisValue(AMOTION_EVENT_AXIS_X, mXFilter.filter(timestamp, coords.getX()));
    coords.setAxisValue(AMOTION_EVENT_AXIS_Y, mYFilter.filter(timestamp, coords.getY()));
}

} // namespace android
