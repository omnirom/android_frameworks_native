// Copyright (C) 2023 The Android Open Source Project
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

//! Pleasant Rust bindings for libnativewindow, including AHardwareBuffer

extern crate nativewindow_bindgen as ffi;

mod handle;
mod surface;

pub use ffi::{AHardwareBuffer_Format, AHardwareBuffer_UsageFlags};
pub use handle::NativeHandle;
pub use surface::{buffer::Buffer, Surface};

use binder::{
    binder_impl::{BorrowedParcel, UnstructuredParcelable},
    impl_deserialize_for_unstructured_parcelable, impl_serialize_for_unstructured_parcelable,
    unstable_api::{status_result, AsNative},
    StatusCode,
};
use ffi::{
    AHardwareBuffer, AHardwareBuffer_Desc, AHardwareBuffer_Plane, AHardwareBuffer_Planes,
    AHardwareBuffer_readFromParcel, AHardwareBuffer_writeToParcel, ARect,
};
use std::ffi::c_void;
use std::fmt::{self, Debug, Formatter};
use std::mem::{forget, ManuallyDrop};
use std::os::fd::{AsRawFd, BorrowedFd, FromRawFd, OwnedFd};
use std::ptr::{self, null, null_mut, NonNull};

/// Wrapper around a C `AHardwareBuffer_Desc`.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HardwareBufferDescription(AHardwareBuffer_Desc);

impl HardwareBufferDescription {
    /// Creates a new `HardwareBufferDescription` with the given parameters.
    pub fn new(
        width: u32,
        height: u32,
        layers: u32,
        format: AHardwareBuffer_Format::Type,
        usage: AHardwareBuffer_UsageFlags,
        stride: u32,
    ) -> Self {
        Self(AHardwareBuffer_Desc {
            width,
            height,
            layers,
            format,
            usage: usage.0,
            stride,
            rfu0: 0,
            rfu1: 0,
        })
    }

    /// Returns the width from the buffer description.
    pub fn width(&self) -> u32 {
        self.0.width
    }

    /// Returns the height from the buffer description.
    pub fn height(&self) -> u32 {
        self.0.height
    }

    /// Returns the number from layers from the buffer description.
    pub fn layers(&self) -> u32 {
        self.0.layers
    }

    /// Returns the format from the buffer description.
    pub fn format(&self) -> AHardwareBuffer_Format::Type {
        self.0.format
    }

    /// Returns the usage bitvector from the buffer description.
    pub fn usage(&self) -> AHardwareBuffer_UsageFlags {
        AHardwareBuffer_UsageFlags(self.0.usage)
    }

    /// Returns the stride from the buffer description.
    pub fn stride(&self) -> u32 {
        self.0.stride
    }
}

impl Default for HardwareBufferDescription {
    fn default() -> Self {
        Self(AHardwareBuffer_Desc {
            width: 0,
            height: 0,
            layers: 0,
            format: 0,
            usage: 0,
            stride: 0,
            rfu0: 0,
            rfu1: 0,
        })
    }
}

/// Wrapper around an opaque C `AHardwareBuffer`.
#[derive(PartialEq, Eq)]
pub struct HardwareBuffer(NonNull<AHardwareBuffer>);

impl HardwareBuffer {
    /// Test whether the given format and usage flag combination is allocatable.  If this function
    /// returns true, it means that a buffer with the given description can be allocated on this
    /// implementation, unless resource exhaustion occurs. If this function returns false, it means
    /// that the allocation of the given description will never succeed.
    ///
    /// Available since API 29
    pub fn is_supported(buffer_description: &HardwareBufferDescription) -> bool {
        // SAFETY: The pointer comes from a reference so must be valid.
        let status = unsafe { ffi::AHardwareBuffer_isSupported(&buffer_description.0) };

        status == 1
    }

