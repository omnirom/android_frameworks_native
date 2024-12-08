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

#define LOG_TAG "TestServerHost"

#include <android-base/unique_fd.h>
#include <binder/IInterface.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <binder/Status.h>
#include <gui/BufferQueue.h>
#include <gui/IConsumerListener.h>
#include <gui/IGraphicBufferConsumer.h>
#include <gui/IGraphicBufferProducer.h>
#include <libgui_test_server/BnTestServer.h>
#include <log/log.h>
#include <utils/Errors.h>

#include <memory>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>

#include "TestServerCommon.h"
#include "TestServerHost.h"

namespace android {

namespace {

pid_t ForkTestServer(const char* filename, char* name) {
    pid_t childPid = fork();
    LOG_ALWAYS_FATAL_IF(childPid == -1);

    if (childPid != 0) {
        return childPid;
    }

    // We forked!
    const char* test_server_flag = "--test-server";
    char* args[] = {
            const_cast<char*>(filename),
            const_cast<char*>(test_server_flag),
            name,
            nullptr,
    };

    int ret = execv(filename, args);
    ALOGE("Failed to exec libgui_test as a TestServer. ret=%d errno=%d (%s)", ret, errno,
          strerror(errno));
    _exit(EXIT_FAILURE);
}

} // namespace

int TestServerHostMain(const char* filename, base::unique_fd sendPipeFd,
                       base::unique_fd recvPipeFd) {
    status_t status = OK;
    LOG_ALWAYS_FATAL_IF(sizeof(status) != write(sendPipeFd.get(), &status, sizeof(status)));

    ALOGE("Launched TestServerHost");

    while (true) {
        CreateServerRequest request = {};
        ssize_t bytes = read(recvPipeFd.get(), &request, sizeof(request));
        LOG_ALWAYS_FATAL_IF(bytes != sizeof(request));
        pid_t childPid = ForkTestServer(filename, request.name);

        CreateServerResponse response = {};
        response.pid = childPid;
        bytes = write(sendPipeFd.get(), &response, sizeof(response));
        LOG_ALWAYS_FATAL_IF(bytes != sizeof(response));
    }

    return 0;
}

} // namespace android