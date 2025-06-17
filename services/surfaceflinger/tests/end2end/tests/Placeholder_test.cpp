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

#include <gtest/gtest.h>

#include <android-base/logging.h>

#include "test_framework/core/TestService.h"

namespace android::surfaceflinger::tests::end2end {
namespace {

struct Placeholder : public ::testing::Test {};

TEST_F(Placeholder, Bringup) {
    auto serviceResult = test_framework::core::TestService::startWithDisplays({
            {.id = 123},
    });
    if (!serviceResult) {
        LOG(WARNING) << "End2End service not available. " << serviceResult.error();
        GTEST_SKIP() << "End2End service not available. " << serviceResult.error();
    }
}

}  // namespace
}  // namespace android::surfaceflinger::tests::end2end
