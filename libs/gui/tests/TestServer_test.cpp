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

#include <gtest/gtest.h>

#include <SurfaceFlingerProperties.h>
#include <android/gui/IDisplayEventConnection.h>
#include <android/gui/ISurfaceComposer.h>
#include <android/hardware/configstore/1.0/ISurfaceFlingerConfigs.h>
#include <android/hardware_buffer.h>
#include <binder/ProcessState.h>
#include <com_android_graphics_libgui_flags.h>
#include <configstore/Utils.h>
#include <gui/AidlUtil.h>
#include <gui/BufferItemConsumer.h>
#include <gui/BufferQueue.h>
#include <gui/CpuConsumer.h>
#include <gui/IConsumerListener.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SyncScreenCaptureListener.h>
#include <private/gui/ComposerService.h>
#include <private/gui/ComposerServiceAIDL.h>
#include <sys/types.h>
#include <system/window.h>
#include <ui/BufferQueueDefs.h>
#include <ui/DisplayMode.h>
#include <ui/GraphicBuffer.h>
#include <ui/Rect.h>
#include <utils/Errors.h>
#include <utils/String8.h>

#include <cstddef>
#include <limits>
#include <thread>

#include "binder/IInterface.h"
#include "testserver/TestServerClient.h"

namespace android {

namespace {

class TestServerTest : public ::testing::Test {
protected:
    TestServerTest() { ProcessState::self()->startThreadPool(); }
};

} // namespace

TEST_F(TestServerTest, Create) {
    EXPECT_NE(nullptr, TestServerClient::Create());
}

TEST_F(TestServerTest, CreateProducer) {
    sp<TestServerClient> client = TestServerClient::Create();
    EXPECT_NE(nullptr, client->CreateProducer());
}

TEST_F(TestServerTest, KillServer) {
    class DeathWaiter : public IBinder::DeathRecipient {
    public:
        virtual void binderDied(const wp<IBinder>&) override { mPromise.set_value(true); }
        std::future<bool> getFuture() { return mPromise.get_future(); }

        std::promise<bool> mPromise;
    };

    sp<TestServerClient> client = TestServerClient::Create();
    sp<IGraphicBufferProducer> producer = client->CreateProducer();
    EXPECT_NE(nullptr, producer);

    sp<DeathWaiter> deathWaiter = sp<DeathWaiter>::make();
    EXPECT_EQ(OK, IInterface::asBinder(producer)->linkToDeath(deathWaiter));

    auto deathWaiterFuture = deathWaiter->getFuture();
    EXPECT_EQ(OK, client->Kill());
    EXPECT_EQ(nullptr, client->CreateProducer());

    EXPECT_TRUE(deathWaiterFuture.get());
}

} // namespace android
