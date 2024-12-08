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

#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <gui/BufferReleaseChannel.h>

using namespace std::string_literals;
using android::gui::BufferReleaseChannel;

namespace android {

namespace {

// Helper function to check if two file descriptors point to the same file.
bool is_same_file(int fd1, int fd2) {
    struct stat stat1;
    if (fstat(fd1, &stat1) != 0) {
        return false;
    }
    struct stat stat2;
    if (fstat(fd2, &stat2) != 0) {
        return false;
    }
    return (stat1.st_dev == stat2.st_dev) && (stat1.st_ino == stat2.st_ino);
}

} // namespace

TEST(BufferReleaseChannelTest, MessageFlattenable) {
    ReleaseCallbackId releaseCallbackId{1, 2};
    sp<Fence> releaseFence = sp<Fence>::make(memfd_create("fake-fence-fd", 0));
    uint32_t maxAcquiredBufferCount = 5;

    std::vector<uint8_t> dataBuffer;
    std::vector<int> fdBuffer;

    // Verify that we can flatten a message
    {
        BufferReleaseChannel::Message message{releaseCallbackId, releaseFence,
                                              maxAcquiredBufferCount};

        dataBuffer.resize(message.getFlattenedSize());
        void* dataPtr = dataBuffer.data();
        size_t dataSize = dataBuffer.size();

        fdBuffer.resize(message.getFdCount());
        int* fdPtr = fdBuffer.data();
        size_t fdSize = fdBuffer.size();

        ASSERT_EQ(OK, message.flatten(dataPtr, dataSize, fdPtr, fdSize));

        // Fence's unique_fd uses fdsan to check ownership of the file descriptor. Normally the file
        // descriptor is passed through the Unix socket and duplicated (and sent to another process)
        // so there's no problem with duplicate file descriptor ownership. For this unit test, we
        // need to set up a duplicate file descriptor to avoid crashing due to duplicate ownership.
        ASSERT_EQ(releaseFence->get(), fdBuffer[0]);
        fdBuffer[0] = message.releaseFence->dup();
    }

    // Verify that we can unflatten a message
    {
        BufferReleaseChannel::Message message;

        const void* dataPtr = dataBuffer.data();
        size_t dataSize = dataBuffer.size();

        const int* fdPtr = fdBuffer.data();
        size_t fdSize = fdBuffer.size();

        ASSERT_EQ(OK, message.unflatten(dataPtr, dataSize, fdPtr, fdSize));
        ASSERT_EQ(releaseCallbackId, message.releaseCallbackId);
        ASSERT_TRUE(is_same_file(releaseFence->get(), message.releaseFence->get()));
        ASSERT_EQ(maxAcquiredBufferCount, message.maxAcquiredBufferCount);
    }
}

// Verify that the BufferReleaseChannel consume returns WOULD_BLOCK when there's no message
// available.
TEST(BufferReleaseChannelTest, ConsumerEndpointIsNonBlocking) {
    std::unique_ptr<BufferReleaseChannel::ConsumerEndpoint> consumer;
    std::shared_ptr<BufferReleaseChannel::ProducerEndpoint> producer;
    ASSERT_EQ(OK, BufferReleaseChannel::open("test-channel"s, consumer, producer));

    ReleaseCallbackId releaseCallbackId;
    sp<Fence> releaseFence;
    uint32_t maxAcquiredBufferCount;
    ASSERT_EQ(WOULD_BLOCK,
              consumer->readReleaseFence(releaseCallbackId, releaseFence, maxAcquiredBufferCount));
}

// Verify that we can write a message to the BufferReleaseChannel producer and read that message
// using the BufferReleaseChannel consumer.
TEST(BufferReleaseChannelTest, ProduceAndConsume) {
    std::unique_ptr<BufferReleaseChannel::ConsumerEndpoint> consumer;
    std::shared_ptr<BufferReleaseChannel::ProducerEndpoint> producer;
    ASSERT_EQ(OK, BufferReleaseChannel::open("test-channel"s, consumer, producer));

    sp<Fence> fence = sp<Fence>::make(memfd_create("fake-fence-fd", 0));

    for (uint64_t i = 0; i < 64; i++) {
        ReleaseCallbackId producerId{i, i + 1};
        uint32_t maxAcquiredBufferCount = i + 2;
        ASSERT_EQ(OK, producer->writeReleaseFence(producerId, fence, maxAcquiredBufferCount));
    }

    for (uint64_t i = 0; i < 64; i++) {
        ReleaseCallbackId expectedId{i, i + 1};
        uint32_t expectedMaxAcquiredBufferCount = i + 2;

        ReleaseCallbackId consumerId;
        sp<Fence> consumerFence;
        uint32_t maxAcquiredBufferCount;
        ASSERT_EQ(OK,
                  consumer->readReleaseFence(consumerId, consumerFence, maxAcquiredBufferCount));

        ASSERT_EQ(expectedId, consumerId);
        ASSERT_TRUE(is_same_file(fence->get(), consumerFence->get()));
        ASSERT_EQ(expectedMaxAcquiredBufferCount, maxAcquiredBufferCount);
    }
}

} // namespace android