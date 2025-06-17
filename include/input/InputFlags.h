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

#pragma once

namespace android {

class InputFlags {
public:
    /**
     * Check if connected displays feature is enabled, either via the feature flag or settings
     * override. Developer setting override allows enabling all the "desktop experiences" features
     * including input related connected_displays_cursor flag.
     *
     * The developer settings override is prioritised over aconfig flags. Any tests that require
     * applicable aconfig flags to be disabled with SCOPED_FLAG_OVERRIDE also need this developer
     * option to be reset locally.
     *
     * Also note the developer setting override is only applicable to the desktop experiences
     * related features.
     *
     * To enable only the input flag run:
     *      adb shell aflags enable com.android.input.flags.connected_displays_cursor
     * To override this flag and enable all "desktop experiences" features run:
     *      adb shell aflags enable com.android.window.flags.enable_desktop_mode_through_dev_option
     *      adb shell setprop persist.wm.debug.desktop_experience_devopts 1
     */
    static bool connectedDisplaysCursorEnabled();

    /**
     * Check if both connectedDisplaysCursor and associatedDisplayCursorBugfix is enabled.
     */
    static bool connectedDisplaysCursorAndAssociatedDisplayCursorBugfixEnabled();
};

} // namespace android
