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

#define LOG_TAG "VirtualInputDevice"

#include <android-base/logging.h>
#include <android/input.h>
#include <android/keycodes.h>
#include <android_companion_virtualdevice_flags.h>
#include <fcntl.h>
#include <input/Input.h>
#include <input/VirtualInputDevice.h>
#include <linux/uinput.h>

#include <string>

using android::base::unique_fd;

namespace {

/**
 * Log debug messages about native virtual input devices.
 * Enable this via "adb shell setprop log.tag.VirtualInputDevice DEBUG"
 */
bool isDebug() {
    return __android_log_is_loggable(ANDROID_LOG_DEBUG, LOG_TAG, ANDROID_LOG_INFO);
}

unique_fd invalidFd() {
    return unique_fd(-1);
}

} // namespace

namespace android {

namespace vd_flags = android::companion::virtualdevice::flags;

/** Creates a new uinput device and assigns a file descriptor. */
unique_fd openUinput(const char* readableName, int32_t vendorId, int32_t productId,
                     const char* phys, DeviceType deviceType, int32_t screenHeight,
                     int32_t screenWidth) {
    unique_fd fd(TEMP_FAILURE_RETRY(::open("/dev/uinput", O_WRONLY | O_NONBLOCK)));
    if (fd < 0) {
        ALOGE("Error creating uinput device: %s", strerror(errno));
        return invalidFd();
    }

    ioctl(fd, UI_SET_PHYS, phys);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_EVBIT, EV_SYN);
    switch (deviceType) {
        case DeviceType::DPAD:
            for (const auto& [_, keyCode] : VirtualDpad::DPAD_KEY_CODE_MAPPING) {
                ioctl(fd, UI_SET_KEYBIT, keyCode);
            }
            break;
        case DeviceType::KEYBOARD:
            for (const auto& [_, keyCode] : VirtualKeyboard::KEY_CODE_MAPPING) {
                ioctl(fd, UI_SET_KEYBIT, keyCode);
            }
            break;
        case DeviceType::MOUSE:
            ioctl(fd, UI_SET_EVBIT, EV_REL);
            ioctl(fd, UI_SET_KEYBIT, BTN_LEFT);
            ioctl(fd, UI_SET_KEYBIT, BTN_RIGHT);
            ioctl(fd, UI_SET_KEYBIT, BTN_MIDDLE);
            ioctl(fd, UI_SET_KEYBIT, BTN_BACK);
            ioctl(fd, UI_SET_KEYBIT, BTN_FORWARD);
            ioctl(fd, UI_SET_RELBIT, REL_X);
            ioctl(fd, UI_SET_RELBIT, REL_Y);
            ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
            ioctl(fd, UI_SET_RELBIT, REL_HWHEEL);
            if (vd_flags::high_resolution_scroll()) {
                ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
                ioctl(fd, UI_SET_RELBIT, REL_HWHEEL_HI_RES);
            }
            break;
        case DeviceType::TOUCHSCREEN:
            ioctl(fd, UI_SET_EVBIT, EV_ABS);
            ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_TOOL_TYPE);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
            ioctl(fd, UI_SET_ABSBIT, ABS_MT_PRESSURE);
            ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
            break;
        case DeviceType::STYLUS:
            ioctl(fd, UI_SET_EVBIT, EV_ABS);
            ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
            ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS);
            ioctl(fd, UI_SET_KEYBIT, BTN_STYLUS2);
            ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_PEN);
            ioctl(fd, UI_SET_KEYBIT, BTN_TOOL_RUBBER);
            ioctl(fd, UI_SET_ABSBIT, ABS_X);
            ioctl(fd, UI_SET_ABSBIT, ABS_Y);
            ioctl(fd, UI_SET_ABSBIT, ABS_TILT_X);
            ioctl(fd, UI_SET_ABSBIT, ABS_TILT_Y);
            ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);
            ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
            break;
        case DeviceType::ROTARY_ENCODER:
            ioctl(fd, UI_SET_EVBIT, EV_REL);
            ioctl(fd, UI_SET_RELBIT, REL_WHEEL);
            if (vd_flags::high_resolution_scroll()) {
                ioctl(fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
            }
            break;
        default:
            ALOGE("Invalid input device type %d", static_cast<int32_t>(deviceType));
            return invalidFd();
    }

    int version;
    if (ioctl(fd, UI_GET_VERSION, &version) == 0 && version >= 5) {
        uinput_setup setup;
        memset(&setup, 0, sizeof(setup));
        std::strncpy(setup.name, readableName, UINPUT_MAX_NAME_SIZE);
        setup.id.version = 1;
        setup.id.bustype = BUS_VIRTUAL;
        setup.id.vendor = vendorId;
        setup.id.product = productId;
        if (deviceType == DeviceType::TOUCHSCREEN) {
            uinput_abs_setup xAbsSetup;
            xAbsSetup.code = ABS_MT_POSITION_X;
            xAbsSetup.absinfo.maximum = screenWidth - 1;
            xAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &xAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput x axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup yAbsSetup;
            yAbsSetup.code = ABS_MT_POSITION_Y;
            yAbsSetup.absinfo.maximum = screenHeight - 1;
            yAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &yAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput y axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup majorAbsSetup;
            majorAbsSetup.code = ABS_MT_TOUCH_MAJOR;
            majorAbsSetup.absinfo.maximum = screenWidth - 1;
            majorAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &majorAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput major axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup pressureAbsSetup;
            pressureAbsSetup.code = ABS_MT_PRESSURE;
            pressureAbsSetup.absinfo.maximum = 255;
            pressureAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &pressureAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput pressure axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup slotAbsSetup;
            slotAbsSetup.code = ABS_MT_SLOT;
            slotAbsSetup.absinfo.maximum = MAX_POINTERS - 1;
            slotAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &slotAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput slots: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup trackingIdAbsSetup;
            trackingIdAbsSetup.code = ABS_MT_TRACKING_ID;
            trackingIdAbsSetup.absinfo.maximum = MAX_POINTERS - 1;
            trackingIdAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &trackingIdAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput tracking ids: %s", strerror(errno));
                return invalidFd();
            }
        } else if (deviceType == DeviceType::STYLUS) {
            uinput_abs_setup xAbsSetup;
            xAbsSetup.code = ABS_X;
            xAbsSetup.absinfo.maximum = screenWidth - 1;
            xAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &xAbsSetup) != 0) {
                ALOGE("Error creating stylus uinput x axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup yAbsSetup;
            yAbsSetup.code = ABS_Y;
            yAbsSetup.absinfo.maximum = screenHeight - 1;
            yAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &yAbsSetup) != 0) {
                ALOGE("Error creating stylus uinput y axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup tiltXAbsSetup;
            tiltXAbsSetup.code = ABS_TILT_X;
            tiltXAbsSetup.absinfo.maximum = 90;
            tiltXAbsSetup.absinfo.minimum = -90;
            if (ioctl(fd, UI_ABS_SETUP, &tiltXAbsSetup) != 0) {
                ALOGE("Error creating stylus uinput tilt x axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup tiltYAbsSetup;
            tiltYAbsSetup.code = ABS_TILT_Y;
            tiltYAbsSetup.absinfo.maximum = 90;
            tiltYAbsSetup.absinfo.minimum = -90;
            if (ioctl(fd, UI_ABS_SETUP, &tiltYAbsSetup) != 0) {
                ALOGE("Error creating stylus uinput tilt y axis: %s", strerror(errno));
                return invalidFd();
            }
            uinput_abs_setup pressureAbsSetup;
            pressureAbsSetup.code = ABS_PRESSURE;
            pressureAbsSetup.absinfo.maximum = 255;
            pressureAbsSetup.absinfo.minimum = 0;
            if (ioctl(fd, UI_ABS_SETUP, &pressureAbsSetup) != 0) {
                ALOGE("Error creating touchscreen uinput pressure axis: %s", strerror(errno));
                return invalidFd();
            }
        }
        if (ioctl(fd, UI_DEV_SETUP, &setup) != 0) {
            ALOGE("Error creating uinput device: %s", strerror(errno));
            return invalidFd();
        }
    } else {
        // UI_DEV_SETUP was not introduced until version 5. Try setting up manually.
        ALOGI("Falling back to version %d manual setup", version);
        uinput_user_dev fallback;
        memset(&fallback, 0, sizeof(fallback));
        std::strncpy(fallback.name, readableName, UINPUT_MAX_NAME_SIZE);
        fallback.id.version = 1;
        fallback.id.bustype = BUS_VIRTUAL;
        fallback.id.vendor = vendorId;
        fallback.id.product = productId;
        if (deviceType == DeviceType::TOUCHSCREEN) {
            fallback.absmin[ABS_MT_POSITION_X] = 0;
            fallback.absmax[ABS_MT_POSITION_X] = screenWidth - 1;
            fallback.absmin[ABS_MT_POSITION_Y] = 0;
            fallback.absmax[ABS_MT_POSITION_Y] = screenHeight - 1;
            fallback.absmin[ABS_MT_TOUCH_MAJOR] = 0;
            fallback.absmax[ABS_MT_TOUCH_MAJOR] = screenWidth - 1;
            fallback.absmin[ABS_MT_PRESSURE] = 0;
            fallback.absmax[ABS_MT_PRESSURE] = 255;
        } else if (deviceType == DeviceType::STYLUS) {
            fallback.absmin[ABS_X] = 0;
            fallback.absmax[ABS_X] = screenWidth - 1;
            fallback.absmin[ABS_Y] = 0;
            fallback.absmax[ABS_Y] = screenHeight - 1;
            fallback.absmin[ABS_TILT_X] = -90;
            fallback.absmax[ABS_TILT_X] = 90;
            fallback.absmin[ABS_TILT_Y] = -90;
            fallback.absmax[ABS_TILT_Y] = 90;
            fallback.absmin[ABS_PRESSURE] = 0;
            fallback.absmax[ABS_PRESSURE] = 255;
        }
        if (TEMP_FAILURE_RETRY(write(fd, &fallback, sizeof(fallback))) != sizeof(fallback)) {
            ALOGE("Error creating uinput device: %s", strerror(errno));
            return invalidFd();
        }
    }

    if (ioctl(fd, UI_DEV_CREATE) != 0) {
        ALOGE("Error creating uinput device: %s", strerror(errno));
        return invalidFd();
    }

    return fd;
}

VirtualInputDevice::VirtualInputDevice(unique_fd fd) : mFd(std::move(fd)) {}

VirtualInputDevice::~VirtualInputDevice() {
    ioctl(mFd, UI_DEV_DESTROY);
}

bool VirtualInputDevice::writeInputEvent(uint16_t type, uint16_t code, int32_t value,
                                         std::chrono::nanoseconds eventTime) {
    std::chrono::seconds seconds = std::chrono::duration_cast<std::chrono::seconds>(eventTime);
    std::chrono::microseconds microseconds =
            std::chrono::duration_cast<std::chrono::microseconds>(eventTime - seconds);
    struct input_event ev = {.type = type, .code = code, .value = value};
    ev.input_event_sec = static_cast<decltype(ev.input_event_sec)>(seconds.count());
    ev.input_event_usec = static_cast<decltype(ev.input_event_usec)>(microseconds.count());

    return TEMP_FAILURE_RETRY(write(mFd, &ev, sizeof(struct input_event))) == sizeof(ev);
}

/** Utility method to write keyboard key events or mouse/stylus button events. */
bool VirtualInputDevice::writeEvKeyEvent(int32_t androidCode, int32_t androidAction,
                                         const std::map<int, int>& evKeyCodeMapping,
                                         const std::map<int, UinputAction>& actionMapping,
                                         std::chrono::nanoseconds eventTime) {
    auto evKeyCodeIterator = evKeyCodeMapping.find(androidCode);
    if (evKeyCodeIterator == evKeyCodeMapping.end()) {
        ALOGE("Unsupported native EV keycode for android code %d", androidCode);
        return false;
    }
    auto actionIterator = actionMapping.find(androidAction);
    if (actionIterator == actionMapping.end()) {
        ALOGE("Unsupported native action for android action %d", androidAction);
        return false;
    }
    int32_t action = static_cast<int32_t>(actionIterator->second);
    uint16_t evKeyCode = static_cast<uint16_t>(evKeyCodeIterator->second);
    if (!writeInputEvent(EV_KEY, evKeyCode, action, eventTime)) {
        ALOGE("Failed to write native action %d and EV keycode %u.", action, evKeyCode);
        return false;
    }
    if (!writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime)) {
        ALOGE("Failed to write SYN_REPORT for EV_KEY event.");
        return false;
    }
    return true;
}

