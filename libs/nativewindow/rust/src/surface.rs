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

//! Rust wrapper for `ANativeWindow` and related types.

pub(crate) mod buffer;

use binder::{
    binder_impl::{BorrowedParcel, UnstructuredParcelable},
    impl_deserialize_for_unstructured_parcelable, impl_serialize_for_unstructured_parcelable,
    unstable_api::{status_result, AsNative},
    StatusCode,
};
use bitflags::bitflags;
use buffer::Buffer;
use nativewindow_bindgen::{
    ADataSpace, AHardwareBuffer_Format, ANativeWindow, ANativeWindow_acquire,
    ANativeWindow_getBuffersDataSpace, ANativeWindow_getBuffersDefaultDataSpace,
    ANativeWindow_getFormat, ANativeWindow_getHeight, ANativeWindow_getWidth, ANativeWindow_lock,
    ANativeWindow_readFromParcel, ANativeWindow_release, ANativeWindow_setBuffersDataSpace,
    ANativeWindow_setBuffersGeometry, ANativeWindow_setBuffersTransform,
    ANativeWindow_unlockAndPost, ANativeWindow_writeToParcel, ARect,
};
use std::error::Error;
use std::fmt::{self, Debug, Display, Formatter};
use std::ptr::{self, null_mut, NonNull};

/// Wrapper around an opaque C `ANativeWindow`.
#[derive(PartialEq, Eq)]
pub struct Surface(NonNull<ANativeWindow>);

impl Surface {
    /// Returns the current width in pixels of the window surface.
    pub fn width(&self) -> Result<u32, ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let width = unsafe { ANativeWindow_getWidth(self.0.as_ptr()) };
        width.try_into().map_err(|_| ErrorCode(width))
    }

    /// Returns the current height in pixels of the window surface.
    pub fn height(&self) -> Result<u32, ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let height = unsafe { ANativeWindow_getHeight(self.0.as_ptr()) };
        height.try_into().map_err(|_| ErrorCode(height))
    }

    /// Returns the current pixel format of the window surface.
    pub fn format(&self) -> Result<AHardwareBuffer_Format::Type, ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let format = unsafe { ANativeWindow_getFormat(self.0.as_ptr()) };
        format.try_into().map_err(|_| ErrorCode(format))
    }

    /// Changes the format and size of the window buffers.
    ///
    /// The width and height control the number of pixels in the buffers, not the dimensions of the
    /// window on screen. If these are different than the window's physical size, then its buffer
    /// will be scaled to match that size when compositing it to the screen. The width and height
    /// must be either both zero or both non-zero. If both are 0 then the window's base value will
    /// come back in force.
    pub fn set_buffers_geometry(
        &mut self,
        width: i32,
        height: i32,
        format: AHardwareBuffer_Format::Type,
    ) -> Result<(), ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let status = unsafe {
            ANativeWindow_setBuffersGeometry(
                self.0.as_ptr(),
                width,
                height,
                format.try_into().expect("Invalid format"),
            )
        };

        if status == 0 {
            Ok(())
        } else {
            Err(ErrorCode(status))
        }
    }

    /// Sets a transfom that will be applied to future buffers posted to the window.
    pub fn set_buffers_transform(&mut self, transform: Transform) -> Result<(), ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let status =
            unsafe { ANativeWindow_setBuffersTransform(self.0.as_ptr(), transform.bits() as i32) };

        if status == 0 {
            Ok(())
        } else {
            Err(ErrorCode(status))
        }
    }

    /// Sets the data space that will be applied to future buffers posted to the window.
    pub fn set_buffers_data_space(&mut self, data_space: ADataSpace) -> Result<(), ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let status = unsafe { ANativeWindow_setBuffersDataSpace(self.0.as_ptr(), data_space.0) };

        if status == 0 {
            Ok(())
        } else {
            Err(ErrorCode(status))
        }
    }

    /// Gets the data space of the buffers in the window.
    pub fn get_buffers_data_space(&mut self) -> Result<ADataSpace, ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let data_space = unsafe { ANativeWindow_getBuffersDataSpace(self.0.as_ptr()) };

        if data_space < 0 {
            Err(ErrorCode(data_space))
        } else {
            Ok(ADataSpace(data_space))
        }
    }

    /// Gets the default data space of the buffers in the window as set by the consumer.
    pub fn get_buffers_default_data_space(&mut self) -> Result<ADataSpace, ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let data_space = unsafe { ANativeWindow_getBuffersDefaultDataSpace(self.0.as_ptr()) };

        if data_space < 0 {
            Err(ErrorCode(data_space))
        } else {
            Ok(ADataSpace(data_space))
        }
    }

    /// Locks the window's next drawing surface for writing, and returns it.
    pub fn lock(&mut self, bounds: Option<&mut ARect>) -> Result<Buffer, ErrorCode> {
        let mut buffer = buffer::EMPTY;
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it. The other pointers must be valid because the come from
        // references, and aren't retained after the function returns.
        let status = unsafe {
            ANativeWindow_lock(
                self.0.as_ptr(),
                &mut buffer,
                bounds.map(ptr::from_mut).unwrap_or(null_mut()),
            )
        };
        if status != 0 {
            return Err(ErrorCode(status));
        }

        Ok(Buffer::new(buffer, self))
    }

    /// Unlocks the window's drawing surface which was previously locked, posting the new buffer to
    /// the display.
    ///
    /// This shouldn't be called directly but via the [`Buffer`], hence is not public here.
    fn unlock_and_post(&mut self) -> Result<(), ErrorCode> {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        let status = unsafe { ANativeWindow_unlockAndPost(self.0.as_ptr()) };
        if status == 0 {
            Ok(())
        } else {
            Err(ErrorCode(status))
        }
    }
}

