/*
 * Copyright (C) 2024 The Android Open Source Project
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

/**
* @defgroup SystemHealth
*
 * SystemHealth provides access to data about how various system resources are used by applications.
 *
 * CPU/GPU headroom APIs are designed to be best used by applications with consistent and intense
 * workload such as games to query the remaining capacity headroom over a short period and perform
 * optimization accordingly. Due to the nature of the fast job scheduling and frequency scaling of
 * CPU and GPU, the headroom by nature will have "TOCTOU" problem which makes it less suitable for
 * apps with inconsistent or low workload to take any useful action but simply monitoring. And to
 * avoid oscillation it's not recommended to adjust workload too frequent (on each polling request)
 * or too aggressively. As the headroom calculation is more based on reflecting past history usage
 * than predicting future capacity. Take game as an example, if the API returns CPU headroom of 0 in
 * one scenario (especially if it's constant across multiple calls), or some value significantly
 * smaller than other scenarios, then it can reason that the recent performance result is more CPU
 * bottlenecked. Then reducing the CPU workload intensity can help reserve some headroom to handle
 * the load variance better, which can result in less frame drops or smooth FPS value. On the other
 * hand, if the API returns large CPU headroom constantly, the app can be more confident to increase
 * the workload and expect higher possibility of device meeting its performance expectation.
 * App can also use thermal APIs to read the current thermal status and headroom first, then poll
 * the CPU and GPU headroom if the device is (about to) getting thermal throttled. If the CPU/GPU
 * headrooms provide enough significance such as one valued at 0 while the other at 100, then it can
 * be used to infer that reducing CPU workload could be more efficient to cool down the device.
 * There is a caveat that the power controller may scale down the frequency of the CPU and GPU due
 * to thermal and other reasons, which can result in a higher than usual percentage usage of the
 * capacity.
 *
* @{
*/

/**
 * @file system_health.h
 */

#ifndef _ANDROID_SYSTEM_HEALTH_H
#define _ANDROID_SYSTEM_HEALTH_H

#include <sys/cdefs.h>

/******************************************************************
 *
 * IMPORTANT NOTICE:
 *
 *   This file is part of Android's set of stable system headers
 *   exposed by the Android NDK (Native Development Kit).
 *
 *   Third-party source AND binary code relies on the definitions
 *   here to be FROZEN ON ALL UPCOMING PLATFORM RELEASES.
 *
 *   - DO NOT MODIFY ENUMS (EXCEPT IF YOU ADD NEW 32-BIT VALUES)
 *   - DO NOT MODIFY CONSTANTS OR FUNCTIONAL MACROS
 *   - DO NOT CHANGE THE SIGNATURE OF FUNCTIONS IN ANY WAY
 *   - DO NOT CHANGE THE LAYOUT OR SIZE OF STRUCTURES
 */


#include <stdint.h>
#include <sys/types.h>

#if !defined(__INTRODUCED_IN)
#define __INTRODUCED_IN(__api_level) /* nothing */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Params used to customize the calculation of CPU headroom.
 *
 * Also see {@link ASystemHealth_getCpuHeadroom}.
 */
typedef struct ACpuHeadroomParams ACpuHeadroomParams;

/**
 * Params used to customize the calculation of GPU headroom.
 *
 * Also see {@link ASystemHealth_getGpuHeadroom}.
 */
typedef struct AGpuHeadroomParams AGpuHeadroomParams;

typedef enum ACpuHeadroomCalculationType : int32_t {
    /**
     * The headroom calculation type bases on minimum value over a specified window.
     * Introduced in API level 36.
     */
    ACPU_HEADROOM_CALCULATION_TYPE_MIN = 0,
    /**
     * The headroom calculation type bases on average value over a specified window.
     * Introduced in API level 36.
     */
    ACPU_HEADROOM_CALCULATION_TYPE_AVERAGE = 1,
} ACpuHeadroomCalculationType;

typedef enum AGpuHeadroomCalculationType : int32_t {
    /**
     * The headroom calculation type bases on minimum value over a specified window.
     * Introduced in API level 36.
     */
    AGPU_HEADROOM_CALCULATION_TYPE_MIN = 0,
    /**
     * The headroom calculation type bases on average value over a specified window.
     * Introduced in API level 36.
     */
    AGPU_HEADROOM_CALCULATION_TYPE_AVERAGE = 1,
} AGpuHeadroomCalculationType;

