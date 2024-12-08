/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBVIBRATOR_TEST_UTILS_H
#define LIBVIBRATOR_TEST_UTILS_H

#include <gtest/gtest.h>

#if !defined(EXPECT_FLOATS_NEARLY_EQ)
#define EXPECT_FLOATS_NEARLY_EQ(expected, actual, length, epsilon)              \
        for (size_t i = 0; i < length; i++) {                                   \
            EXPECT_NEAR(expected[i], actual[i], epsilon) << " at Index: " << i; \
        }
#else
#error Macro EXPECT_FLOATS_NEARLY_EQ already defined
#endif

#endif //LIBVIBRATOR_TEST_UTILS_H
