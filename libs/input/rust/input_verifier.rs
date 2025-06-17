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

//! Contains the InputVerifier, used to validate a stream of input events.

use crate::ffi::RustPointerProperties;
use crate::input::{DeviceId, MotionAction, MotionButton, MotionFlags, Source, SourceClass};
use log::info;
use std::collections::HashMap;
use std::collections::HashSet;

/// Represents a movement or state change event from a pointer device. The Rust equivalent of the
/// C++ NotifyMotionArgs struct.
#[derive(Clone, Copy)]
pub struct NotifyMotionArgs<'a> {
    /// The ID of the device that emitted the event.
    pub device_id: DeviceId,

    /// The type of device that emitted the event.
    pub source: Source,

    /// The type of event that took place.
    pub action: MotionAction,

    /// The properties of each of the pointers involved in the event.
    pub pointer_properties: &'a [RustPointerProperties],

    /// Flags applied to the event.
    pub flags: MotionFlags,

    /// The set of buttons that were pressed at the time of the event.
    ///
    /// We allow DOWN events to include buttons in their state for which BUTTON_PRESS events haven't
    /// been sent yet. In that case, the DOWN should be immediately followed by BUTTON_PRESS events
    /// for those buttons, building up to a button state matching that of the DOWN. For example, if
    /// the user presses the primary and secondary buttons at exactly the same time, we'd expect
    /// this sequence:
    ///
    /// | Action         | Action button | Button state           |
    /// |----------------|---------------|------------------------|
    /// | `HOVER_EXIT`   | -             | -                      |
    /// | `DOWN`         | -             | `PRIMARY`, `SECONDARY` |
    /// | `BUTTON_PRESS` | `PRIMARY`     | `PRIMARY`              |
    /// | `BUTTON_PRESS` | `SECONDARY`   | `PRIMARY`, `SECONDARY` |
    /// | `MOVE`         | -             | `PRIMARY`, `SECONDARY` |
    pub button_state: MotionButton,
}

/// Verifies the properties of an event that should always be true, regardless of the current state.
fn verify_event(event: NotifyMotionArgs<'_>, verify_buttons: bool) -> Result<(), String> {
    let pointer_count = event.pointer_properties.len();
    if pointer_count < 1 {
        return Err(format!("Invalid {} event: no pointers", event.action));
    }
    match event.action {
        MotionAction::Down
        | MotionAction::HoverEnter
        | MotionAction::HoverExit
        | MotionAction::HoverMove
        | MotionAction::Up => {
            if pointer_count != 1 {
                return Err(format!(
                    "Invalid {} event: there are {} pointers in the event",
                    event.action, pointer_count
                ));
            }
        }

        MotionAction::Cancel => {
            if !event.flags.contains(MotionFlags::CANCELED) {
                return Err(format!(
                    "For ACTION_CANCEL, must set FLAG_CANCELED. Received flags: {:#?}",
                    event.flags
                ));
            }
        }

        MotionAction::PointerDown { action_index } | MotionAction::PointerUp { action_index } => {
            if action_index >= pointer_count {
                return Err(format!(
                    "Got {}, but event has {} pointer(s)",
                    event.action, pointer_count
                ));
            }
        }

        MotionAction::ButtonPress { action_button }
        | MotionAction::ButtonRelease { action_button } => {
            if verify_buttons {
                let button_count = action_button.iter().count();
                if button_count != 1 {
                    return Err(format!(
                        "Invalid {} event: must specify a single action button, not {} action \
                         buttons",
                        event.action, button_count
                    ));
                }
            }
        }

        _ => {}
    }
    Ok(())
}

/// Keeps track of the button state for a single device and verifies events against it.
#[derive(Default)]
struct ButtonVerifier {
    /// The current button state of the device.
    button_state: MotionButton,

    /// The set of "pending buttons", which were seen in the last DOWN event's button state but
    /// for which we haven't seen BUTTON_PRESS events yet (see [`NotifyMotionArgs::button_state`]).
    pending_buttons: MotionButton,
}