/**
 * Sets the CPU headroom calculation window size in milliseconds.
 *
 * Available since API level 36.
 *
 * @param params The params to be set.
 * @param windowMillis The window size in milliseconds ranges from
 *                     {@link ASystemHealth_getCpuHeadroomCalculationWindowRange}. The smaller the
 *                     window size, the larger fluctuation in the headroom value should be expected.
 *                     The default value can be retrieved from the
 *                     {@link #ACpuHeadroomParams_getCalculationWindowMillis} if not set. The device
 *                     will try to use the closest feasible window size to this param.
 */
void ACpuHeadroomParams_setCalculationWindowMillis(ACpuHeadroomParams *_Nonnull params,
                                                   int windowMillis)
__INTRODUCED_IN(36);

/**
 * Gets the CPU headroom calculation window size in milliseconds.
 *
 * This will return the default value chosen by the device if not set.
 *
 * Available since API level 36.
 *
 * @param params The params to read from.
 * @return This will return the default value chosen by the device if the params is not set.
 */
int ACpuHeadroomParams_getCalculationWindowMillis(ACpuHeadroomParams* _Nonnull params)
__INTRODUCED_IN(36);

/**
 * Sets the GPU headroom calculation window size in milliseconds.
 *
 * Available since API level 36.
 *
 * @param params The params to be set.
 * @param windowMillis The window size in milliseconds ranges from
 *                     {@link ASystemHealth_getGpuHeadroomCalculationWindowRange}. The smaller the
 *                     window size, the larger fluctuation in the headroom value should be expected.
 *                     The default value can be retrieved from the
 *                     {@link #AGpuHeadroomParams_getCalculationWindowMillis} if not set. The device
 *                     will try to use the closest feasible window size to this param.
 */
void AGpuHeadroomParams_setCalculationWindowMillis(AGpuHeadroomParams* _Nonnull params,
                                                   int windowMillis)
__INTRODUCED_IN(36);

/**
 * Gets the GPU headroom calculation window size in milliseconds.
 *
 * This will return the default value chosen by the device if not set.
 *
 * Available since API level 36.
 *
 * @param params The params to read from.
 * @return This will return the default value chosen by the device if the params is not set.
 */
int AGpuHeadroomParams_getCalculationWindowMillis(AGpuHeadroomParams* _Nonnull params)
__INTRODUCED_IN(36);

/**
 * Sets the CPU headroom calculation type in {@link ACpuHeadroomParams}.
 *
 * Available since API level 36.
 *
 * @param params The params to be set.
 * @param calculationType The headroom calculation type.
 */
void ACpuHeadroomParams_setCalculationType(ACpuHeadroomParams* _Nonnull params,
                                           ACpuHeadroomCalculationType calculationType)
__INTRODUCED_IN(36);

/**
 * Gets the CPU headroom calculation type in {@link ACpuHeadroomParams}.
 *
 * This will return the default value chosen by the device if not set.
 *
 * Available since API level 36.
 *
 * @param params The params to read from.
 * @return The headroom calculation type.
 */
ACpuHeadroomCalculationType
ACpuHeadroomParams_getCalculationType(ACpuHeadroomParams* _Nonnull params)
__INTRODUCED_IN(36);

/**
 * Sets the GPU headroom calculation type in {@link AGpuHeadroomParams}.
 *
 * Available since API level 36.
 *
 * @param params The params to be set.
 * @param calculationType The headroom calculation type.
 */
void AGpuHeadroomParams_setCalculationType(AGpuHeadroomParams* _Nonnull params,
                                           AGpuHeadroomCalculationType calculationType)
__INTRODUCED_IN(36);

/**
 * Gets the GPU headroom calculation type in {@link AGpuHeadroomParams}.
 *
 * This will return the default value chosen by the device if not set.
 *
 * Available since API level 36.
 *
 * @param params The params to read from.
 * @return The headroom calculation type.
 */
AGpuHeadroomCalculationType
AGpuHeadroomParams_getCalculationType(AGpuHeadroomParams* _Nonnull params)
__INTRODUCED_IN(36);