impl Drop for Surface {
    fn drop(&mut self) {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        unsafe { ANativeWindow_release(self.0.as_ptr()) }
    }
}

impl Debug for Surface {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        f.debug_struct("Surface")
            .field("width", &self.width())
            .field("height", &self.height())
            .field("format", &self.format())
            .finish()
    }
}

impl Clone for Surface {
    fn clone(&self) -> Self {
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        unsafe { ANativeWindow_acquire(self.0.as_ptr()) };
        Self(self.0)
    }
}

impl UnstructuredParcelable for Surface {
    fn write_to_parcel(&self, parcel: &mut BorrowedParcel) -> Result<(), StatusCode> {
        let status =
        // SAFETY: The ANativeWindow pointer we pass is guaranteed to be non-null and valid because
        // it must have been allocated by `ANativeWindow_allocate` or `ANativeWindow_readFromParcel`
        // and we have not yet released it.
        unsafe { ANativeWindow_writeToParcel(self.0.as_ptr(), parcel.as_native_mut()) };
        status_result(status)
    }

    fn from_parcel(parcel: &BorrowedParcel) -> Result<Self, StatusCode> {
        let mut buffer = null_mut();

        let status =
        // SAFETY: Both pointers must be valid because they are obtained from references.
        // `ANativeWindow_readFromParcel` doesn't store them or do anything else special
        // with them. If it returns success then it will have allocated a new
        // `ANativeWindow` and incremented the reference count, so we can use it until we
        // release it.
            unsafe { ANativeWindow_readFromParcel(parcel.as_native(), &mut buffer) };

        status_result(status)?;

        Ok(Self(
            NonNull::new(buffer)
                .expect("ANativeWindow_readFromParcel returned success but didn't allocate buffer"),
        ))
    }
}

impl_deserialize_for_unstructured_parcelable!(Surface);
impl_serialize_for_unstructured_parcelable!(Surface);

// SAFETY: The underlying *ANativeWindow can be moved between threads.
unsafe impl Send for Surface {}

// SAFETY: The underlying *ANativeWindow can be used from multiple threads concurrently.
unsafe impl Sync for Surface {}

/// An error code returned by methods on [`Surface`].
#[derive(Copy, Clone, Debug, Eq, PartialEq)]
pub struct ErrorCode(i32);

impl Error for ErrorCode {}

impl Display for ErrorCode {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        write!(f, "Error {}", self.0)
    }
}

bitflags! {
    /// Transforms that can be applied to buffers as they are displayed to a window.
    #[derive(Copy, Clone, Debug, Eq, PartialEq)]
    pub struct Transform: u32 {
        const MIRROR_HORIZONTAL = 0x01;
        const MIRROR_VERTICAL = 0x02;
        const ROTATE_90 = 0x04;
    }
}

impl Transform {
    pub const IDENTITY: Self = Self::empty();
    pub const ROTATE_180: Self = Self::MIRROR_HORIZONTAL.union(Self::MIRROR_VERTICAL);
    pub const ROTATE_270: Self = Self::ROTATE_180.union(Self::ROTATE_90);
}