    /// Allocates a buffer that matches the passed AHardwareBuffer_Desc. If allocation succeeds, the
    /// buffer can be used according to the usage flags specified in its description. If a buffer is
    /// used in ways not compatible with its usage flags, the results are undefined and may include
    /// program termination.
    ///
    /// Available since API level 26.
    #[inline]
    pub fn new(buffer_description: &HardwareBufferDescription) -> Option<Self> {
        let mut ptr = ptr::null_mut();
        // SAFETY: The returned pointer is valid until we drop/deallocate it. The function may fail
        // and return a status, but we check it later.
        let status = unsafe { ffi::AHardwareBuffer_allocate(&buffer_description.0, &mut ptr) };

        if status == 0 {
            Some(Self(NonNull::new(ptr).expect("Allocated AHardwareBuffer was null")))
        } else {
            None
        }
    }

    /// Creates a `HardwareBuffer` from a native handle.
    ///
    /// The native handle is cloned, so this doesn't take ownership of the original handle passed
    /// in.
    pub fn create_from_handle(
        handle: &NativeHandle,
        buffer_description: &HardwareBufferDescription,
    ) -> Result<Self, StatusCode> {
        let mut buffer = ptr::null_mut();
        // SAFETY: The caller guarantees that `handle` is valid, and the buffer pointer is valid
        // because it comes from a reference. The method we pass means that
        // `AHardwareBuffer_createFromHandle` will clone the handle rather than taking ownership of
        // it.
        let status = unsafe {
            ffi::AHardwareBuffer_createFromHandle(
                &buffer_description.0,
                handle.as_raw().as_ptr(),
                ffi::CreateFromHandleMethod_AHARDWAREBUFFER_CREATE_FROM_HANDLE_METHOD_CLONE
                    .try_into()
                    .unwrap(),
                &mut buffer,
            )
        };
        status_result(status)?;
        Ok(Self(NonNull::new(buffer).expect("Allocated AHardwareBuffer was null")))
    }

    /// Returns a clone of the native handle of the buffer.
    ///
    /// Returns `None` if the operation fails for any reason.
    pub fn cloned_native_handle(&self) -> Option<NativeHandle> {
        // SAFETY: The AHardwareBuffer pointer we pass is guaranteed to be non-null and valid
        // because it must have been allocated by `AHardwareBuffer_allocate`,
        // `AHardwareBuffer_readFromParcel` or the caller of `from_raw` and we have not yet
        // released it.
        let native_handle = unsafe { ffi::AHardwareBuffer_getNativeHandle(self.0.as_ptr()) };
        NonNull::new(native_handle.cast_mut()).and_then(|native_handle| {
            // SAFETY: `AHardwareBuffer_getNativeHandle` should have returned a valid pointer which
            // is valid at least as long as the buffer is, and `clone_from_raw` clones it rather
            // than taking ownership of it so the original `native_handle` isn't stored.
            unsafe { NativeHandle::clone_from_raw(native_handle) }
        })
    }

    /// Adopts the given raw pointer and wraps it in a Rust HardwareBuffer.
    ///
    /// # Safety
    ///
    /// This function takes ownership of the pointer and does NOT increment the refcount on the
    /// buffer. If the caller uses the pointer after the created object is dropped it will cause
    /// undefined behaviour. If the caller wants to continue using the pointer after calling this
    /// then use [`clone_from_raw`](Self::clone_from_raw) instead.
    pub unsafe fn from_raw(buffer_ptr: NonNull<AHardwareBuffer>) -> Self {
        Self(buffer_ptr)
    }

    /// Creates a new Rust HardwareBuffer to wrap the given `AHardwareBuffer` without taking
    /// ownership of it.
    ///
    /// Unlike [`from_raw`](Self::from_raw) this method will increment the refcount on the buffer.
    /// This means that the caller can continue to use the raw buffer it passed in, and must call
    /// [`AHardwareBuffer_release`](ffi::AHardwareBuffer_release) when it is finished with it to
    /// avoid a memory leak.
    ///
    /// # Safety
    ///
    /// The buffer pointer must point to a valid `AHardwareBuffer`.
    pub unsafe fn clone_from_raw(buffer: NonNull<AHardwareBuffer>) -> Self {
        // SAFETY: The caller guarantees that the AHardwareBuffer pointer is valid.
        unsafe { ffi::AHardwareBuffer_acquire(buffer.as_ptr()) };
        Self(buffer)
    }

