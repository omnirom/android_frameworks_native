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

//! Implementation of the `ISerialManager` AIDL interface. It can be used to list, listen to and
//! open serial ports.

use android_hardware_serialservice::aidl::android::hardware::serialservice::{
    ISerialManager::ISerialManagerAsyncServer, ISerialPortListener::ISerialPortListener,
    SerialPortInfo::SerialPortInfo,
};
use android_hardware_serialservice::binder;
use async_trait::async_trait;
use binder::{
    DeathRecipient, ExceptionCode, ParcelFileDescriptor, Result, SpIBinder, Status, Strong,
    ThreadState,
};
use futures::StreamExt;
use nix::libc;
use rustutils::users::{AID_ROOT, AID_SYSTEM};
use std::collections::{BTreeMap, HashMap};
use std::fs::OpenOptions;
use std::io::Write;
use std::os::fd::AsRawFd;
use std::os::unix::fs::OpenOptionsExt;
use std::path::Path;
use std::sync::{Arc, Mutex};
use ueventd::device_node::watcher::Watcher;

use crate::device_events_handler::{DeviceEventCallback, DeviceEventsHandler};
use crate::driver_type_finder::{DriverTypeFinder, DriverTypeFinderImpl};

// This function is wrapped in a module because ioctl_none_bad! macro generates a `pub` function.
mod raw {
    use nix::ioctl_none_bad;
    use nix::libc;

    // Puts the terminal into exclusive mode.
    // See also: https://man7.org/linux/man-pages/man2/TIOCEXCL.2const.html
    ioctl_none_bad!(tiocexcl, libc::TIOCEXCL);
    // Disable exclusive mode.
    ioctl_none_bad!(tiocnxcl, libc::TIOCNXCL);
}

/// The `ISerialManager` implementation.
#[derive(Clone)]
pub struct SerialManager {
    serial_ports: Arc<Mutex<HashMap<String, SerialPortInfo>>>,
    listeners: Arc<Mutex<BTreeMap<SpIBinder, ListenerEntry>>>,
}

struct ListenerEntry {
    listener: Strong<dyn ISerialPortListener>,
    _death_recipient: DeathRecipient,
}

impl SerialManager {
    /// Creates an instance of `SerialManager` and starts `DeviceEventsHandler`.
    pub async fn new() -> Self {
        let instance = SerialManager {
            serial_ports: Mutex::new(HashMap::new()).into(),
            listeners: Mutex::new(BTreeMap::new()).into(),
        };
        let (mut watcher, stream) = Watcher::new().await.expect("failed to watch /dev");
        tokio::spawn(async move {
            watcher.run_event_loop().await;
        });
        DeviceEventsHandler::start_new(
            stream.boxed(),
            Box::new(instance.clone()) as Box<dyn DeviceEventCallback + Send>,
            Arc::new(Mutex::new(DriverTypeFinderImpl::new()))
                as Arc<Mutex<dyn DriverTypeFinder + Send>>,
        )
        .await;
        instance
    }
}

impl binder::Interface for SerialManager {
    fn dump(
        &self,
        file: &mut dyn Write,
        _: &[&std::ffi::CStr],
    ) -> std::result::Result<(), binder::StatusCode> {
        let serial_port_map = self.serial_ports.lock().unwrap();
        write(file, format!("Has {} port(s).\n", serial_port_map.len()))?;
        for port in serial_port_map.values() {
            write(file, format!("Port {}:\n", port.name))?;
            write(file, format!("   Subsystem: {}\n", port.subsystem))?;
            write(file, format!("   Driver Type: {}\n", port.driverType))?;
            write(file, format!("   Vendor ID: {}\n", port.vendorId))?;
            write(file, format!("   Product ID: {}\n", port.productId))?;
        }
        Ok(())
    }
}

fn write(file: &mut dyn Write, message: String) -> std::result::Result<(), binder::StatusCode> {
    file.write_all(message.as_bytes()).map_err(|_| binder::StatusCode::PERMISSION_DENIED)
}

#[async_trait]
impl ISerialManagerAsyncServer for SerialManager {
    async fn getSerialPorts(&self) -> Result<Vec<SerialPortInfo>> {
        let serial_ports_map = self.serial_ports.lock().unwrap();
        Ok(serial_ports_map.values().cloned().collect())
    }

