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

use android_hardware_common::{
    aidl::android::hardware::common::NativeHandle::NativeHandle as AidlNativeHandle,
    binder::ParcelFileDescriptor,
};
use std::{
    ffi::c_int,
    mem::forget,
    os::fd::{BorrowedFd, FromRawFd, IntoRawFd, OwnedFd},
    ptr::NonNull,
};

/// Rust wrapper around `native_handle_t`.
///
/// This owns the `native_handle_t` and its file descriptors, and will close them and free it when
/// it is dropped.
#[derive(Debug)]
pub struct NativeHandle(NonNull<ffi::native_handle_t>);

impl NativeHandle {
    /// Creates a new `NativeHandle` with the given file descriptors and integer values.
    ///
    /// The `NativeHandle` will take ownership of the file descriptors and close them when it is
    /// dropped.
    pub fn new(fds: Vec<OwnedFd>, ints: &[c_int]) -> Option<Self> {
        let fd_count = fds.len();
        // SAFETY: native_handle_create doesn't have any safety requirements.
        let handle = unsafe {
            ffi::native_handle_create(fd_count.try_into().unwrap(), ints.len().try_into().unwrap())
        };
        let handle = NonNull::new(handle)?;
        for (i, fd) in fds.into_iter().enumerate() {
            // SAFETY: `handle` must be valid because it was just created, and the array offset is
            // within the bounds of what we allocated above.
            unsafe {
                *(*handle.as_ptr()).data.as_mut_ptr().add(i) = fd.into_raw_fd();
            }
        }
        for (i, value) in ints.iter().enumerate() {
            // SAFETY: `handle` must be valid because it was just created, and the array offset is
            // within the bounds of what we allocated above. Note that `data` is uninitialized
            // until after this so we can't use `slice::from_raw_parts_mut` or similar to create a
            // reference to it so we use raw pointers arithmetic instead.
            unsafe {
                *(*handle.as_ptr()).data.as_mut_ptr().add(fd_count + i) = *value;
            }
        }
        // SAFETY: `handle` must be valid because it was just created.
        unsafe {
            ffi::native_handle_set_fdsan_tag(handle.as_ptr());
        }
        Some(Self(handle))
    }

    /// Returns a borrowed view of all the file descriptors in this native handle.
    pub fn fds(&self) -> Vec<BorrowedFd> {
        self.data()[..self.fd_count()]
            .iter()
            .map(|fd| {
                // SAFETY: The `native_handle_t` maintains ownership of the file descriptor so it
                // won't be closed until this `NativeHandle` is destroyed. The `BorrowedFd` will
                // have a lifetime constrained to that of `&self`, so it can't outlive it.
                unsafe { BorrowedFd::borrow_raw(*fd) }
            })
            .collect()
    }

    /// Returns the integer values in this native handle.
    pub fn ints(&self) -> &[c_int] {
        &self.data()[self.fd_count()..]
    }

    /// Destroys the `NativeHandle`, taking ownership of the file descriptors it contained.
    pub fn into_fds(self) -> Vec<OwnedFd> {
        // Unset FDSan tag since this `native_handle_t` is no longer the owner of the file
        // descriptors after this function.
        // SAFETY: Our wrapped `native_handle_t` pointer is always valid.
        unsafe {
            ffi::native_handle_unset_fdsan_tag(self.as_ref());
        }
        let fds = self.data()[..self.fd_count()]
            .iter()
            .map(|fd| {
                // SAFETY: The `native_handle_t` has ownership of the file descriptor, and
                // after this we destroy it without closing the file descriptor so we can take over
                // ownership of it.
                unsafe { OwnedFd::from_raw_fd(*fd) }
            })
            .collect();

        // SAFETY: Our wrapped `native_handle_t` pointer is always valid, and it won't be accessed
        // after this because we own it and forget it.
        unsafe {
            assert_eq!(ffi::native_handle_delete(self.0.as_ptr()), 0);
        }
        // Don't drop self, as that would cause `native_handle_close` to be called and close the
        // file descriptors.
        forget(self);
        fds
    }

    /// Returns a reference to the underlying `native_handle_t`.
    fn as_ref(&self) -> &ffi::native_handle_t {
        // SAFETY: All the ways of creating a `NativeHandle` ensure that the `native_handle_t` is
        // valid and initialised, and lives as long as the `NativeHandle`. We enforce Rust's
        // aliasing rules by giving the reference a lifetime matching that of `&self`.
        unsafe { self.0.as_ref() }
    }

    /// Returns the number of file descriptors included in the native handle.
    fn fd_count(&self) -> usize {
        self.as_ref().numFds.try_into().unwrap()
    }

    /// Returns the number of integer values included in the native handle.
    fn int_count(&self) -> usize {
        self.as_ref().numInts.try_into().unwrap()
    }

    /// Returns a slice reference for all the used `data` field of the native handle, including both
    /// file descriptors and integers.
    fn data(&self) -> &[c_int] {
        let total_count = self.fd_count() + self.int_count();
        // SAFETY: The data must have been initialised with this number of elements when the
        // `NativeHandle` was created.
        unsafe { self.as_ref().data.as_slice(total_count) }
    }

    /// Wraps a raw `native_handle_t` pointer, taking ownership of it.
    ///
    /// # Safety
    ///
    /// `native_handle` must be a valid pointer to a `native_handle_t`, and must not be used
    ///  anywhere else after calling this method.
    pub unsafe fn from_raw(native_handle: NonNull<ffi::native_handle_t>) -> Self {
        Self(native_handle)
    }