    /// Returns the internal `AHardwareBuffer` pointer.
    ///
    /// This is only valid as long as this `HardwareBuffer` exists, so shouldn't be stored. It can
    /// be used to provide a pointer for a C/C++ API over FFI.
    pub fn as_raw(&self) -> NonNull<AHardwareBuffer> {
        self.0
    }

    /// Gets the internal `AHardwareBuffer` pointer without decrementing the refcount. This can
    /// be used for a C/C++ API which takes ownership of the pointer.
    ///
    /// The caller is responsible for releasing the `AHardwareBuffer` pointer by calling
    /// `AHardwareBuffer_release` when it is finished with it, or may convert it back to a Rust
    /// `HardwareBuffer` by calling [`HardwareBuffer::from_raw`].
    pub fn into_raw(self) -> NonNull<AHardwareBuffer> {
        let buffer = ManuallyDrop::new(self);
        buffer.0
    }

    /// Get the system wide unique id for an AHardwareBuffer. This function may panic in extreme
    /// and undocumented circumstances.
    ///
    /// Available since API level 31.
    pub fn id(&self) -> u64 {
        let mut out_id = 0;
        // SAFETY: The AHardwareBuffer pointer we pass is guaranteed to be non-null and valid
        // because it must have been allocated by `AHardwareBuffer_allocate`,
        // `AHardwareBuffer_readFromParcel` or the caller of `from_raw` and we have not yet
        // released it. The id pointer must be valid because it comes from a reference.
        let status = unsafe { ffi::AHardwareBuffer_getId(self.0.as_ptr(), &mut out_id) };
        assert_eq!(status, 0, "id() failed for AHardwareBuffer with error code: {status}");

        out_id
    }

    /// Returns the description of this buffer.
    pub fn description(&self) -> HardwareBufferDescription {
        let mut buffer_desc = ffi::AHardwareBuffer_Desc {
            width: 0,
            height: 0,
            layers: 0,
            format: 0,
            usage: 0,
            stride: 0,
            rfu0: 0,
            rfu1: 0,
        };
        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid, and the
        // AHardwareBuffer_Desc pointer is valid because it comes from a reference.
        unsafe { ffi::AHardwareBuffer_describe(self.0.as_ref(), &mut buffer_desc) };
        HardwareBufferDescription(buffer_desc)
    }

