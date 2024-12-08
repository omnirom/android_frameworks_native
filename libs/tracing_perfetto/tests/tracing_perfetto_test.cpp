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

#include "tracing_perfetto.h"

#include <android_os.h>
#include <flag_macros.h>
#include <thread>
#include <unistd.h>

#include "gtest/gtest.h"
#include "perfetto/public/abi/data_source_abi.h"
#include "perfetto/public/abi/heap_buffer.h"
#include "perfetto/public/abi/pb_decoder_abi.h"
#include "perfetto/public/abi/tracing_session_abi.h"
#include "perfetto/public/abi/track_event_abi.h"
#include "perfetto/public/data_source.h"
#include "perfetto/public/pb_decoder.h"
#include "perfetto/public/producer.h"
#include "perfetto/public/protos/config/trace_config.pzc.h"
#include "perfetto/public/protos/trace/interned_data/interned_data.pzc.h"
#include "perfetto/public/protos/trace/test_event.pzc.h"
#include "perfetto/public/protos/trace/trace.pzc.h"
#include "perfetto/public/protos/trace/trace_packet.pzc.h"
#include "perfetto/public/protos/trace/track_event/debug_annotation.pzc.h"
#include "perfetto/public/protos/trace/track_event/track_descriptor.pzc.h"
#include "perfetto/public/protos/trace/track_event/track_event.pzc.h"
#include "perfetto/public/protos/trace/trigger.pzc.h"
#include "perfetto/public/te_category_macros.h"
#include "perfetto/public/te_macros.h"
#include "perfetto/public/track_event.h"
#include "trace_categories.h"
#include "utils.h"

#include "protos/perfetto/trace/trace.pb.h"
#include "protos/perfetto/trace/trace_packet.pb.h"
#include "protos/perfetto/trace/interned_data/interned_data.pb.h"