/**
 * Sets the thread TIDs to track in {@link ACpuHeadroomParams}.
 *
 * The TIDs should belong to the same of the process that will make the headroom call. And they
 * should not have different core affinity.
 *
 * If not set or set to empty, the headroom will be based on the PID of the process making the call.
 *
 * Available since API level 36.
 *
 * @param params The params to be set.
 * @param tids Non-null array of TIDs, where maximum size can be read from
 *             {@link ASystemHealth_getMaxCpuHeadroomTidsSize}.
 * @param tidsSize The size of the tids array.
 */
void ACpuHeadroomParams_setTids(ACpuHeadroomParams* _Nonnull params, const int* _Nonnull tids,
                                size_t tidsSize)
__INTRODUCED_IN(36);

/**
 * Creates a new instance of {@link ACpuHeadroomParams}.
 *
 * When the client finishes using {@link ACpuHeadroomParams},
 * {@link ACpuHeadroomParams_destroy} must be called to destroy
 * and free up the resources associated with {@link ACpuHeadroomParams}.
 *
 * Available since API level 36.
 *
 * @return A new instance of {@link ACpuHeadroomParams}.
 */
ACpuHeadroomParams* _Nonnull ACpuHeadroomParams_create(void)
__INTRODUCED_IN(36);

/**
 * Creates a new instance of {@link AGpuHeadroomParams}.
 *
 * When the client finishes using {@link AGpuHeadroomParams},
 * {@link AGpuHeadroomParams_destroy} must be called to destroy
 * and free up the resources associated with {@link AGpuHeadroomParams}.
 *
 * Available since API level 36.
 *
 * @return A new instance of {@link AGpuHeadroomParams}.
 */
AGpuHeadroomParams* _Nonnull AGpuHeadroomParams_create(void)
__INTRODUCED_IN(36);

/**
 * Deletes the {@link ACpuHeadroomParams} instance.
 *
 * Available since API level 36.
 *
 * @param params The params to be deleted.
 */
void ACpuHeadroomParams_destroy(ACpuHeadroomParams* _Nullable params)
__INTRODUCED_IN(36);

/**
 * Deletes the {@link AGpuHeadroomParams} instance.
 *
 * Available since API level 36.
 *
 * @param params The params to be deleted.
 */
void AGpuHeadroomParams_destroy(AGpuHeadroomParams* _Nullable params)
__INTRODUCED_IN(36);

/**
 * Gets the maximum number of TIDs this device supports for getting CPU headroom.
 *
 * See {@link ACpuHeadroomParams_setTids}.
 *
 * Available since API level 36.
 *
 * @param outSize Non-null output pointer to the max size.
 * @return 0 on success.
 *         ENOTSUP if the CPU headroom API is unsupported.
 */
int ASystemHealth_getMaxCpuHeadroomTidsSize(size_t* _Nonnull outSize);

/**
 * Gets the range of the calculation window size for CPU headroom.
 *
 * Available since API level 36.
 *
 * @param outMinMillis Non-null output pointer to be set to the minimum window size in milliseconds.
 * @param outMaxMillis Non-null output pointer to be set to the maximum window size in milliseconds.
 * @return 0 on success.
 *         ENOTSUP if API is unsupported.
 */
int ASystemHealth_getCpuHeadroomCalculationWindowRange(int32_t* _Nonnull outMinMillis,
                                                       int32_t* _Nonnull outMaxMillis)
__INTRODUCED_IN(36);

/**
 * Gets the range of the calculation window size for GPU headroom.
 *
 * Available since API level 36.
 *
 * @param outMinMillis Non-null output pointer to be set to the minimum window size in milliseconds.
 * @param outMaxMillis Non-null output pointer to be set to the maximum window size in milliseconds.
 * @return 0 on success.
 *         ENOTSUP if API is unsupported.
 */
int ASystemHealth_getGpuHeadroomCalculationWindowRange(int32_t* _Nonnull outMinMillis,
                                                       int32_t* _Nonnull outMaxMillis)
__INTRODUCED_IN(36);

