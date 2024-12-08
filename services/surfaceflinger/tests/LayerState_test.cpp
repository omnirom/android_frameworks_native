/*
 * Copyright 2020 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <binder/Binder.h>
#include <binder/Parcel.h>

#include <gui/LayerState.h>

namespace android {
using gui::DisplayCaptureArgs;
using gui::LayerCaptureArgs;
using gui::ScreenCaptureResults;

namespace test {

TEST(LayerStateTest, ParcellingScreenCaptureResultsWithFence) {
    ScreenCaptureResults results;
    results.buffer = sp<GraphicBuffer>::make(100u, 200u, PIXEL_FORMAT_RGBA_8888, 1u, 0u);
    results.fenceResult = sp<Fence>::make(dup(fileno(tmpfile())));
    results.capturedSecureLayers = true;
    results.capturedDataspace = ui::Dataspace::DISPLAY_P3;

    Parcel p;
    results.writeToParcel(&p);
    p.setDataPosition(0);

    ScreenCaptureResults results2;
    results2.readFromParcel(&p);

    // GraphicBuffer object is reallocated so compare the data in the graphic buffer
    // rather than the object itself
    ASSERT_EQ(results.buffer->getWidth(), results2.buffer->getWidth());
    ASSERT_EQ(results.buffer->getHeight(), results2.buffer->getHeight());
    ASSERT_EQ(results.buffer->getPixelFormat(), results2.buffer->getPixelFormat());
    ASSERT_TRUE(results.fenceResult.ok());
    ASSERT_TRUE(results2.fenceResult.ok());
    ASSERT_EQ(results.fenceResult.value()->isValid(), results2.fenceResult.value()->isValid());
    ASSERT_EQ(results.capturedSecureLayers, results2.capturedSecureLayers);
    ASSERT_EQ(results.capturedDataspace, results2.capturedDataspace);
}

TEST(LayerStateTest, ParcellingScreenCaptureResultsWithNoFenceOrError) {
    ScreenCaptureResults results;

    Parcel p;
    results.writeToParcel(&p);
    p.setDataPosition(0);

    ScreenCaptureResults results2;
    results2.readFromParcel(&p);

    ASSERT_TRUE(results2.fenceResult.ok());
    ASSERT_EQ(results2.fenceResult.value(), Fence::NO_FENCE);
}

TEST(LayerStateTest, ParcellingScreenCaptureResultsWithFenceError) {
    ScreenCaptureResults results;
    results.fenceResult = base::unexpected(BAD_VALUE);

    Parcel p;
    results.writeToParcel(&p);
    p.setDataPosition(0);

    ScreenCaptureResults results2;
    results2.readFromParcel(&p);

    ASSERT_FALSE(results.fenceResult.ok());
    ASSERT_FALSE(results2.fenceResult.ok());
    ASSERT_EQ(results.fenceResult.error(), results2.fenceResult.error());
}

} // namespace test
} // namespace android