    /// Locks the hardware buffer for direct CPU access.
    ///
    /// # Safety
    ///
    /// - If `fence` is `None`, the caller must ensure that all writes to the buffer have completed
    ///   before calling this function.
    /// - If the buffer has `AHARDWAREBUFFER_FORMAT_BLOB`, multiple threads or process may lock the
    ///   buffer simultaneously, but the caller must ensure that they don't access it simultaneously
    ///   and break Rust's aliasing rules, like any other shared memory.
    /// - Otherwise if `usage` includes `AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY` or
    ///   `AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN`, the caller must ensure that no other threads or
    ///   processes lock the buffer simultaneously for any usage.
    /// - Otherwise, the caller must ensure that no other threads lock the buffer for writing
    ///   simultaneously.
    /// - If `rect` is not `None`, the caller must not modify the buffer outside of that rectangle.
    pub unsafe fn lock<'a>(
        &'a self,
        usage: AHardwareBuffer_UsageFlags,
        fence: Option<BorrowedFd>,
        rect: Option<&ARect>,
    ) -> Result<HardwareBufferGuard<'a>, StatusCode> {
        let fence = if let Some(fence) = fence { fence.as_raw_fd() } else { -1 };
        let rect = rect.map(ptr::from_ref).unwrap_or(null());
        let mut address = null_mut();
        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid, and the buffer address out
        // pointer is valid because it comes from a reference. Our caller promises that writes have
        // completed and there will be no simultaneous read/write locks.
        let status = unsafe {
            ffi::AHardwareBuffer_lock(self.0.as_ptr(), usage.0, fence, rect, &mut address)
        };
        status_result(status)?;
        Ok(HardwareBufferGuard {
            buffer: self,
            address: NonNull::new(address)
                .expect("AHardwareBuffer_lock set a null outVirtualAddress"),
        })
    }

    /// Lock a potentially multi-planar hardware buffer for direct CPU access.
    ///
    /// # Safety
    ///
    /// - If `fence` is `None`, the caller must ensure that all writes to the buffer have completed
    ///   before calling this function.
    /// - If the buffer has `AHARDWAREBUFFER_FORMAT_BLOB`, multiple threads or process may lock the
    ///   buffer simultaneously, but the caller must ensure that they don't access it simultaneously
    ///   and break Rust's aliasing rules, like any other shared memory.
    /// - Otherwise if `usage` includes `AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY` or
    ///   `AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN`, the caller must ensure that no other threads or
    ///   processes lock the buffer simultaneously for any usage.
    /// - Otherwise, the caller must ensure that no other threads lock the buffer for writing
    ///   simultaneously.
    /// - If `rect` is not `None`, the caller must not modify the buffer outside of that rectangle.
    pub unsafe fn lock_planes<'a>(
        &'a self,
        usage: AHardwareBuffer_UsageFlags,
        fence: Option<BorrowedFd>,
        rect: Option<&ARect>,
    ) -> Result<Vec<PlaneGuard<'a>>, StatusCode> {
        let fence = if let Some(fence) = fence { fence.as_raw_fd() } else { -1 };
        let rect = rect.map(ptr::from_ref).unwrap_or(null());
        let mut planes = AHardwareBuffer_Planes {
            planeCount: 0,
            planes: [const { AHardwareBuffer_Plane { data: null_mut(), pixelStride: 0, rowStride: 0 } };
                4],
        };

        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid, and the various out
        // pointers are valid because they come from references. Our caller promises that writes have
        // completed and there will be no simultaneous read/write locks.
        let status = unsafe {
            ffi::AHardwareBuffer_lockPlanes(self.0.as_ptr(), usage.0, fence, rect, &mut planes)
        };
        status_result(status)?;
        let plane_count = planes.planeCount.try_into().unwrap();
        Ok(planes.planes[..plane_count]
            .iter()
            .map(|plane| PlaneGuard {
                guard: HardwareBufferGuard {
                    buffer: self,
                    address: NonNull::new(plane.data)
                        .expect("AHardwareBuffer_lockAndGetInfo set a null outVirtualAddress"),
                },
                pixel_stride: plane.pixelStride,
                row_stride: plane.rowStride,
            })
            .collect())
    }

    /// Locks the hardware buffer for direct CPU access, returning information about the bytes per
    /// pixel and stride as well.
    ///
    /// # Safety
    ///
    /// - If `fence` is `None`, the caller must ensure that all writes to the buffer have completed
    ///   before calling this function.
    /// - If the buffer has `AHARDWAREBUFFER_FORMAT_BLOB`, multiple threads or process may lock the
    ///   buffer simultaneously, but the caller must ensure that they don't access it simultaneously
    ///   and break Rust's aliasing rules, like any other shared memory.
    /// - Otherwise if `usage` includes `AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY` or
    ///   `AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN`, the caller must ensure that no other threads or
    ///   processes lock the buffer simultaneously for any usage.
    /// - Otherwise, the caller must ensure that no other threads lock the buffer for writing
    ///   simultaneously.
    pub unsafe fn lock_and_get_info<'a>(
        &'a self,
        usage: AHardwareBuffer_UsageFlags,
        fence: Option<BorrowedFd>,
        rect: Option<&ARect>,
    ) -> Result<LockedBufferInfo<'a>, StatusCode> {
        let fence = if let Some(fence) = fence { fence.as_raw_fd() } else { -1 };
        let rect = rect.map(ptr::from_ref).unwrap_or(null());
        let mut address = null_mut();
        let mut bytes_per_pixel = 0;
        let mut stride = 0;
        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid, and the various out
        // pointers are valid because they come from references. Our caller promises that writes have
        // completed and there will be no simultaneous read/write locks.
        let status = unsafe {
            ffi::AHardwareBuffer_lockAndGetInfo(
                self.0.as_ptr(),
                usage.0,
                fence,
                rect,
                &mut address,
                &mut bytes_per_pixel,
                &mut stride,
            )
        };
        status_result(status)?;
        Ok(LockedBufferInfo {
            guard: HardwareBufferGuard {
                buffer: self,
                address: NonNull::new(address)
                    .expect("AHardwareBuffer_lockAndGetInfo set a null outVirtualAddress"),
            },
            bytes_per_pixel: bytes_per_pixel as u32,
            stride: stride as u32,
        })
    }

    /// Unlocks the hardware buffer from direct CPU access.
    ///
    /// Must be called after all changes to the buffer are completed by the caller. This will block
    /// until the unlocking is complete and the buffer contents are updated.
    fn unlock(&self) -> Result<(), StatusCode> {
        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid.
        let status = unsafe { ffi::AHardwareBuffer_unlock(self.0.as_ptr(), null_mut()) };
        status_result(status)?;
        Ok(())
    }

    /// Unlocks the hardware buffer from direct CPU access.
    ///
    /// Must be called after all changes to the buffer are completed by the caller.
    ///
    /// This may not block until all work is completed, but rather will return a file descriptor
    /// which will be signalled once the unlocking is complete and the buffer contents is updated.
    /// If `Ok(None)` is returned then unlocking has already completed and no further waiting is
    /// necessary. The file descriptor may be passed to a subsequent call to [`Self::lock`].
    pub fn unlock_with_fence(
        &self,
        guard: HardwareBufferGuard,
    ) -> Result<Option<OwnedFd>, StatusCode> {
        // Forget the guard so that its `Drop` implementation doesn't try to unlock the
        // HardwareBuffer again.
        forget(guard);

        let mut fence = -2;
        // SAFETY: The `AHardwareBuffer` pointer we wrap is always valid.
        let status = unsafe { ffi::AHardwareBuffer_unlock(self.0.as_ptr(), &mut fence) };
        let fence = if fence < 0 {
            None
        } else {
            // SAFETY: `AHardwareBuffer_unlock` gives us ownership of the fence file descriptor.
            Some(unsafe { OwnedFd::from_raw_fd(fence) })
        };
        status_result(status)?;
        Ok(fence)
    }
}