impl ButtonVerifier {
    pub fn process_event(&mut self, event: NotifyMotionArgs<'_>) -> Result<(), String> {
        if !self.pending_buttons.is_empty() {
            // We just saw a DOWN with some additional buttons in its state, so it should be
            // immediately followed by ButtonPress events for those buttons.
            match event.action {
                MotionAction::ButtonPress { action_button }
                    if self.pending_buttons.contains(action_button) =>
                {
                    self.pending_buttons -= action_button;
                }
                _ => {
                    return Err(format!(
                        "After DOWN event, expected BUTTON_PRESS event(s) for {:?}, but got {}",
                        self.pending_buttons, event.action
                    ));
                }
            }
        }
        let expected_state = match event.action {
            MotionAction::Down => {
                if self.button_state - event.button_state != MotionButton::empty() {
                    return Err(format!(
                        "DOWN event button state is missing {:?}",
                        self.button_state - event.button_state
                    ));
                }
                self.pending_buttons = event.button_state - self.button_state;
                // We've already checked that the state isn't missing any already-down buttons, and
                // extra buttons are valid on DOWN actions, so bypass the expected state check.
                event.button_state
            }
            MotionAction::ButtonPress { action_button } => {
                if self.button_state.contains(action_button) {
                    return Err(format!(
                        "Duplicate BUTTON_PRESS; button state already contains {action_button:?}"
                    ));
                }
                self.button_state | action_button
            }
            MotionAction::ButtonRelease { action_button } => {
                if !self.button_state.contains(action_button) {
                    return Err(format!(
                        "Invalid BUTTON_RELEASE; button state doesn't contain {action_button:?}",
                    ));
                }
                self.button_state - action_button
            }
            _ => self.button_state,
        };
        if event.button_state != expected_state {
            return Err(format!(
                "Expected {} button state to be {:?}, but was {:?}",
                event.action, expected_state, event.button_state
            ));
        }
        // DOWN events can have pending buttons, so don't update the state for them.
        if event.action != MotionAction::Down {
            self.button_state = event.button_state;
        }
        Ok(())
    }
}

/// The InputVerifier is used to validate a stream of input events.
pub struct InputVerifier {
    name: String,
    should_log: bool,
    verify_buttons: bool,
    touching_pointer_ids_by_device: HashMap<DeviceId, HashSet<i32>>,
    hovering_pointer_ids_by_device: HashMap<DeviceId, HashSet<i32>>,
    button_verifier_by_device: HashMap<DeviceId, ButtonVerifier>,
}

impl InputVerifier {
    /// Create a new InputVerifier.
    pub fn new(name: &str, should_log: bool, verify_buttons: bool) -> Self {
        logger::init(
            logger::Config::default()
                .with_tag_on_device("InputVerifier")
                .with_max_level(log::LevelFilter::Trace),
        );
        Self {
            name: name.to_owned(),
            should_log,
            verify_buttons,
            touching_pointer_ids_by_device: HashMap::new(),
            hovering_pointer_ids_by_device: HashMap::new(),
            button_verifier_by_device: HashMap::new(),
        }
    }