#include <fstream>
#include <iterator>
namespace tracing_perfetto {

using ::perfetto::protos::Trace;
using ::perfetto::protos::TracePacket;
using ::perfetto::protos::EventCategory;
using ::perfetto::protos::EventName;
using ::perfetto::protos::FtraceEvent;
using ::perfetto::protos::FtraceEventBundle;
using ::perfetto::protos::InternedData;

using ::perfetto::shlib::test_utils::TracingSession;

const auto PERFETTO_SDK_TRACING = ACONFIG_FLAG(android::os, perfetto_sdk_tracing);

// TODO(b/303199244): Add tests for all the library functions.
class TracingPerfettoTest : public testing::Test {
 protected:
  void SetUp() override {
    tracing_perfetto::registerWithPerfetto(false /* test */);
  }
};

Trace stopSession(TracingSession& tracing_session) {
  tracing_session.FlushBlocking(5000);
  tracing_session.StopBlocking();
  std::vector<uint8_t> data = tracing_session.ReadBlocking();
  std::string data_string(data.begin(), data.end());

  perfetto::protos::Trace trace;
  trace.ParseFromString(data_string);

  return trace;
}

void verifyTrackEvent(const Trace& trace, const std::string expected_category,
                      const std::string& expected_name) {
  bool found = false;
  for (const TracePacket& packet: trace.packet()) {
    if (packet.has_track_event() && packet.has_interned_data()) {

      const InternedData& interned_data = packet.interned_data();
      if (interned_data.event_categories_size() > 0) {
        const EventCategory& event_category = packet.interned_data().event_categories(0);
        if (event_category.name() == expected_category) {
          found = true;
        }
      }

      if (interned_data.event_names_size() > 0) {
        const EventName& event_name = packet.interned_data().event_names(0);
        if (event_name.name() == expected_name) {
          found &= true;
        }
      }

      if (found) {
        break;
      }
    }
  }
  EXPECT_TRUE(found);
}

void verifyAtraceEvent(const Trace& trace, const std::string& expected_name) {
  std::string expected_print_buf = "I|" + std::to_string(gettid()) + "|" + expected_name + "\n";

  bool found = false;
  for (const TracePacket& packet: trace.packet()) {
    if (packet.has_ftrace_events()) {
      const FtraceEventBundle& ftrace_events_bundle = packet.ftrace_events();

      if (ftrace_events_bundle.event_size() > 0) {
        const FtraceEvent& ftrace_event = ftrace_events_bundle.event(0);
        if (ftrace_event.has_print() && (ftrace_event.print().buf() == expected_print_buf)) {
          found = true;
          break;
        }
      }
    }
  }
  EXPECT_TRUE(found);
}

TEST_F_WITH_FLAGS(TracingPerfettoTest, traceInstantWithPerfetto,
                  REQUIRES_FLAGS_ENABLED(PERFETTO_SDK_TRACING)) {
  std::string event_category = "input";
  std::string event_name = "traceInstantWithPerfetto";

  TracingSession tracing_session =
      TracingSession::Builder().add_enabled_category(event_category).Build();

  tracing_perfetto::traceInstant(TRACE_CATEGORY_INPUT, event_name.c_str());

  Trace trace = stopSession(tracing_session);

  verifyTrackEvent(trace, event_category, event_name);
}

TEST_F_WITH_FLAGS(TracingPerfettoTest, traceInstantWithAtrace,
                  REQUIRES_FLAGS_ENABLED(PERFETTO_SDK_TRACING)) {
  std::string event_category = "input";
  std::string event_name = "traceInstantWithAtrace";

  TracingSession tracing_session =
      TracingSession::Builder().add_atrace_category(event_category).Build();

  tracing_perfetto::traceInstant(TRACE_CATEGORY_INPUT, event_name.c_str());

  Trace trace = stopSession(tracing_session);

  verifyAtraceEvent(trace, event_name);
}

TEST_F_WITH_FLAGS(TracingPerfettoTest, traceInstantWithPerfettoAndAtrace,
                  REQUIRES_FLAGS_ENABLED(PERFETTO_SDK_TRACING)) {
  std::string event_category = "input";
  std::string event_name = "traceInstantWithPerfettoAndAtrace";

  TracingSession tracing_session =
      TracingSession::Builder()
      .add_atrace_category(event_category)
      .add_enabled_category(event_category).Build();

  tracing_perfetto::traceInstant(TRACE_CATEGORY_INPUT, event_name.c_str());

  Trace trace = stopSession(tracing_session);

  verifyAtraceEvent(trace, event_name);
}

TEST_F_WITH_FLAGS(TracingPerfettoTest, traceInstantWithPerfettoAndAtraceAndPreferTrackEvent,
                  REQUIRES_FLAGS_ENABLED(PERFETTO_SDK_TRACING)) {
  std::string event_category = "input";
  std::string event_name = "traceInstantWithPerfettoAndAtraceAndPreferTrackEvent";

  TracingSession tracing_session =
      TracingSession::Builder()
      .add_atrace_category(event_category)
      .add_atrace_category_prefer_sdk(event_category)
      .add_enabled_category(event_category).Build();

  tracing_perfetto::traceInstant(TRACE_CATEGORY_INPUT, event_name.c_str());

  Trace trace = stopSession(tracing_session);

  verifyTrackEvent(trace, event_category, event_name);
}

TEST_F_WITH_FLAGS(TracingPerfettoTest, traceInstantWithPerfettoAndAtraceConcurrently,
                  REQUIRES_FLAGS_ENABLED(PERFETTO_SDK_TRACING)) {
  std::string event_category = "input";
  std::string event_name = "traceInstantWithPerfettoAndAtraceConcurrently";

  TracingSession perfetto_tracing_session =
      TracingSession::Builder()
      .add_atrace_category(event_category)
      .add_atrace_category_prefer_sdk(event_category)
      .add_enabled_category(event_category).Build();

  TracingSession atrace_tracing_session =
      TracingSession::Builder()
      .add_atrace_category(event_category)
      .add_enabled_category(event_category).Build();

  tracing_perfetto::traceInstant(TRACE_CATEGORY_INPUT, event_name.c_str());

  Trace atrace_trace = stopSession(atrace_tracing_session);
  Trace perfetto_trace = stopSession(perfetto_tracing_session);

  verifyAtraceEvent(atrace_trace, event_name);
  verifyAtraceEvent(perfetto_trace, event_name);
}
}  // namespace tracing_perfetto
