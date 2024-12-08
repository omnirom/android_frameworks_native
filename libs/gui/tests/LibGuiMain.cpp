/*
 * Copyright 2023 The Android Open Source Project
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

#include <android-base/unique_fd.h>
#include <gtest/gtest.h>
#include <log/log.h>

#include "testserver/TestServer.h"
#include "testserver/TestServerClient.h"
#include "testserver/TestServerHost.h"

using namespace android;

namespace {

class TestCaseLogger : public ::testing::EmptyTestEventListener {
    void OnTestStart(const ::testing::TestInfo& testInfo) override {
        ALOGD("Begin test: %s#%s", testInfo.test_suite_name(), testInfo.name());
    }

    void OnTestEnd(const testing::TestInfo& testInfo) override {
        ALOGD("End test:   %s#%s", testInfo.test_suite_name(), testInfo.name());
    }
};

} // namespace

int main(int argc, char** argv) {
    // There are three modes that we can run in to support the libgui TestServer:
    //
    // - libgui_test : normal mode, runs tests and fork/execs the testserver host process
    // - libgui_test --test-server-host $recvPipeFd $sendPipeFd : TestServerHost mode, listens on
    //   $recvPipeFd for commands and sends responses over $sendPipeFd
    // - libgui_test --test-server $name : TestServer mode, starts a ITestService binder service
    //   under $name
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--test-server-host") {
            LOG_ALWAYS_FATAL_IF(argc < (i + 2), "--test-server-host requires two pipe fds");
            // Note that the send/recv are from our perspective.
            base::unique_fd recvPipeFd = base::unique_fd(atoi(argv[i + 1]));
            base::unique_fd sendPipeFd = base::unique_fd(atoi(argv[i + 2]));
            return TestServerHostMain(argv[0], std::move(sendPipeFd), std::move(recvPipeFd));
        }
        if (arg == "--test-server") {
            LOG_ALWAYS_FATAL_IF(argc < (i + 1), "--test-server requires a name");
            return TestServerMain(argv[i + 1]);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    testing::UnitTest::GetInstance()->listeners().Append(new TestCaseLogger());

    // This has to be run *before* any test initialization, because it fork/execs a TestServerHost,
    // which will later create new binder service. You can't do that in a forked thread after you've
    // initialized any binder stuff, which some tests do.
    TestServerClient::InitializeOrDie(argv[0]);

    return RUN_ALL_TESTS();
}