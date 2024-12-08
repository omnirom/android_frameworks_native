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

// Copied from //external/perfetto/src/shared_lib/test/utils.cc

#include "utils.h"

#include "perfetto/public/abi/heap_buffer.h"
#include "perfetto/public/pb_msg.h"
#include "perfetto/public/pb_utils.h"
#include "perfetto/public/protos/config/data_source_config.pzc.h"
#include "perfetto/public/protos/config/trace_config.pzc.h"
#include "perfetto/public/protos/config/track_event/track_event_config.pzc.h"
#include "perfetto/public/tracing_session.h"

#include "protos/perfetto/config/ftrace/ftrace_config.pb.h"
#include "protos/perfetto/config/track_event/track_event_config.pb.h"
#include "protos/perfetto/config/data_source_config.pb.h"
#include "protos/perfetto/config/trace_config.pb.h"

namespace perfetto {
namespace shlib {
namespace test_utils {
namespace {

std::string ToHexChars(uint8_t val) {
  std::string ret;
  uint8_t high_nibble = (val & 0xF0) >> 4;
  uint8_t low_nibble = (val & 0xF);
  static const char hex_chars[] = "0123456789ABCDEF";
  ret.push_back(hex_chars[high_nibble]);
  ret.push_back(hex_chars[low_nibble]);
  return ret;
}

}  // namespace

TracingSession TracingSession::Builder::Build() {
  perfetto::protos::TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(1024);

  auto* track_event_ds_config = trace_config.add_data_sources()->mutable_config();
  auto* ftrace_ds_config = trace_config.add_data_sources()->mutable_config();

  track_event_ds_config->set_name("track_event");
  track_event_ds_config->set_target_buffer(0);

  ftrace_ds_config->set_name("linux.ftrace");
  ftrace_ds_config->set_target_buffer(0);

  {
    auto* ftrace_config = ftrace_ds_config->mutable_ftrace_config();
    if (!atrace_categories_.empty()) {
      ftrace_config->add_ftrace_events("ftrace/print");
      for (const std::string& cat : atrace_categories_) {
        ftrace_config->add_atrace_categories(cat);
      }

      for (const std::string& cat : atrace_categories_prefer_sdk_) {
        ftrace_config->add_atrace_categories_prefer_sdk(cat);
      }
    }
  }

  {
    auto* track_event_config = track_event_ds_config->mutable_track_event_config();
    if (!enabled_categories_.empty() || !disabled_categories_.empty()) {
      for (const std::string& cat : enabled_categories_) {
        track_event_config->add_enabled_categories(cat);
      }

      for (const std::string& cat : disabled_categories_) {
        track_event_config->add_disabled_categories(cat);
      }
    }
  }

  struct PerfettoTracingSessionImpl* ts =
      PerfettoTracingSessionCreate(PERFETTO_BACKEND_SYSTEM);

  std::string trace_config_string;
  trace_config.SerializeToString(&trace_config_string);

  PerfettoTracingSessionSetup(ts, trace_config_string.data(), trace_config_string.length());

  // Fails to start here
  PerfettoTracingSessionStartBlocking(ts);

  return TracingSession::Adopt(ts);
}

TracingSession TracingSession::Adopt(struct PerfettoTracingSessionImpl* session) {
  TracingSession ret;
  ret.session_ = session;
  ret.stopped_ = std::make_unique<WaitableEvent>();
  PerfettoTracingSessionSetStopCb(
      ret.session_,
      [](struct PerfettoTracingSessionImpl*, void* arg) {
        static_cast<WaitableEvent*>(arg)->Notify();
      },
      ret.stopped_.get());
  return ret;
}

TracingSession::TracingSession(TracingSession&& other) noexcept {
  session_ = other.session_;
  other.session_ = nullptr;
  stopped_ = std::move(other.stopped_);
  other.stopped_ = nullptr;
}

TracingSession::~TracingSession() {
  if (!session_) {
    return;
  }
  if (!stopped_->IsNotified()) {
    PerfettoTracingSessionStopBlocking(session_);
    stopped_->WaitForNotification();
  }
  PerfettoTracingSessionDestroy(session_);
}

bool TracingSession::FlushBlocking(uint32_t timeout_ms) {
  WaitableEvent notification;
  bool result;
  auto* cb = new std::function<void(bool)>([&](bool success) {
    result = success;
    notification.Notify();
  });
  PerfettoTracingSessionFlushAsync(
      session_, timeout_ms,
      [](PerfettoTracingSessionImpl*, bool success, void* user_arg) {
        auto* f = reinterpret_cast<std::function<void(bool)>*>(user_arg);
        (*f)(success);
        delete f;
      },
      cb);
  notification.WaitForNotification();
  return result;
}

void TracingSession::WaitForStopped() {
  stopped_->WaitForNotification();
}

void TracingSession::StopBlocking() {
  PerfettoTracingSessionStopBlocking(session_);
}

std::vector<uint8_t> TracingSession::ReadBlocking() {
  std::vector<uint8_t> data;
  PerfettoTracingSessionReadTraceBlocking(
      session_,
      [](struct PerfettoTracingSessionImpl*, const void* trace_data,
         size_t size, bool, void* user_arg) {
        auto& dst = *static_cast<std::vector<uint8_t>*>(user_arg);
        auto* src = static_cast<const uint8_t*>(trace_data);
        dst.insert(dst.end(), src, src + size);
      },
      &data);
  return data;
}

}  // namespace test_utils
}  // namespace shlib
}  // namespace perfetto

void PrintTo(const PerfettoPbDecoderField& field, std::ostream* pos) {
  std::ostream& os = *pos;
  PerfettoPbDecoderStatus status =
      static_cast<PerfettoPbDecoderStatus>(field.status);
  switch (status) {
    case PERFETTO_PB_DECODER_ERROR:
      os << "MALFORMED PROTOBUF";
      break;
    case PERFETTO_PB_DECODER_DONE:
      os << "DECODER DONE";
      break;
    case PERFETTO_PB_DECODER_OK:
      switch (field.wire_type) {
        case PERFETTO_PB_WIRE_TYPE_DELIMITED:
          os << "\"";
          for (size_t i = 0; i < field.value.delimited.len; i++) {
            os << perfetto::shlib::test_utils::ToHexChars(
                      field.value.delimited.start[i])
               << " ";
          }
          os << "\"";
          break;
        case PERFETTO_PB_WIRE_TYPE_VARINT:
          os << "varint: " << field.value.integer64;
          break;
        case PERFETTO_PB_WIRE_TYPE_FIXED32:
          os << "fixed32: " << field.value.integer32;
          break;
        case PERFETTO_PB_WIRE_TYPE_FIXED64:
          os << "fixed64: " << field.value.integer64;
          break;
      }
      break;
  }
}