// --- VirtualKeyboard ---
const std::map<int, UinputAction> VirtualKeyboard::KEY_ACTION_MAPPING = {
        {AKEY_EVENT_ACTION_DOWN, UinputAction::PRESS},
        {AKEY_EVENT_ACTION_UP, UinputAction::RELEASE},
};

// Keycode mapping from https://source.android.com/devices/input/keyboard-devices
const std::map<int, int> VirtualKeyboard::KEY_CODE_MAPPING = {
        {AKEYCODE_0, KEY_0},
        {AKEYCODE_1, KEY_1},
        {AKEYCODE_2, KEY_2},
        {AKEYCODE_3, KEY_3},
        {AKEYCODE_4, KEY_4},
        {AKEYCODE_5, KEY_5},
        {AKEYCODE_6, KEY_6},
        {AKEYCODE_7, KEY_7},
        {AKEYCODE_8, KEY_8},
        {AKEYCODE_9, KEY_9},
        {AKEYCODE_A, KEY_A},
        {AKEYCODE_B, KEY_B},
        {AKEYCODE_C, KEY_C},
        {AKEYCODE_D, KEY_D},
        {AKEYCODE_E, KEY_E},
        {AKEYCODE_F, KEY_F},
        {AKEYCODE_G, KEY_G},
        {AKEYCODE_H, KEY_H},
        {AKEYCODE_I, KEY_I},
        {AKEYCODE_J, KEY_J},
        {AKEYCODE_K, KEY_K},
        {AKEYCODE_L, KEY_L},
        {AKEYCODE_M, KEY_M},
        {AKEYCODE_N, KEY_N},
        {AKEYCODE_O, KEY_O},
        {AKEYCODE_P, KEY_P},
        {AKEYCODE_Q, KEY_Q},
        {AKEYCODE_R, KEY_R},
        {AKEYCODE_S, KEY_S},
        {AKEYCODE_T, KEY_T},
        {AKEYCODE_U, KEY_U},
        {AKEYCODE_V, KEY_V},
        {AKEYCODE_W, KEY_W},
        {AKEYCODE_X, KEY_X},
        {AKEYCODE_Y, KEY_Y},
        {AKEYCODE_Z, KEY_Z},
        {AKEYCODE_GRAVE, KEY_GRAVE},
        {AKEYCODE_MINUS, KEY_MINUS},
        {AKEYCODE_EQUALS, KEY_EQUAL},
        {AKEYCODE_LEFT_BRACKET, KEY_LEFTBRACE},
        {AKEYCODE_RIGHT_BRACKET, KEY_RIGHTBRACE},
        {AKEYCODE_BACKSLASH, KEY_BACKSLASH},
        {AKEYCODE_SEMICOLON, KEY_SEMICOLON},
        {AKEYCODE_APOSTROPHE, KEY_APOSTROPHE},
        {AKEYCODE_COMMA, KEY_COMMA},
        {AKEYCODE_PERIOD, KEY_DOT},
        {AKEYCODE_SLASH, KEY_SLASH},
        {AKEYCODE_ALT_LEFT, KEY_LEFTALT},
        {AKEYCODE_ALT_RIGHT, KEY_RIGHTALT},
        {AKEYCODE_CTRL_LEFT, KEY_LEFTCTRL},
        {AKEYCODE_CTRL_RIGHT, KEY_RIGHTCTRL},
        {AKEYCODE_SHIFT_LEFT, KEY_LEFTSHIFT},
        {AKEYCODE_SHIFT_RIGHT, KEY_RIGHTSHIFT},
        {AKEYCODE_META_LEFT, KEY_LEFTMETA},
        {AKEYCODE_META_RIGHT, KEY_RIGHTMETA},
        {AKEYCODE_CAPS_LOCK, KEY_CAPSLOCK},
        {AKEYCODE_SCROLL_LOCK, KEY_SCROLLLOCK},
        {AKEYCODE_NUM_LOCK, KEY_NUMLOCK},
        {AKEYCODE_ENTER, KEY_ENTER},
        {AKEYCODE_TAB, KEY_TAB},
        {AKEYCODE_SPACE, KEY_SPACE},
        {AKEYCODE_DPAD_DOWN, KEY_DOWN},
        {AKEYCODE_DPAD_UP, KEY_UP},
        {AKEYCODE_DPAD_LEFT, KEY_LEFT},
        {AKEYCODE_DPAD_RIGHT, KEY_RIGHT},
        {AKEYCODE_MOVE_END, KEY_END},
        {AKEYCODE_MOVE_HOME, KEY_HOME},
        {AKEYCODE_PAGE_DOWN, KEY_PAGEDOWN},
        {AKEYCODE_PAGE_UP, KEY_PAGEUP},
        {AKEYCODE_DEL, KEY_BACKSPACE},
        {AKEYCODE_FORWARD_DEL, KEY_DELETE},
        {AKEYCODE_INSERT, KEY_INSERT},
        {AKEYCODE_ESCAPE, KEY_ESC},
        {AKEYCODE_BREAK, KEY_PAUSE},
        {AKEYCODE_F1, KEY_F1},
        {AKEYCODE_F2, KEY_F2},
        {AKEYCODE_F3, KEY_F3},
        {AKEYCODE_F4, KEY_F4},
        {AKEYCODE_F5, KEY_F5},
        {AKEYCODE_F6, KEY_F6},
        {AKEYCODE_F7, KEY_F7},
        {AKEYCODE_F8, KEY_F8},
        {AKEYCODE_F9, KEY_F9},
        {AKEYCODE_F10, KEY_F10},
        {AKEYCODE_F11, KEY_F11},
        {AKEYCODE_F12, KEY_F12},
        {AKEYCODE_BACK, KEY_BACK},
        {AKEYCODE_FORWARD, KEY_FORWARD},
        {AKEYCODE_NUMPAD_1, KEY_KP1},
        {AKEYCODE_NUMPAD_2, KEY_KP2},
        {AKEYCODE_NUMPAD_3, KEY_KP3},
        {AKEYCODE_NUMPAD_4, KEY_KP4},
        {AKEYCODE_NUMPAD_5, KEY_KP5},
        {AKEYCODE_NUMPAD_6, KEY_KP6},
        {AKEYCODE_NUMPAD_7, KEY_KP7},
        {AKEYCODE_NUMPAD_8, KEY_KP8},
        {AKEYCODE_NUMPAD_9, KEY_KP9},
        {AKEYCODE_NUMPAD_0, KEY_KP0},
        {AKEYCODE_NUMPAD_ADD, KEY_KPPLUS},
        {AKEYCODE_NUMPAD_SUBTRACT, KEY_KPMINUS},
        {AKEYCODE_NUMPAD_MULTIPLY, KEY_KPASTERISK},
        {AKEYCODE_NUMPAD_DIVIDE, KEY_KPSLASH},
        {AKEYCODE_NUMPAD_DOT, KEY_KPDOT},
        {AKEYCODE_NUMPAD_ENTER, KEY_KPENTER},
        {AKEYCODE_NUMPAD_EQUALS, KEY_KPEQUAL},
        {AKEYCODE_NUMPAD_COMMA, KEY_KPCOMMA},
        {AKEYCODE_LANGUAGE_SWITCH, KEY_LANGUAGE},
};

