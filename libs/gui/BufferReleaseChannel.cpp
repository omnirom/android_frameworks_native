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

#define LOG_TAG "BufferReleaseChannel"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <android-base/result.h>
#include <android/binder_status.h>
#include <binder/Parcel.h>
#include <utils/Flattenable.h>

#include <gui/BufferReleaseChannel.h>
#include <private/gui/ParcelUtils.h>

using android::base::Result;

namespace android::gui {

namespace {

template <typename T>
static void readAligned(const void*& buffer, size_t& size, T& value) {
    size -= FlattenableUtils::align<alignof(T)>(buffer);
    FlattenableUtils::read(buffer, size, value);
}

template <typename T>
static void writeAligned(void*& buffer, size_t& size, T value) {
    size -= FlattenableUtils::align<alignof(T)>(buffer);
    FlattenableUtils::write(buffer, size, value);
}

template <typename T>
static void addAligned(size_t& size, T /* value */) {
    size = FlattenableUtils::align<sizeof(T)>(size);
    size += sizeof(T);
}

template <typename T>
static inline constexpr uint32_t low32(const T n) {
    return static_cast<uint32_t>(static_cast<uint64_t>(n));
}

template <typename T>
static inline constexpr uint32_t high32(const T n) {
    return static_cast<uint32_t>(static_cast<uint64_t>(n) >> 32);
}

template <typename T>
static inline constexpr T to64(const uint32_t lo, const uint32_t hi) {
    return static_cast<T>(static_cast<uint64_t>(hi) << 32 | lo);
}

} // namespace

size_t BufferReleaseChannel::Message::getPodSize() const {
    size_t size = 0;
    addAligned(size, low32(releaseCallbackId.bufferId));
    addAligned(size, high32(releaseCallbackId.bufferId));
    addAligned(size, low32(releaseCallbackId.framenumber));
    addAligned(size, high32(releaseCallbackId.framenumber));
    addAligned(size, maxAcquiredBufferCount);
    return size;
}

size_t BufferReleaseChannel::Message::getFlattenedSize() const {
    size_t size = releaseFence->getFlattenedSize();
    size = FlattenableUtils::align<4>(size);
    size += getPodSize();
    return size;
}

status_t BufferReleaseChannel::Message::flatten(void*& buffer, size_t& size, int*& fds,
                                                size_t& count) const {
    if (status_t err = releaseFence->flatten(buffer, size, fds, count); err != OK) {
        return err;
    }
    size -= FlattenableUtils::align<4>(buffer);

    // Check we still have enough space
    if (size < getPodSize()) {
        return NO_MEMORY;
    }

    writeAligned(buffer, size, low32(releaseCallbackId.bufferId));
    writeAligned(buffer, size, high32(releaseCallbackId.bufferId));
    writeAligned(buffer, size, low32(releaseCallbackId.framenumber));
    writeAligned(buffer, size, high32(releaseCallbackId.framenumber));
    writeAligned(buffer, size, maxAcquiredBufferCount);
    return OK;
}

status_t BufferReleaseChannel::Message::unflatten(void const*& buffer, size_t& size,
                                                  int const*& fds, size_t& count) {
    releaseFence = new Fence();
    if (status_t err = releaseFence->unflatten(buffer, size, fds, count); err != OK) {
        return err;
    }
    size -= FlattenableUtils::align<4>(buffer);

    // Check we still have enough space
    if (size < getPodSize()) {
        return OK;
    }

    uint32_t bufferIdLo = 0, bufferIdHi = 0;
    uint32_t frameNumberLo = 0, frameNumberHi = 0;

    readAligned(buffer, size, bufferIdLo);
    readAligned(buffer, size, bufferIdHi);
    releaseCallbackId.bufferId = to64<int64_t>(bufferIdLo, bufferIdHi);
    readAligned(buffer, size, frameNumberLo);
    readAligned(buffer, size, frameNumberHi);
    releaseCallbackId.framenumber = to64<uint64_t>(frameNumberLo, frameNumberHi);
    readAligned(buffer, size, maxAcquiredBufferCount);

    return OK;
}

status_t BufferReleaseChannel::ConsumerEndpoint::readReleaseFence(
        ReleaseCallbackId& outReleaseCallbackId, sp<Fence>& outReleaseFence,
        uint32_t& outMaxAcquiredBufferCount) {
    Message message;
    mFlattenedBuffer.resize(message.getFlattenedSize());
    std::array<uint8_t, CMSG_SPACE(sizeof(int))> controlMessageBuffer;

    iovec iov{
            .iov_base = mFlattenedBuffer.data(),
            .iov_len = mFlattenedBuffer.size(),
    };

    msghdr msg{
            .msg_iov = &iov,
            .msg_iovlen = 1,
            .msg_control = controlMessageBuffer.data(),
            .msg_controllen = controlMessageBuffer.size(),
    };

    int result;
    do {
        result = recvmsg(mFd, &msg, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return WOULD_BLOCK;
        }
        ALOGE("Error reading release fence from socket: error %#x (%s)", errno, strerror(errno));
        return UNKNOWN_ERROR;
    }

    if (msg.msg_iovlen != 1) {
        ALOGE("Error reading release fence from socket: bad data length");
        return UNKNOWN_ERROR;
    }

    if (msg.msg_controllen % sizeof(int) != 0) {
        ALOGE("Error reading release fence from socket: bad fd length");
        return UNKNOWN_ERROR;
    }

    size_t dataLen = msg.msg_iov->iov_len;
    const void* data = static_cast<const void*>(msg.msg_iov->iov_base);
    if (!data) {
        ALOGE("Error reading release fence from socket: no buffer data");
        return UNKNOWN_ERROR;
    }

    size_t fdCount = 0;
    const int* fdData = nullptr;
    if (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg)) {
        fdData = reinterpret_cast<const int*>(CMSG_DATA(cmsg));
        fdCount = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
    }

