/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup NativeActivity Native Activity
 * @{
 */

/**
 * @file display_luts.h
 */
#pragma once

#include <inttypes.h>

__BEGIN_DECLS

/**
 * The dimension of the lut
 */
enum ADisplayLuts_Dimension : int32_t {
    ADISPLAYLUTS_ONE_DIMENSION = 1,
    ADISPLAYLUTS_THREE_DIMENSION = 3,
};
typedef enum ADisplayLuts_Dimension ADisplayLuts_Dimension;

/**
 * The sampling key used by the lut
 */
enum ADisplayLuts_SamplingKey : int32_t {
    ADISPLAYLUTS_SAMPLINGKEY_RGB = 0,
    ADISPLAYLUTS_SAMPLINGKEY_MAX_RGB = 1,
    ADISPLAYLUTS_SAMPLINGKEY_CIE_Y = 2,
};
typedef enum ADisplayLuts_SamplingKey ADisplayLuts_SamplingKey;

/**
 * Used to get and set display luts entry
 */
typedef struct ADisplayLutsEntry ADisplayLutsEntry;

/**
 * Used to get and set display luts
 */
typedef struct ADisplayLuts ADisplayLuts;

/**
 * Creates a \a ADisplayLutsEntry entry.
 *
 * You are responsible for managing the memory of the returned object.
 * Always call \a ADisplayLutsEntry_destroy to release it after use.
 *
 * Functions like \a ADisplayLuts_set create their own copies of entries,
 * therefore they don't take the ownership of the instance created by
 * \a ADisplayLutsEntry_create.
 *
 * 1D Lut(s) are treated as gain curves.
 *
 * 3D Lut(s) are used for direct color manipulations.
 * For 3D Lut(s), the values should be normalized to the range 0.0
 * to 1.0 , inclusive. And 1.0 is the maximum panel luminance.
 * And If N is the size of each dimension, the data is arranged in RGB order:
 * R(0, 0, 0), R(0, 0, 1), ..., R(0, 0, N - 1),
 * R(0, 1, 0), ..., R(0, 1, N - 1), ..., R(0, N - 1, N - 1),
 * R(1, 0, 0), ..., R(1, 0, N - 1), ..., R(1, N - 1, N - 1), ..., R(N - 1, N - 1, N - 1),
 * G(0, 0, 0), ..., G(N - 1, N - 1, N - 1),
 * B(0, 0, 0), ..., B(N - 1, N - 1, N - 1).
 *
 * When a GPU shader samples 3D Lut data, it's accessed in a flat, one-dimensional arrangement.
 * Assuming that we have a 3D array ORIGINAL[N][N][N],
 * then ORIGINAL[x][y][z] is mapped to FLAT[z + N * (y + N * x)].
 *
 * @param buffer The lut raw buffer. The function creates a copy of it and does not need to
 * outlive the life of the ADisplayLutsEntry.
 * @param length The length of lut raw buffer
 * @param dimension The dimension of the lut. see \a ADisplayLuts_Dimension
 * @param key The sampling key used by the lut. see \a ADisplayLuts_SamplingKey
 * @return a new \a ADisplayLutsEntry instance.
 */
ADisplayLutsEntry* _Nonnull ADisplayLutsEntry_createEntry(float* _Nonnull buffer,
    int32_t length, ADisplayLuts_Dimension dimension, ADisplayLuts_SamplingKey key)
    __INTRODUCED_IN(36);

/**
 * Destroys the \a ADisplayLutsEntry instance.
 *
 * @param entry The entry to be destroyed
 */
void ADisplayLutsEntry_destroy(ADisplayLutsEntry* _Nullable entry) __INTRODUCED_IN(36);

/**
 * Gets the dimension of the entry.
 *
 * The function is only valid for the lifetime of the `entry`.
 *
 * @param entry The entry to query
 * @return the dimension of the lut
 */
ADisplayLuts_Dimension ADisplayLutsEntry_getDimension(const ADisplayLutsEntry* _Nonnull entry)
        __INTRODUCED_IN(36);

/**
 * Gets the size for each dimension of the entry.
 *
 * The function is only valid for the lifetime of the `entry`.
 *
 * @param entry The entry to query
 * @return the size of each dimension of the lut
 */
int32_t ADisplayLutsEntry_getSize(const ADisplayLutsEntry* _Nonnull entry)
        __INTRODUCED_IN(36);

/**
 * Gets the sampling key used by the entry.
 *
 * The function is only valid for the lifetime of the `entry`.
 *
 * @param entry The entry to query
 * @return the sampling key used by the lut
 */
ADisplayLuts_SamplingKey ADisplayLutsEntry_getSamplingKey(const ADisplayLutsEntry* _Nonnull entry)
        __INTRODUCED_IN(36);

/**
 * Gets the lut buffer of the entry.
 *
 * The function is only valid for the lifetime of the `entry`.
 *
 * @param entry The entry to query
 * @return a pointer to the raw lut buffer
 */
const float* _Nonnull ADisplayLutsEntry_getBuffer(const ADisplayLutsEntry* _Nonnull entry)
        __INTRODUCED_IN(36);

/**
 * Creates a \a ADisplayLuts instance.
 *
 * You are responsible for managing the memory of the returned object.
 * Always call \a ADisplayLuts_destroy to release it after use. E.g., after calling
 * the function \a ASurfaceTransaction_setLuts.
 *
 * @return a new \a ADisplayLuts instance
 */
ADisplayLuts* _Nonnull ADisplayLuts_create() __INTRODUCED_IN(36);

/**
 * Sets Luts in order to be applied.
 *
 * The function accepts a single 1D Lut, or a single 3D Lut or both 1D and 3D Lut in order.
 * And the function will replace any previously set lut(s).
 * If you want to clear the previously set lut(s), set `entries` to be nullptr,
 * and `numEntries` will be internally ignored.
 *
 * @param luts the pointer of the \a ADisplayLuts instance
 * @param entries the pointer of the array of lut entries to be applied
 * @param numEntries the number of lut entries. The value should be either 1 or 2.
 */
void ADisplayLuts_setEntries(ADisplayLuts* _Nonnull luts,
        ADisplayLutsEntry* _Nullable *_Nullable entries, int32_t numEntries) __INTRODUCED_IN(36);

/**
 * Deletes the \a ADisplayLuts instance.
 *
 * @param luts The luts to be destroyed
 */
void ADisplayLuts_destroy(ADisplayLuts* _Nullable luts) __INTRODUCED_IN(36);

__END_DECLS

/** @} */