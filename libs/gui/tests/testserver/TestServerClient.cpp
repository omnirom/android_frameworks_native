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

#include <sys/wait.h>
#include <cerrno>
#define LOG_TAG "TestServerClient"

#include <android-base/stringprintf.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <libgui_test_server/ITestServer.h>
#include <log/log.h>
#include <utils/Errors.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <string>

#include "TestServerClient.h"
#include "TestServerCommon.h"

namespace android {

namespace {

std::string GetUniqueServiceName() {
    static std::atomic<int> uniqueId = 1;

    pid_t pid = getpid();
    int id = uniqueId++;
    return base::StringPrintf("Libgui-TestServer-%d-%d", pid, id);
}

struct RemoteTestServerHostHolder {
    RemoteTestServerHostHolder(pid_t pid, int sendFd, int recvFd)
          : mPid(pid), mSendFd(sendFd), mRecvFd(recvFd) {}
    ~RemoteTestServerHostHolder() {
        std::lock_guard lock(mMutex);

        kill(mPid, SIGKILL);
        close(mSendFd);
        close(mRecvFd);
    }

    pid_t CreateTestServerOrDie(std::string name) {
        std::lock_guard lock(mMutex);

        CreateServerRequest request;
        strlcpy(request.name, name.c_str(), sizeof(request.name) / sizeof(request.name[0]));

        ssize_t bytes = write(mSendFd, &request, sizeof(request));
        LOG_ALWAYS_FATAL_IF(bytes != sizeof(request));

        CreateServerResponse response;
        bytes = read(mRecvFd, &response, sizeof(response));
        LOG_ALWAYS_FATAL_IF(bytes != sizeof(response));

        return response.pid;
    }

private:
    std::mutex mMutex;

    pid_t mPid;
    int mSendFd;
    int mRecvFd;
};

std::unique_ptr<RemoteTestServerHostHolder> g_remoteTestServerHostHolder = nullptr;

} // namespace

void TestServerClient::InitializeOrDie(const char* filename) {
    int sendPipeFds[2];
    int ret = pipe(sendPipeFds);
    LOG_ALWAYS_FATAL_IF(ret, "Unable to create subprocess send pipe");

    int recvPipeFds[2];
    ret = pipe(recvPipeFds);
    LOG_ALWAYS_FATAL_IF(ret, "Unable to create subprocess recv pipe");

    pid_t childPid = fork();
    LOG_ALWAYS_FATAL_IF(childPid < 0, "Unable to fork child process");

    if (childPid == 0) {
        // We forked!
        close(sendPipeFds[1]);
        close(recvPipeFds[0]);

        // We'll be reading from the parent's "send" and writing to the parent's "recv".
        std::string sendPipe = std::to_string(sendPipeFds[0]);
        std::string recvPipe = std::to_string(recvPipeFds[1]);
        char* args[] = {
                const_cast<char*>(filename),
                const_cast<char*>("--test-server-host"),
                const_cast<char*>(sendPipe.c_str()),
                const_cast<char*>(recvPipe.c_str()),
                nullptr,
        };

        ret = execv(filename, args);
        ALOGE("Failed to exec libguiTestServer. ret=%d errno=%d (%s)", ret, errno, strerror(errno));
        status_t status = -errno;
        write(recvPipeFds[1], &status, sizeof(status));
        _exit(EXIT_FAILURE);
    }

    close(sendPipeFds[0]);
    close(recvPipeFds[1]);

    // Check for an OK status that the host started. If so, we're good to go.
    status_t status;
    ret = read(recvPipeFds[0], &status, sizeof(status));
    LOG_ALWAYS_FATAL_IF(ret != sizeof(status), "Unable to read from pipe: %d", ret);
    LOG_ALWAYS_FATAL_IF(OK != status, "Pipe returned failed status: %d", status);

    g_remoteTestServerHostHolder =
            std::make_unique<RemoteTestServerHostHolder>(childPid, sendPipeFds[1], recvPipeFds[0]);
}

sp<TestServerClient> TestServerClient::Create() {
    std::string serviceName = GetUniqueServiceName();

    pid_t childPid = g_remoteTestServerHostHolder->CreateTestServerOrDie(serviceName);
    ALOGD("Created child server %s with pid %d", serviceName.c_str(), childPid);

    sp<libgui_test_server::ITestServer> server =
            waitForService<libgui_test_server::ITestServer>(String16(serviceName.c_str()));
    LOG_ALWAYS_FATAL_IF(server == nullptr);
    ALOGD("Created connected to child server %s", serviceName.c_str());

    return sp<TestServerClient>::make(server);
}

TestServerClient::TestServerClient(const sp<libgui_test_server::ITestServer>& server)
      : mServer(server) {}

TestServerClient::~TestServerClient() {
    Kill();
}

sp<IGraphicBufferProducer> TestServerClient::CreateProducer() {
    std::lock_guard<std::mutex> lock(mMutex);

    if (!mIsAlive) {
        return nullptr;
    }

    view::Surface surface;
    binder::Status status = mServer->createProducer(&surface);

    if (!status.isOk()) {
        ALOGE("Failed to create remote producer. Error: %s", status.exceptionMessage().c_str());
        return nullptr;
    }

    if (!surface.graphicBufferProducer) {
        ALOGE("Remote producer returned no IGBP.");
        return nullptr;
    }

    return surface.graphicBufferProducer;
}

status_t TestServerClient::Kill() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mIsAlive) {
        return DEAD_OBJECT;
    }

    mServer->killNow();
    mServer = nullptr;
    mIsAlive = false;

    return OK;
}

} // namespace android
