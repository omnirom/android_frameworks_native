/*
 * Copyright (C) 2025 The Android Open Source Project
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

package android.os;

/**
 * Modes in which the pointer can be captured by an app, view, or window. See the public
 * documentation in android.view.View for full descriptions of the modes.
 * @hide
 */
@Backing(type="int")
enum PointerCaptureMode {
    /** The pointer is not captured. */
    UNCAPTURED = 0,
    /**
     * The pointer is captured in absolute mode, in which touchpads report absolute touch locations.
     * Mice still report relative movements.
     */
    ABSOLUTE = 1,
    /**
     * The pointer is captured in relative mode, in which touchpads report relative movements, just
     * like mice.
     */
    RELATIVE = 2,

    // When adding new modes:
    // * use consecutive integer values
    // * also add them to android.view.View and frameworks/native/include/input/Input.h
    // * update the checks in InputManagerService and InputManagerGlobal#requestPointerCapture
}
