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

#include <fcntl.h>

namespace android {

/*
 * Test -> TestServerHost Request to create a new ITestServer fork.
 */
struct CreateServerRequest {
    /*
     * Service name for new ITestServer.
     */
    char name[128];
};

/*
 * TestServerHost -> Test Response for creating an ITestServer fork.
 */
struct CreateServerResponse {
    /*
     * pid of new ITestServer.
     */
    pid_t pid;
};

} // namespace android