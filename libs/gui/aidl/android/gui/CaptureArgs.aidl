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

    // True if capturing secure layers is permitted
    boolean captureSecureLayers = false;

    // UID whose content we want to screenshot
    int uid = UNSET_UID;

    // Force capture to be in a color space. If the value is ui::Dataspace::UNKNOWN, the captured
    // result will be in a colorspace appropriate for capturing the display contents
    // The display may use non-RGB dataspace (ex. displayP3) that could cause pixel data could be
    // different from SRGB (byte per color), and failed when checking colors in tests.
    // NOTE: In normal cases, we want the screen to be captured in display's colorspace.
    int /*ui::Dataspace*/ dataspace = 0;

    // The receiver of the capture can handle protected buffer. A protected buffer has
    // GRALLOC_USAGE_PROTECTED usage bit and must not be accessed unprotected behaviour.
    // Any read/write access from unprotected context will result in undefined behaviour.
    // Protected contents are typically DRM contents. This has no direct implication to the
    // secure property of the surface, which is specified by the application explicitly to avoid
    // the contents being accessed/captured by screenshot or unsecure display.
    boolean allowProtected = false;

    // True if the content should be captured in grayscale
    boolean grayscale = false;

    // List of layers to exclude capturing from
    IBinder[] excludeHandles;

    // Hint that the caller will use the screenshot animation as part of a transition animation.
    // The canonical example would be screen rotation - in such a case any color shift in the
    // screenshot is a detractor so composition in the display's colorspace is required.
    // Otherwise, the system may choose a colorspace that is more appropriate for use-cases
    // such as file encoding or for blending HDR content into an ap's UI, where the display's
    // exact colorspace is not an appropriate intermediate result.
    // Note that if the caller is requesting a specific dataspace, this hint does nothing.
    boolean hintForSeamlessTransition = false;

    // Allows the screenshot to attach a gainmap, which allows for a per-pixel
    // transformation of the screenshot to another luminance range, typically
    // mapping an SDR base image into HDR.
    boolean attachGainmap = false;
}

