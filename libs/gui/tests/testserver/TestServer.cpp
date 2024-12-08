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

#define LOG_TAG "TestServer"

#include <android-base/stringprintf.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/Status.h>
#include <gui/BufferQueue.h>
#include <gui/IConsumerListener.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/view/Surface.h>
#include <libgui_test_server/BnTestServer.h>
#include <log/log.h>
#include <utils/Errors.h>

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include "TestServer.h"

namespace android {

namespace {
class TestConsumerListener : public BnConsumerListener {
    virtual void onFrameAvailable(const BufferItem&) override {}
    virtual void onBuffersReleased() override {}
    virtual void onSidebandStreamChanged() override {}
};

class TestServiceImpl : public libgui_test_server::BnTestServer {
public:
    TestServiceImpl(const char* name) : mName(name) {}

    virtual binder::Status createProducer(view::Surface* out) override {
        std::lock_guard<std::mutex> lock(mMutex);

        BufferQueueHolder bq;
        BufferQueue::createBufferQueue(&bq.producer, &bq.consumer);
        sp<TestConsumerListener> listener = sp<TestConsumerListener>::make();
        bq.consumer->consumerConnect(listener, /*controlledByApp*/ true);

        uint64_t id = 0;
        bq.producer->getUniqueId(&id);
        std::string name = base::StringPrintf("%s-%" PRIu64, mName, id);

        out->name = String16(name.c_str());
        out->graphicBufferProducer = bq.producer;
        mBqs.push_back(std::move(bq));

        return binder::Status::ok();
    }

    virtual binder::Status killNow() override {
        ALOGE("LibGUI Test Service %s dying in response to killNow", mName);
        _exit(0);
        // Not reached:
        return binder::Status::ok();
    }

private:
    std::mutex mMutex;
    const char* mName;

    struct BufferQueueHolder {
        sp<IGraphicBufferProducer> producer;
        sp<IGraphicBufferConsumer> consumer;
    };

    std::vector<BufferQueueHolder> mBqs;
};
} // namespace

int TestServerMain(const char* name) {
    ProcessState::self()->startThreadPool();

    sp<TestServiceImpl> testService = sp<TestServiceImpl>::make(name);
    ALOGE("service");
    sp<IServiceManager> serviceManager(defaultServiceManager());
    LOG_ALWAYS_FATAL_IF(OK != serviceManager->addService(String16(name), testService));

    ALOGD("LibGUI Test Service %s STARTED", name);

    IPCThreadState::self()->joinThreadPool();

    ALOGW("LibGUI Test Service %s DIED", name);

    return 0;
}

} // namespace android