VirtualKeyboard::VirtualKeyboard(unique_fd fd) : VirtualInputDevice(std::move(fd)) {}

VirtualKeyboard::~VirtualKeyboard() {}

bool VirtualKeyboard::writeKeyEvent(int32_t androidKeyCode, int32_t androidAction,
                                    std::chrono::nanoseconds eventTime) {
    return writeEvKeyEvent(androidKeyCode, androidAction, KEY_CODE_MAPPING, KEY_ACTION_MAPPING,
                           eventTime);
}

// --- VirtualDpad ---
// Dpad keycode mapping from https://source.android.com/devices/input/keyboard-devices
const std::map<int, int> VirtualDpad::DPAD_KEY_CODE_MAPPING = {
        // clang-format off
        {AKEYCODE_DPAD_DOWN, KEY_DOWN},
        {AKEYCODE_DPAD_UP, KEY_UP},
        {AKEYCODE_DPAD_LEFT, KEY_LEFT},
        {AKEYCODE_DPAD_RIGHT, KEY_RIGHT},
        {AKEYCODE_DPAD_CENTER, KEY_SELECT},
        {AKEYCODE_BACK, KEY_BACK},
        // clang-format on
};

VirtualDpad::VirtualDpad(unique_fd fd) : VirtualInputDevice(std::move(fd)) {}

