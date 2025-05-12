// Copyright (C) 2024 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

use super::{ErrorCode, Surface};
use nativewindow_bindgen::{AHardwareBuffer_Format, ANativeWindow_Buffer};
use std::ptr::null_mut;

/// An empty `ANativeWindow_Buffer`.
pub const EMPTY: ANativeWindow_Buffer = ANativeWindow_Buffer {
    width: 0,
    height: 0,
    stride: 0,
    format: 0,
    bits: null_mut(),
    reserved: [0; 6],
};

/// Rust wrapper for `ANativeWindow_Buffer`, representing a locked buffer from a [`Surface`].
pub struct Buffer<'a> {
    /// The wrapped `ANativeWindow_Buffer`.
    pub buffer: ANativeWindow_Buffer,
    surface: &'a mut Surface,
}

impl<'a> Buffer<'a> {
    pub(crate) fn new(buffer: ANativeWindow_Buffer, surface: &'a mut Surface) -> Self {
        Self { buffer, surface }
    }

    /// Unlocks the window's drawing surface which was previously locked to create this buffer,
    /// posting the buffer to the display.
    pub fn unlock_and_post(self) -> Result<(), ErrorCode> {
        self.surface.unlock_and_post()
    }

    /// The number of pixels that are shown horizontally.
    pub fn width(&self) -> i32 {
        self.buffer.width
    }

    /// The number of pixels that are shown vertically.
    pub fn height(&self) -> i32 {
        self.buffer.height
    }

    /// The number of pixels that a line in the buffer takes in memory.
    ///
    /// This may be greater than the width.
    pub fn stride(&self) -> i32 {
        self.buffer.stride
    }

    /// The pixel format of the buffer.
    pub fn format(&self) -> Result<AHardwareBuffer_Format::Type, ErrorCode> {
        self.buffer.format.try_into().map_err(|_| ErrorCode(self.buffer.format))
    }
}