    async fn registerSerialPortListener(
        &self,
        listener: &Strong<dyn ISerialPortListener>,
    ) -> Result<()> {
        check_permissions()?;
        let binder = listener.as_binder();
        let binder_clone = binder.clone();
        let death_recipient = {
            let weak_listeners = Arc::downgrade(&self.listeners);
            DeathRecipient::new(move || {
                weak_listeners
                    .upgrade()
                    .map(|listeners| listeners.lock().unwrap().remove(&binder_clone));
            })
        };
        let entry = ListenerEntry { listener: listener.clone(), _death_recipient: death_recipient };
        if self.listeners.lock().unwrap().insert(binder, entry).is_some() {
            return Err(Status::new_service_specific_error_str(-1, Some("Duplicate listener")));
        }
        Ok(())
    }

    async fn unregisterSerialPortListener(
        &self,
        listener: &Strong<dyn ISerialPortListener>,
    ) -> Result<()> {
        check_permissions()?;
        let binder = listener.as_binder();
        (self.listeners.lock().unwrap())
            .remove(&binder)
            .ok_or(Status::new_service_specific_error_str(-1, Some("Listener not found")))?;
        Ok(())
    }

    async fn requestOpen(
        &self,
        port_name: &str,
        flags: i32,
        exclusive: bool,
    ) -> Result<ParcelFileDescriptor> {
        check_permissions()?;
        if !self.serial_ports.lock().unwrap().contains_key(port_name) {
            return Err(Status::new_exception_str(
                ExceptionCode::ILLEGAL_ARGUMENT,
                Some(format!("port_name {} does not exist", port_name)),
            ));
        };
        let path = Path::new("/dev").join(port_name);
        match OpenOptions::new()
            .read(flags & (libc::O_RDONLY | libc::O_RDWR) != 0)
            .write(flags & (libc::O_WRONLY | libc::O_RDWR) != 0)
            // Always set O_NOCTTY flag to prevent controlling this process by another process.
            .custom_flags(flags | libc::O_NOCTTY)
            .open(&path)
        {
            Ok(file) => {
                // SAFETY:
                // The file descriptor is valid as it comes from the File that we just opened.
                if let Err(e) = unsafe {
                    if exclusive {
                        raw::tiocexcl(file.as_raw_fd())
                    } else {
                        raw::tiocnxcl(file.as_raw_fd())
                    }
                } {
                    return Err(Status::new_exception_str(
                        ExceptionCode::SERVICE_SPECIFIC,
                        Some(format!("ioctl() failed, errno={}", e as i32)),
                    ));
                }
                Ok(ParcelFileDescriptor::new(file))
            }
            Err(e) => {
                return Err(Status::new_exception_str(
                    ExceptionCode::SERVICE_SPECIFIC,
                    Some(format!("open() failed, errno={}", e.raw_os_error().unwrap_or(0))),
                ));
            }
        }
    }
}

impl DeviceEventCallback for SerialManager {
    fn on_device_added(&mut self, info: SerialPortInfo) {
        self.serial_ports.lock().unwrap().insert(info.name.clone(), info.clone());
        for listener_entry in self.listeners.lock().unwrap().values() {
            listener_entry.listener.onSerialPortConnected(&info).unwrap_or_else(|error| {
                log::warn!("Error notifying listener: {error:?}");
            });
        }
    }

    fn on_device_removed(&mut self, name: &str) {
        let Some(info) = self.serial_ports.lock().unwrap().remove(name) else {
            return;
        };
        for listener_entry in self.listeners.lock().unwrap().values() {
            listener_entry.listener.onSerialPortDisconnected(&info).unwrap_or_else(|error| {
                log::warn!("Error notifying listener: {error:?}");
            });
        }
    }
}

fn check_permissions() -> Result<()> {
    let calling_uid = ThreadState::get_calling_uid();
    // This should only be called by system server, or root while testing
    if calling_uid != AID_SYSTEM && calling_uid != AID_ROOT {
        Err(Status::new_exception(ExceptionCode::SECURITY, None))
    } else {
        Ok(())
    }
}
