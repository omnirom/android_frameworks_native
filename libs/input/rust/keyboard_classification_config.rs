/*
 * Copyright 2024 The Android Open Source Project
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

use crate::input::KeyboardType;

// TODO(b/263559234): Categorize some of these to KeyboardType::None based on ability to produce
//  key events at all. (Requires setup allowing InputDevice to dynamically add/remove
//  KeyboardInputMapper based on blocklist and KeyEvents in case a KeyboardType::None device ends
//  up producing a key event)
pub static CLASSIFIED_DEVICES: &[(
    /* vendorId */ u16,
    /* productId */ u16,
    KeyboardType,
    /* is_finalized */ bool,
)] = &[
    // HP X4000 Wireless Mouse
    (0x03f0, 0xa407, KeyboardType::NonAlphabetic, true),
    // Microsoft Wireless Mobile Mouse 6000
    (0x045e, 0x0745, KeyboardType::NonAlphabetic, true),
    // Microsoft Surface Precision Mouse
    (0x045e, 0x0821, KeyboardType::NonAlphabetic, true),
    // Microsoft Pro IntelliMouse
    (0x045e, 0x082a, KeyboardType::NonAlphabetic, true),
    // Microsoft Bluetooth Mouse
    (0x045e, 0x082f, KeyboardType::NonAlphabetic, true),
    // Xbox One Elite Series 2 gamepad
    (0x045e, 0x0b05, KeyboardType::NonAlphabetic, true),
    // Logitech T400
    (0x046d, 0x4026, KeyboardType::NonAlphabetic, true),
    // Logitech M720 Triathlon (Unifying)
    (0x046d, 0x405e, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master 2S (Unifying)
    (0x046d, 0x4069, KeyboardType::NonAlphabetic, true),
    // Logitech M585 (Unifying)
    (0x046d, 0x406b, KeyboardType::NonAlphabetic, true),
    // Logitech MX Anywhere 2 (Unifying)
    (0x046d, 0x4072, KeyboardType::NonAlphabetic, true),
    // Logitech Pebble M350
    (0x046d, 0x4080, KeyboardType::NonAlphabetic, true),
    // Logitech T630 Ultrathin
    (0x046d, 0xb00d, KeyboardType::NonAlphabetic, true),
    // Logitech M558
    (0x046d, 0xb011, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master (Bluetooth)
    (0x046d, 0xb012, KeyboardType::NonAlphabetic, true),
    // Logitech MX Anywhere 2 (Bluetooth)
    (0x046d, 0xb013, KeyboardType::NonAlphabetic, true),
    // Logitech M720 Triathlon (Bluetooth)
    (0x046d, 0xb015, KeyboardType::NonAlphabetic, true),
    // Logitech M535
    (0x046d, 0xb016, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master / Anywhere 2 (Bluetooth)
    (0x046d, 0xb017, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master 2S (Bluetooth)
    (0x046d, 0xb019, KeyboardType::NonAlphabetic, true),
    // Logitech MX Anywhere 2S (Bluetooth)
    (0x046d, 0xb01a, KeyboardType::NonAlphabetic, true),
    // Logitech M585/M590 (Bluetooth)
    (0x046d, 0xb01b, KeyboardType::NonAlphabetic, true),
    // Logitech G603 Lightspeed Gaming Mouse (Bluetooth)
    (0x046d, 0xb01c, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master (Bluetooth)
    (0x046d, 0xb01e, KeyboardType::NonAlphabetic, true),
    // Logitech MX Anywhere 2 (Bluetooth)
    (0x046d, 0xb01f, KeyboardType::NonAlphabetic, true),
    // Logitech MX Master 3 (Bluetooth)
    (0x046d, 0xb023, KeyboardType::NonAlphabetic, true),
    // Logitech G604 Lightspeed Gaming Mouse (Bluetooth)
    (0x046d, 0xb024, KeyboardType::NonAlphabetic, true),
    // Logitech Spotlight Presentation Remote (Bluetooth)
    (0x046d, 0xb503, KeyboardType::NonAlphabetic, true),
    // Logitech R500 (Bluetooth)
    (0x046d, 0xb505, KeyboardType::NonAlphabetic, true),
    // Logitech M500s
    (0x046d, 0xc093, KeyboardType::NonAlphabetic, true),
    // Logitech Spotlight Presentation Remote (USB dongle)
    (0x046d, 0xc53e, KeyboardType::NonAlphabetic, true),
    // Elecom Enelo IR LED Mouse 350
    (0x056e, 0x0134, KeyboardType::NonAlphabetic, true),
    // Elecom EPRIM Blue LED 5 button mouse 228
    (0x056e, 0x0141, KeyboardType::NonAlphabetic, true),
    // Elecom Blue LED Mouse 203
    (0x056e, 0x0159, KeyboardType::NonAlphabetic, true),
    // Zebra LS2208 barcode scanner
    (0x05e0, 0x1200, KeyboardType::NonAlphabetic, true),
    // RDing FootSwitch1F1
    (0x0c45, 0x7403, KeyboardType::NonAlphabetic, true),
    // SteelSeries Sensei RAW Frost Blue
    (0x1038, 0x1369, KeyboardType::NonAlphabetic, true),
    // SteelSeries Rival 3 Wired
    (0x1038, 0x1824, KeyboardType::NonAlphabetic, true),
    // SteelSeries Rival 3 Wireless (USB dongle)
    (0x1038, 0x1830, KeyboardType::NonAlphabetic, true),
    // Yubico.com Yubikey
    (0x1050, 0x0010, KeyboardType::NonAlphabetic, true),
    // Yubico.com Yubikey 4 OTP+U2F+CCID
    (0x1050, 0x0407, KeyboardType::NonAlphabetic, true),
    // Lenovo USB-C Wired Compact Mouse
    (0x17ef, 0x6123, KeyboardType::NonAlphabetic, true),
    // Corsair Katar Pro Wireless (USB dongle)
    (0x1b1c, 0x1b94, KeyboardType::NonAlphabetic, true),
    // Corsair Katar Pro Wireless (Bluetooth)
    (0x1bae, 0x1b1c, KeyboardType::NonAlphabetic, true),
    // Kensington Pro Fit Full-size
    (0x1bcf, 0x08a0, KeyboardType::NonAlphabetic, true),
    // Huion HS64
    (0x256c, 0x006d, KeyboardType::NonAlphabetic, true),
    // XP-Pen Star G640
    (0x28bd, 0x0914, KeyboardType::NonAlphabetic, true),
    // XP-Pen Artist 12 Pro
    (0x28bd, 0x091f, KeyboardType::NonAlphabetic, true),
    // XP-Pen Deco mini7W
    (0x28bd, 0x0928, KeyboardType::NonAlphabetic, true),
];
