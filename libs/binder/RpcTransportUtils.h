/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <binder/unique_fd.h>
#include <binder/Functional.h>
#include <poll.h>

#include "Constants.h"
#include "FdTrigger.h"
#include "RpcState.h"

// For just this file:
// #undef LOG_RPC_DETAIL
// #define LOG_RPC_DETAIL ALOGE

namespace android {

template <typename SendOrReceive>
status_t interruptableReadOrWrite(
        const android::RpcTransportFd& socket, FdTrigger* fdTrigger, iovec* iovs, int niovs,
        SendOrReceive sendOrReceiveFun, const char* funName, int16_t event,
        const std::optional<binder::impl::SmallFunction<status_t()>>& altPoll) {
    MAYBE_WAIT_IN_FLAKE_MODE;

    if (niovs < 0) {
        return BAD_VALUE;
    }

    // Since we didn't poll, we need to manually check to see if it was triggered. Otherwise, we
    // may never know we should be shutting down.
    if (fdTrigger->isTriggered()) {
        return DEAD_OBJECT;
    }

    // EMPTY IOVEC ISSUE
    // If iovs has one or more empty vectors at the end and
    // we somehow advance past all the preceding vectors and
    // pass some or all of the empty ones to sendmsg/recvmsg,
    // the call will return processSize == 0. In that case
    // we should be returning OK but instead return DEAD_OBJECT.
    // To avoid this problem, we make sure here that the last
    // vector at iovs[niovs - 1] has a non-zero length.
    while (niovs > 0 && iovs[niovs - 1].iov_len == 0) {
        niovs--;
    }
    if (niovs == 0) {
        // The vectors are all empty, so we have nothing to send.
        return OK;
    }

    // We should never break up a message by default, but we do reduce chunk size on
    // ENOMEM conditions.
    constexpr size_t kChunkMax = binder::kRpcTransactionLimitBytes;
    const size_t kChunkMin = getpagesize(); // typical allocated granularity for sockets
    size_t chunkSize = kChunkMax;

    // b/419364025 - we have confirmed vhost-vsock ENOMEM for non-blocking sockets,
    //   but more analysis is needed to see how this affects other settings/impls.
    // These are how long we are waiting on repeated enomems for memory to be available.
    constexpr size_t kEnomemWaitStartUs = 20'000;
    constexpr size_t kEnomemWaitMaxUs = 1'000'000;       // don't risk ANR
    constexpr size_t kEnomemWaitTotalMaxUs = 30'000'000; // ANR at 30s anyway, so avoid hang
    size_t enomemWaitUs = 0;
    size_t enomemTotalUs = 0;

    size_t numSysCalls = 0;
    auto syscallBugCatcher = binder::impl::make_scope_guard([&]() {
        if (numSysCalls > 1000) {
            ALOGE("Too many calls to %s! Spinning? %zu", funName, numSysCalls);
        }
    });

    bool havePolled = false;
    while (true) {
        ssize_t processSize = -1;

        // This block dynamically adjusts packet sizes down to work around a
        // limitation in the vsock driver where large packets are sometimes
        // dropped (b/419364025 b/399469406 b/421244320), though since this is
        // fixed, it's only used in ENOMEM conditions to try to allocate
        // smaller packets.
        {
            size_t chunkRemaining = chunkSize;
            int i = 0;
            for (i = 0; i < niovs; i++) {
                if (iovs[i].iov_len >= chunkRemaining) {
                    break;
                }
                chunkRemaining -= iovs[i].iov_len;
            }
            bool canSendFullTransaction = i == niovs;

            int old_niovs = niovs;
            size_t old_len = 0xDEADBEEF;

            if (!canSendFullTransaction) {
                // pretend like we have fewer iovecs
                niovs = i + 1; // to restore (A)
                old_len = iovs[i].iov_len;
                // only send up to remaining chunkRemaining from this iovec
                iovs[i].iov_len = chunkRemaining; // to restore (B)
                LOG_ALWAYS_FATAL_IF(chunkRemaining == 0,
                                    "chunkRemaining never zero - see EMPTY IOVEC ISSUE above");
            }

            ++numSysCalls;

            // MAIN ACTION
            if (MAYBE_TRUE_IN_FLAKE_MODE) {
                LOG_RPC_DETAIL("Injecting ENOMEM.");
                processSize = -1;
                errno = ENOMEM;
            } else {
                processSize = sendOrReceiveFun(iovs, niovs);
            }
            // MAIN ACTION

            if (!canSendFullTransaction) {
                // Now put the iovecs back. As far as the following logic
                // is concerned, this just looks like a partial read, up
                // to limit.
                niovs = old_niovs;         // (A) - restored
                iovs[i].iov_len = old_len; // (B) - restored
            }
        }

        // HANDLE RESULT OF SEND OR RECEIVE
        if (processSize < 0) {
            int savedErrno = errno;

            if (savedErrno == ENOMEM) {
                LOG_RPC_DETAIL("RpcTransport %s(): %s", funName, strerror(savedErrno));

                // Since this is the limit only for this call to send this packet
                // we don't ever restore this. Assume it will be hard to get more
                // memory if we're already having difficulty sending this out.
                chunkSize = std::max(chunkSize / 2, kChunkMin);
                LOG_RPC_DETAIL("Chunk size is now %zu due to ENOMEM.", chunkSize);

                // When we've gotten down to the minimum send size, add a timer
                // to give time for more memory to be freed up. This means even
                // a single page is not available, so we have to wait.
                if (chunkSize <= kChunkMin) {
                    if (enomemWaitUs == 0) {
                        enomemWaitUs = kEnomemWaitStartUs;
                    } else {
                        enomemWaitUs = std::min(enomemWaitUs * 2, kEnomemWaitMaxUs);
                    }
                    enomemTotalUs += enomemWaitUs;

                    if (enomemTotalUs > kEnomemWaitTotalMaxUs) {
                        // by this time WatchDog should be kicking in
                        return -ENOMEM;
                    }

                    LOG_RPC_DETAIL("Sleeping %zuus due to ENOMEM.", enomemWaitUs);
                    usleep(enomemWaitUs);
                }
            } else if (havePolled || (savedErrno != EAGAIN && savedErrno != EWOULDBLOCK)) {
                // Still return the error on later passes, since it would expose
                // a problem with polling
                LOG_RPC_DETAIL("RpcTransport %s(): %s", funName, strerror(savedErrno));
                return -savedErrno;
            }
        } else if (processSize == 0) {
            return DEAD_OBJECT;
        } else {
            // success - reset error exponential backoffs
            enomemWaitUs = 0;
            enomemTotalUs = 0;

            while (processSize > 0 && niovs > 0) {
                auto& iov = iovs[0];
                if (static_cast<size_t>(processSize) < iov.iov_len) {
                    // Advance the base of the current iovec
                    iov.iov_base = reinterpret_cast<char*>(iov.iov_base) + processSize;
                    iov.iov_len -= processSize;
                    break;
                }

                // The current iovec was fully written
                processSize -= iov.iov_len;
                iovs++;
                niovs--;
            }
            if (niovs == 0) {
                LOG_ALWAYS_FATAL_IF(processSize > 0,
                                    "Reached the end of iovecs "
                                    "with %zd bytes remaining",
                                    processSize);
                return OK;
            }
        }

        // METHOD OF POLLING
        if (altPoll) {
            if (status_t status = (*altPoll)(); status != OK) return status;
            if (fdTrigger->isTriggered()) { // altPoll may not check this
                return DEAD_OBJECT;
            }
        } else {
            if (status_t status = fdTrigger->triggerablePoll(socket, event); status != OK) {
                return status;
            }
            if (!havePolled) havePolled = true;
        }
    }
}

} // namespace android
