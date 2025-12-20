/*
 * Copyright (C) 2010, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.serialservice;

@RustDerive(Clone=true)
parcelable SerialPortInfo {
    /** Device name: the dev node name under /dev, e.g. ttyUSB0, ttyACM1. */
    String name;

    /** Subsystem of the device: "virtual", "platform" (built-in UARTs), "usb", "usb-serial". */
    String subsystem;

    /** The type of the driver of the device: "serial", "console", "system", "pty". */
    String driverType;

    /** Vendor ID of this serial port if it is a USB device, otherwise -1. */
    int vendorId;

    /** Product ID of this serial port if it is a USB device, otherwise -1. */
    int productId;
}
