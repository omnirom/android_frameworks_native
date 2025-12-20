/*
 * Copyright 2023 The Android Open Source Project
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

import android.gui.ARect;
import android.gui.CaptureMode;
import android.gui.ProtectedLayerMode;
import android.gui.SecureLayerMode;

// Common arguments for capturing content on-screen
parcelable CaptureArgs {
    const int UNSET_UID = -1;

    // Desired pixel format of the final screenshotted buffer
    int /*ui::PixelFormat*/ pixelFormat = 1;

    // Crop in layer space: all content outside of the crop will not be captured.
    ARect sourceCrop;

    // Scale in the x-direction for the screenshotted result.
    float frameScaleX = 1.0f;

    // Scale in the y-direction for the screenshotted result.
    float frameScaleY = 1.0f;

    // Specifies how to handle secure layers. If the client wants DPU composition only,
    // they should specify SecureLayerMode.Error. Otherwise, set to SecureLayerMode.Redact (default)
    // to redact secure layers or SecureLayerMode.Capture to capture them.
    SecureLayerMode secureLayerMode = SecureLayerMode.Redact;

    // UID whose content we want to screenshot
    int uid = UNSET_UID;

    // Force capture to be in a color space. If the value is ui::Dataspace::UNKNOWN, the captured
    // result will be in a colorspace appropriate for capturing the display contents
    // The display may use non-RGB dataspace (ex. displayP3) that could cause pixel data could be
    // different from SRGB (byte per color), and failed when checking colors in tests.
    // NOTE: In normal cases, we want the screen to be captured in display's colorspace.
    int /*ui::Dataspace*/ dataspace = 0;

    // Specifies how to handle protected layers. A protected buffer has GRALLOC_USAGE_PROTECTED
    // usage bit. If the client wants DPU composition only, they should specify
    // ProtectedLayerMode.Error. Otherwise, set to ProtectedLayerMode.Redact (default).
    ProtectedLayerMode protectedLayerMode = ProtectedLayerMode.Redact;

    // True if the content should be captured in grayscale
    boolean grayscale = false;

    // List of layers to exclude capturing from
    IBinder[] excludeHandles;

    // Hint that the caller will use the screenshot animation as part of a transition animation.
    // The canonical example would be screen rotation - in such a case any color shift in the
    // screenshot is a detractor so composition in the display's colorspace is required.
    // Otherwise, the system may choose a colorspace that is more appropriate for use-cases
    // such as file encoding or for blending HDR content into an app's UI, where the display's
    // exact colorspace is not an appropriate intermediate result.
    // Note that if the caller is requesting a specific dataspace, this hint does nothing.
    boolean preserveDisplayColors = false;

    // Specifies the capture mode. CaptureMode.None is the default. It uses the GPU path and
    // applies any optimizations such as DPU if possible. CaptureMode.RequireOptimized attempts
    // to use the DPU optimized path; if not possible, a screenshot error is returned.
    CaptureMode captureMode = CaptureMode.None;

    // If true, the system renders the buffer in display installation orientation.
    boolean useDisplayInstallationOrientation = false;
    // If true, the screenshot will include system overlay layers, such as the screen decor layers.
    boolean includeAllLayers = false;
}
