/*
 * Copyright 2025 The Android Open Source Project
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

#include <cstdlib>
#include <string_view>

#include <android-base/logging.h>
#include <android/binder_process.h>
#include <gtest/gtest.h>

namespace {

void init(int argc, char** argv) {
    using namespace std::string_view_literals;

    ::testing::InitGoogleTest(&argc, argv);
    ::android::base::InitLogging(argv, android::base::StderrLogger);

    auto minimumSeverity = android::base::INFO;
    for (int i = 1; i < argc; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const std::string_view arg = argv[i];

        if (arg == "-v"sv) {
            minimumSeverity = android::base::DEBUG;
        } else if (arg == "-vv"sv) {
            minimumSeverity = android::base::VERBOSE;
        }
    }
    ::android::base::SetMinimumLogSeverity(minimumSeverity);
}

}  // namespace

auto main(int argc, char** argv) -> int {
    init(argc, argv);

    ABinderProcess_setThreadPoolMaxThreadCount(1);
    ABinderProcess_startThreadPool();

    return RUN_ALL_TESTS();
}