VirtualDpad::~VirtualDpad() {}

bool VirtualDpad::writeDpadKeyEvent(int32_t androidKeyCode, int32_t androidAction,
                                    std::chrono::nanoseconds eventTime) {
    return writeEvKeyEvent(androidKeyCode, androidAction, DPAD_KEY_CODE_MAPPING,
                           VirtualKeyboard::KEY_ACTION_MAPPING, eventTime);
}

// --- VirtualMouse ---
const std::map<int, UinputAction> VirtualMouse::BUTTON_ACTION_MAPPING = {
        {AMOTION_EVENT_ACTION_BUTTON_PRESS, UinputAction::PRESS},
        {AMOTION_EVENT_ACTION_BUTTON_RELEASE, UinputAction::RELEASE},
};

// Button code mapping from https://source.android.com/devices/input/touch-devices
const std::map<int, int> VirtualMouse::BUTTON_CODE_MAPPING = {
        // clang-format off
        {AMOTION_EVENT_BUTTON_PRIMARY, BTN_LEFT},
        {AMOTION_EVENT_BUTTON_SECONDARY, BTN_RIGHT},
        {AMOTION_EVENT_BUTTON_TERTIARY, BTN_MIDDLE},
        {AMOTION_EVENT_BUTTON_BACK, BTN_BACK},
        {AMOTION_EVENT_BUTTON_FORWARD, BTN_FORWARD},
        // clang-format on
};

