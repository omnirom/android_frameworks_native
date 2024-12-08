/**
 * Copyright 2024 The Android Open Source Project
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

#include <queue>
#include <string>

#include <android-base/result.h>
#include <gtest/gtest.h>
#include <input/InputTransport.h>
#include <utils/Errors.h>

namespace android {

class TestInputChannel final : public InputChannel {
public:
    explicit TestInputChannel(const std::string& name);

    /**
     * Enqueues a message in mReceivedMessages.
     */
    void enqueueMessage(const InputMessage& message);

    /**
     * Pushes message to mSentMessages. In the default implementation, InputChannel sends messages
     * through a file descriptor. TestInputChannel, on the contrary, stores sent messages in
     * mSentMessages for assertion reasons.
     */
    status_t sendMessage(const InputMessage* message) override;

    /**
     * Returns an InputMessage from mReceivedMessages. This is done instead of retrieving data
     * directly from fd.
     */
    base::Result<InputMessage> receiveMessage() override;

    /**
     * Returns if mReceivedMessages is not empty.
     */
    bool probablyHasInput() const override;

    void assertFinishMessage(uint32_t seq, bool handled);

    void assertNoSentMessages() const;

private:
    // InputMessages received by the endpoint.
    std::queue<InputMessage> mReceivedMessages;
    // InputMessages sent by the endpoint.
    std::queue<InputMessage> mSentMessages;
};
} // namespace android
