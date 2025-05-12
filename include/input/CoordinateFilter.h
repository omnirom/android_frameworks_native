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

#pragma once

#include <chrono>

#include <input/Input.h>
#include <input/OneEuroFilter.h>

namespace android {

/**
 * Pair of OneEuroFilters that independently filter X and Y coordinates. Both filters share the same
 * constructor's parameters. The minimum cutoff frequency is the base cutoff frequency, that is, the
 * resulting cutoff frequency in the absence of signal's speed. Likewise, beta is a scaling factor
 * of the signal's speed that sets how much the signal's speed contributes to the resulting cutoff
 * frequency. The adaptive cutoff frequency criterion is f_c = f_c_min + β|̇x_filtered|
 */
class CoordinateFilter {
public:
    explicit CoordinateFilter(float minCutoffFreq, float beta);

    /**
     * Filters in place only the AXIS_X and AXIS_Y fields from coords. Each call to filter must
     * provide a timestamp strictly greater than the timestamp of the previous call. The first time
     * this method is invoked no filtering takes place. Subsequent calls do overwrite `coords` with
     * filtered data.
     *
     * @param timestamp The timestamps at which to filter. It must be greater than the one passed in
     * the previous call.
     * @param coords Coordinates to be overwritten by the corresponding filtered coordinates.
     */
    void filter(std::chrono::nanoseconds timestamp, PointerCoords& coords);

private:
    OneEuroFilter mXFilter;
    OneEuroFilter mYFilter;
};

} // namespace android