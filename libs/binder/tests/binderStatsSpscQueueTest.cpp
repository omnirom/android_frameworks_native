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
#include <gtest/gtest.h>
#include <log/log.h>
#include <algorithm>
#include <atomic>
#include <latch>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <../BinderStatsSpscQueue.h>
#include <../BinderStatsUtils.h> // Include the definition of BinderCallData

using namespace android;

namespace android {
// Comparison operators for BinderCallData, used in tests.
bool operator<(const BinderCallData& lhs, const BinderCallData& rhs) {
    return std::tie(lhs.transactionCode, lhs.senderUid, lhs.interfaceDescriptor, lhs.startTimeNanos,
                    lhs.endTimeNanos) < std::tie(rhs.transactionCode, rhs.senderUid,
                                                 rhs.interfaceDescriptor, rhs.startTimeNanos,
                                                 rhs.endTimeNanos);
}

bool operator==(const BinderCallData& lhs, const BinderCallData& rhs) {
    return lhs.transactionCode == rhs.transactionCode && lhs.senderUid == rhs.senderUid &&
            lhs.interfaceDescriptor == rhs.interfaceDescriptor &&
            lhs.startTimeNanos == rhs.startTimeNanos && lhs.endTimeNanos == rhs.endTimeNanos;
}
} // namespace android
namespace {

// Test fixture for BinderStatsSpscQueue
class SpscQueueTest : public ::testing::Test {
protected:
    BinderStatsSpscQueue queue;
};

// Test pushing and popping a single item
TEST_F(SpscQueueTest, SinglePushPop) {
    BinderCallData data = {.interfaceDescriptor = String16("IFoo"),
                           .transactionCode = 1,
                           .startTimeNanos = 1000,
                           .endTimeNanos = 2000,
                           .senderUid = 1001};
    ASSERT_TRUE(queue.push(data));
    ASSERT_EQ(queue.length(), 1);
    std::optional<BinderCallData> popped = queue.tryPop();
    ASSERT_TRUE(popped.has_value());
    ASSERT_EQ(popped.value(), data);
    ASSERT_EQ(queue.length(), 0);
}

// Test pushing and popping multiple items
TEST_F(SpscQueueTest, MultiplePushPop) {
    std::vector<BinderCallData> data_vec;
    for (int i = 0; i < BinderStatsSpscQueue::kQueueSize; ++i) {
        BinderCallData data = {.interfaceDescriptor =
                                       String16(("Iface" + std::to_string(i)).c_str()),
                               .transactionCode = static_cast<uint32_t>(i),
                               .startTimeNanos = i * 100,
                               .endTimeNanos = (i + 1) * 100,
                               .senderUid = static_cast<uid_t>(1000 + i)};
        data_vec.push_back(data);
        ASSERT_TRUE(queue.push(data));
    }
    ASSERT_EQ(queue.length(), BinderStatsSpscQueue::kQueueSize);

    for (int i = 0; i < BinderStatsSpscQueue::kQueueSize; ++i) {
        std::optional<BinderCallData> popped = queue.tryPop();
        ASSERT_TRUE(popped.has_value());
        ASSERT_EQ(popped.value(), data_vec[i]);
    }
    ASSERT_EQ(queue.length(), 0);
}

// Test pushing to a full queue
TEST_F(SpscQueueTest, FullQueue) {
    for (int i = 0; i < BinderStatsSpscQueue::kQueueSize; ++i) {
        BinderCallData data = {.transactionCode = static_cast<uint32_t>(i)};
        ASSERT_TRUE(queue.push(data));
    }
    BinderCallData extra_data = {.transactionCode = BinderStatsSpscQueue::kQueueSize + 1};
    ASSERT_FALSE(queue.push(extra_data)); // Should fail as queue is full
    ASSERT_EQ(queue.length(), BinderStatsSpscQueue::kQueueSize);
}

// Test popping from an empty queue
TEST_F(SpscQueueTest, EmptyQueue) {
    std::optional<BinderCallData> popped = queue.tryPop();
    ASSERT_FALSE(popped.has_value());
}

// Test concurrent push and non-blocking pop
TEST_F(SpscQueueTest, ConcurrentPushNonBlockingPop) {
    std::vector<BinderCallData> pushed_values;
    std::vector<BinderCallData> popped_values;
    // Latch to synchronize the producer and consumer threads
    std::latch startTest{2};

    // Start with some data in the queue
    for (int i = 0; i < BinderStatsSpscQueue::kQueueSize / 2; ++i) {
        BinderCallData data = {.transactionCode = static_cast<uint32_t>(i)};
        queue.push(data);
        pushed_values.push_back(data);
    }

    std::thread producer([this, &pushed_values, &startTest]() {
        startTest.count_down();
        startTest.wait();
        for (int i = BinderStatsSpscQueue::kQueueSize / 2; i < BinderStatsSpscQueue::kQueueSize;
             ++i) {
            BinderCallData data = {.transactionCode = static_cast<uint32_t>(i)};
            while (!queue.push(data)) {
                // Spin-wait if push fails (queue is temporarily full)
                std::this_thread::yield();
            }
            pushed_values.push_back(data);
        }
    });

    // Capture producer by reference to check its state.
    std::thread consumer([this, &popped_values, &startTest, &producer]() {
        startTest.count_down();
        startTest.wait();
        size_t popped_count = 0;
        // Try to pop BinderStatsSpscQueue::kQueueSize items, but stop if the queue becomes empty
        while (popped_count < BinderStatsSpscQueue::kQueueSize) {
            std::optional<BinderCallData> popped = queue.tryPop();
            if (popped.has_value()) {
                popped_values.push_back(popped.value());
                popped_count++;
            } else {
                // Queue might be temporarily empty, yield and retry
                std::this_thread::yield();
            }
        }
    });

    producer.join(); // Wait for producer to finish
    consumer.join(); // Wait for consumer to finish

    ALOGI("Size of pushed_values: %zu", pushed_values.size());
    ALOGI("Size of popped_values: %zu", popped_values.size());

    ASSERT_EQ(popped_values.size(), pushed_values.size());
    ASSERT_EQ(popped_values, pushed_values);
}

// Helper function to create dummy BinderCallData
BinderCallData createDummyData(int id) {
    return {.interfaceDescriptor = String16(("ITest" + std::to_string(id)).c_str()),
            .transactionCode = static_cast<uint32_t>(id),
            .startTimeNanos = id * 100,
            .endTimeNanos = (id + 1) * 100,
            .senderUid = static_cast<uid_t>(1000 + id)};
}

// Test fixture for BinderStatsCollector
class BinderStatsCollectorTest : public ::testing::Test {
protected:
    BinderStatsCollector collector;
};

// Test registering a single queue and consuming data from it
TEST_F(BinderStatsCollectorTest, RegisterSingleQueueAndConsume) {
    auto queue = std::make_shared<BinderStatsSpscQueue>();
    ASSERT_NE(queue, nullptr);

    BinderCallData data1 = createDummyData(1);
    BinderCallData data2 = createDummyData(2);

    ASSERT_TRUE(queue->push(data1));
    ASSERT_TRUE(queue->push(data2));
    ASSERT_EQ(queue->length(), 2);

    collector.registerQueue(queue);

    std::vector<BinderCallData> consumedData = collector.consumeData();

    ASSERT_EQ(consumedData.size(), 2);
    // consumeData doesn't guarantee order if multiple queues exist,
    // but with one queue, the order should be preserved.
    EXPECT_EQ(consumedData[0], data1);
    EXPECT_EQ(consumedData[1], data2);

    // Verify the queue is now empty
    EXPECT_EQ(queue->length(), 0);
    EXPECT_FALSE(queue->tryPop().has_value());
}

// Test registering multiple queues and consuming data
TEST_F(BinderStatsCollectorTest, RegisterMultipleQueuesAndConsume) {
    auto queue1 = std::make_shared<BinderStatsSpscQueue>();
    auto queue2 = std::make_shared<BinderStatsSpscQueue>();
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    BinderCallData data1 = createDummyData(1);
    BinderCallData data2 = createDummyData(2);
    BinderCallData data3 = createDummyData(3);

    ASSERT_TRUE(queue1->push(data1));
    ASSERT_TRUE(queue2->push(data2));
    ASSERT_TRUE(queue1->push(data3));

    ASSERT_EQ(queue1->length(), 2);
    ASSERT_EQ(queue2->length(), 1);

    collector.registerQueue(queue1);
    collector.registerQueue(queue2);

    std::vector<BinderCallData> consumedData = collector.consumeData();

    ASSERT_EQ(consumedData.size(), 3);

    // Verify the queues are now empty
    EXPECT_EQ(queue1->length(), 0);
    EXPECT_EQ(queue2->length(), 0);
    EXPECT_FALSE(queue1->tryPop().has_value());
    EXPECT_FALSE(queue2->tryPop().has_value());

    std::vector<BinderCallData> expected = {data1, data2, data3};
    std::sort(expected.begin(), expected.end());
    std::sort(consumedData.begin(), consumedData.end());
    EXPECT_EQ(consumedData, expected);
}

// Test consuming data when no queues are registered
TEST_F(BinderStatsCollectorTest, ConsumeWithNoQueues) {
    std::vector<BinderCallData> consumedData = collector.consumeData();
    EXPECT_TRUE(consumedData.empty());
}

// Test consuming data from empty queues
TEST_F(BinderStatsCollectorTest, ConsumeFromEmptyQueues) {
    auto queue1 = std::make_shared<BinderStatsSpscQueue>();
    auto queue2 = std::make_shared<BinderStatsSpscQueue>();
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    collector.registerQueue(queue1);
    collector.registerQueue(queue2);

    std::vector<BinderCallData> consumedData = collector.consumeData();
    EXPECT_TRUE(consumedData.empty());
    EXPECT_EQ(queue1->length(), 0);
    EXPECT_EQ(queue2->length(), 0);
}

// Test deregistering a queue
TEST_F(BinderStatsCollectorTest, DeregisterQueue) {
    auto queue1 = std::make_shared<BinderStatsSpscQueue>();
    auto queue2 = std::make_shared<BinderStatsSpscQueue>();
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    BinderCallData data1 = createDummyData(1);
    BinderCallData data2 = createDummyData(2);
    BinderCallData data3 = createDummyData(3); // Will be added after deregister
    BinderCallData data4 = createDummyData(4); // Will be added after deregister

    // Register both queues
    collector.registerQueue(queue1);
    collector.registerQueue(queue2);

    // Add initial data
    ASSERT_TRUE(queue1->push(data1));
    ASSERT_TRUE(queue2->push(data2));
    ASSERT_EQ(queue1->length(), 1);
    ASSERT_EQ(queue2->length(), 1);

    // Deregister queue1
    collector.deregisterQueue(queue1);

    // Consume data - this should consume data1 and data2, and mark queue1 for deletion
    std::vector<BinderCallData> consumedData1 = collector.consumeData();
    ASSERT_EQ(consumedData1.size(), 2);
    EXPECT_EQ(queue1->length(), 0); // queue1 should be empty now
    EXPECT_EQ(queue2->length(), 0);

    // Add new data *after* deregistering and consuming once
    ASSERT_TRUE(queue1->push(data3)); // Data pushed to the deregistered queue
    ASSERT_TRUE(queue2->push(data4)); // Data pushed to the still registered queue
    ASSERT_EQ(queue1->length(), 1);
    ASSERT_EQ(queue2->length(), 1);

    // Consume data again - this should only consume data4 from queue2
    // because queue1 should have been removed during the cleanup phase
    // of the previous consumeData call.
    std::vector<BinderCallData> consumedData2 = collector.consumeData();
    ASSERT_EQ(consumedData2.size(), 1);

    // Verify only data4 was consumed
    bool found4 = false;
    for (const auto& item : consumedData2) {
        if (item == data4) found4 = true;
    }
    EXPECT_TRUE(found4);

    // Verify queue1 still contains data3 (it wasn't consumed) and queue2 is empty
    EXPECT_EQ(queue1->length(), 1);
    EXPECT_EQ(queue2->length(), 0);
    auto remainingInQueue1 = queue1->tryPop();
    ASSERT_TRUE(remainingInQueue1.has_value());
    EXPECT_EQ(remainingInQueue1.value(), data3);
}

// Test deregistering a queue that was never registered
TEST_F(BinderStatsCollectorTest, DeregisterNonExistentQueue) {
    auto queue1 = std::make_shared<BinderStatsSpscQueue>();
    auto queue2 = std::make_shared<BinderStatsSpscQueue>();
    ASSERT_NE(queue1, nullptr);
    ASSERT_NE(queue2, nullptr);

    collector.registerQueue(queue1);
    // Attempt to deregister queue2 which was never registered
    collector.deregisterQueue(queue2);

    // Add data to queue1
    BinderCallData data1 = createDummyData(1);
    ASSERT_TRUE(queue1->push(data1));

    // Consume data - should still get data from queue1
    std::vector<BinderCallData> consumedData = collector.consumeData();
    ASSERT_EQ(consumedData.size(), 1);
    EXPECT_EQ(consumedData[0], data1);
    EXPECT_EQ(queue1->length(), 0);
}

// Test concurrent push from multiple queues and a single consumer
TEST_F(BinderStatsCollectorTest, ConcurrentMultiQueuePushAndConsume) {
    const int numProducerThreads = 4;
    const int itemsPerThread = 1000 * BinderStatsSpscQueue::kQueueSize;
    const int totalItems = numProducerThreads * itemsPerThread;

    std::vector<std::shared_ptr<BinderStatsSpscQueue>> queues;
    for (int i = 0; i < numProducerThreads; ++i) {
        auto queue = std::make_shared<BinderStatsSpscQueue>();
        ASSERT_NE(queue, nullptr);
        queues.push_back(queue);
    }

    std::vector<std::thread> producers;
    std::vector<BinderCallData> allPushedItems;
    allPushedItems.reserve(totalItems);
    // populate allPushedItems
    for (int i = 0; i < numProducerThreads; ++i) {
        for (int j = 0; j < itemsPerThread; ++j) {
            // Create unique data for each item from each thread
            BinderCallData data = createDummyData(i * itemsPerThread + j);
            allPushedItems.push_back(data);
        }
    }

    // Latch to synchronize the start of all threads
    std::latch startTestLatch(numProducerThreads + 1);

    for (int i = 0; i < numProducerThreads; ++i) {
        producers.emplace_back([&, i]() {
            startTestLatch.count_down();
            startTestLatch.wait();
            collector.registerQueue(queues[i]);
            std::vector<BinderCallData> threadPushedItems;
            for (int j = 0; j < itemsPerThread; ++j) {
                // Create unique data for each item from each thread
                BinderCallData data = createDummyData(i * itemsPerThread + j);
                while (!queues[i]->push(data)) {
                    std::this_thread::yield();
                }
                threadPushedItems.push_back(data);
            }
            collector.deregisterQueue(queues[i]);
        });
    }

    std::vector<BinderCallData> consumedItems;
    consumedItems.reserve(totalItems);

    std::thread consumer([&]() {
        startTestLatch.count_down();
        startTestLatch.wait();
        while (consumedItems.size() < totalItems) {
            std::vector<BinderCallData> batch = collector.consumeData();
            consumedItems.insert(consumedItems.end(), batch.begin(), batch.end());
            if (batch.empty() && consumedItems.size() < totalItems) {
                std::this_thread::yield(); // Avoid busy-waiting if consumer is faster
            }
        }
    });

    for (auto& producer : producers) {
        producer.join();
    }
    consumer.join();

    ASSERT_EQ(consumedItems.size(), totalItems);
    ASSERT_EQ(allPushedItems.size(), totalItems);

    // The order of items from different queues isn't guaranteed by consumeData,
    // so we sort both lists before comparing.
    std::sort(allPushedItems.begin(), allPushedItems.end());
    std::sort(consumedItems.begin(), consumedItems.end());
    EXPECT_EQ(consumedItems, allPushedItems);

    // Verify all queues are empty
    for (const auto& q : queues) {
        EXPECT_EQ(q->length(), 0);
    }
}

} // namespace
