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
#include <ui/RingBuffer.h>

namespace android::ui {

TEST(RingBuffer, basic) {
    RingBuffer<int, 5> rb;

    rb.next() = 1;
    ASSERT_EQ(1, rb.size());
    ASSERT_EQ(1, rb.back());
    ASSERT_EQ(1, rb.front());

    rb.next() = 2;
    ASSERT_EQ(2, rb.size());
    ASSERT_EQ(2, rb.back());
    ASSERT_EQ(1, rb.front());
    ASSERT_EQ(1, rb[-1]);

    rb.next() = 3;
    ASSERT_EQ(3, rb.size());
    ASSERT_EQ(3, rb.back());
    ASSERT_EQ(1, rb.front());
    ASSERT_EQ(2, rb[-1]);
    ASSERT_EQ(1, rb[-2]);

    rb.next() = 4;
    ASSERT_EQ(4, rb.size());
    ASSERT_EQ(4, rb.back());
    ASSERT_EQ(1, rb.front());
    ASSERT_EQ(3, rb[-1]);
    ASSERT_EQ(2, rb[-2]);
    ASSERT_EQ(1, rb[-3]);

    rb.next() = 5;
    ASSERT_EQ(5, rb.size());
    ASSERT_EQ(5, rb.back());
    ASSERT_EQ(1, rb.front());
    ASSERT_EQ(4, rb[-1]);
    ASSERT_EQ(3, rb[-2]);
    ASSERT_EQ(2, rb[-3]);
    ASSERT_EQ(1, rb[-4]);

    rb.next() = 6;
    ASSERT_EQ(5, rb.size());
    ASSERT_EQ(6, rb.back());
    ASSERT_EQ(2, rb.front());
    ASSERT_EQ(5, rb[-1]);
    ASSERT_EQ(4, rb[-2]);
    ASSERT_EQ(3, rb[-3]);
    ASSERT_EQ(2, rb[-4]);
}

} // namespace android::ui