VirtualMouse::VirtualMouse(unique_fd fd)
      : VirtualInputDevice(std::move(fd)),
        mAccumulatedHighResScrollX(0),
        mAccumulatedHighResScrollY(0) {}

VirtualMouse::~VirtualMouse() {}

bool VirtualMouse::writeButtonEvent(int32_t androidButtonCode, int32_t androidAction,
                                    std::chrono::nanoseconds eventTime) {
    return writeEvKeyEvent(androidButtonCode, androidAction, BUTTON_CODE_MAPPING,
                           BUTTON_ACTION_MAPPING, eventTime);
}

bool VirtualMouse::writeRelativeEvent(float relativeX, float relativeY,
                                      std::chrono::nanoseconds eventTime) {
    return writeInputEvent(EV_REL, REL_X, relativeX, eventTime) &&
            writeInputEvent(EV_REL, REL_Y, relativeY, eventTime) &&
            writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
}

bool VirtualMouse::writeScrollEvent(float xAxisMovement, float yAxisMovement,
                                    std::chrono::nanoseconds eventTime) {
    if (!vd_flags::high_resolution_scroll()) {
        return writeInputEvent(EV_REL, REL_HWHEEL, static_cast<int32_t>(xAxisMovement),
                               eventTime) &&
                writeInputEvent(EV_REL, REL_WHEEL, static_cast<int32_t>(yAxisMovement),
                                eventTime) &&
                writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
    }

    const auto highResScrollX =
            static_cast<int32_t>(xAxisMovement * kEvdevHighResScrollUnitsPerDetent);
    const auto highResScrollY =
            static_cast<int32_t>(yAxisMovement * kEvdevHighResScrollUnitsPerDetent);
    bool highResScrollResult =
            writeInputEvent(EV_REL, REL_HWHEEL_HI_RES, highResScrollX, eventTime) &&
            writeInputEvent(EV_REL, REL_WHEEL_HI_RES, highResScrollY, eventTime);
    if (!highResScrollResult) {
        return false;
    }

    // According to evdev spec, a high-resolution mouse needs to emit REL_WHEEL / REL_HWHEEL events
    // in addition to high-res scroll events. Regular scroll events can approximate high-res scroll
    // events, so we send a regular scroll event when the accumulated scroll motion reaches a detent
    // (single mouse wheel click).
    mAccumulatedHighResScrollX += highResScrollX;
    mAccumulatedHighResScrollY += highResScrollY;
    const int32_t scrollX = mAccumulatedHighResScrollX / kEvdevHighResScrollUnitsPerDetent;
    const int32_t scrollY = mAccumulatedHighResScrollY / kEvdevHighResScrollUnitsPerDetent;
    if (scrollX != 0) {
        if (!writeInputEvent(EV_REL, REL_HWHEEL, scrollX, eventTime)) {
            return false;
        }
        mAccumulatedHighResScrollX %= kEvdevHighResScrollUnitsPerDetent;
    }
    if (scrollY != 0) {
        if (!writeInputEvent(EV_REL, REL_WHEEL, scrollY, eventTime)) {
            return false;
        }
        mAccumulatedHighResScrollY %= kEvdevHighResScrollUnitsPerDetent;
    }

    return writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
}

