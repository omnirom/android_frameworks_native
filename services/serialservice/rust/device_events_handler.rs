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

use android_hardware_serialservice::aidl::android::hardware::serialservice::SerialPortInfo::SerialPortInfo;
use anyhow::Result;
use futures::stream::BoxStream;
use std::sync::{Arc, Mutex};
use tokio::task::JoinHandle;
use tokio_stream::StreamExt;
use ueventd::device::{Device, FilesystemAttributeMap};
use ueventd::event::{DeviceEvent, DeviceType, EventType};

use crate::driver_type_finder::DriverTypeFinder;

#[mockall::automock]
pub trait DeviceEventCallback {
    fn on_device_added(&mut self, info: SerialPortInfo);
    fn on_device_removed(&mut self, name: &str);
}

/// Handles the stream of /dev events coming from ueventd Watcher.
pub struct DeviceEventsHandler {
    stream: BoxStream<'static, DeviceEvent>,
    callback: Box<dyn DeviceEventCallback + Send>,
    driver_type_finder: Arc<Mutex<dyn DriverTypeFinder + Send>>,
}

struct UsbDeviceId {
    vendor_id: i32,
    product_id: i32,
}

impl DeviceEventsHandler {
    pub async fn start_new(
        stream: BoxStream<'static, DeviceEvent>,
        callback: Box<dyn DeviceEventCallback + Send>,
        driver_type_finder: Arc<Mutex<dyn DriverTypeFinder + Send>>,
    ) -> JoinHandle<()> {
        let handler = DeviceEventsHandler { stream, callback, driver_type_finder };
        tokio::spawn(handler.run())
    }

    async fn run(mut self) {
        while let Some(device_event) = self.stream.next().await {
            self.handle_device_event(device_event).await;
        }
    }

    async fn handle_device_event(&mut self, device_event: DeviceEvent) {
        let DeviceType::DeviceNode { devnode_path } = device_event.device_type else {
            log::error!("unexpected device type {:?}", device_event.device_type);
            return;
        };
        let Some(name) = devnode_path.file_name() else {
            log::error!("no device name in {}", devnode_path.display());
            return;
        };
        let name = name.to_str().expect("Device paths should not have non-UTF-8 characters");
        match device_event.event_type {
            EventType::Add => {
                let Ok(driver_type) = ({
                    let driver_type_finder = self.driver_type_finder.lock().unwrap();
                    driver_type_finder.find(&devnode_path)
                }) else {
                    log::debug!("unsupported device type {}", devnode_path.display());
                    return;
                };
                // E.g. /sys/<device-dir>/device/subsystem -> .../bus/usb
                // If such a dir doesn't exist, we report "virtual" subsystem
                let subsystem_opt = device_event.device.device().and_then(|d| d.subsystem());
                let usb_device_id = UsbDeviceId::find_for_device(&device_event.device);
                let info = SerialPortInfo {
                    name: name.to_string(),
                    subsystem: subsystem_opt.unwrap_or("virtual".to_string()),
                    driverType: driver_type,
                    vendorId: usb_device_id.vendor_id,
                    productId: usb_device_id.product_id,
                };
                self.callback.on_device_added(info);
            }
            EventType::Remove => {
                self.callback.on_device_removed(name);
            }
            EventType::Change => {
                // We don't care about change events for now.
            }
        }
    }
}

impl UsbDeviceId {
    // Check each parent folder until we find idVendor file,
    // e.g. from /sys/devices/pci0000:00/0000:00:14.0/usb3/3-8/3-8:1.1/tty/ttyACM0
    // to /sys/devices/pci0000:00/0000:00:14.0/usb3/3-8
    fn find_for_device(device: &Device) -> Self {
        let subsystem = "usb";
        let mut current_opt = device.parent_with_subsystem(subsystem);
        while let Some(current) = current_opt {
            let attrs = current.sysattrs();
            let vendor_id = Self::read_hex_attr(&attrs, "idVendor");
            if vendor_id.is_ok() {
                let product_id = Self::read_hex_attr(&attrs, "idProduct");
                return Self {
                    vendor_id: vendor_id.unwrap(),
                    product_id: product_id.unwrap_or(-1),
                };
            }
            current_opt = current.parent_with_subsystem(subsystem);
        }
        Self { vendor_id: -1, product_id: -1 }
    }

