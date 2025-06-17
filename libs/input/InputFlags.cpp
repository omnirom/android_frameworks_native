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

#include <input/InputFlags.h>

#include <android-base/logging.h>
#include <com_android_input_flags.h>
#include <com_android_window_flags.h>
#include <cutils/properties.h>

#include <string>

namespace android {

bool InputFlags::connectedDisplaysCursorEnabled() {
    if (!com::android::window::flags::enable_desktop_mode_through_dev_option()) {
        return com::android::input::flags::connected_displays_cursor();
    }
    static std::optional<bool> cachedDevOption;
    if (!cachedDevOption.has_value()) {
        char value[PROPERTY_VALUE_MAX];
        constexpr static auto sysprop_name = "persist.wm.debug.desktop_experience_devopts";
        const int devOptionEnabled =
                property_get(sysprop_name, value, nullptr) > 0 ? std::atoi(value) : 0;
        cachedDevOption = devOptionEnabled == 1;
    }
    if (cachedDevOption.value_or(false)) {
        return true;
    }
    return com::android::input::flags::connected_displays_cursor();
}

bool InputFlags::connectedDisplaysCursorAndAssociatedDisplayCursorBugfixEnabled() {
    return connectedDisplaysCursorEnabled() &&
            com::android::input::flags::connected_displays_associated_display_cursor_bugfix();
}

} // namespace android