// --- VirtualTouchscreen ---
const std::map<int, UinputAction> VirtualTouchscreen::TOUCH_ACTION_MAPPING = {
        {AMOTION_EVENT_ACTION_DOWN, UinputAction::PRESS},
        {AMOTION_EVENT_ACTION_UP, UinputAction::RELEASE},
        {AMOTION_EVENT_ACTION_MOVE, UinputAction::MOVE},
        {AMOTION_EVENT_ACTION_CANCEL, UinputAction::CANCEL},
};

// Tool type mapping from https://source.android.com/devices/input/touch-devices
const std::map<int, int> VirtualTouchscreen::TOOL_TYPE_MAPPING = {
        {AMOTION_EVENT_TOOL_TYPE_FINGER, MT_TOOL_FINGER},
        {AMOTION_EVENT_TOOL_TYPE_PALM, MT_TOOL_PALM},
};

VirtualTouchscreen::VirtualTouchscreen(unique_fd fd) : VirtualInputDevice(std::move(fd)) {}

VirtualTouchscreen::~VirtualTouchscreen() {}

bool VirtualTouchscreen::isValidPointerId(int32_t pointerId, UinputAction uinputAction) {
    if (pointerId < -1 || pointerId >= (int)MAX_POINTERS) {
        ALOGE("Virtual touch event has invalid pointer id %d; value must be between -1 and %zu",
              pointerId, MAX_POINTERS - 0);
        return false;
    }

    if (uinputAction == UinputAction::PRESS && mActivePointers.test(pointerId)) {
        ALOGE("Repetitive action DOWN event received on a pointer %d that is already down.",
              pointerId);
        return false;
    }
    if (uinputAction == UinputAction::RELEASE && !mActivePointers.test(pointerId)) {
        ALOGE("PointerId %d action UP received with no prior action DOWN on touchscreen %d.",
              pointerId, mFd.get());
        return false;
    }
    return true;
}

bool VirtualTouchscreen::writeTouchEvent(int32_t pointerId, int32_t toolType, int32_t action,
                                         float locationX, float locationY, float pressure,
                                         float majorAxisSize, std::chrono::nanoseconds eventTime) {
    auto actionIterator = TOUCH_ACTION_MAPPING.find(action);
    if (actionIterator == TOUCH_ACTION_MAPPING.end()) {
        return false;
    }
    UinputAction uinputAction = actionIterator->second;
    if (!isValidPointerId(pointerId, uinputAction)) {
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_MT_SLOT, pointerId, eventTime)) {
        return false;
    }
    auto toolTypeIterator = TOOL_TYPE_MAPPING.find(toolType);
    if (toolTypeIterator == TOOL_TYPE_MAPPING.end()) {
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_MT_TOOL_TYPE, static_cast<int32_t>(toolTypeIterator->second),
                         eventTime)) {
        return false;
    }
    if (uinputAction == UinputAction::PRESS && !handleTouchDown(pointerId, eventTime)) {
        return false;
    }
    if (uinputAction == UinputAction::RELEASE && !handleTouchUp(pointerId, eventTime)) {
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_MT_POSITION_X, locationX, eventTime)) {
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_MT_POSITION_Y, locationY, eventTime)) {
        return false;
    }
    if (!isnan(pressure)) {
        if (!writeInputEvent(EV_ABS, ABS_MT_PRESSURE, pressure, eventTime)) {
            return false;
        }
    }
    if (!isnan(majorAxisSize)) {
        if (!writeInputEvent(EV_ABS, ABS_MT_TOUCH_MAJOR, majorAxisSize, eventTime)) {
            return false;
        }
    }
    return writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
}

