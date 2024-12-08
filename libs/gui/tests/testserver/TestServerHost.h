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

#include <android-base/unique_fd.h>

#include <string>

namespace android {

/*
 * Main method for a host process for TestServers.
 *
 * This must be called without any binder setup having been done, because you can't fork and do
 * binder things once ProcessState is set up.
 * @param filename File name of this binary / the binary to execve into
 * @param sendPipeFd Pipe FD to send data to.
 * @param recvPipeFd Pipe FD to receive data from.
 * @return retcode
 */
int TestServerHostMain(const char* filename, base::unique_fd sendPipeFd,
                       base::unique_fd recvPipeFd);

} // namespace android
