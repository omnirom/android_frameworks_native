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

#include <array>
#include <atomic>
#include "BinderStatsUtils.h"

namespace android {

/**
 * Single producer Single consumer concurrent queue.
 * This is made to collect and store binder transaction data for Binder metrics.
 *
 * Each IPCThreadState has one of these and will push all collected data to this queue.
 *
 * Each IPCThreadState, in its constructor, should register this queue with the
 * BinderStatsCollector.
 *
 * The push and tryPop can be called concurrently from two threads.
 * The push should be called from a single thread at all times or be behind a lock.
 * The tryPop should be called from a single thread at all times or be behind a lock.
 *
 * Length is best effort and can be called from any thread.
 */
class BinderStatsSpscQueue {
public:
    static constexpr size_t kQueueSize = 128;
private:
    static_assert((kQueueSize & (kQueueSize - 1)) == 0, "Size must be a power of 2");
    static constexpr size_t CAPACITY = kQueueSize;

    // Cache-line aligned array of items
    alignas(64) std::array<BinderCallData, CAPACITY> mBuffer{};

    // Cache-line aligned counters to prevent false sharing
    alignas(64) std::atomic<uint64_t> mHead{0};
    alignas(64) std::atomic<uint64_t> mTail{0};

public:
    /**
     * @brief Attempts to push an item into the queue
     *
     * @param item The item to push (will be moved into the queue)
     * @return true if the item was successfully pushed
     * @return false if the queue was full
     */
    __attribute__((no_sanitize("unsigned-integer-overflow")))
    bool push(const BinderCallData& item) {
        const uint64_t head = mHead.load(std::memory_order_acquire);
        const uint64_t tail = mTail.load(std::memory_order_relaxed);

        // Check if full using distance
        if (tail - head == CAPACITY) {
            return false;
        }

        mBuffer.at(tail % CAPACITY) = item;
        mTail.store(tail + 1, std::memory_order_release);

        return true;
    }

    /**
     * @brief Best effort length of the queue. Best used for knowing if a lazy
     * consumer should start consuming.
     * @return approximate size of queue
     */
    __attribute__((no_sanitize("unsigned-integer-overflow")))
    size_t length() {
        const uint64_t head = mHead.load(std::memory_order_acquire);
        const uint64_t tail = mTail.load(std::memory_order_acquire);
        return tail - head;
    }

    /**
     * @brief Pops an item from the queue if available
     * @return std::optional<BinderCallData> The popped item
     */
    __attribute__((no_sanitize("unsigned-integer-overflow")))
    std::optional<BinderCallData> tryPop() {
        const uint64_t tail = mTail.load(std::memory_order_acquire);
        const uint64_t head = mHead.load(std::memory_order_relaxed);

        if (head != tail) {
            BinderCallData item = std::move(mBuffer.at(head % CAPACITY));
            mHead.store(head + 1, std::memory_order_release);
            return item;
        }
        return std::nullopt;
    }
};

/**
 * Keeps track of SpscQueues corresponding to threads producing binder stats
 * and allows the data from them to be consumed.
 * Each binder thread must register its queue at creation time and deregister it
 * before it is being destroyed
 */
class BinderStatsCollector {
public:
    /**
     * @brief Consumes all data in the registered queues
     *
     * This is a thread safe operation
     */
    std::vector<BinderCallData> consumeData() {
        std::vector<BinderCallData> data;
        std::vector<std::shared_ptr<BinderStatsSpscQueue>> queuesCopy;
        std::vector<std::shared_ptr<BinderStatsSpscQueue>> deregisteredQueuesCopy;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            queuesCopy = mQueues;
            deregisteredQueuesCopy.swap(mDeregisteredQueues);
        }
        // reserving some amount to reduce number of allocations.
        data.reserve(BinderStatsSpscQueue::kQueueSize);
        for (auto& queue : queuesCopy) {
            LOG_ALWAYS_FATAL_IF(queue == nullptr, "queue pointer null");
            while (auto item = queue->tryPop()) {
                data.emplace_back(std::move(*item));
            }
        }
        for (auto& queue : deregisteredQueuesCopy) {
            LOG_ALWAYS_FATAL_IF(queue == nullptr, "queue pointer null");
            while (auto item = queue->tryPop()) {
                data.emplace_back(std::move(*item));
            }
            queue.reset();
        }
        return data;
    }

    /**
     * @brief Register queue
     *
     * This is a thread safe operation
     */
    void registerQueue(std::shared_ptr<BinderStatsSpscQueue> spscQueue) {
        LOG_ALWAYS_FATAL_IF(spscQueue == nullptr, "queue pointer null");
        std::lock_guard<std::mutex> lock(mMutex);
        mQueues.push_back(spscQueue);
    }
    /**
     * @brief Deregister queue
     *
     * This is a thread safe operation
     */
    void deregisterQueue(std::shared_ptr<BinderStatsSpscQueue> spscQueue) {
        LOG_ALWAYS_FATAL_IF(spscQueue == nullptr, "queue pointer null");
        std::lock_guard<std::mutex> lock(mMutex);
        mDeregisteredQueues.push_back(spscQueue);
        auto it = std::find(mQueues.begin(), mQueues.end(), spscQueue);
        if (it != mQueues.end()) {
            mQueues.erase(it);
        }
    }

private:
    std::vector<std::shared_ptr<BinderStatsSpscQueue>> mQueues;
    std::vector<std::shared_ptr<BinderStatsSpscQueue>> mDeregisteredQueues;
    std::mutex mMutex;
};

} // namespace android