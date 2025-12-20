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
#include <unistd.h>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>

#include <bits/epoll_event.h>
#include <linux/eventfd.h>
#include <linux/eventpoll.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android-base/thread_annotations.h>  // NOLINT(misc-include-cleaner)
#include <android-base/unique_fd.h>
#include <fmt/format.h>
#include <ftl/finalizer.h>
#include <ftl/ignore.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/surfaceflinger/PollFdThread.h"

namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger {

static_assert(EPOLLIN == static_cast<int>(PollFdThread::PollFlags::IN));
static_assert(EPOLLOUT == static_cast<int>(PollFdThread::PollFlags::OUT));
static_assert(EPOLLERR == static_cast<int>(PollFdThread::PollFlags::ERR));

struct PollFdThread::Passkey final {};

auto PollFdThread::make() -> base::expected<std::shared_ptr<PollFdThread>, std::string> {
    using namespace std::string_literals;

    auto instance = std::make_unique<PollFdThread>(Passkey{});
    if (instance == nullptr) {
        return base::unexpected("Failed to construct a PollFdThread"s);
    }
    if (auto result = instance->init(); !result) {
        return base::unexpected("Failed to init a PollFdThread: " + result.error());
    }
    return std::move(instance);
}

PollFdThread::PollFdThread(Passkey passkey) {
    ftl::ignore(passkey);
}

auto PollFdThread::init() -> base::expected<void, std::string> {
    mEventFd = base::unique_fd(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
    CHECK(mEventFd.get() >= 0);
    mPollFd = base::unique_fd(epoll_create1(EPOLL_CLOEXEC));
    CHECK(mPollFd.get() >= 0);
    addFileDescriptor(mEventFd.get(), {PollFlags::IN, PollFlags::ERR}, [this]() {
        int64_t ignoredCount = 0;
        read(mEventFd.get(), &ignoredCount, sizeof(ignoredCount));
    });

    mThread = std::thread(&PollFdThread::threadMain, this);

    mCleanup = ftl::Finalizer([this]() {
        stopThread();

        const std::lock_guard lock(mMutex);
        mCallbacks.erase(mEventFd.get());
        CHECK(mCallbacks.empty()) << "Not all finalizers were called.";
    });

    return {};
}

// PollFlagMask (ftl::Flags) is not trivially copyable, but should be.
// NOLINTNEXTLINE(performance-unnecessary-value-param)
void PollFdThread::addFileDescriptor(int descriptor, PollFlagMask flags, EventCallback callback) {
    LOG(VERBOSE) << __func__ << " descriptor " << descriptor << " mask " << flags.string();

    ftl::FinalizerStd cleanup;
    {
        const std::lock_guard lock(mMutex);
        cleanup = mCallbacks[descriptor].set(callback);
    }
    cleanup();

    epoll_event event{.events = flags.get(), .data = {.fd = descriptor}};
    const int result = epoll_ctl(mPollFd.get(), EPOLL_CTL_ADD, descriptor, &event);

    if (result != 0) {
        auto err = std::generic_category().default_error_condition(errno);
        LOG(FATAL) << fmt::format("Result {} errno: {} ({})", result, err.message(), err.value());
    }
}

void PollFdThread::removeFileDescriptor(int descriptor) {
    LOG(VERBOSE) << __func__ << " descriptor " << descriptor;

    {
        const std::lock_guard lock(mMutex);
        mCallbacks.erase(descriptor);
    }

    CHECK(epoll_ctl(mPollFd.get(), EPOLL_CTL_DEL, descriptor, nullptr) == 0);
}

void PollFdThread::threadMain() {
    LOG(VERBOSE) << __func__ << " begins";
    while (!mStop) {
        constexpr int kMaxEvents = 10;
        std::array<epoll_event, kMaxEvents> events = {};
        LOG(VERBOSE) << __func__ << " epoll_wait()";
        const int eventCount = epoll_wait(mPollFd.get(), events.data(), kMaxEvents, -1);
        if (eventCount < 0 && errno == EINTR) {
            LOG(VERBOSE) << "epoll_wait() interrupted";
            continue;
        }
        LOG(VERBOSE) << __func__ << " " << eventCount << " events";

        if (eventCount < 0) {
            auto err = std::generic_category().default_error_condition(errno);
            LOG(FATAL) << fmt::format("Result {} errno: {} ({})", eventCount, err.message(),
                                      err.value());
        }

        const std::lock_guard lock(mMutex);
        for (int i = 0; i < eventCount; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
            if (const auto found = mCallbacks.find(events[i].data.fd); found != mCallbacks.end()) {
                found->second();
            }
        }
    }
    LOG(VERBOSE) << __func__ << " ends";
}

void PollFdThread::wakeNow() {
    const int64_t one = 1;
    write(mEventFd.get(), &one, sizeof(one));
}

void PollFdThread::stopThread() {
    if (mThread.joinable()) {
        mStop = true;
        wakeNow();
        LOG(VERBOSE) << "Waiting for thread to stop...";
        mThread.join();
        LOG(VERBOSE) << "Stopped.";
    }
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::surfaceflinger