bool VirtualTouchscreen::handleTouchUp(int32_t pointerId, std::chrono::nanoseconds eventTime) {
    if (!writeInputEvent(EV_ABS, ABS_MT_TRACKING_ID, static_cast<int32_t>(-1), eventTime)) {
        return false;
    }
    // When a pointer is no longer in touch, remove the pointer id from the corresponding
    // entry in the unreleased touches map.
    mActivePointers.reset(pointerId);
    ALOGD_IF(isDebug(), "Pointer %d erased from the touchscreen %d", pointerId, mFd.get());

    // Only sends the BTN UP event when there's no pointers on the touchscreen.
    if (mActivePointers.none()) {
        if (!writeInputEvent(EV_KEY, BTN_TOUCH, static_cast<int32_t>(UinputAction::RELEASE),
                             eventTime)) {
            return false;
        }
        ALOGD_IF(isDebug(), "No pointers on touchscreen %d, BTN UP event sent.", mFd.get());
    }
    return true;
}

bool VirtualTouchscreen::handleTouchDown(int32_t pointerId, std::chrono::nanoseconds eventTime) {
    // When a new pointer is down on the touchscreen, add the pointer id in the corresponding
    // entry in the unreleased touches map.
    if (mActivePointers.none()) {
        // Only sends the BTN Down event when the first pointer on the touchscreen is down.
        if (!writeInputEvent(EV_KEY, BTN_TOUCH, static_cast<int32_t>(UinputAction::PRESS),
                             eventTime)) {
            return false;
        }
        ALOGD_IF(isDebug(), "First pointer %d down under touchscreen %d, BTN DOWN event sent",
                 pointerId, mFd.get());
    }

    mActivePointers.set(pointerId);
    ALOGD_IF(isDebug(), "Added pointer %d under touchscreen %d in the map", pointerId, mFd.get());
    if (!writeInputEvent(EV_ABS, ABS_MT_TRACKING_ID, static_cast<int32_t>(pointerId), eventTime)) {
        return false;
    }
    return true;
}

// --- VirtualStylus ---
const std::map<int, int> VirtualStylus::TOOL_TYPE_MAPPING = {
        {AMOTION_EVENT_TOOL_TYPE_STYLUS, BTN_TOOL_PEN},
        {AMOTION_EVENT_TOOL_TYPE_ERASER, BTN_TOOL_RUBBER},
};

// Button code mapping from https://source.android.com/devices/input/touch-devices
const std::map<int, int> VirtualStylus::BUTTON_CODE_MAPPING = {
        {AMOTION_EVENT_BUTTON_STYLUS_PRIMARY, BTN_STYLUS},
        {AMOTION_EVENT_BUTTON_STYLUS_SECONDARY, BTN_STYLUS2},
};

VirtualStylus::VirtualStylus(unique_fd fd)
      : VirtualInputDevice(std::move(fd)), mIsStylusDown(false) {}

VirtualStylus::~VirtualStylus() {}