impl Drop for HardwareBuffer {
    fn drop(&mut self) {
        // SAFETY: The AHardwareBuffer pointer we pass is guaranteed to be non-null and valid
        // because it must have been allocated by `AHardwareBuffer_allocate`,
        // `AHardwareBuffer_readFromParcel` or the caller of `from_raw` and we have not yet
        // released it.
        unsafe { ffi::AHardwareBuffer_release(self.0.as_ptr()) }
    }
}

impl Debug for HardwareBuffer {
    fn fmt(&self, f: &mut Formatter) -> fmt::Result {
        f.debug_struct("HardwareBuffer").field("id", &self.id()).finish()
    }
}

impl Clone for HardwareBuffer {
    fn clone(&self) -> Self {
        // SAFETY: ptr is guaranteed to be non-null and the acquire can not fail.
        unsafe { ffi::AHardwareBuffer_acquire(self.0.as_ptr()) };
        Self(self.0)
    }
}

impl UnstructuredParcelable for HardwareBuffer {
    fn write_to_parcel(&self, parcel: &mut BorrowedParcel) -> Result<(), StatusCode> {
        let status =
        // SAFETY: The AHardwareBuffer pointer we pass is guaranteed to be non-null and valid
        // because it must have been allocated by `AHardwareBuffer_allocate`,
        // `AHardwareBuffer_readFromParcel` or the caller of `from_raw` and we have not yet
        // released it.
            unsafe { AHardwareBuffer_writeToParcel(self.0.as_ptr(), parcel.as_native_mut()) };
        status_result(status)
    }

