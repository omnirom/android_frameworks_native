/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include <memory>

#include "InputTracingBackendInterface.h"
#include "RawEvent.h"

namespace android {

/**
 * Tracer implementation for InputReader and associated components.
 */
class InputReaderTracer {
public:
    explicit InputReaderTracer(std::shared_ptr<input_trace::InputTracingBackendInterface> backend);
    ~InputReaderTracer() = default;
    InputReaderTracer(const InputReaderTracer&) = delete;
    InputReaderTracer& operator=(const InputReaderTracer&) = delete;

    void traceRawEvent(const RawEvent& event);

private:
    std::shared_ptr<input_trace::InputTracingBackendInterface> mBackend;
};

} // namespace android
