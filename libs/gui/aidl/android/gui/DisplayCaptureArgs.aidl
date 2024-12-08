/*
 * Copyright 2021 The Android Open Source Project
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

package android.gui;

import android.gui.CaptureArgs;

// Arguments for screenshotting an entire display
parcelable DisplayCaptureArgs {
    CaptureArgs captureArgs;

    // The display that we want to screenshot
    IBinder displayToken;

    // The width of the render area when we screenshot
    int width = 0;
    // The length of the render area when we screenshot
    int height = 0;
}

