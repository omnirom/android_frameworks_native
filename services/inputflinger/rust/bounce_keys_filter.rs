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

//! Bounce keys input filter implementation.
//! Bounce keys is an accessibility feature to aid users who have physical disabilities, that
//! allows the user to configure the device to ignore rapid, repeated key presses of the same key.
use crate::input_filter::{Filter, VIRTUAL_KEYBOARD_DEVICE_ID};

use android_hardware_input_common::aidl::android::hardware::input::common::Source::Source;
use com_android_server_inputflinger::aidl::com::android::server::inputflinger::{
    DeviceInfo::DeviceInfo, KeyEvent::KeyEvent, KeyEventAction::KeyEventAction,
};
use input::KeyboardType;
use log::debug;
use std::collections::{HashMap, HashSet};

#[derive(Debug)]
struct LastUpKeyEvent {
    keycode: i32,
    event_time: i64,
}

#[derive(Debug)]
struct BlockedEvent {
    device_id: i32,
    keycode: i32,
}

pub struct BounceKeysFilter {
    next: Box<dyn Filter + Send + Sync>,
    key_event_map: HashMap<i32, LastUpKeyEvent>,
    blocked_events: Vec<BlockedEvent>,
    supported_devices: HashSet<i32>,
    bounce_key_threshold_ns: i64,
}

impl BounceKeysFilter {
    /// Create a new BounceKeysFilter instance.
    pub fn new(
        next: Box<dyn Filter + Send + Sync>,
        bounce_key_threshold_ns: i64,
    ) -> BounceKeysFilter {
        Self {
            next,
            key_event_map: HashMap::new(),
            blocked_events: Vec::new(),
            supported_devices: HashSet::new(),
            bounce_key_threshold_ns,
        }
    }
}

impl Filter for BounceKeysFilter {
    fn notify_key(&mut self, event: &KeyEvent) {
        // Check if it is a supported device and event source contains Source::KEYBOARD
        if !(self.supported_devices.contains(&event.deviceId)
            && event.source.0 & Source::KEYBOARD.0 != 0)
        {
            self.next.notify_key(event);
            return;
        }
        match event.action {
            KeyEventAction::DOWN => match self.key_event_map.get(&event.deviceId) {
                None => self.next.notify_key(event),
                Some(last_up_event) => {
                    if event.keyCode == last_up_event.keycode
                        && event.eventTime < last_up_event.event_time + self.bounce_key_threshold_ns
                    {
                        self.blocked_events.push(BlockedEvent {
                            device_id: event.deviceId,
                            keycode: event.keyCode,
                        });
                        debug!("Event dropped because last up was too recent");
                    } else {
                        self.key_event_map.remove(&event.deviceId);
                        self.next.notify_key(event);
                    }
                }
            },
            KeyEventAction::UP => {
                self.key_event_map.insert(
                    event.deviceId,
                    LastUpKeyEvent { keycode: event.keyCode, event_time: event.eventTime },
                );
                if let Some(index) = self.blocked_events.iter().position(|blocked_event| {
                    blocked_event.device_id == event.deviceId
                        && blocked_event.keycode == event.keyCode
                }) {
                    self.blocked_events.remove(index);
                    debug!("Event dropped because key down was already dropped");
                } else {
                    self.next.notify_key(event);
                }
            }
            _ => (),
        }
    }

    fn notify_devices_changed(&mut self, device_infos: &[DeviceInfo]) {
        self.key_event_map.retain(|id, _| device_infos.iter().any(|x| *id == x.deviceId));
        self.blocked_events.retain(|blocked_event| {
            device_infos.iter().any(|x| blocked_event.device_id == x.deviceId)
        });
        self.supported_devices.clear();
        for device_info in device_infos {
            if device_info.deviceId == VIRTUAL_KEYBOARD_DEVICE_ID {
                continue;
            }
            if device_info.keyboardType == KeyboardType::None as i32 {
                continue;
            }
            // Support Alphabetic keyboards and Non-alphabetic external keyboards
            if device_info.external || device_info.keyboardType == KeyboardType::Alphabetic as i32 {
                self.supported_devices.insert(device_info.deviceId);
            }
        }
        self.next.notify_devices_changed(device_infos);
    }

    fn destroy(&mut self) {
        self.next.destroy();
    }

    fn dump(&mut self, dump_str: String) -> String {
        let mut result = "Bounce Keys filter: \n".to_string();
        result += &format!("\tthreshold = {:?}ns\n", self.bounce_key_threshold_ns);
        result += &format!("\tkey_event_map = {:?}\n", self.key_event_map);
        result += &format!("\tblocked_events = {:?}\n", self.blocked_events);
        result += &format!("\tsupported_devices = {:?}\n", self.supported_devices);
        self.next.dump(dump_str + &result)
    }
}

#[cfg(test)]
mod tests {
    use crate::bounce_keys_filter::BounceKeysFilter;
    use crate::input_filter::{test_filter::TestFilter, Filter, VIRTUAL_KEYBOARD_DEVICE_ID};
    use android_hardware_input_common::aidl::android::hardware::input::common::Source::Source;
    use com_android_server_inputflinger::aidl::com::android::server::inputflinger::{
        DeviceInfo::DeviceInfo, KeyEvent::KeyEvent, KeyEventAction::KeyEventAction,
    };
    use input::KeyboardType;

    static BASE_KEY_EVENT: KeyEvent = KeyEvent {
        id: 1,
        deviceId: 1,
        downTime: 0,
        readTime: 0,
        eventTime: 0,
        source: Source::KEYBOARD,
        displayId: 0,
        policyFlags: 0,
        action: KeyEventAction::DOWN,
        flags: 0,
        keyCode: 1,
        scanCode: 0,
        metaState: 0,
    };