    fn read_hex_attr(attrs: &FilesystemAttributeMap, name: &str) -> Result<i32> {
        let attr_value = attrs.get(name)?;
        let hex_value = attr_value.trim();
        Ok(i32::from_str_radix(hex_value, 16)?)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use anyhow::anyhow;
    use futures::StreamExt;
    use mockall::predicate::eq;
    use std::collections::HashMap;
    use std::path::Path;
    use ueventd::device::Device;
    use ueventd::mock_sysfs::{MockSysfs, SysfsFile};

    use crate::driver_type_finder::MockDriverTypeFinder;

    fn init_test_logging() {
        android_logger::init_once(
            android_logger::Config::default()
                .with_tag("device_events_handler_tests")
                .with_max_level(log::LevelFilter::Debug),
        );
    }

    #[tokio::test]
    async fn test_handle_add_serial_device() {
        init_test_logging();
        let (device, _sysfs_dir) = create_usb_device_in_mock_sysfs();
        let stream = tokio_stream::iter(vec![DeviceEvent {
            event_type: EventType::Add,
            device_type: DeviceType::DeviceNode {
                devnode_path: Path::new("/dev/ttyACM0").to_path_buf(),
            },
            device,
        }])
        .boxed();
        let mut callback = MockDeviceEventCallback::new();
        callback.expect_on_device_added().times(1).returning(|info| {
            assert_eq!(info.name, "ttyACM0".to_string());
            assert_eq!(info.subsystem, "usb".to_string());
            assert_eq!(info.driverType, "serial".to_string());
            assert_eq!(info.vendorId, 0x0694);
            assert_eq!(info.productId, 0x0009);
        });
        callback.expect_on_device_removed().never();
        let mut driver_type_finder = MockDriverTypeFinder::new();
        driver_type_finder
            .expect_find()
            .with(eq(Path::new("/dev/ttyACM0")))
            .times(1)
            .returning(|_| Ok("serial".to_string()));

        let handle = DeviceEventsHandler::start_new(
            stream,
            Box::new(callback),
            Arc::new(Mutex::new(driver_type_finder)) as Arc<Mutex<dyn DriverTypeFinder + Send>>,
        )
        .await;
        handle.await.unwrap();
    }

    #[tokio::test]
    async fn test_handle_add_device_without_usb_id() {
        init_test_logging();
        let (device, _sysfs_dir) = create_uart_device_in_mock_sysfs();
        let stream = tokio_stream::iter(vec![DeviceEvent {
            event_type: EventType::Add,
            device_type: DeviceType::DeviceNode {
                devnode_path: Path::new("/dev/ttyS0").to_path_buf(),
            },
            device,
        }])
        .boxed();
        let mut callback = MockDeviceEventCallback::new();
        callback.expect_on_device_added().times(1).returning(|info| {
            assert_eq!(info.name, "ttyS0".to_string());
            assert_eq!(info.subsystem, "serial-base".to_string());
            assert_eq!(info.driverType, "serial".to_string());
            assert_eq!(info.vendorId, -1);
            assert_eq!(info.productId, -1);
        });
        callback.expect_on_device_removed().never();
        let mut driver_type_finder = MockDriverTypeFinder::new();
        driver_type_finder
            .expect_find()
            .with(eq(Path::new("/dev/ttyS0")))
            .times(1)
            .returning(|_| Ok("serial".to_string()));

        let handle = DeviceEventsHandler::start_new(
            stream,
            Box::new(callback),
            Arc::new(Mutex::new(driver_type_finder)) as Arc<Mutex<dyn DriverTypeFinder + Send>>,
        )
        .await;
        handle.await.unwrap();
    }

    #[tokio::test]
    async fn test_handle_add_and_remove_serial_device() {
        init_test_logging();
        let (device, _sysfs_dir) = create_usb_device_in_mock_sysfs();
        let stream = tokio_stream::iter(vec![
            DeviceEvent {
                event_type: EventType::Add,
                device_type: DeviceType::DeviceNode {
                    devnode_path: Path::new("/dev/ttyACM0").to_path_buf(),
                },
                device: device.clone(),
            },
            DeviceEvent {
                event_type: EventType::Remove,
                device_type: DeviceType::DeviceNode {
                    devnode_path: Path::new("/dev/ttyACM0").to_path_buf(),
                },
                device,
            },
        ])
        .boxed();
        let mut callback = MockDeviceEventCallback::new();
        callback.expect_on_device_added().times(1).returning(|info| {
            assert_eq!(info.name, "ttyACM0".to_string());
            assert_eq!(info.subsystem, "usb".to_string());
            assert_eq!(info.driverType, "serial".to_string());
            assert_eq!(info.vendorId, 0x0694);
            assert_eq!(info.productId, 0x0009);
        });
        callback.expect_on_device_removed().times(1).returning(|name| {
            assert_eq!(name, "ttyACM0");
        });
        let mut driver_type_finder = MockDriverTypeFinder::new();
        driver_type_finder
            .expect_find()
            .with(eq(Path::new("/dev/ttyACM0")))
            .times(1)
            .returning(|_| Ok("serial".to_string()));

        let handle = DeviceEventsHandler::start_new(
            stream,
            Box::new(callback),
            Arc::new(Mutex::new(driver_type_finder)) as Arc<Mutex<dyn DriverTypeFinder + Send>>,
        )
        .await;
        handle.await.unwrap();
    }

    #[tokio::test]
    async fn test_handle_add_alien_device() {
        init_test_logging();
        let (device, _sysfs_dir) = create_usb_device_in_mock_sysfs();
        let stream = tokio_stream::iter(vec![DeviceEvent {
            event_type: EventType::Add,
            device_type: DeviceType::DeviceNode {
                devnode_path: Path::new("/dev/alien").to_path_buf(),
            },
            device,
        }])
        .boxed();
        let mut callback = MockDeviceEventCallback::new();
        callback.expect_on_device_added().never();
        callback.expect_on_device_removed().never();
        let mut driver_type_finder = MockDriverTypeFinder::new();
        driver_type_finder
            .expect_find()
            .with(eq(Path::new("/dev/alien")))
            .times(1)
            .returning(|_| Err(anyhow!("Driver type not found")));

        let handle = DeviceEventsHandler::start_new(
            stream,
            Box::new(callback),
            Arc::new(Mutex::new(driver_type_finder)) as Arc<Mutex<dyn DriverTypeFinder + Send>>,
        )
        .await;
        handle.await.unwrap();
    }

    fn create_usb_device_in_mock_sysfs() -> (Device, MockSysfs) {
        let sysfs = SysfsFile::Dir(HashMap::from([
            (
                "devices/pci0000:00/0000:00:14.0/usb3/3-8",
                SysfsFile::Dir(HashMap::from([
                    (
                        "3-8:1.1",
                        SysfsFile::Dir(HashMap::from([
                            (
                                "tty/ttyACM0",
                                SysfsFile::Dir(HashMap::from([
                                    (
                                        "device",
                                        SysfsFile::Dir(HashMap::from([
                                            (
                                                "subsystem",
                                                SysfsFile::Symlink(
                                                    "../../../../../../../../../bus/usb",
                                                ),
                                            ),
                                            ("uevent", SysfsFile::RegularFile("")),
                                        ])),
                                    ),
                                    (
                                        "subsystem",
                                        SysfsFile::Symlink("../../../../../../../../class/tty"),
                                    ),
                                    ("uevent", SysfsFile::RegularFile("")),
                                ])),
                            ),
                            ("subsystem", SysfsFile::Symlink("../../../../../../bus/usb")),
                            ("uevent", SysfsFile::RegularFile("")),
                        ])),
                    ),
                    ("subsystem", SysfsFile::Symlink("../../../../../bus/usb")),
                    ("idVendor", SysfsFile::RegularFile("0694\n")),
                    ("idProduct", SysfsFile::RegularFile("0009\n")),
                    ("uevent", SysfsFile::RegularFile("")),
                ])),
            ),
            ("bus/usb", SysfsFile::Dir(HashMap::new())),
            ("class/tty", SysfsFile::Dir(HashMap::new())),
        ]));
        let sysfs_dir = match MockSysfs::new(sysfs) {
            Ok(ms) => ms,
            Err(e) => panic!("Could not create mock sysfs: {}", e),
        };
        let sysfs_path =
            sysfs_dir.path().join("devices/pci0000:00/0000:00:14.0/usb3/3-8/3-8:1.1/tty/ttyACM0");
        let device = Device::with_root_and_syspath(sysfs_dir.path(), &sysfs_path).unwrap();
        (device, sysfs_dir)
    }

    fn create_uart_device_in_mock_sysfs() -> (Device, MockSysfs) {
        let sysfs = SysfsFile::Dir(HashMap::from([
            (
                "devices/pci0000:00/0000:00:1e.0/dw-apb-uart.6/dw-apb-uart.6:0/dw-apb-uart.6:0.0",
                SysfsFile::Dir(HashMap::from([
                    (
                        "tty/ttyS0",
                        SysfsFile::Dir(HashMap::from([
                            (
                                "device",
                                SysfsFile::Dir(HashMap::from([
                                    (
                                        "subsystem",
                                        SysfsFile::Symlink(
                                            "../../../../../../../../../bus/serial-base",
                                        ),
                                    ),
                                    ("uevent", SysfsFile::RegularFile("")),
                                ])),
                            ),
                            (
                                "subsystem",
                                SysfsFile::Symlink("../../../../../../../../../class/tty"),
                            ),
                            ("uevent", SysfsFile::RegularFile("")),
                        ])),
                    ),
                    ("subsystem", SysfsFile::Symlink("../../../../../../../bus/serial-base")),
                    ("uevent", SysfsFile::RegularFile("")),
                ])),
            ),
            ("bus/serial-base", SysfsFile::Dir(HashMap::new())),
            ("class/tty", SysfsFile::Dir(HashMap::new())),
        ]));
        let sysfs_dir = match MockSysfs::new(sysfs) {
            Ok(ms) => ms,
            Err(e) => panic!("Could not create mock sysfs: {}", e),
        };
        let sysfs_path = sysfs_dir.path().join(
            "devices/pci0000:00/0000:00:1e.0/dw-apb-uart.6/dw-apb-uart.6:0/dw-apb-uart.6:0.0/tty/ttyS0");
        let device = Device::with_root_and_syspath(sysfs_dir.path(), &sysfs_path).unwrap();
        (device, sysfs_dir)
    }
}
