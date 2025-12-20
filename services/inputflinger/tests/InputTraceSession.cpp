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

#include "InputTraceSession.h"

#include <NotifyArgsBuilders.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <gtest/gtest.h>
#include <input/InputEventLabels.h>
#include <input/PrintTools.h>
#include <perfetto/trace/android/android_input_event.pbzero.h>
#include <perfetto/trace/android/winscope_extensions.pbzero.h>
#include <perfetto/trace/android/winscope_extensions_impl.pbzero.h>
#include <perfetto/trace/evdev.pbzero.h>

#include <utility>

namespace android {

using perfetto::protos::pbzero::AndroidInputEvent;
using perfetto::protos::pbzero::AndroidInputEventConfig;
using perfetto::protos::pbzero::AndroidKeyEvent;
using perfetto::protos::pbzero::AndroidMotionEvent;
using perfetto::protos::pbzero::AndroidWindowInputDispatchEvent;
using perfetto::protos::pbzero::EvdevEvent;
using perfetto::protos::pbzero::WinscopeExtensions;
using perfetto::protos::pbzero::WinscopeExtensionsImpl;

// These operator<< definitions must be in the global namespace for them to be accessible to the
// GTEST library. They cannot be in the anonymous namespace.
static std::ostream& operator<<(std::ostream& out,
                                const std::variant<KeyEvent, MotionEvent>& event) {
    std::visit([&](const auto& e) { out << e; }, event);
    return out;
}

static std::ostream& operator<<(std::ostream& out,
                                const InputTraceSession::WindowDispatchEvent& event) {
    out << "Window dispatch to windowId: " << event.window->getId() << ", event: " << event.event;
    return out;
}

namespace {

inline uint32_t getId(const std::variant<KeyEvent, MotionEvent>& event) {
    return std::visit([&](const auto& e) { return e.getId(); }, event);
}

std::unique_ptr<perfetto::TracingSession> startTrace(
        const std::function<void(protozero::HeapBuffered<AndroidInputEventConfig>&)>& configure) {
    protozero::HeapBuffered<AndroidInputEventConfig> inputEventConfig{};
    configure(inputEventConfig);

    perfetto::TraceConfig config;
    config.add_buffers()->set_size_kb(1024); // Record up to 1 MiB.
    auto* dataSourceConfig = config.add_data_sources()->mutable_config();
    dataSourceConfig->set_name("android.input.inputevent");
    dataSourceConfig->set_android_input_event_config_raw(inputEventConfig.SerializeAsString());

    std::unique_ptr<perfetto::TracingSession> tracingSession(perfetto::Tracing::NewTrace());
    tracingSession->Setup(config);
    tracingSession->StartBlocking();
    return tracingSession;
}

std::string stopTrace(std::unique_ptr<perfetto::TracingSession> tracingSession) {
    tracingSession->StopBlocking();
    std::vector<char> traceChars(tracingSession->ReadTraceBlocking());
    return {traceChars.data(), traceChars.size()};
}

// Decodes the trace, and returns all of the traced input events, and whether they were each
// traced as a redacted event.
auto decodeTrace(const std::string& rawTrace) {
    using namespace perfetto::protos::pbzero;

    ArrayMap<AndroidMotionEvent::Decoder, bool /*redacted*/> tracedMotions;
    ArrayMap<AndroidKeyEvent::Decoder, bool /*redacted*/> tracedKeys;
    ArrayMap<AndroidWindowInputDispatchEvent::Decoder, bool /*redacted*/> tracedWindowDispatches;
    std::vector<EvdevEvent::Decoder> tracedRawEvents;

    Trace::Decoder trace{rawTrace};
    if (trace.has_packet()) {
        for (auto it = trace.packet(); it; it++) {
            TracePacket::Decoder packet{it->as_bytes()};
            if (packet.has_evdev_event()) {
                tracedRawEvents.emplace_back(packet.evdev_event());
            }
            if (!packet.has_winscope_extensions()) {
                continue;
            }

            WinscopeExtensions::Decoder extensions{packet.winscope_extensions()};
            const auto& field =
                    extensions.Get(WinscopeExtensionsImpl::kAndroidInputEventFieldNumber);
            if (!field.valid()) {
                continue;
            }

            EXPECT_TRUE(packet.has_timestamp());
            EXPECT_TRUE(packet.has_timestamp_clock_id());
            EXPECT_EQ(packet.timestamp_clock_id(),
                      static_cast<uint32_t>(perfetto::protos::pbzero::BUILTIN_CLOCK_MONOTONIC));

            AndroidInputEvent::Decoder event{field.as_bytes()};
            if (event.has_dispatcher_motion_event()) {
                tracedMotions.emplace_back(event.dispatcher_motion_event(),
                                           /*redacted=*/false);
            }
            if (event.has_dispatcher_motion_event_redacted()) {
                tracedMotions.emplace_back(event.dispatcher_motion_event_redacted(),
                                           /*redacted=*/true);
            }
            if (event.has_dispatcher_key_event()) {
                tracedKeys.emplace_back(event.dispatcher_key_event(),
                                        /*redacted=*/false);
            }
            if (event.has_dispatcher_key_event_redacted()) {
                tracedKeys.emplace_back(event.dispatcher_key_event_redacted(),
                                        /*redacted=*/true);
            }
            if (event.has_dispatcher_window_dispatch_event()) {
                tracedWindowDispatches.emplace_back(event.dispatcher_window_dispatch_event(),
                                                    /*redacted=*/false);
            }
            if (event.has_dispatcher_window_dispatch_event_redacted()) {
                tracedWindowDispatches
                        .emplace_back(event.dispatcher_window_dispatch_event_redacted(),
                                      /*redacted=*/true);
            }
        }
    }
    return std::tuple{std::move(tracedMotions), std::move(tracedKeys),
                      std::move(tracedWindowDispatches), std::move(tracedRawEvents)};
}

bool eventMatches(const MotionEvent& expected, const AndroidMotionEvent::Decoder& traced) {
    return static_cast<uint32_t>(expected.getId()) == traced.event_id();
}

bool eventMatches(const KeyEvent& expected, const AndroidKeyEvent::Decoder& traced) {
    return static_cast<uint32_t>(expected.getId()) == traced.event_id();
}

bool eventMatches(const InputTraceSession::WindowDispatchEvent& expected,
                  const AndroidWindowInputDispatchEvent::Decoder& traced) {
    return static_cast<uint32_t>(getId(expected.event)) == traced.event_id() &&
            expected.window->getId() == traced.window_id();
}

template <typename ExpectedEvents, typename TracedEvents>
void verifyExpectedEventsTraced(const ExpectedEvents& expectedEvents,
                                const TracedEvents& tracedEvents, std::string_view name) {
    uint32_t totalExpectedCount = 0;

    for (const auto& [expectedEvent, expectedLevel] : expectedEvents) {
        int32_t totalMatchCount = 0;
        int32_t redactedMatchCount = 0;
        for (const auto& [tracedEvent, isRedacted] : tracedEvents) {
            if (eventMatches(expectedEvent, tracedEvent)) {
                totalMatchCount++;
                if (isRedacted) {
                    redactedMatchCount++;
                }
            }
        }
        switch (expectedLevel) {
            case Level::NONE:
                ASSERT_EQ(totalMatchCount, 0) << "Event should not be traced, but it was traced"
                                              << "\n\tExpected event: " << expectedEvent;
                break;
            case Level::REDACTED:
            case Level::COMPLETE:
                ASSERT_EQ(totalMatchCount, 1)
                        << "Event should match exactly one traced event, but it matched: "
                        << totalMatchCount << "\n\tExpected event: " << expectedEvent;
                ASSERT_EQ(redactedMatchCount, expectedLevel == Level::REDACTED ? 1 : 0);
                totalExpectedCount++;
                break;
        }
    }

    ASSERT_EQ(tracedEvents.size(), totalExpectedCount)
            << "The number of traced " << name
            << " events does not exactly match the number of expected events";
}

} // namespace

InputTraceSession::InputTraceSession(
        std::function<void(protozero::HeapBuffered<AndroidInputEventConfig>&)> configure)
      : mPerfettoSession(startTrace(std::move(configure))) {}

InputTraceSession::~InputTraceSession() {
    const auto rawTrace = stopTrace(std::move(mPerfettoSession));
    verifyExpectations(rawTrace);
}

void InputTraceSession::expectMotionTraced(Level level, const MotionEvent& event) {
    mExpectedMotions.emplace_back(event, level);
}

void InputTraceSession::expectKeyTraced(Level level, const KeyEvent& event) {
    mExpectedKeys.emplace_back(event, level);
}

void InputTraceSession::expectDispatchTraced(Level level, const WindowDispatchEvent& event) {
    mExpectedWindowDispatches.emplace_back(event, level);
}

void InputTraceSession::expectRawEventTraced(uint16_t type, uint16_t code, int32_t value) {
    mExpectedRawEvents.emplace_back(RawEvent{.type = type, .code = code, .value = value});
}

void InputTraceSession::verifyExpectations(const std::string& rawTrace) {
    auto [tracedMotions, tracedKeys, tracedWindowDispatches, tracedRawEvents] =
            decodeTrace(rawTrace);

    verifyExpectedEventsTraced(mExpectedMotions, tracedMotions, "motion");
    verifyExpectedEventsTraced(mExpectedKeys, tracedKeys, "key");
    verifyExpectedEventsTraced(mExpectedWindowDispatches, tracedWindowDispatches,
                               "window dispatch");
    verifyRawEvents(tracedRawEvents);
}

void InputTraceSession::verifyRawEvents(const std::vector<EvdevEvent::Decoder>& tracedRawEvents) {
    // TODO(b/394861376): tests that check raw event tracing currently fail if an evdev event from
    //   a real input device comes in during the run (e.g. from a real finger on the touchscreen).
    //   We should filter the raw events from the trace and only consider ones from devices
    //   registered by the test. This will be easier once we're tracing device addition events, as
    //   currently it's difficult to get the EventHub ID of the uinput device created in the test.
    ASSERT_EQ(tracedRawEvents.size(), mExpectedRawEvents.size())
            << "Incorrect number of raw events traced.";
    for (size_t i = 0; i < mExpectedRawEvents.size(); i++) {
        const RawEvent& expected = mExpectedRawEvents[i];
        const EvdevEventLabel expectedLabels =
                InputEventLookup::getLinuxEvdevLabel(expected.type, expected.code, expected.value);
        SCOPED_TRACE(::testing::Message()
                     << "Expected raw event " << i << " (" << expectedLabels.type << " "
                     << expectedLabels.code << " " << expectedLabels.value << ")");
        const EvdevEvent::Decoder& traced = tracedRawEvents[i];
        ASSERT_TRUE(traced.has_input_event());
        const EvdevEvent::InputEvent::Decoder& tracedInputEvent{traced.input_event()};
        const EvdevEventLabel tracedLabels =
                InputEventLookup::getLinuxEvdevLabel(tracedInputEvent.type(),
                                                     tracedInputEvent.code(),
                                                     tracedInputEvent.value());
        ASSERT_EQ(tracedInputEvent.type(), static_cast<uint32_t>(expected.type))
                << "Expected " << expectedLabels.type << ", got " << tracedLabels.type;
        ASSERT_EQ(tracedInputEvent.code(), static_cast<uint32_t>(expected.code))
                << "Expected " << expectedLabels.code << ", got " << tracedLabels.code;
        ASSERT_EQ(tracedInputEvent.value(), expected.value)
                << "Expected " << expectedLabels.value << ", got " << tracedLabels.value;
    }
}

} // namespace android