    /// Creates a new `NativeHandle` wrapping a clone of the given `native_handle_t` pointer.
    ///
    /// Unlike [`from_raw`](Self::from_raw) this doesn't take ownership of the pointer passed in, so
    /// the caller remains responsible for closing and freeing it.
    ///
    /// # Safety
    ///
    /// `native_handle` must be a valid pointer to a `native_handle_t`.
    pub unsafe fn clone_from_raw(native_handle: NonNull<ffi::native_handle_t>) -> Option<Self> {
        // SAFETY: The caller promised that `native_handle` was valid.
        let cloned = unsafe { ffi::native_handle_clone(native_handle.as_ptr()) };
        NonNull::new(cloned).map(Self)
    }

    /// Returns a raw pointer to the wrapped `native_handle_t`.
    ///
    /// This is only valid as long as this `NativeHandle` exists, so shouldn't be stored. It mustn't
    /// be closed or deleted.
    pub fn as_raw(&self) -> NonNull<ffi::native_handle_t> {
        self.0
    }

    /// Turns the `NativeHandle` into a raw `native_handle_t`.
    ///
    /// The caller takes ownership of the `native_handle_t` and its file descriptors, so is
    /// responsible for closing and freeing it.
    pub fn into_raw(self) -> NonNull<ffi::native_handle_t> {
        let raw = self.0;
        forget(self);
        raw
    }
}

impl Clone for NativeHandle {
    fn clone(&self) -> Self {
        // SAFETY: Our wrapped `native_handle_t` pointer is always valid.
        unsafe { Self::clone_from_raw(self.0) }.expect("native_handle_clone returned null")
    }
}

impl Drop for NativeHandle {
    fn drop(&mut self) {
        // SAFETY: Our wrapped `native_handle_t` pointer is always valid, and it won't be accessed
        // after this because we own it and are being dropped.
        unsafe {
            assert_eq!(ffi::native_handle_close(self.0.as_ptr()), 0);
            assert_eq!(ffi::native_handle_delete(self.0.as_ptr()), 0);
        }
    }
}

impl From<AidlNativeHandle> for NativeHandle {
    fn from(aidl_native_handle: AidlNativeHandle) -> Self {
        let fds = aidl_native_handle.fds.into_iter().map(OwnedFd::from).collect();
        Self::new(fds, &aidl_native_handle.ints).unwrap()
    }
}

impl From<NativeHandle> for AidlNativeHandle {
    fn from(native_handle: NativeHandle) -> Self {
        let ints = native_handle.ints().to_owned();
        let fds = native_handle.into_fds().into_iter().map(ParcelFileDescriptor::new).collect();
        Self { ints, fds }
    }
}

// SAFETY: `NativeHandle` owns the `native_handle_t`, which just contains some integers and file
// descriptors, which aren't tied to any particular thread.
unsafe impl Send for NativeHandle {}

// SAFETY: A `NativeHandle` can be used from different threads simultaneously, as is is just
// integers and file descriptors.
unsafe impl Sync for NativeHandle {}

#[cfg(test)]
mod test {
    use super::*;
    use std::fs::File;

    #[test]
    fn create_empty() {
        let handle = NativeHandle::new(vec![], &[]).unwrap();
        assert_eq!(handle.fds().len(), 0);
        assert_eq!(handle.ints(), &[]);
    }

    #[test]
    fn create_with_ints() {
        let handle = NativeHandle::new(vec![], &[1, 2, 42]).unwrap();
        assert_eq!(handle.fds().len(), 0);
        assert_eq!(handle.ints(), &[1, 2, 42]);
    }

    #[test]
    fn create_with_fd() {
        let file = File::open("/dev/null").unwrap();
        let handle = NativeHandle::new(vec![file.into()], &[]).unwrap();
        assert_eq!(handle.fds().len(), 1);
        assert_eq!(handle.ints(), &[]);
    }

    #[test]
    fn clone() {
        let file = File::open("/dev/null").unwrap();
        let original = NativeHandle::new(vec![file.into()], &[42]).unwrap();
        assert_eq!(original.ints(), &[42]);
        assert_eq!(original.fds().len(), 1);

        let cloned = original.clone();
        drop(original);

        assert_eq!(cloned.ints(), &[42]);
        assert_eq!(cloned.fds().len(), 1);

        drop(cloned);
    }

    #[test]
    fn to_fds() {
        let file = File::open("/dev/null").unwrap();
        let original = NativeHandle::new(vec![file.into()], &[42]).unwrap();
        assert_eq!(original.ints(), &[42]);
        assert_eq!(original.fds().len(), 1);

        let fds = original.into_fds();
        assert_eq!(fds.len(), 1);
    }

    #[test]
    fn to_aidl() {
        let file = File::open("/dev/null").unwrap();
        let original = NativeHandle::new(vec![file.into()], &[42]).unwrap();
        assert_eq!(original.ints(), &[42]);
        assert_eq!(original.fds().len(), 1);

        let aidl = AidlNativeHandle::from(original);
        assert_eq!(&aidl.ints, &[42]);
        assert_eq!(aidl.fds.len(), 1);
    }

    #[test]
    fn to_from_aidl() {
        let file = File::open("/dev/null").unwrap();
        let original = NativeHandle::new(vec![file.into()], &[42]).unwrap();
        assert_eq!(original.ints(), &[42]);
        assert_eq!(original.fds().len(), 1);

        let aidl = AidlNativeHandle::from(original);
        assert_eq!(&aidl.ints, &[42]);
        assert_eq!(aidl.fds.len(), 1);

        let converted_back = NativeHandle::from(aidl);
        assert_eq!(converted_back.ints(), &[42]);
        assert_eq!(converted_back.fds().len(), 1);
    }
}