    if (status_t err = message.unflatten(data, dataLen, fdData, fdCount); err != OK) {
        return err;
    }

    outReleaseCallbackId = message.releaseCallbackId;
    outReleaseFence = std::move(message.releaseFence);
    outMaxAcquiredBufferCount = message.maxAcquiredBufferCount;

    return OK;
}

int BufferReleaseChannel::ProducerEndpoint::writeReleaseFence(const ReleaseCallbackId& callbackId,
                                                              const sp<Fence>& fence,
                                                              uint32_t maxAcquiredBufferCount) {
    Message message{callbackId, fence ? fence : Fence::NO_FENCE, maxAcquiredBufferCount};
    mFlattenedBuffer.resize(message.getFlattenedSize());
    int flattenedFd;
    {
        // Make copies of needed items since flatten modifies them, and we don't
        // want to send anything if there's an error during flatten.
        void* flattenedBufferPtr = mFlattenedBuffer.data();
        size_t flattenedBufferSize = mFlattenedBuffer.size();
        int* flattenedFdPtr = &flattenedFd;
        size_t flattenedFdCount = 1;
        if (status_t err = message.flatten(flattenedBufferPtr, flattenedBufferSize, flattenedFdPtr,
                                           flattenedFdCount);
            err != OK) {
            ALOGE("Failed to flatten BufferReleaseChannel message.");
            return err;
        }
    }

    iovec iov{
            .iov_base = mFlattenedBuffer.data(),
            .iov_len = mFlattenedBuffer.size(),
    };

    msghdr msg{
            .msg_iov = &iov,
            .msg_iovlen = 1,
    };

    std::array<uint8_t, CMSG_SPACE(sizeof(int))> controlMessageBuffer;
    if (fence && fence->isValid()) {
        msg.msg_control = controlMessageBuffer.data();
        msg.msg_controllen = controlMessageBuffer.size();

        cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &flattenedFd, sizeof(int));
    }

    int result;
    do {
        result = sendmsg(mFd, &msg, 0);
    } while (result == -1 && errno == EINTR);
    if (result == -1) {
        ALOGD("Error writing release fence to socket: error %#x (%s)", errno, strerror(errno));
        return -errno;
    }

    return OK;
}