    fn from_parcel(parcel: &BorrowedParcel) -> Result<Self, StatusCode> {
        let mut buffer = null_mut();

        let status =
        // SAFETY: Both pointers must be valid because they are obtained from references.
        // `AHardwareBuffer_readFromParcel` doesn't store them or do anything else special
        // with them. If it returns success then it will have allocated a new
        // `AHardwareBuffer` and incremented the reference count, so we can use it until we
        // release it.
            unsafe { AHardwareBuffer_readFromParcel(parcel.as_native(), &mut buffer) };

        status_result(status)?;

        Ok(Self(
            NonNull::new(buffer).expect(
                "AHardwareBuffer_readFromParcel returned success but didn't allocate buffer",
            ),
        ))
    }
}

impl_deserialize_for_unstructured_parcelable!(HardwareBuffer);
impl_serialize_for_unstructured_parcelable!(HardwareBuffer);

// SAFETY: The underlying *AHardwareBuffers can be moved between threads.
unsafe impl Send for HardwareBuffer {}

// SAFETY: The underlying *AHardwareBuffers can be used from multiple threads.
//
// AHardwareBuffers are backed by C++ GraphicBuffers, which are mostly immutable. The only cases
// where they are not immutable are:
//
//   - reallocation (which is never actually done across the codebase and requires special
//     privileges/platform code access to do)
//   - "locking" for reading/writing (which is explicitly allowed to be done across multiple threads
//     according to the docs on the underlying gralloc calls)
unsafe impl Sync for HardwareBuffer {}

/// A guard for when a `HardwareBuffer` is locked.
///
/// The `HardwareBuffer` will be unlocked when this is dropped, or may be unlocked via
/// [`HardwareBuffer::unlock_with_fence`].
#[derive(Debug)]
pub struct HardwareBufferGuard<'a> {
    buffer: &'a HardwareBuffer,
    /// The address of the buffer in memory.
    pub address: NonNull<c_void>,
}

impl<'a> Drop for HardwareBufferGuard<'a> {
    fn drop(&mut self) {
        self.buffer
            .unlock()
            .expect("Failed to unlock HardwareBuffer when dropping HardwareBufferGuard");
    }
}

/// A guard for when a `HardwareBuffer` is locked, with additional information about the number of
/// bytes per pixel and stride.
#[derive(Debug)]
pub struct LockedBufferInfo<'a> {
    /// The locked buffer guard.
    pub guard: HardwareBufferGuard<'a>,
    /// The number of bytes used for each pixel in the buffer.
    pub bytes_per_pixel: u32,
    /// The stride in bytes between rows in the buffer.
    pub stride: u32,
}

