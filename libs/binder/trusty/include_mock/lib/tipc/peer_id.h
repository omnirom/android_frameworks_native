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
#include <uapi/trusty_peer_id.h>

/**
 * enum - Discriminant for trusty_peer_id kinds added by tipc
 */
enum : trusty_peer_id_kind_t {
    /**
     * @TRUSTY_PEER_ID_KIND_SEQUENCE_ID: A peer identified by their sequence ID.
     * This peer is always a client, a remote TA that's connected via the
     * authmgr-be.
     */
    TRUSTY_PEER_ID_KIND_SEQUENCE_ID = TRUSTY_PEER_ID_KIND_FIRST_USERSPACE_AVAILABLE,
};

/**
 * struct trusty_peer_id_sequence_id - trusty_peer_id type for peers identified
 * by sequence ID.
 *
 * @kind: Set to TRUSTY_PEER_ID_KIND_SEQUENCE_ID.
 * @id:   The peer's sequence id.
 */
struct trusty_peer_id_sequence_id {
    trusty_peer_id_kind_t kind;
    int64_t id;
};

#define TRUSTY_PEER_ID_SEQUENCE_ID_VALUE(sequence_id_value) \
    {.kind = TRUSTY_PEER_ID_KIND_SEQUENCE_ID, .id = sequence_id_value}

static_assert(sizeof(struct trusty_peer_id_storage) >= sizeof(struct trusty_peer_id_sequence_id),
              "trusty_peer_id_sequence_id must fit inside trusty_peer_id_storage.");
static_assert(sizeof(struct trusty_peer_id_sequence_id) ==
                      sizeof(struct trusty_peer_id) +
                              sizeof((struct trusty_peer_id_sequence_id){}.id),
              "trusty_peer_id_sequence_id must not have implicit padding.");