status_t BufferReleaseChannel::ProducerEndpoint::readFromParcel(const android::Parcel* parcel) {
    if (!parcel) return STATUS_BAD_VALUE;
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &mName);
    SAFE_PARCEL(parcel->readUniqueFileDescriptor, &mFd);
    return STATUS_OK;
}

status_t BufferReleaseChannel::ProducerEndpoint::writeToParcel(android::Parcel* parcel) const {
    if (!parcel) return STATUS_BAD_VALUE;
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, mName);
    SAFE_PARCEL(parcel->writeUniqueFileDescriptor, mFd);
    return STATUS_OK;
}

status_t BufferReleaseChannel::open(std::string name,
                                    std::unique_ptr<ConsumerEndpoint>& outConsumer,
                                    std::shared_ptr<ProducerEndpoint>& outProducer) {
    outConsumer.reset();
    outProducer.reset();

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets)) {
        ALOGE("[%s] Failed to create socket pair. errorno=%d message='%s'", name.c_str(), errno,
              strerror(errno));
        return -errno;
    }

    android::base::unique_fd consumerFd(sockets[0]);
    android::base::unique_fd producerFd(sockets[1]);

    // Socket buffer size. The default is typically about 128KB, which is much larger than
    // we really need.
    size_t bufferSize = 32 * 1024;
    if (setsockopt(consumerFd.get(), SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) ==
        -1) {
        ALOGE("[%s] Failed to set consumer socket send buffer size. errno=%d message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }
    if (setsockopt(consumerFd.get(), SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) ==
        -1) {
        ALOGE("[%s] Failed to set consumer socket receive buffer size. errno=%d "
              "message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }
    if (setsockopt(producerFd.get(), SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(bufferSize)) ==
        -1) {
        ALOGE("[%s] Failed to set producer socket send buffer size. errno=%d message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }
    if (setsockopt(producerFd.get(), SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(bufferSize)) ==
        -1) {
        ALOGE("[%s] Failed to set producer socket receive buffer size. errno=%d "
              "message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }

    // Configure the consumer socket to be non-blocking.
    int flags = fcntl(consumerFd.get(), F_GETFL, 0);
    if (flags == -1) {
        ALOGE("[%s] Failed to get consumer socket flags. errno=%d message='%s'", name.c_str(),
              errno, strerror(errno));
        return -errno;
    }
    if (fcntl(consumerFd.get(), F_SETFL, flags | O_NONBLOCK) == -1) {
        ALOGE("[%s] Failed to set consumer socket to non-blocking mode. errno=%d "
              "message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }

    // Configure a timeout for the producer socket.
    const timeval timeout{.tv_sec = 1, .tv_usec = 0};
    if (setsockopt(producerFd.get(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeval)) == -1) {
        ALOGE("[%s] Failed to set producer socket timeout. errno=%d message='%s'", name.c_str(),
              errno, strerror(errno));
        return -errno;
    }

    // Make the consumer read-only
    if (shutdown(consumerFd.get(), SHUT_WR) == -1) {
        ALOGE("[%s] Failed to shutdown writing on consumer socket. errno=%d message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }

    // Make the producer write-only
    if (shutdown(producerFd.get(), SHUT_RD) == -1) {
        ALOGE("[%s] Failed to shutdown reading on producer socket. errno=%d message='%s'",
              name.c_str(), errno, strerror(errno));
        return -errno;
    }

    outConsumer = std::make_unique<ConsumerEndpoint>(name, std::move(consumerFd));
    outProducer = std::make_shared<ProducerEndpoint>(std::move(name), std::move(producerFd));
    return STATUS_OK;
}

} // namespace android::gui