    /// Process a pointer movement event from an InputDevice.
    /// If the event is not valid, we return an error string that describes the issue.
    pub fn process_movement(&mut self, event: NotifyMotionArgs<'_>) -> Result<(), String> {
        if !event.source.is_from_class(SourceClass::Pointer) {
            // Skip non-pointer sources like MOUSE_RELATIVE for now
            return Ok(());
        }
        if self.should_log {
            info!(
                "Processing {} for device {:?} ({} pointer{}) on {}",
                event.action,
                event.device_id,
                event.pointer_properties.len(),
                if event.pointer_properties.len() == 1 { "" } else { "s" },
                self.name
            );
        }

        verify_event(event, self.verify_buttons)?;

        if self.verify_buttons {
            self.button_verifier_by_device
                .entry(event.device_id)
                .or_default()
                .process_event(event)?;
        }

        match event.action {
            MotionAction::Down => {
                if self.touching_pointer_ids_by_device.contains_key(&event.device_id) {
                    return Err(format!(
                        "{}: Invalid DOWN event - pointers already down for device {:?}: {:?}",
                        self.name, event.device_id, self.touching_pointer_ids_by_device
                    ));
                }
                let it = self.touching_pointer_ids_by_device.entry(event.device_id).or_default();
                it.insert(event.pointer_properties[0].id);
            }
            MotionAction::PointerDown { action_index } => {
                if !self.touching_pointer_ids_by_device.contains_key(&event.device_id) {
                    return Err(format!(
                        "{}: Received POINTER_DOWN but no pointers are currently down \
                        for device {:?}",
                        self.name, event.device_id
                    ));
                }
                let it = self.touching_pointer_ids_by_device.get_mut(&event.device_id).unwrap();
                if it.len() != event.pointer_properties.len() - 1 {
                    return Err(format!(
                        "{}: There are currently {} touching pointers, but the incoming \
                         POINTER_DOWN event has {}",
                        self.name,
                        it.len(),
                        event.pointer_properties.len()
                    ));
                }
                let pointer_id = event.pointer_properties[action_index].id;
                if it.contains(&pointer_id) {
                    return Err(format!(
                        "{}: Pointer with id={} already present found in the properties",
                        self.name, pointer_id
                    ));
                }
                it.insert(pointer_id);
            }
            MotionAction::Move => {
                if !self.ensure_touching_pointers_match(event.device_id, event.pointer_properties) {
                    return Err(format!(
                        "{}: ACTION_MOVE touching pointers don't match",
                        self.name
                    ));
                }
            }
            MotionAction::PointerUp { action_index } => {
                if !self.ensure_touching_pointers_match(event.device_id, event.pointer_properties) {
                    return Err(format!(
                        "{}: ACTION_POINTER_UP touching pointers don't match",
                        self.name
                    ));
                }
                let it = self.touching_pointer_ids_by_device.get_mut(&event.device_id).unwrap();
                let pointer_id = event.pointer_properties[action_index].id;
                it.remove(&pointer_id);
            }
            MotionAction::Up => {
                if !self.touching_pointer_ids_by_device.contains_key(&event.device_id) {
                    return Err(format!(
                        "{} Received ACTION_UP but no pointers are currently down for device {:?}",
                        self.name, event.device_id
                    ));
                }
                let it = self.touching_pointer_ids_by_device.get_mut(&event.device_id).unwrap();
                if it.len() != 1 {
                    return Err(format!(
                        "{}: Got ACTION_UP, but we have pointers: {:?} for device {:?}",
                        self.name, it, event.device_id
                    ));
                }
                let pointer_id = event.pointer_properties[0].id;
                if !it.contains(&pointer_id) {
                    return Err(format!(
                        "{}: Got ACTION_UP, but pointerId {} is not touching. Touching pointers:\
                        {:?} for device {:?}",
                        self.name, pointer_id, it, event.device_id
                    ));
                }
                self.touching_pointer_ids_by_device.remove(&event.device_id);
            }
            MotionAction::Cancel => {
                if !self.ensure_touching_pointers_match(event.device_id, event.pointer_properties) {
                    return Err(format!(
                        "{}: Got ACTION_CANCEL, but the pointers don't match. \
                        Existing pointers: {:?}",
                        self.name, self.touching_pointer_ids_by_device
                    ));
                }
                self.touching_pointer_ids_by_device.remove(&event.device_id);
            }
            /*
             * The hovering protocol currently supports a single pointer only, because we do not
             * have ACTION_HOVER_POINTER_ENTER or ACTION_HOVER_POINTER_EXIT.
             * Still, we are keeping the infrastructure here pretty general in case that is
             * eventually supported.
             */
            MotionAction::HoverEnter => {
                if self.hovering_pointer_ids_by_device.contains_key(&event.device_id) {
                    return Err(format!(
                        "{}: Invalid HOVER_ENTER event - pointers already hovering for device {:?}:\
                        {:?}",
                        self.name, event.device_id, self.hovering_pointer_ids_by_device
                    ));
                }
                let it = self.hovering_pointer_ids_by_device.entry(event.device_id).or_default();
                it.insert(event.pointer_properties[0].id);
            }
            MotionAction::HoverMove => {
                // For compatibility reasons, we allow HOVER_MOVE without a prior HOVER_ENTER.
                // If there was no prior HOVER_ENTER, just start a new hovering pointer.
                let it = self.hovering_pointer_ids_by_device.entry(event.device_id).or_default();
                it.insert(event.pointer_properties[0].id);
            }
            MotionAction::HoverExit => {
                if !self.hovering_pointer_ids_by_device.contains_key(&event.device_id) {
                    return Err(format!(
                        "{}: Invalid HOVER_EXIT event - no pointers are hovering for device {:?}",
                        self.name, event.device_id
                    ));
                }
                let pointer_id = event.pointer_properties[0].id;
                let it = self.hovering_pointer_ids_by_device.get_mut(&event.device_id).unwrap();
                it.remove(&pointer_id);

                if !it.is_empty() {
                    return Err(format!(
                        "{}: Removed hovering pointer {}, but pointers are still\
                               hovering for device {:?}: {:?}",
                        self.name, pointer_id, event.device_id, it
                    ));
                }
                self.hovering_pointer_ids_by_device.remove(&event.device_id);
            }
            _ => return Ok(()),
        }
        Ok(())
    }

