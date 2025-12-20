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
#pragma once

#include <assert.h>
#include <stdint.h>
#include <uapi/trusty_uuid.h>

typedef uint64_t trusty_peer_id_kind_t;

/**
 * enum - Discriminant for trusty_peer_id kinds
 */
enum : trusty_peer_id_kind_t {
    /**
     * @TRUSTY_PEER_ID_KIND_INVALID: Indicates an invalid peer ID. The ID data
     * should be zeroed.
     */
    TRUSTY_PEER_ID_KIND_INVALID = 0x0,

    /**
     * @TRUSTY_PEER_ID_KIND_UUID: Indicates a peer identified by a uuid. The
     * peer is a local TA.
     */
    TRUSTY_PEER_ID_KIND_UUID = 0x1,

    /**
     * @TRUSTY_PEER_ID_KIND_VMID_FFA: Indicates a peer running on an FF-A
     * partition VM, identified by a vmid. Primarily used internally by the
     * authmgr protocol. In legitimate settings, only the authmgr-fe in the
     * remote pVM should connect this way.
     */
    TRUSTY_PEER_ID_KIND_VMID_FFA = 0x2,

    /**
     * @TRUSTY_PEER_ID_KIND_FIRST_USERSPACE_AVAILABLE: Peer ID kind values less
     * than this value are reserved for kernel use.
     */
    TRUSTY_PEER_ID_KIND_FIRST_USERSPACE_AVAILABLE = 0x10000,
};

/**
 * struct trusty_peer_id - DST that contains any type of peer identifier.
 *
 * @kind: The type of peer identifier stored in @data.
 * @data: Byte array containing the identifier specified by @kind.
 */
struct trusty_peer_id {
    trusty_peer_id_kind_t kind;
    char data[];
};

/**
 * struct trusty_peer_id_storage - Sized type that has space to contain any peer
 * identifier defined by the kernel.
 *
 * @kind: The type of peer identifier stored in @data.
 * @data: Byte array containing the identifier specified by @kind. Any unused
 *        bytes should be zero.
 */
struct trusty_peer_id_storage {
    trusty_peer_id_kind_t kind;
    char data[16];
};

#define TRUSTY_PEER_ID_STORAGE_INITIAL_VALUE() {.kind = TRUSTY_PEER_ID_KIND_INVALID, .data = {0}}

/**
 * struct trusty_peer_id_uuid - trusty_peer_id type for peers identified by uuid
 *
 * @kind: Set to TRUSTY_PEER_ID_KIND_UUID.
 * @id:   The peer's uuid.
 */
struct trusty_peer_id_uuid {
    trusty_peer_id_kind_t kind;
    uuid_t id;
};

#define TRUSTY_PEER_ID_UUID_VALUE(uuid_value) {.kind = TRUSTY_PEER_ID_KIND_UUID, .id = uuid_value}

static_assert(sizeof(struct trusty_peer_id_storage) >= sizeof(struct trusty_peer_id_uuid),
              "trusty_peer_id_uuid must fit inside trusty_peer_id_storage.");
static_assert(sizeof(struct trusty_peer_id_uuid) ==
                      sizeof(struct trusty_peer_id) + sizeof((struct trusty_peer_id_uuid){}.id),
              "trusty_peer_id_uuid must not have implicit padding.");

/**
 * struct trusty_peer_id_vmid_ffa - trusty_peer_id type for peers identified by
 * an FF-A VM ID.
 *
 * @kind:       Set to TRUSTY_PEER_ID_KIND_VMID_FFA.
 * @reserved_1: Set to 1. Reserved for future use.
 * @id:         The peer ID for the peer VM. Will be within [0..u16::MAX].
 * @padding:    Explicit padding so this struct's size doesn't change between 32
 *              and 64-bit builds, e.g. 32-bit userspace with 64-bit kernel.
 *              Zeroed.
 */
struct trusty_peer_id_vmid_ffa {
    trusty_peer_id_kind_t kind;
    uint64_t reserved_1;
    uint16_t id;
    uint8_t padding[6];
};

#define TRUSTY_PEER_ID_VMID_FFA_VALUE(vmid_value) \
    {.kind = TRUSTY_PEER_ID_KIND_VMID_FFA, .reserved_1 = 1, .id = vmid_value, .padding = {0}}

static_assert(sizeof(struct trusty_peer_id_storage) >= sizeof(struct trusty_peer_id_vmid_ffa),
              "trusty_peer_id_vmid_ffa must fit inside trusty_peer_id_storage.");
static_assert(sizeof(struct trusty_peer_id_vmid_ffa) ==
                      sizeof(struct trusty_peer_id) +
                              sizeof((struct trusty_peer_id_vmid_ffa){}.reserved_1) +
                              sizeof((struct trusty_peer_id_vmid_ffa){}.id) +
                              sizeof((struct trusty_peer_id_vmid_ffa){}.padding),
              "trusty_peer_id_vmid_ffa must not have implicit padding.");
