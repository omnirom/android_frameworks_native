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

#include <android/hardware_buffer.h>
#include <android/native_window.h>
#include <gui/BufferItemConsumer.h>
#include <gui/Surface.h>
#include <hardware/gralloc.h>
#include <system/window.h>
#include <ui/DisplayId.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <utils/Errors.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

namespace android {

class VirtualDisplayThreadManager;

/**
 * VirtualDisplayThread runs tasks for a Virtual Display on a separate thread.
 *
 * This is useful for tasks that interact with the application-provided sink surface, which can
 * arbitrarily block or deadlock.
 *
 * Tasks are submitted to the thread via submitWork, which queues the task and notifies the thread.
 * The thread then processes tasks in a FIFO manner.
 *
 * The thread can be destroyed via destroy(), which submits a final task to set the state to
 * SHUT_DOWN. No more tasks can be scheduled after destroy(). The thread will then exit after
 * processing these tasks.
 *
 * The thread can be checked for freezing via isFrozen, which returns true if the thread has not
 * processed a task in a certain amount of time. This is useful for detecting deadlocks in the
 * application-provided sink surface.
 */
class VirtualDisplayThread : public std::enable_shared_from_this<VirtualDisplayThread> {
public:
    using Task = std::function<void()>;

    static std::shared_ptr<VirtualDisplayThread> create();
    void destroy();

    /**
     * An RAII handle to a VirtualDisplayThread.
     *
     * It can only be constructed by the VirtualDisplayThreadManager. On destruction, it notifies
     * the VirtualDisplayThreadManager.
     */
    class Client {
    public:
        ~Client();

        Client(const Client&) = delete;
        Client(Client&&) = delete;
        Client& operator=(const Client&) = delete;
        Client& operator=(Client&&) = delete;

        bool isFrozen() const;
        void submitWork(Task&& task);

    private:
        friend VirtualDisplayThreadManager;

        explicit Client(uid_t uid, std::shared_ptr<VirtualDisplayThread> thread);

        const uid_t mUid;
        std::shared_ptr<VirtualDisplayThread> mThread;
    };

    Client createClient();

private:
    enum State : uint8_t { INITIALIZING, RUNNING, SHUTTING_DOWN, SHUT_DOWN };

    void init();

    static void vdThreadMain(std::shared_ptr<VirtualDisplayThread> self);
    bool vdThreadPollAndWork();

    void submitWorkLocked(Task&& task);
    void clearQueueLocked();

    std::thread mThread;

    mutable std::mutex mWorkMutex;
    std::condition_variable mCondVar;
    std::atomic<State> mState = INITIALIZING;
    std::queue<std::function<void()>> mWorkQueue;
    bool mIsWorking = false;
    std::chrono::time_point<std::chrono::steady_clock> mLastWorkStartedTimeNs;
};

} // namespace android