    #[test]
    fn test_is_notify_key_for_external_keyboard() {
        let mut next = TestFilter::new();
        let mut filter = setup_filter_with_external_device(
            Box::new(next.clone()),
            1,   /* device_id */
            100, /* threshold */
            KeyboardType::Alphabetic,
        );

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        next.clear();
        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { eventTime: 100, action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { eventTime: 200, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_for_tv_remote() {
        let mut next = TestFilter::new();
        let mut filter = setup_filter_with_external_device(
            Box::new(next.clone()),
            1,   /* device_id */
            100, /* threshold */
            KeyboardType::NonAlphabetic,
        );

        let source = Source(Source::KEYBOARD.0 | Source::DPAD.0);
        let event = KeyEvent { action: KeyEventAction::DOWN, source, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::UP, source, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        next.clear();
        let event = KeyEvent { action: KeyEventAction::DOWN, source, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event =
            KeyEvent { eventTime: 100, action: KeyEventAction::UP, source, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event =
            KeyEvent { eventTime: 200, action: KeyEventAction::DOWN, source, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_blocks_for_internal_keyboard() {
        let mut next = TestFilter::new();
        let mut filter = setup_filter_with_internal_device(
            Box::new(next.clone()),
            1,   /* device_id */
            100, /* threshold */
            KeyboardType::Alphabetic,
        );

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        next.clear();
        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { eventTime: 100, action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { eventTime: 200, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_doesnt_block_for_internal_non_alphabetic_keyboard() {
        let next = TestFilter::new();
        let mut filter = setup_filter_with_internal_device(
            Box::new(next.clone()),
            1,   /* device_id */
            100, /* threshold */
            KeyboardType::NonAlphabetic,
        );

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_doesnt_block_for_virtual_keyboard() {
        let next = TestFilter::new();
        let mut filter = setup_filter_with_internal_device(
            Box::new(next.clone()),
            VIRTUAL_KEYBOARD_DEVICE_ID, /* device_id */
            100,                        /* threshold */
            KeyboardType::Alphabetic,
        );

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_doesnt_block_for_external_stylus() {
        let next = TestFilter::new();
        let mut filter = setup_filter_with_external_device(
            Box::new(next.clone()),
            1,   /* device_id */
            100, /* threshold */
            KeyboardType::NonAlphabetic,
        );

        let event =
            KeyEvent { action: KeyEventAction::DOWN, source: Source::STYLUS, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event =
            KeyEvent { action: KeyEventAction::UP, source: Source::STYLUS, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event =
            KeyEvent { action: KeyEventAction::DOWN, source: Source::STYLUS, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_for_multiple_external_keyboards() {
        let mut next = TestFilter::new();
        let mut filter = setup_filter_with_devices(
            Box::new(next.clone()),
            &[
                DeviceInfo {
                    deviceId: 1,
                    external: true,
                    keyboardType: KeyboardType::Alphabetic as i32,
                },
                DeviceInfo {
                    deviceId: 2,
                    external: true,
                    keyboardType: KeyboardType::Alphabetic as i32,
                },
            ],
            100, /* threshold */
        );

        // Bounce key scenario on the external keyboard
        let event = KeyEvent { deviceId: 1, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { deviceId: 1, action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        next.clear();
        let event = KeyEvent { deviceId: 1, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { deviceId: 2, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    #[test]
    fn test_is_notify_key_for_external_and_internal_alphabetic_keyboards() {
        let mut next = TestFilter::new();
        let mut filter = setup_filter_with_devices(
            Box::new(next.clone()),
            &[
                DeviceInfo {
                    deviceId: 1,
                    external: false,
                    keyboardType: KeyboardType::Alphabetic as i32,
                },
                DeviceInfo {
                    deviceId: 2,
                    external: true,
                    keyboardType: KeyboardType::Alphabetic as i32,
                },
            ],
            100, /* threshold */
        );

        // Bounce key scenario on the internal keyboard
        let event = KeyEvent { deviceId: 1, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        let event = KeyEvent { deviceId: 1, action: KeyEventAction::UP, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);

        next.clear();
        let event = KeyEvent { deviceId: 1, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert!(next.last_event().is_none());

        let event = KeyEvent { deviceId: 2, action: KeyEventAction::DOWN, ..BASE_KEY_EVENT };
        filter.notify_key(&event);
        assert_eq!(next.last_event().unwrap(), event);
    }

    fn setup_filter_with_external_device(
        next: Box<dyn Filter + Send + Sync>,
        device_id: i32,
        threshold: i64,
        keyboard_type: KeyboardType,
    ) -> BounceKeysFilter {
        setup_filter_with_devices(
            next,
            &[DeviceInfo {
                deviceId: device_id,
                external: true,
                keyboardType: keyboard_type as i32,
            }],
            threshold,
        )
    }

    fn setup_filter_with_internal_device(
        next: Box<dyn Filter + Send + Sync>,
        device_id: i32,
        threshold: i64,
        keyboard_type: KeyboardType,
    ) -> BounceKeysFilter {
        setup_filter_with_devices(
            next,
            &[DeviceInfo {
                deviceId: device_id,
                external: false,
                keyboardType: keyboard_type as i32,
            }],
            threshold,
        )
    }

    fn setup_filter_with_devices(
        next: Box<dyn Filter + Send + Sync>,
        devices: &[DeviceInfo],
        threshold: i64,
    ) -> BounceKeysFilter {
        let mut filter = BounceKeysFilter::new(next, threshold);
        filter.notify_devices_changed(devices);
        filter
    }
}
