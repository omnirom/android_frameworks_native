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

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/macros.h>
#include <android-base/strings.h>
#include <android-base/thread_annotations.h>
#include <android-base/unique_fd.h>
#include <cutils/sockets.h>
#include <inttypes.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

struct AdbdAuthPacketAuthenticated {
  std::string public_key;
};

struct AdbdAuthPacketDisconnected {
  std::string public_key;
};

struct AdbdAuthPacketRequestAuthorization {
  std::string public_key;
};

struct AdbdPacketTlsDeviceConnected {
  uint8_t transport_type;
  std::string public_key;
};

struct AdbdPacketTlsDeviceDisconnected {
  uint8_t transport_type;
  std::string public_key;
};

struct AdbdPacketTlsServerPort {
  uint16_t port;
};

using AdbdAuthPacket =
    std::variant<AdbdAuthPacketAuthenticated, AdbdAuthPacketDisconnected,
                 AdbdAuthPacketRequestAuthorization,
                 AdbdPacketTlsDeviceConnected, AdbdPacketTlsDeviceDisconnected,
                 AdbdPacketTlsServerPort>;

struct AdbdAuthContext {
  static constexpr uint64_t kEpollConstSocket = 0;
  static constexpr uint64_t kEpollConstEventFd = 1;
  static constexpr uint64_t kEpollConstFramework = 2;

 public:

  // For testing purposes, this method accepts a server_fd. This will make the Context use that
  // server socket instead of retrieving init created "adbd" socket. This socket should use
  // SOCK_SEQPACKET to respect message boundaries.
  explicit AdbdAuthContext(const AdbdAuthCallbacksV1* callbacks, std::optional<int> server_fd = {});
  virtual ~AdbdAuthContext() {
    Stop();
  }

  AdbdAuthContext(const AdbdAuthContext& copy) = delete;
  AdbdAuthContext(AdbdAuthContext&& move) = delete;
  AdbdAuthContext& operator=(const AdbdAuthContext& copy) = delete;
  AdbdAuthContext& operator=(AdbdAuthContext&& move) = delete;

  uint64_t NextId() { return next_id_++; }

  void DispatchPendingPrompt() REQUIRES(mutex_);
  void UpdateFrameworkWritable() REQUIRES(mutex_);
  void ReplaceFrameworkFd(android::base::unique_fd new_fd) REQUIRES(mutex_);
  void HandlePacket(std::string_view packet) EXCLUDES(mutex_);
  void AllowUsbDevice(std::string_view buf) EXCLUDES(mutex_);
  void DenyUsbDevice(std::string_view buf) EXCLUDES(mutex_);
  void KeyRemoved(std::string_view buf) EXCLUDES(mutex_);
  bool SendPacket() REQUIRES(mutex_);
  void Run();
  void IteratePublicKeys(bool (*callback)(void*, const char*, size_t), void* opaque);
  uint64_t PromptUser(std::string_view public_key, void* arg) EXCLUDES(mutex_);
  uint64_t NotifyAuthenticated(std::string_view public_key) EXCLUDES(mutex_);
  void NotifyDisconnected(uint64_t id) EXCLUDES(mutex_);
  uint64_t NotifyTlsDeviceConnected(AdbTransportType type, std::string_view public_key)
      EXCLUDES(mutex_);
  void NotifyTlsDeviceDisconnected(AdbTransportType type, uint64_t id)
      EXCLUDES(mutex_);
  void SendTLSServerPort(uint16_t port);
  void Interrupt();
  virtual void InitFrameworkHandlers();

  void Stop();

  bool IsRunning() {
    return running_;
  }

  size_t ReceivedPackets() {
    return received_packets_;
  }

 protected:
  // The file descriptor from epoll_create().
  android::base::unique_fd epoll_fd_;

  // Interrupt server_socket to make epoll_wait return when we have something to write.
  android::base::unique_fd interrupt_fd_;

  // Server socket. Created when the context is created and never changes.
  android::base::unique_fd sock_fd_;

  // The "active" socket when frameworks connects to the server socket.
  // This is where we read/write adbd/Framework messages.
  android::base::unique_fd framework_fd_;

  // Response/Request are matched thanks to a counter. We take outgoing request
  // with this counter id.
  std::atomic<uint64_t> next_id_;

  // Message received from Framework end in a callback. This is were where store
  // the callback targets.
  AdbdAuthCallbacksV1 callbacks_;

  std::mutex mutex_;
  std::unordered_map<uint64_t, std::string> keys_ GUARDED_BY(mutex_);

  // We keep two separate queues: one to handle backpressure from the socket
  // (output_queue_) and one to make sure we only dispatch one authrequest at a
  // time (pending_prompts_).
  std::deque<AdbdAuthPacket> output_queue_ GUARDED_BY(mutex_);

  std::optional<std::tuple<uint64_t, std::string, void*>> dispatched_prompt_ GUARDED_BY(mutex_);
  std::deque<std::tuple<uint64_t, std::string, void*>> pending_prompts_ GUARDED_BY(mutex_);

  std::atomic<bool> running_ = false;

  // This is a list of commands that the framework could send to us.
  using FrameworkHandlerCb = std::function<void(std::string_view)>;
  struct FrameworkPktHandler {
    const char* code;
    FrameworkHandlerCb cb;
  };
  std::vector<FrameworkPktHandler> framework_handlers_;

  std::atomic<size_t> received_packets_ = 0;
};

class AdbdAuthContextV2 : public AdbdAuthContext {
 public:
  explicit AdbdAuthContextV2(const AdbdAuthCallbacksV2* callbacks, std::optional<int> server_fd = {});
  virtual ~AdbdAuthContextV2() = default;
  virtual void InitFrameworkHandlers();
  void StartAdbWifi(std::string_view buf) EXCLUDES(mutex_);
  void StopAdbWifi(std::string_view buf) EXCLUDES(mutex_);

 protected:
  AdbdAuthCallbacksV2 callbacks_v2_;
};