    /// Notify the verifier that the device has been reset, which will cause the verifier to erase
    /// the current internal state for this device. Subsequent events from this device are expected
    //// to start a new gesture.
    pub fn reset_device(&mut self, device_id: DeviceId) {
        self.touching_pointer_ids_by_device.remove(&device_id);
        self.hovering_pointer_ids_by_device.remove(&device_id);
    }

    fn ensure_touching_pointers_match(
        &self,
        device_id: DeviceId,
        pointer_properties: &[RustPointerProperties],
    ) -> bool {
        let Some(pointers) = self.touching_pointer_ids_by_device.get(&device_id) else {
            return false;
        };

        if pointers.len() != pointer_properties.len() {
            return false;
        }

        for pointer_property in pointer_properties.iter() {
            let pointer_id = pointer_property.id;
            if !pointers.contains(&pointer_id) {
                return false;
            }
        }
        true
    }
}

#[cfg(test)]
mod tests {
    use crate::input::MotionButton;
    use crate::input_verifier::InputVerifier;
    use crate::input_verifier::NotifyMotionArgs;
    use crate::DeviceId;
    use crate::MotionAction;
    use crate::MotionFlags;
    use crate::RustPointerProperties;
    use crate::Source;

    const BASE_POINTER_PROPERTIES: [RustPointerProperties; 1] = [RustPointerProperties { id: 0 }];
    const BASE_EVENT: NotifyMotionArgs = NotifyMotionArgs {
        device_id: DeviceId(1),
        source: Source::Touchscreen,
        action: MotionAction::Down,
        pointer_properties: &BASE_POINTER_PROPERTIES,
        flags: MotionFlags::empty(),
        button_state: MotionButton::empty(),
    };
    const BASE_MOUSE_EVENT: NotifyMotionArgs =
        NotifyMotionArgs { source: Source::Mouse, ..BASE_EVENT };

