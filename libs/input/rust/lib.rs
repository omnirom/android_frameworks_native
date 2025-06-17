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

//! The rust component of libinput.

mod data_store;
mod input;
mod input_verifier;
mod keyboard_classification_config;
mod keyboard_classifier;

pub use data_store::{DataStore, DefaultFileReaderWriter};
pub use input::{
    DeviceClass, DeviceId, InputDevice, KeyboardType, ModifierState, MotionAction, MotionButton,
    MotionFlags, Source,
};
pub use input_verifier::{InputVerifier, NotifyMotionArgs};
pub use keyboard_classifier::KeyboardClassifier;

#[cxx::bridge(namespace = "android::input")]
#[allow(clippy::needless_maybe_sized)]
#[allow(unsafe_op_in_unsafe_fn)]
mod ffi {
    #[namespace = "android"]
    unsafe extern "C++" {
        include!("ffi/FromRustToCpp.h");
        fn shouldLog(tag: &str) -> bool;
    }

    #[namespace = "android::input::verifier"]
    extern "Rust" {
        /// Used to validate the incoming motion stream.
        /// This class is not thread-safe.
        /// State is stored in the "InputVerifier" object
        /// that can be created via the 'create' method.
        /// Usage:
        ///
        /// ```ignore
        /// Box<InputVerifier> verifier = create("inputChannel name");
        /// result = process_movement(verifier, ...);
        /// if (result) {
        ///    crash(result.error_message());
        /// }
        /// ```
        type InputVerifier;
        #[cxx_name = create]
        fn create_input_verifier(name: String, verify_buttons: bool) -> Box<InputVerifier>;
        #[allow(clippy::too_many_arguments)]
        fn process_movement(
            verifier: &mut InputVerifier,
            device_id: i32,
            source: u32,
            action: u32,
            action_button: u32,
            pointer_properties: &[RustPointerProperties],
            flags: u32,
            button_state: u32,
        ) -> String;
        fn reset_device(verifier: &mut InputVerifier, device_id: i32);
    }

    #[namespace = "android::input::keyboardClassifier"]
    extern "Rust" {
        /// Used to classify a keyboard into alphabetic and non-alphabetic
        type KeyboardClassifier;
        #[cxx_name = create]
        fn create_keyboard_classifier() -> Box<KeyboardClassifier>;
        #[cxx_name = notifyKeyboardChanged]
        fn notify_keyboard_changed(
            classifier: &mut KeyboardClassifier,
            device_id: i32,
            identifier: RustInputDeviceIdentifier,
            device_classes: u32,
        );
        #[cxx_name = getKeyboardType]
        fn get_keyboard_type(classifier: &mut KeyboardClassifier, device_id: i32) -> u32;
        #[cxx_name = isFinalized]
        fn is_finalized(classifier: &mut KeyboardClassifier, device_id: i32) -> bool;
        #[cxx_name = processKey]
        fn process_key(
            classifier: &mut KeyboardClassifier,
            device_id: i32,
            evdev_code: i32,
            modifier_state: u32,
        );
    }

    #[derive(Debug, Copy, Clone, Eq, PartialEq, Hash)]
    pub struct RustPointerProperties {
        pub id: i32,
    }

    #[derive(Debug)]
    pub struct RustInputDeviceIdentifier {
        pub name: String,
        pub location: String,
        pub unique_id: String,
        pub bus: u16,
        pub vendor: u16,
        pub product: u16,
        pub version: u16,
        pub descriptor: String,
    }
}

use crate::ffi::{RustInputDeviceIdentifier, RustPointerProperties};

fn create_input_verifier(name: String, verify_buttons: bool) -> Box<InputVerifier> {
    Box::new(InputVerifier::new(&name, ffi::shouldLog("InputVerifierLogEvents"), verify_buttons))
}

