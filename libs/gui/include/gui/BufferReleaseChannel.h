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

#pragma once

#include <string>
#include <vector>

#include <android-base/unique_fd.h>

#include <binder/Parcelable.h>
#include <gui/ITransactionCompletedListener.h>
#include <ui/Fence.h>
#include <utils/Errors.h>

namespace android::gui {

/**
 * IPC wrapper to pass release fences from SurfaceFlinger to apps via a local unix domain socket.
 */
class BufferReleaseChannel {
private:
    class Endpoint {
    public:
        Endpoint(std::string name, android::base::unique_fd fd)
              : mName(std::move(name)), mFd(std::move(fd)) {}
        Endpoint() {}

        Endpoint(Endpoint&&) noexcept = default;
        Endpoint& operator=(Endpoint&&) noexcept = default;

        Endpoint(const Endpoint&) = delete;
        void operator=(const Endpoint&) = delete;

        const android::base::unique_fd& getFd() const { return mFd; }

    protected:
        std::string mName;
        android::base::unique_fd mFd;
    };

public:
    class ConsumerEndpoint : public Endpoint {
    public:
        ConsumerEndpoint(std::string name, android::base::unique_fd fd)
              : Endpoint(std::move(name), std::move(fd)) {}

        /**
         * Reads a release fence from the BufferReleaseChannel.
         *
         * Returns OK on success.
         * Returns WOULD_BLOCK if there is no fence present.
         * Other errors probably indicate that the channel is broken.
         */
        status_t readReleaseFence(ReleaseCallbackId& outReleaseCallbackId,
                                  sp<Fence>& outReleaseFence, uint32_t& maxAcquiredBufferCount);

    private:
        std::vector<uint8_t> mFlattenedBuffer;
    };

    class ProducerEndpoint : public Endpoint, public Parcelable {
    public:
        ProducerEndpoint(std::string name, android::base::unique_fd fd)
              : Endpoint(std::move(name), std::move(fd)) {}
        ProducerEndpoint() {}

        status_t readFromParcel(const android::Parcel* parcel) override;
        status_t writeToParcel(android::Parcel* parcel) const override;

        status_t writeReleaseFence(const ReleaseCallbackId&, const sp<Fence>& releaseFence,
                                   uint32_t maxAcquiredBufferCount);

    private:
        std::vector<uint8_t> mFlattenedBuffer;
    };

    /**
     * Create two endpoints that make up the BufferReleaseChannel.
     *
     * Return OK on success.
     */
    static status_t open(const std::string name, std::unique_ptr<ConsumerEndpoint>& outConsumer,
                         std::shared_ptr<ProducerEndpoint>& outProducer);

    struct Message : public Flattenable<Message> {
        ReleaseCallbackId releaseCallbackId;
        sp<Fence> releaseFence = Fence::NO_FENCE;
        uint32_t maxAcquiredBufferCount;

        Message() = default;
        Message(ReleaseCallbackId releaseCallbackId, sp<Fence> releaseFence,
                uint32_t maxAcquiredBufferCount)
              : releaseCallbackId{releaseCallbackId},
                releaseFence{std::move(releaseFence)},
                maxAcquiredBufferCount{maxAcquiredBufferCount} {}

        // Flattenable protocol
        size_t getFlattenedSize() const;

        size_t getFdCount() const { return releaseFence->getFdCount(); }

        status_t flatten(void*& buffer, size_t& size, int*& fds, size_t& count) const;

        status_t unflatten(void const*& buffer, size_t& size, int const*& fds, size_t& count);

    private:
        size_t getPodSize() const;
    };
};

} // namespace android::gui
