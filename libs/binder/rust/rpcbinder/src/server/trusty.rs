/*
 * Copyright (C) 2023 The Android Open Source Project
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

use alloc::boxed::Box;
use binder::{unstable_api::AsNative, SpIBinder};
use binder_rpc_server_bindgen::{
    trusty_peer_id, AIBinder, ARpcServerTrusty, ARpcServerTrusty_delete,
    ARpcServerTrusty_handleChannelCleanup, ARpcServerTrusty_handleConnect,
    ARpcServerTrusty_handleDisconnect, ARpcServerTrusty_handleMessage,
    ARpcServerTrusty_newPerSession,
};
use std::{ffi::c_void, ptr};
use tipc::{
    ClientIdentifier, ConnectResult, Handle, MessageResult, PortCfg, TipcError, UnbufferedService,
    Uuid,
};

/// Trait alias for the callback passed into the per-session constructor of the RpcServer.
/// Note: this is used in this file only, although it is marked as pub to be able to be used in
/// the definition of the pub constructor.
pub trait PerSessionCallback:
    Fn(ClientIdentifier) -> Option<SpIBinder> + Send + Sync + 'static
{
}
impl<T> PerSessionCallback for T where
    T: Fn(ClientIdentifier) -> Option<SpIBinder> + Send + Sync + 'static
{
}

pub struct RpcServer {
    inner: *mut ARpcServerTrusty,
}

/// SAFETY: The opaque handle points to a heap allocation
/// that should be process-wide and not tied to the current thread.
unsafe impl Send for RpcServer {}
/// SAFETY: The underlying C++ RpcServer class is thread-safe.
unsafe impl Sync for RpcServer {}

impl Drop for RpcServer {
    fn drop(&mut self) {
        // SAFETY: `ARpcServerTrusty_delete` is the correct destructor to call
        // on pointers returned by `ARpcServerTrusty_new`.
        unsafe {
            ARpcServerTrusty_delete(self.inner);
        }
    }
}

impl RpcServer {
    /// Allocates a new RpcServer object.
    pub fn new(service: SpIBinder) -> RpcServer {
        Self::new_per_session(move |_uuid| Some(service.clone()))
    }

    /// Allocates a new per-session RpcServer object.
    ///
    /// Per-session objects take a closure that gets called once
    /// for every new connection. The closure gets the `ClientIdentifier` of
    /// the peer and can accept or reject that connection.
    pub fn new_per_session<F: PerSessionCallback>(f: F) -> RpcServer {
        // Safety: ARpcServerTrusty_newPerSession promises that while the returned ARpcServerTrusty
        // is alive, it will pass the pointer to f as the argument to any calls it makes to
        // per_session_callback_wrapper. Then when the ARpcServerTrusty's lifetime ends, it will
        // pass the pointer to f to call per_session_callback_deleter. No other calls will be made
        // to per_session_callback_deleter.
        //
        // The pointer to f is currently valid to pass to both functions (i.e. it's valid to convert
        // to &mut F or to Box<F>), and it will remain a valid argument after any calls to
        // per_session_callback_wrapper. We don't retain any other pointers or references to f, so
        // nothing other than ARpcServerTrusty will modify it, so ARpcServerTrusty's use of it must
        // be sound.
        let inner = unsafe {
            ARpcServerTrusty_newPerSession(
                Some(per_session_callback_wrapper::<F>),
                Box::into_raw(Box::new(f)).cast(),
                Some(per_session_callback_deleter::<F>),
            )
        };
        RpcServer { inner }
    }
}

/// # Safety
///   * cb_ptr must be a pointer which is valid to convert into a &mut F, i.e.
///     * It's properly aligned for F,
///     * non-null,
///     * dereferenceable
///     * points to a valid value of type F (esp. it hasn't been dropped by drop_in_place,
///       per_session_callback_deleter, etc.)
///     * not aliased for the duration of this call
///   * peer is similarly valid to convert to &trusty_peer_id
///   * peer_len is the correct runtime length for *peer
unsafe extern "C" fn per_session_callback_wrapper<F: PerSessionCallback>(
    peer: *const trusty_peer_id,
    peer_len: usize,
    cb_ptr: *mut c_void,
) -> *mut AIBinder {
    // SAFETY: Function preconditions guarantee cb_ptr can be converted to a &mut F
    let cb = unsafe { &mut *cb_ptr.cast::<F>() };

    // SAFETY: Function preconditions guarantee peer is valid to convert to &trusty_peer_id
    let peer = unsafe { &*peer };
    // SAFETY: Function preconditions guarantee peer_len is the correct length for *peer
    let peer = unsafe { trusty_sys::TrustyPeerIdRef::from_raw_parts(peer, peer_len) };

    cb(ClientIdentifier::from_c_repr(peer)).map_or_else(ptr::null_mut, |b| {
        // Prevent AIBinder_decStrong from being called before AIBinder_toPlatformBinder.
        // The per-session callback in C++ is supposed to call AIBinder_decStrong on the
        // pointer we return here.
        std::mem::ManuallyDrop::new(b).as_native_mut().cast()
    })
}

/// # Safety
///   * cb must be a pointer previously obtained with Box::<F>::into_raw.
///   * cb must _still_ point to a valid F, i.e. Box::from_raw, per_session_callback_deleter, or
///     drop_in_place, etc. must not have already been called on it.
///   * *cb must not be accessed or referenced by anything else during or after this function call.
unsafe extern "C" fn per_session_callback_deleter<F: PerSessionCallback>(cb: *mut c_void) {
    // Safety: Function preconditions mean we have ownership over the underlying F value and since
    // was created with Box::into_raw, we can convert back into a Box.
    let cb = unsafe { Box::<F>::from_raw(cb.cast()) };
    drop(cb);
}

pub struct RpcServerConnection {
    ctx: *mut c_void,
}

// SAFETY: The opaque handle: `ctx` points into a dynamic allocation,
// and not tied to anything specific to the current thread.
unsafe impl Send for RpcServerConnection {}

impl Drop for RpcServerConnection {
    fn drop(&mut self) {
        // We do not need to close handle_fd since we do not own it.
        unsafe {
            ARpcServerTrusty_handleChannelCleanup(self.ctx);
        }
    }
}

impl UnbufferedService for RpcServer {
    type Connection = RpcServerConnection;

    fn on_connect(
        &self,
        port: &PortCfg,
        handle: &Handle,
        peer: &Uuid,
    ) -> tipc::Result<ConnectResult<Self::Connection>> {
        let peer = ClientIdentifier::UUID(peer.clone());
        self.on_new_connection(port, handle, &peer)
    }

    fn on_message(
        &self,
        conn: &Self::Connection,
        _handle: &Handle,
        _buffer: &mut [u8],
    ) -> tipc::Result<MessageResult> {
        let rc = unsafe { ARpcServerTrusty_handleMessage(conn.ctx) };
        if rc < 0 {
            Err(TipcError::from_uapi(rc.into()))
        } else {
            Ok(MessageResult::MaintainConnection)
        }
    }

    fn on_disconnect(&self, conn: &Self::Connection) {
        unsafe { ARpcServerTrusty_handleDisconnect(conn.ctx) };
    }

    fn on_new_connection(
        &self,
        _port: &PortCfg,
        handle: &Handle,
        peer: &ClientIdentifier,
    ) -> tipc::Result<ConnectResult<Self::Connection>> {
        let mut conn = RpcServerConnection { ctx: std::ptr::null_mut() };
        let peer = peer.c_repr();
        let (peer_ref, peer_len) = peer.as_generic().into_raw_parts();

        // SAFETY:
        //   * Because of invariants on RpcServer, self.inner is a pointer obtained from
        //     ARpcServerTrusty_newPerSession, which has not yet been passed to
        //     ARpcServerTrusty_delete. RpcServer is !Sync, so self.inner isn't concurrently
        //     accessed. Therefore, it's valid to call ARpcServerTrusty methods on it.
        //   * handle is valid for borrowing since we have &Handle
        //   * peer_ref is a correctly constructed trusty_peer_id and peer_len is its correct
        //     runtime length since we got them from TrustyPeerIdRef::into_raw_parts.
        //   * conn.ctx will be valid for writing for the duration of the call since it's a local
        let rc = unsafe {
            ARpcServerTrusty_handleConnect(
                self.inner,
                handle.as_raw_fd(),
                peer_ref,
                peer_len,
                &mut conn.ctx,
            )
        };
        if rc < 0 {
            Err(TipcError::from_uapi(rc.into()))
        } else {
            Ok(ConnectResult::Accept(conn))
        }
    }
}
