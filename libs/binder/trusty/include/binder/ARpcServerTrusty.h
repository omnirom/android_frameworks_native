/*
 * Copyright (C) 2024 The Android Open Source Project
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
#pragma once

#include <stddef.h>

#include <lib/tipc/tipc_srv.h>
#include <trusty_ipc.h>
#include <uapi/trusty_peer_id.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct AIBinder;
struct ARpcServerTrusty;

struct ARpcServerTrusty* ARpcServerTrusty_newPerSession(
        struct AIBinder* (*cb)(const struct trusty_peer_id*, size_t peer_len, void*), void* cbArg,
        void (*deleteCbArg)(void*));
void ARpcServerTrusty_delete(struct ARpcServerTrusty* rpcServer);

int ARpcServerTrusty_handleConnect(struct ARpcServerTrusty* rpcServer, handle_t chan,
                                   const struct trusty_peer_id* peer, size_t peer_len,
                                   void** ctx_p);
int ARpcServerTrusty_handleMessage(void* ctx);
void ARpcServerTrusty_handleDisconnect(void* ctx);
void ARpcServerTrusty_handleChannelCleanup(void* ctx);

#if defined(__cplusplus)
}
#endif
