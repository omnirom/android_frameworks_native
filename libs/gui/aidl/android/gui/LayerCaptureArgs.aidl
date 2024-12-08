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

// Arguments for capturing a layer and/or its children
parcelable LayerCaptureArgs {
    CaptureArgs captureArgs;

    // The Layer that we may want to capture. We would also capture its children
    IBinder layerHandle;
    // True if we don't actually want to capture the layer and want to capture
    // its children instead.
    boolean childrenOnly = false;
}