#[allow(clippy::too_many_arguments)]
fn process_movement(
    verifier: &mut InputVerifier,
    device_id: i32,
    source: u32,
    action: u32,
    action_button: u32,
    pointer_properties: &[RustPointerProperties],
    flags: u32,
    button_state: u32,
) -> String {
    let Some(converted_source) = Source::from_bits(source) else {
        panic!(
            "The conversion of source 0x{source:08x} failed, please check if some sources have not \
             been added to Source."
        );
    };
    let Some(motion_flags) = MotionFlags::from_bits(flags) else {
        panic!(
            "The conversion of flags 0x{:08x} failed, please check if some flags have not been \
            added to MotionFlags.",
            flags
        );
    };
    let Some(motion_action_button) = MotionButton::from_bits(action_button) else {
        panic!(
            "The conversion of action button 0x{action_button:08x} failed, please check if some \
             buttons need to be added to MotionButton."
        );
    };
    let Some(motion_button_state) = MotionButton::from_bits(button_state) else {
        panic!(
            "The conversion of button state 0x{button_state:08x} failed, please check if some \
             buttons need to be added to MotionButton."
        );
    };
    let motion_action = MotionAction::from_code(action, motion_action_button);
    if motion_action_button != MotionButton::empty() {
        match motion_action {
            MotionAction::ButtonPress { action_button: _ }
            | MotionAction::ButtonRelease { action_button: _ } => {}
            _ => {
                return format!(
                    "Invalid {motion_action} event: has action button {motion_action_button:?} but \
                     is not a button action"
                );
            }
        }
    }
    let result = verifier.process_movement(NotifyMotionArgs {
        device_id: DeviceId(device_id),
        source: converted_source,
        action: motion_action,
        pointer_properties,
        flags: motion_flags,
        button_state: motion_button_state,
    });
    match result {
        Ok(()) => "".to_string(),
        Err(e) => e,
    }
}

fn reset_device(verifier: &mut InputVerifier, device_id: i32) {
    verifier.reset_device(DeviceId(device_id));
}

fn create_keyboard_classifier() -> Box<KeyboardClassifier> {
    // Future design: Make this data store singleton by passing it to C++ side and making it global
    // and pass by reference to components that need to store persistent data.
    //
    // Currently only used by rust keyboard classifier so keeping it here.
    let data_store = DataStore::new(Box::new(DefaultFileReaderWriter::new(
        "/data/system/inputflinger-data.json".to_string(),
    )));
    Box::new(KeyboardClassifier::new(data_store))
}

fn notify_keyboard_changed(
    classifier: &mut KeyboardClassifier,
    device_id: i32,
    identifier: RustInputDeviceIdentifier,
    device_classes: u32,
) {
    let classes = DeviceClass::from_bits(device_classes);
    if classes.is_none() {
        panic!(
            "The conversion of device class 0x{:08x} failed, please check if some device classes
             have not been added to DeviceClass.",
            device_classes
        );
    }
    classifier.notify_keyboard_changed(InputDevice {
        device_id: DeviceId(device_id),
        identifier,
        classes: classes.unwrap(),
    });
}

fn get_keyboard_type(classifier: &mut KeyboardClassifier, device_id: i32) -> u32 {
    classifier.get_keyboard_type(DeviceId(device_id)) as u32
}

fn is_finalized(classifier: &mut KeyboardClassifier, device_id: i32) -> bool {
    classifier.is_finalized(DeviceId(device_id))
}

fn process_key(
    classifier: &mut KeyboardClassifier,
    device_id: i32,
    evdev_code: i32,
    meta_state: u32,
) {
    let modifier_state = ModifierState::from_bits(meta_state);
    if modifier_state.is_none() {
        panic!(
            "The conversion of meta state 0x{:08x} failed, please check if some meta state
             have not been added to ModifierState.",
            meta_state
        );
    }
    classifier.process_key(DeviceId(device_id), evdev_code, modifier_state.unwrap());
}

#[cfg(test)]
mod tests {
    use crate::create_input_verifier;
    use crate::process_movement;
    use crate::RustPointerProperties;

    const BASE_POINTER_PROPERTIES: [RustPointerProperties; 1] = [RustPointerProperties { id: 0 }];

    #[test]
    fn verify_nonbutton_action_with_action_button() {
        let mut verifier = create_input_verifier("Test".to_string(), /*verify_buttons*/ true);
        assert!(process_movement(
            &mut verifier,
            1,
            input_bindgen::AINPUT_SOURCE_MOUSE,
            input_bindgen::AMOTION_EVENT_ACTION_HOVER_ENTER,
            input_bindgen::AMOTION_EVENT_BUTTON_PRIMARY,
            &BASE_POINTER_PROPERTIES,
            0,
            0,
        )
        .contains("button action"));
    }

    #[test]
    fn verify_nonbutton_action_with_action_button_and_button_state() {
        let mut verifier = create_input_verifier("Test".to_string(), /*verify_buttons*/ true);
        assert!(process_movement(
            &mut verifier,
            1,
            input_bindgen::AINPUT_SOURCE_MOUSE,
            input_bindgen::AMOTION_EVENT_ACTION_HOVER_ENTER,
            input_bindgen::AMOTION_EVENT_BUTTON_PRIMARY,
            &BASE_POINTER_PROPERTIES,
            0,
            input_bindgen::AMOTION_EVENT_BUTTON_PRIMARY,
        )
        .contains("button action"));
    }
}
