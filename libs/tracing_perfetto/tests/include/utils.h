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

// Copied from //external/perfetto/src/shared_lib/test/utils.h

#ifndef UTILS_H
#define UTILS_H

#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

#include "perfetto/public/abi/pb_decoder_abi.h"
#include "perfetto/public/pb_utils.h"
#include "perfetto/public/tracing_session.h"

// Pretty printer for gtest
void PrintTo(const PerfettoPbDecoderField& field, std::ostream*);

namespace perfetto {
namespace shlib {
namespace test_utils {

class WaitableEvent {
 public:
  WaitableEvent() = default;
  void Notify() {
    std::unique_lock<std::mutex> lock(m_);
    notified_ = true;
    cv_.notify_one();
  }
  bool WaitForNotification() {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [this] { return notified_; });
    return notified_;
  }
  bool IsNotified() {
    std::unique_lock<std::mutex> lock(m_);
    return notified_;
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  bool notified_ = false;
};

class TracingSession {
 public:
  class Builder {
   public:
    Builder() = default;
    Builder& add_enabled_category(std::string category) {
      enabled_categories_.push_back(std::move(category));
      return *this;
    }
    Builder& add_disabled_category(std::string category) {
      disabled_categories_.push_back(std::move(category));
      return *this;
    }
    Builder& add_atrace_category(std::string category) {
      atrace_categories_.push_back(std::move(category));
      return *this;
    }
    Builder& add_atrace_category_prefer_sdk(std::string category) {
      atrace_categories_prefer_sdk_.push_back(std::move(category));
      return *this;
    }
    TracingSession Build();

   private:
    std::vector<std::string> enabled_categories_;
    std::vector<std::string> disabled_categories_;
    std::vector<std::string> atrace_categories_;
    std::vector<std::string> atrace_categories_prefer_sdk_;
  };

  static TracingSession Adopt(struct PerfettoTracingSessionImpl*);
  static TracingSession FromBytes(void *buf, size_t len);

  TracingSession(TracingSession&&) noexcept;

  ~TracingSession();

  struct PerfettoTracingSessionImpl* session() const {
    return session_;
  }

  bool FlushBlocking(uint32_t timeout_ms);
  void WaitForStopped();
  void StopBlocking();
  std::vector<uint8_t> ReadBlocking();

 private:
  TracingSession() = default;
  struct PerfettoTracingSessionImpl* session_;
  std::unique_ptr<WaitableEvent> stopped_;
};

}  // namespace test_utils
}  // namespace shlib
}  // namespace perfetto

#endif  // UTILS_H