/**
 * Provides an estimate of available CPU capacity headroom of the device.
 *
 * The value can be used by the calling application to determine if the workload was CPU bound and
 * then take action accordingly to ensure that the workload can be completed smoothly. It can also
 * be used with the thermal status and headroom to determine if reducing the CPU bound workload can
 * help reduce the device temperature to avoid thermal throttling.
 *
 * If the params are valid, each call will perform at least one synchronous binder transaction that
 * can take more than 1ms. So it's not recommended to call or wait for this on critical threads.
 * Some devices may implement this as an on-demand API with lazy initialization, so the caller
 * should expect higher latency when making the first call (especially with non-default params)
 * since app starts or after changing params, as the device may need to change its data collection.
 *
 * Available since API level 36.
 *
 * @param params The params to customize the CPU headroom calculation, or nullptr to use default.
 * @param outHeadroom Non-null output pointer to a single float, which will be set to the CPU
 *                    headroom value. The value will be a single value or `Float.NaN` if it's
 *                    temporarily unavailable due to server error or not enough user CPU workload.
 *                    Each valid value ranges from [0, 100], where 0 indicates no more cpu resources
 *                    can be granted.
 * @return 0 on success.
 *         EPIPE if failed to get the CPU headroom.
 *         EPERM if the TIDs do not belong to the same process.
 *         ENOTSUP if API or requested params is unsupported.
 */
int ASystemHealth_getCpuHeadroom(const ACpuHeadroomParams* _Nullable params,
                                 float* _Nonnull outHeadroom)
__INTRODUCED_IN(36);

/**
 * Provides an estimate of available GPU capacity headroom of the device.
 *
 * The value can be used by the calling application to determine if the workload was GPU bound and
 * then take action accordingly to ensure that the workload can be completed smoothly. It can also
 * be used with the thermal status and headroom to determine if reducing the GPU bound workload can
 * help reduce the device temperature to avoid thermal throttling.
 *
 * If the params are valid, each call will perform at least one synchronous binder transaction that
 * can take more than 1ms. So it's not recommended to call or wait for this on critical threads.
 * Some devices may implement this as an on-demand API with lazy initialization, so the caller
 * should expect higher latency when making the first call (especially with non-default params)
 * since app starts or after changing params, as the device may need to change its data collection.
 *
 * Available since API level 36
 *
 * @param params The params to customize the GPU headroom calculation, or nullptr to use default
 * @param outHeadroom Non-null output pointer to a single float, which will be set to the GPU
 *                    headroom value. The value will be a single value or `Float.NaN` if it's
 *                    temporarily unavailable.
 *                    Each valid value ranges from [0, 100], where 0 indicates no more gpu resources
 *                    can be granted.
 * @return 0 on success.
 *         EPIPE if failed to get the GPU headroom.
 *         ENOTSUP if API or requested params is unsupported.
 */
int ASystemHealth_getGpuHeadroom(const AGpuHeadroomParams* _Nullable params,
                                 float* _Nonnull outHeadroom)
__INTRODUCED_IN(36);

/**
 * Gets minimum polling interval for calling {@link ASystemHealth_getCpuHeadroom} in milliseconds.
 *
 * The {@link ASystemHealth_getCpuHeadroom} API may return cached result if called more frequently
 * than the interval.
 *
 * Available since API level 36.
 *
 * @param outMinIntervalMillis Non-null output pointer to a int64_t, which
 *                will be set to the minimum polling interval in milliseconds.
 * @return 0 on success.
 *         ENOTSUP if API is unsupported.
 */
int ASystemHealth_getCpuHeadroomMinIntervalMillis(int64_t* _Nonnull outMinIntervalMillis)
__INTRODUCED_IN(36);

/**
 * Gets minimum polling interval for calling {@link ASystemHealth_getGpuHeadroom} in milliseconds.
 *
 * The {@link ASystemHealth_getGpuHeadroom} API may return cached result if called more frequently
 * than the interval.
 *
 * Available since API level 36.
 *
 * @param outMinIntervalMillis Non-null output pointer to a int64_t, which
 *                will be set to the minimum polling interval in milliseconds.
 * @return 0 on success.
 *         ENOTSUP if API is unsupported.
 */
int ASystemHealth_getGpuHeadroomMinIntervalMillis(int64_t* _Nonnull outMinIntervalMillis)
__INTRODUCED_IN(36);

#ifdef __cplusplus
}
#endif

#endif // _ANDROID_SYSTEM_HEALTH_H

/** @} */
