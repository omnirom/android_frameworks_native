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

namespace android::binder {

/**
 * See also BINDER_VM_SIZE. In kernel binder, the sum of all transactions must be allocated in this
 * space. Large transactions are very error prone. In general, we should work to reduce this limit.
 * The same limit is used in RPC binder for consistency.
 */
constexpr size_t kLogTransactionsOverBytes = 300 * 1024;

/**
 * See b/392575419 - this limit is chosen for a specific usecase, because RPC binder does not have
 * support for shared memory in the Android Baklava timeframe. This was 100 KB during and before
 * Android V.
 *
 * Keeping this low helps preserve overall system performance. Transactions of this size are far too
 * expensive to make multiple copies over binder or sockets, and they should be avoided if at all
 * possible and transition to shared memory.
 */
constexpr size_t kRpcTransactionLimitBytes = 600 * 1024;

} // namespace android::binder
