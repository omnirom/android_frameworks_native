/*
 * Copyright 2013 The Android Open Source Project
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

#define LOG_TAG "HalfTest"

#include <math.h>
#include <stdlib.h>

#include <math/half.h>
#include <math/vec4.h>

#include <gtest/gtest.h>

namespace android {

TEST(HalfTest, TestHalfSize) {
    EXPECT_EQ(2UL, sizeof(half));
}

TEST(HalfTest, TestZero) {
    EXPECT_EQ(half().getBits(), 0x0000);
    EXPECT_EQ(half(0.0f).getBits(), 0x0000);
    EXPECT_EQ(half(-0.0f).getBits(), 0x8000);
}

TEST(HalfTest, TestNaN) {
    EXPECT_EQ(half(NAN).getBits(), 0x7e00);
    EXPECT_EQ(std::numeric_limits<half>::quiet_NaN().getBits(), 0x7FFF);
    EXPECT_EQ(std::numeric_limits<half>::signaling_NaN().getBits(), 0x7DFF);
}

TEST(HalfTest, TestInfinity) {
    EXPECT_EQ(std::numeric_limits<half>::infinity().getBits(), 0x7C00);
}

TEST(HalfTest, TestNumericLimits) {
    EXPECT_EQ(std::numeric_limits<half>::min().getBits(), 0x0400);
    EXPECT_EQ(std::numeric_limits<half>::max().getBits(), 0x7BFF);
    EXPECT_EQ(std::numeric_limits<half>::lowest().getBits(), 0xFBFF);
}

TEST(HalfTest, TestEpsilon) {
    EXPECT_EQ(std::numeric_limits<half>::epsilon().getBits(), 0x1400);
}

TEST(HalfTest, TestDenormals) {
    EXPECT_EQ(half(std::numeric_limits<half>::denorm_min()).getBits(), 0x0001);
    EXPECT_EQ(half(std::numeric_limits<half>::denorm_min() * 2).getBits(), 0x0002);
    EXPECT_EQ(half(std::numeric_limits<half>::denorm_min() * 3).getBits(), 0x0003);
    // test a few known denormals
    EXPECT_EQ(half(6.09756e-5).getBits(), 0x03FF);
    EXPECT_EQ(half(5.96046e-8).getBits(), 0x0001);
    EXPECT_EQ(half(-6.09756e-5).getBits(), 0x83FF);
    EXPECT_EQ(half(-5.96046e-8).getBits(), 0x8001);
}

TEST(HalfTest, TestNormal) {
    // test a few known values
    EXPECT_EQ(half(1.0009765625).getBits(), 0x3C01);
    EXPECT_EQ(half(-2).getBits(), 0xC000);
    EXPECT_EQ(half(6.10352e-5).getBits(), 0x0400);
    EXPECT_EQ(half(65504).getBits(), 0x7BFF);
    EXPECT_EQ(half(1.0f / 3).getBits(), 0x3555);

    // test all exactly representable integers
    for (int i = -2048; i <= 2048; ++i) {
        half h = i;
        EXPECT_EQ(float(h), i);
    }
}

TEST(HalfTest, TestLiterals) {
    half one = 1.0_hf;
    half pi = 3.1415926_hf;
    half minusTwo = -2.0_hf;

    EXPECT_EQ(half(1.0f), one);
    EXPECT_EQ(half(3.1415926), pi);
    EXPECT_EQ(half(-2.0f), minusTwo);
}

TEST(HalfTest, TestVec) {
    float4 f4(1, 2, 3, 4);
    half4 h4(f4);
    half3 h3(f4.xyz);
    half2 h2(f4.xy);

    EXPECT_EQ(f4, h4);
    EXPECT_EQ(f4.xyz, h3);
    EXPECT_EQ(f4.xy, h2);
}

TEST(HalfTest, TestHash) {
    float4 f4a(1, 2, 3, 4);
    float4 f4b(2, 2, 3, 4);
    half4 h4a(f4a), h4b(f4b);

    EXPECT_NE(std::hash<half4>{}(h4a), std::hash<half4>{}(h4b));
}

TEST(HalfTest, TestHalfToFloat) {
    EXPECT_EQ(4.25f, float(4.25_hf));
    EXPECT_EQ(3.05175781e-05f, float(3.05175781e-05_hf));
}

}; // namespace android