/// A guard for a single plane of a locked `HardwareBuffer`, with additional information about the
/// stride.
#[derive(Debug)]
pub struct PlaneGuard<'a> {
    /// The locked buffer guard.
    pub guard: HardwareBufferGuard<'a>,
    /// The stride in bytes between the color channel for one pixel to the next pixel.
    pub pixel_stride: u32,
    /// The stride in bytes between rows in the buffer.
    pub row_stride: u32,
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn create_valid_buffer_returns_ok() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            512,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ));
        assert!(buffer.is_some());
    }

    #[test]
    fn create_invalid_buffer_returns_err() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            512,
            512,
            1,
            0,
            AHardwareBuffer_UsageFlags(0),
            0,
        ));
        assert!(buffer.is_none());
    }

    #[test]
    fn from_raw_allows_getters() {
        let buffer_desc = ffi::AHardwareBuffer_Desc {
            width: 1024,
            height: 512,
            layers: 1,
            format: AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            usage: AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN.0,
            stride: 0,
            rfu0: 0,
            rfu1: 0,
        };
        let mut raw_buffer_ptr = ptr::null_mut();

        // SAFETY: The pointers are valid because they come from references, and
        // `AHardwareBuffer_allocate` doesn't retain them after it returns.
        let status = unsafe { ffi::AHardwareBuffer_allocate(&buffer_desc, &mut raw_buffer_ptr) };
        assert_eq!(status, 0);

        // SAFETY: The pointer must be valid because it was just allocated successfully, and we
        // don't use it after calling this.
        let buffer = unsafe { HardwareBuffer::from_raw(NonNull::new(raw_buffer_ptr).unwrap()) };
        assert_eq!(buffer.description().width(), 1024);
    }

    #[test]
    fn basic_getters() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Buffer with some basic parameters was not created successfully");

        let description = buffer.description();
        assert_eq!(description.width(), 1024);
        assert_eq!(description.height(), 512);
        assert_eq!(description.layers(), 1);
        assert_eq!(
            description.format(),
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM
        );
        assert_eq!(
            description.usage(),
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN
        );
    }

    #[test]
    fn id_getter() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Buffer with some basic parameters was not created successfully");

        assert_ne!(0, buffer.id());
    }

    #[test]
    fn clone() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Buffer with some basic parameters was not created successfully");
        let buffer2 = buffer.clone();

        assert_eq!(buffer, buffer2);
    }

    #[test]
    fn into_raw() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Buffer with some basic parameters was not created successfully");
        let buffer2 = buffer.clone();

        let raw_buffer = buffer.into_raw();
        // SAFETY: This is the same pointer we had before.
        let remade_buffer = unsafe { HardwareBuffer::from_raw(raw_buffer) };

        assert_eq!(remade_buffer, buffer2);
    }

    #[test]
    fn native_handle_and_back() {
        let buffer_description = HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            1024,
        );
        let buffer = HardwareBuffer::new(&buffer_description)
            .expect("Buffer with some basic parameters was not created successfully");

        let native_handle =
            buffer.cloned_native_handle().expect("Failed to get native handle for buffer");
        let buffer2 = HardwareBuffer::create_from_handle(&native_handle, &buffer_description)
            .expect("Failed to create buffer from native handle");

        assert_eq!(buffer.description(), buffer_description);
        assert_eq!(buffer2.description(), buffer_description);
    }

    #[test]
    fn lock() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Failed to create buffer");

        // SAFETY: No other threads or processes have access to the buffer.
        let guard = unsafe {
            buffer.lock(
                AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
                None,
                None,
            )
        }
        .unwrap();

        drop(guard);
    }

    #[test]
    fn lock_with_rect() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Failed to create buffer");
        let rect = ARect { left: 10, right: 20, top: 35, bottom: 45 };

        // SAFETY: No other threads or processes have access to the buffer.
        let guard = unsafe {
            buffer.lock(
                AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
                None,
                Some(&rect),
            )
        }
        .unwrap();

        drop(guard);
    }

    #[test]
    fn unlock_with_fence() {
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            1024,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Failed to create buffer");

        // SAFETY: No other threads or processes have access to the buffer.
        let guard = unsafe {
            buffer.lock(
                AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
                None,
                None,
            )
        }
        .unwrap();

        buffer.unlock_with_fence(guard).unwrap();
    }

    #[test]
    fn lock_with_info() {
        const WIDTH: u32 = 1024;
        let buffer = HardwareBuffer::new(&HardwareBufferDescription::new(
            WIDTH,
            512,
            1,
            AHardwareBuffer_Format::AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM,
            AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            0,
        ))
        .expect("Failed to create buffer");

        // SAFETY: No other threads or processes have access to the buffer.
        let info = unsafe {
            buffer.lock_and_get_info(
                AHardwareBuffer_UsageFlags::AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
                None,
                None,
            )
        }
        .unwrap();

        assert_eq!(info.bytes_per_pixel, 4);
        assert_eq!(info.stride, WIDTH * 4);
        drop(info);
    }
}
