/*
 * Copyright 2025 The Android Open Source Project
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

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include <android-base/expected.h>
#include <android-base/thread_annotations.h>
#include <android-base/unique_fd.h>
#include <ftl/finalizer.h>
#include <ftl/flags.h>
#include <ftl/function.h>

#include "test_framework/core/AsyncFunction.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

class PollFdThread final {
    struct Passkey;  // Uses the passkey idiom to restrict construction.

  public:
    enum class PollFlags : uint8_t {
        IN = 0x01,   // EPOLLIN
        OUT = 0x04,  // EPOLLOUT
        ERR = 0x08,  // EPOLLERR
    };
    using PollFlagMask = ftl::Flags<PollFlags>;

    using EventCallback = ftl::Function<void()>;

    [[nodiscard]] static auto make() -> base::expected<std::shared_ptr<PollFdThread>, std::string>;

    explicit PollFdThread(Passkey passkey);

    // Adds the given file descriptor to the polling set, and invokes the event callback for events.
    void addFileDescriptor(int descriptor, PollFlagMask flags, EventCallback callback);

    // Removes the given file descriptor from the polling set.
    void removeFileDescriptor(int descriptor);

  private:
    using AsyncEventCallback = core::AsyncFunctionStd<void()>;
    using CallbackMap = std::unordered_map<int, AsyncEventCallback>;

    [[nodiscard]] auto init() -> base::expected<void, std::string>;

    void threadMain();
    void wakeNow();
    void stopThread();

    base::unique_fd mEventFd;
    base::unique_fd mPollFd;
    std::thread mThread;
    std::atomic_bool mStop{false};
    std::mutex mMutex;
    CallbackMap mCallbacks GUARDED_BY(mMutex);

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl mCleanup;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