    #[test]
    /**
     * Send a DOWN event with 2 pointers and ensure that it's marked as invalid.
     */
    fn bad_down_event() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ true, /*verify_buttons*/ true);
        let pointer_properties =
            Vec::from([RustPointerProperties { id: 0 }, RustPointerProperties { id: 1 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_err());
    }

    #[test]
    fn single_pointer_stream() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        let pointer_properties = Vec::from([RustPointerProperties { id: 0 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Move,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Up,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn two_pointer_stream() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        let pointer_properties = Vec::from([RustPointerProperties { id: 0 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        // POINTER 1 DOWN
        let two_pointer_properties =
            Vec::from([RustPointerProperties { id: 0 }, RustPointerProperties { id: 1 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::PointerDown { action_index: 1 },
                pointer_properties: &two_pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        // POINTER 0 UP
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::PointerUp { action_index: 0 },
                pointer_properties: &two_pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        // ACTION_UP for pointer id=1
        let pointer_1_properties = Vec::from([RustPointerProperties { id: 1 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Up,
                pointer_properties: &pointer_1_properties,
                ..BASE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn multi_device_stream() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(1),
                action: MotionAction::Down,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(1),
                action: MotionAction::Move,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(2),
                action: MotionAction::Down,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(2),
                action: MotionAction::Move,
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(1),
                action: MotionAction::Up,
                ..BASE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn action_cancel() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                flags: MotionFlags::empty(),
                ..BASE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Cancel,
                flags: MotionFlags::CANCELED,
                ..BASE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn invalid_action_cancel() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::Down, ..BASE_EVENT })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::Cancel, ..BASE_EVENT })
            .is_err());
    }

    #[test]
    fn invalid_up() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::Up, ..BASE_EVENT })
            .is_err());
    }

    #[test]
    fn correct_hover_sequence() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverEnter, ..BASE_EVENT })
            .is_ok());

        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverMove, ..BASE_EVENT })
            .is_ok());

        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverExit, ..BASE_EVENT })
            .is_ok());

        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverEnter, ..BASE_EVENT })
            .is_ok());
    }

    #[test]
    fn double_hover_enter() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverEnter, ..BASE_EVENT })
            .is_ok());

        assert!(verifier
            .process_movement(NotifyMotionArgs { action: MotionAction::HoverEnter, ..BASE_EVENT })
            .is_err());
    }

    // Send a MOVE without a preceding DOWN event. This is OK because it's from source
    // MOUSE_RELATIVE, which is used during pointer capture. The verifier should allow such event.
    #[test]
    fn relative_mouse_move() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                device_id: DeviceId(2),
                source: Source::MouseRelative,
                action: MotionAction::Move,
                ..BASE_EVENT
            })
            .is_ok());
    }

    // Send a MOVE event with incorrect number of pointers (one of the pointers is missing).
    #[test]
    fn move_with_wrong_number_of_pointers() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        let pointer_properties = Vec::from([RustPointerProperties { id: 0 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        // POINTER 1 DOWN
        let two_pointer_properties =
            Vec::from([RustPointerProperties { id: 0 }, RustPointerProperties { id: 1 }]);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::PointerDown { action_index: 1 },
                pointer_properties: &two_pointer_properties,
                ..BASE_EVENT
            })
            .is_ok());
        // MOVE event with 1 pointer missing (the pointer with id = 1). It should be rejected
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Move,
                pointer_properties: &pointer_properties,
                ..BASE_EVENT
            })
            .is_err());
    }

    #[test]
    fn correct_button_press() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn button_press_without_action_button() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::empty() },
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn button_press_with_multiple_action_buttons() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress {
                    action_button: MotionButton::Back | MotionButton::Forward
                },
                button_state: MotionButton::Back | MotionButton::Forward,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn button_press_without_action_button_in_state() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn button_release_with_action_button_in_state() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonRelease { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn nonbutton_action_with_button_state_change() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::HoverEnter,
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::HoverMove,
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn nonbutton_action_missing_button_state() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::HoverEnter,
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Back },
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::HoverMove,
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn up_without_button_release() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        // This UP event shouldn't change the button state; a BUTTON_RELEASE before it should.
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Up,
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn button_press_for_already_pressed_button() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Back },
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Back },
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn button_release_for_unpressed_button() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonRelease { action_button: MotionButton::Back },
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn correct_multiple_button_presses_without_down() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Back },
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Forward },
                button_state: MotionButton::Back | MotionButton::Forward,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn correct_down_with_button_press() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                button_state: MotionButton::Primary | MotionButton::Secondary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Secondary },
                button_state: MotionButton::Primary | MotionButton::Secondary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        // Also check that the MOVE afterwards is OK, as that's where errors would be raised if not
        // enough BUTTON_PRESSes were sent.
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Move,
                button_state: MotionButton::Primary | MotionButton::Secondary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
    }

    #[test]
    fn down_with_button_state_change_not_followed_by_button_press() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        // The DOWN event itself is OK, but it needs to be immediately followed by a BUTTON_PRESS.
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Move,
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn down_with_button_state_change_not_followed_by_enough_button_presses() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                button_state: MotionButton::Primary | MotionButton::Secondary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        // The DOWN event itself is OK, but it needs to be immediately followed by two
        // BUTTON_PRESSes, one for each button.
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Primary },
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Move,
                button_state: MotionButton::Primary,
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }

    #[test]
    fn down_missing_already_pressed_button() {
        let mut verifier =
            InputVerifier::new("Test", /*should_log*/ false, /*verify_buttons*/ true);
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::ButtonPress { action_button: MotionButton::Back },
                button_state: MotionButton::Back,
                ..BASE_MOUSE_EVENT
            })
            .is_ok());
        assert!(verifier
            .process_movement(NotifyMotionArgs {
                action: MotionAction::Down,
                button_state: MotionButton::empty(),
                ..BASE_MOUSE_EVENT
            })
            .is_err());
    }
}