bool VirtualStylus::writeMotionEvent(int32_t toolType, int32_t action, int32_t locationX,
                                     int32_t locationY, int32_t pressure, int32_t tiltX,
                                     int32_t tiltY, std::chrono::nanoseconds eventTime) {
    auto actionIterator = VirtualTouchscreen::TOUCH_ACTION_MAPPING.find(action);
    if (actionIterator == VirtualTouchscreen::TOUCH_ACTION_MAPPING.end()) {
        ALOGE("Unsupported action passed for stylus: %d.", action);
        return false;
    }
    UinputAction uinputAction = actionIterator->second;
    auto toolTypeIterator = TOOL_TYPE_MAPPING.find(toolType);
    if (toolTypeIterator == TOOL_TYPE_MAPPING.end()) {
        ALOGE("Unsupported tool type passed for stylus: %d.", toolType);
        return false;
    }
    uint16_t tool = static_cast<uint16_t>(toolTypeIterator->second);
    if (uinputAction == UinputAction::PRESS && !handleStylusDown(tool, eventTime)) {
        return false;
    }
    if (!mIsStylusDown) {
        ALOGE("Action UP or MOVE received with no prior action DOWN for stylus %d.", mFd.get());
        return false;
    }
    if (uinputAction == UinputAction::RELEASE && !handleStylusUp(tool, eventTime)) {
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_X, locationX, eventTime)) {
        ALOGE("Unsupported x-axis location passed for stylus: %d.", locationX);
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_Y, locationY, eventTime)) {
        ALOGE("Unsupported y-axis location passed for stylus: %d.", locationY);
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_TILT_X, tiltX, eventTime)) {
        ALOGE("Unsupported x-axis tilt passed for stylus: %d.", tiltX);
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_TILT_Y, tiltY, eventTime)) {
        ALOGE("Unsupported y-axis tilt passed for stylus: %d.", tiltY);
        return false;
    }
    if (!writeInputEvent(EV_ABS, ABS_PRESSURE, pressure, eventTime)) {
        ALOGE("Unsupported pressure passed for stylus: %d.", pressure);
        return false;
    }
    if (!writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime)) {
        ALOGE("Failed to write SYN_REPORT for stylus motion event.");
        return false;
    }
    return true;
}

bool VirtualStylus::writeButtonEvent(int32_t androidButtonCode, int32_t androidAction,
                                     std::chrono::nanoseconds eventTime) {
    return writeEvKeyEvent(androidButtonCode, androidAction, BUTTON_CODE_MAPPING,
                           VirtualMouse::BUTTON_ACTION_MAPPING, eventTime);
}

bool VirtualStylus::handleStylusDown(uint16_t tool, std::chrono::nanoseconds eventTime) {
    if (mIsStylusDown) {
        ALOGE("Repetitive action DOWN event received for a stylus that is already down.");
        return false;
    }
    if (!writeInputEvent(EV_KEY, tool, static_cast<int32_t>(UinputAction::PRESS), eventTime)) {
        ALOGE("Failed to write EV_KEY for stylus tool type: %u.", tool);
        return false;
    }
    if (!writeInputEvent(EV_KEY, BTN_TOUCH, static_cast<int32_t>(UinputAction::PRESS), eventTime)) {
        ALOGE("Failed to write BTN_TOUCH for stylus press.");
        return false;
    }
    mIsStylusDown = true;
    return true;
}

bool VirtualStylus::handleStylusUp(uint16_t tool, std::chrono::nanoseconds eventTime) {
    if (!writeInputEvent(EV_KEY, tool, static_cast<int32_t>(UinputAction::RELEASE), eventTime)) {
        ALOGE("Failed to write EV_KEY for stylus tool type: %u.", tool);
        return false;
    }
    if (!writeInputEvent(EV_KEY, BTN_TOUCH, static_cast<int32_t>(UinputAction::RELEASE),
                         eventTime)) {
        ALOGE("Failed to write BTN_TOUCH for stylus release.");
        return false;
    }
    mIsStylusDown = false;
    return true;
}

// --- VirtualRotaryEncoder ---
VirtualRotaryEncoder::VirtualRotaryEncoder(unique_fd fd)
      : VirtualInputDevice(std::move(fd)), mAccumulatedHighResScrollAmount(0) {}

VirtualRotaryEncoder::~VirtualRotaryEncoder() {}

bool VirtualRotaryEncoder::writeScrollEvent(float scrollAmount,
                                            std::chrono::nanoseconds eventTime) {
    if (!vd_flags::high_resolution_scroll()) {
        return writeInputEvent(EV_REL, REL_WHEEL, static_cast<int32_t>(scrollAmount), eventTime) &&
                writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
    }

    const auto highResScrollAmount =
            static_cast<int32_t>(scrollAmount * kEvdevHighResScrollUnitsPerDetent);
    if (!writeInputEvent(EV_REL, REL_WHEEL_HI_RES, highResScrollAmount, eventTime)) {
        return false;
    }

    // According to evdev spec, a high-resolution scroll device needs to emit REL_WHEEL / REL_HWHEEL
    // events in addition to high-res scroll events. Regular scroll events can approximate high-res
    // scroll events, so we send a regular scroll event when the accumulated scroll motion reaches a
    // detent (single wheel click).
    mAccumulatedHighResScrollAmount += highResScrollAmount;
    const int32_t scroll = mAccumulatedHighResScrollAmount / kEvdevHighResScrollUnitsPerDetent;
    if (scroll != 0) {
        if (!writeInputEvent(EV_REL, REL_WHEEL, scroll, eventTime)) {
            return false;
        }
        mAccumulatedHighResScrollAmount %= kEvdevHighResScrollUnitsPerDetent;
    }

    return writeInputEvent(EV_SYN, SYN_REPORT, 0, eventTime);
}

} // namespace android
