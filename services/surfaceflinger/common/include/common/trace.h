
/*
 * Copyright 2024 The Android Open Source Project
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

#ifndef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#endif

#include <cutils/trace.h>
#include <tracing_perfetto.h>

// prevent using atrace directly, calls should go through tracing_perfetto lib
#undef ATRACE_ENABLED
#undef ATRACE_BEGIN
#undef ATRACE_END
#undef ATRACE_ASYNC_BEGIN
#undef ATRACE_ASYNC_END
#undef ATRACE_ASYNC_FOR_TRACK_BEGIN
#undef ATRACE_ASYNC_FOR_TRACK_END
#undef ATRACE_INSTANT
#undef ATRACE_INSTANT_FOR_TRACK
#undef ATRACE_INT
#undef ATRACE_INT64
#undef ATRACE_CALL
#undef ATRACE_NAME
#undef ATRACE_FORMAT
#undef ATRACE_FORMAT_INSTANT

#define SFTRACE_ENABLED() ::tracing_perfetto::isTagEnabled(ATRACE_TAG)
#define SFTRACE_BEGIN(name) ::tracing_perfetto::traceBegin(ATRACE_TAG, name)
#define SFTRACE_END() ::tracing_perfetto::traceEnd(ATRACE_TAG)
#define SFTRACE_ASYNC_BEGIN(name, cookie) \
    ::tracing_perfetto::traceAsyncBegin(ATRACE_TAG, name, cookie)
#define SFTRACE_ASYNC_END(name, cookie) ::tracing_perfetto::traceAsyncEnd(ATRACE_TAG, name, cookie)
#define SFTRACE_ASYNC_FOR_TRACK_BEGIN(track_name, name, cookie) \
    ::tracing_perfetto::traceAsyncBeginForTrack(ATRACE_TAG, name, track_name, cookie)
#define SFTRACE_ASYNC_FOR_TRACK_END(track_name, cookie) \
    ::tracing_perfetto::traceAsyncEndForTrack(ATRACE_TAG, track_name, cookie)
#define SFTRACE_INSTANT(name) ::tracing_perfetto::traceInstant(ATRACE_TAG, name)
#define SFTRACE_FORMAT_INSTANT(fmt, ...) \
    ::tracing_perfetto::traceFormatInstant(ATRACE_TAG, fmt, ##__VA_ARGS__)
#define SFTRACE_INSTANT_FOR_TRACK(trackName, name) \
    ::tracing_perfetto::traceInstantForTrack(ATRACE_TAG, trackName, name)
#define SFTRACE_INT(name, value) ::tracing_perfetto::traceCounter32(ATRACE_TAG, name, value)
#define SFTRACE_INT64(name, value) ::tracing_perfetto::traceCounter(ATRACE_TAG, name, value)

// SFTRACE_NAME traces from its location until the end of its enclosing scope.
#define _PASTE(x, y) x##y
#define PASTE(x, y) _PASTE(x, y)
#define SFTRACE_NAME(name) ::android::ScopedTrace PASTE(___tracer, __LINE__)(name)
// SFTRACE_CALL is an SFTRACE_NAME that uses the current function name.
#define SFTRACE_CALL() SFTRACE_NAME(__FUNCTION__)

#define SFTRACE_FORMAT(fmt, ...) \
    ::android::ScopedTrace PASTE(___tracer, __LINE__)(fmt, ##__VA_ARGS__)

#define ALOGE_AND_TRACE(fmt, ...)                   \
    do {                                            \
        ALOGE(fmt, ##__VA_ARGS__);                  \
        SFTRACE_FORMAT_INSTANT(fmt, ##__VA_ARGS__); \
    } while (false)

namespace android {

class ScopedTrace {
public:
    template <typename... Args>
    inline ScopedTrace(const char* fmt, Args&&... args) {
        ::tracing_perfetto::traceFormatBegin(ATRACE_TAG, fmt, std::forward<Args>(args)...);
    }
    inline ScopedTrace(const char* name) { SFTRACE_BEGIN(name); }
    inline ~ScopedTrace() { SFTRACE_END(); }
};

} // namespace android
