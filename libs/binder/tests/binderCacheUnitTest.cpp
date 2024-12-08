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

#include <android-base/logging.h>
#include <android/os/IServiceManager.h>
#include <binder/IBinder.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/IServiceManagerUnitTestHelper.h>
#include "fakeservicemanager/FakeServiceManager.h"

#include <sys/prctl.h>
#include <thread>

using namespace android;

#ifdef LIBBINDER_CLIENT_CACHE
constexpr bool kUseLibbinderCache = true;
#else
constexpr bool kUseLibbinderCache = false;
#endif

// A service name which is in the static list of cachable services
const String16 kCachedServiceName = String16("isub");

#define EXPECT_OK(status)                 \
    do {                                  \
        binder::Status stat = (status);   \
        EXPECT_TRUE(stat.isOk()) << stat; \
    } while (false)

const String16 kServerName = String16("binderCacheUnitTest");

class FooBar : public BBinder {
public:
    status_t onTransact(uint32_t, const Parcel&, Parcel*, uint32_t) {
        // exit the server
        std::thread([] { exit(EXIT_FAILURE); }).detach();
        return OK;
    }
    void killServer(sp<IBinder> binder) {
        Parcel data, reply;
        binder->transact(0, data, &reply, 0);
    }
};

class MockAidlServiceManager : public os::IServiceManagerDefault {
public:
    MockAidlServiceManager() : innerSm() {}

    binder::Status checkService(const ::std::string& name, os::Service* _out) override {
        sp<IBinder> binder = innerSm.getService(String16(name.c_str()));
        *_out = os::Service::make<os::Service::Tag::binder>(binder);
        return binder::Status::ok();
    }

    binder::Status addService(const std::string& name, const sp<IBinder>& service,
                              bool allowIsolated, int32_t dumpPriority) override {
        return binder::Status::fromStatusT(
                innerSm.addService(String16(name.c_str()), service, allowIsolated, dumpPriority));
    }

    FakeServiceManager innerSm;
};

class LibbinderCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        sp<MockAidlServiceManager> sm = sp<MockAidlServiceManager>::make();
        mServiceManager = getServiceManagerShimFromAidlServiceManagerForTests(sm);
    }

    void TearDown() override {}

public:
    void cacheAndConfirmCacheHit(const sp<IBinder>& binder1, const sp<IBinder>& binder2) {
        // Add a service
        EXPECT_EQ(OK, mServiceManager->addService(kCachedServiceName, binder1));
        // Get the service. This caches it.
        sp<IBinder> result = mServiceManager->checkService(kCachedServiceName);
        ASSERT_EQ(binder1, result);

        // Add the different binder and replace the service.
        // The cache should still hold the original binder.
        EXPECT_EQ(OK, mServiceManager->addService(kCachedServiceName, binder2));

        result = mServiceManager->checkService(kCachedServiceName);
        if (kUseLibbinderCache) {
            // If cache is enabled, we should get the binder to Service Manager.
            EXPECT_EQ(binder1, result);
        } else {
            // If cache is disabled, then we should get the newer binder
            EXPECT_EQ(binder2, result);
        }
    }

    sp<android::IServiceManager> mServiceManager;
};

TEST_F(LibbinderCacheTest, AddLocalServiceAndConfirmCacheHit) {
    sp<IBinder> binder1 = sp<BBinder>::make();
    sp<IBinder> binder2 = sp<BBinder>::make();

    cacheAndConfirmCacheHit(binder1, binder2);
}

TEST_F(LibbinderCacheTest, AddRemoteServiceAndConfirmCacheHit) {
    sp<IBinder> binder1 = defaultServiceManager()->checkService(kServerName);
    ASSERT_NE(binder1, nullptr);
    sp<IBinder> binder2 = IInterface::asBinder(mServiceManager);

    cacheAndConfirmCacheHit(binder1, binder2);
}

TEST_F(LibbinderCacheTest, RemoveFromCacheOnServerDeath) {
    sp<IBinder> binder1 = defaultServiceManager()->checkService(kServerName);
    FooBar foo = FooBar();

    EXPECT_EQ(OK, mServiceManager->addService(kCachedServiceName, binder1));

    // Check Service, this caches the binder
    sp<IBinder> result = mServiceManager->checkService(kCachedServiceName);
    ASSERT_EQ(binder1, result);

    // Kill the server, this should remove from cache.
    pid_t pid;
    ASSERT_EQ(OK, binder1->getDebugPid(&pid));
    foo.killServer(binder1);
    system(("kill -9 " + std::to_string(pid)).c_str());

    sp<IBinder> binder2 = sp<BBinder>::make();

    // Add new service with the same name.
    // This will replace the service in FakeServiceManager.
    EXPECT_EQ(OK, mServiceManager->addService(kCachedServiceName, binder2));

    // Confirm that new service is returned instead of old.
    sp<IBinder> result2 = mServiceManager->checkService(kCachedServiceName);
    ASSERT_EQ(binder2, result2);
}

TEST_F(LibbinderCacheTest, NullBinderNotCached) {
    sp<IBinder> binder1 = nullptr;
    sp<IBinder> binder2 = sp<BBinder>::make();

    // Check for a cacheble service which isn't registered.
    // FakeServiceManager should return nullptr.
    // This shouldn't be cached.
    sp<IBinder> result = mServiceManager->checkService(kCachedServiceName);
    ASSERT_EQ(binder1, result);

    // Add the same service
    EXPECT_EQ(OK, mServiceManager->addService(kCachedServiceName, binder2));

    // This should return the newly added service.
    result = mServiceManager->checkService(kCachedServiceName);
    EXPECT_EQ(binder2, result);
}

TEST_F(LibbinderCacheTest, DoNotCacheServiceNotInList) {
    sp<IBinder> binder1 = sp<BBinder>::make();
    sp<IBinder> binder2 = sp<BBinder>::make();
    String16 serviceName = String16("NewLibbinderCacheTest");
    // Add a service
    EXPECT_EQ(OK, mServiceManager->addService(serviceName, binder1));
    // Get the service. This shouldn't caches it.
    sp<IBinder> result = mServiceManager->checkService(serviceName);
    ASSERT_EQ(binder1, result);

    // Add the different binder and replace the service.
    EXPECT_EQ(OK, mServiceManager->addService(serviceName, binder2));

    // Confirm that we get the new service
    result = mServiceManager->checkService(serviceName);
    EXPECT_EQ(binder2, result);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    if (fork() == 0) {
        prctl(PR_SET_PDEATHSIG, SIGHUP);

        // Start a FooBar service and add it to the servicemanager.
        sp<IBinder> server = new FooBar();
        defaultServiceManager()->addService(kServerName, server);

        IPCThreadState::self()->joinThreadPool(true);
        exit(1); // should not reach
    }

    status_t err = ProcessState::self()->setThreadPoolMaxThreadCount(3);
    ProcessState::self()->startThreadPool();
    CHECK_EQ(ProcessState::self()->isThreadPoolStarted(), true);
    CHECK_GT(ProcessState::self()->getThreadPoolMaxTotalThreadCount(), 0);

    auto binder = defaultServiceManager()->waitForService(kServerName);
    CHECK_NE(nullptr, binder.get());
    return RUN_ALL_TESTS();
}
