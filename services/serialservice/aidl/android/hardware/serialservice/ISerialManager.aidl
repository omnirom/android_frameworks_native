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

package android.hardware.serialservice;

import android.hardware.serialservice.SerialPortInfo;
import android.hardware.serialservice.ISerialPortListener;

/** @hide */
interface ISerialManager {
    /** Returns a list of all available serial ports */
    List<SerialPortInfo> getSerialPorts();

    /** Registers a listener to monitor serial port connections and disconnections. */
    void registerSerialPortListener(in ISerialPortListener listener);

    /** Unregisters a previously registered listener. */
    void unregisterSerialPortListener(in ISerialPortListener listener);

    /**
     * Requests opening a file descriptor for the serial port.
     *
     * @param flags     flags for the Linux function {@code open(2)} call.
     *                  See https://man7.org/linux/man-pages/man2/open.2.html
     * @param exclusive whether the app needs exclusive access with TIOCEXCL(2const)
     * @return          the file descriptor of the pseudo-file.
     */
    ParcelFileDescriptor requestOpen(in String portName, in int flags, in boolean exclusive);
}
