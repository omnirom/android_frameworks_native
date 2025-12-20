/*
 * Copyright (C) 2019 The Android Open Source Project
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

#define ANDROID_BASE_UNIQUE_FD_DISABLE_IMPLICIT_CONVERSION

#include "include/adbd_auth.h"

#include "adbd_auth_internal.h"

using android::base::unique_fd;

static constexpr uint32_t kAuthVersion = 2;

static std::set<AdbdAuthFeature> supported_features = {
    AdbdAuthFeature::WifiLifeCycle};

AdbdAuthContext::AdbdAuthContext(const AdbdAuthCallbacksV1* callbacks, std::optional<int> server_fd)
    : next_id_(0), callbacks_(*callbacks) {
  epoll_fd_.reset(epoll_create1(EPOLL_CLOEXEC));
  if (epoll_fd_ == -1) {
    PLOG(FATAL) << "adbd_auth: failed to create epoll fd";
  }

  interrupt_fd_.reset(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK));
  if (interrupt_fd_ == -1) {
    PLOG(FATAL) << "adbd_auth: failed to create interrupt_fd";
  }

  if (server_fd.has_value()) {
    sock_fd_.reset(server_fd.value());
  } else {
    sock_fd_.reset(android_get_control_socket("adbd"));
  }

  if (sock_fd_ == -1) {
    PLOG(ERROR) << "adbd_auth: failed to get adbd authentication socket";
  } else {
    if (fcntl(sock_fd_.get(), F_SETFD, FD_CLOEXEC) != 0) {
      PLOG(FATAL)
          << "adbd_auth: failed to make adbd authentication socket cloexec";
    }

    if (fcntl(sock_fd_.get(), F_SETFL, O_NONBLOCK) != 0) {
      PLOG(FATAL)
          << "adbd_auth: failed to make adbd authentication socket nonblocking";
    }

    if (listen(sock_fd_.get(), 4) != 0) {
      PLOG(FATAL)
          << "adbd_auth: failed to listen on adbd authentication socket";
    }
  }
}

void AdbdAuthContext::DispatchPendingPrompt() REQUIRES(mutex_) {
  if (dispatched_prompt_) {
    LOG(INFO) << "adbd_auth: prompt currently pending, skipping";
    return;
  }

  if (pending_prompts_.empty()) {
    LOG(INFO) << "adbd_auth: no prompts to send";
    return;
  }

  LOG(INFO) << "adbd_auth: prompting user for adb authentication";
  auto [id, public_key, arg] = std::move(pending_prompts_.front());
  pending_prompts_.pop_front();

  this->output_queue_.emplace_back(
      AdbdAuthPacketRequestAuthorization{.public_key = public_key});

  Interrupt();
  dispatched_prompt_ = std::make_tuple(id, public_key, arg);
}

void AdbdAuthContext::UpdateFrameworkWritable() REQUIRES(mutex_) {
  // This might result in redundant calls to EPOLL_CTL_MOD if, for example, we
  // get notified at the same time as a framework connection, but that's
  // unlikely and this doesn't need to be fast anyway.
  if (framework_fd_ != -1) {
    struct epoll_event event;
    event.events = EPOLLIN;
    if (!output_queue_.empty()) {
      LOG(INFO) << "adbd_auth: marking framework writable";
      event.events |= EPOLLOUT;
    }
    event.data.u64 = kEpollConstFramework;
    CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_MOD, framework_fd_.get(),
                          &event));
  }
}

void AdbdAuthContext::ReplaceFrameworkFd(unique_fd new_fd) REQUIRES(mutex_) {
  LOG(INFO) << "adbd_auth: received new framework fd " << new_fd.get()
            << " (current = " << framework_fd_.get() << ")";

  // If we already had a framework fd, clean up after ourselves.
  if (framework_fd_ != -1) {
    output_queue_.clear();
    dispatched_prompt_.reset();
    CHECK_EQ(0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_DEL, framework_fd_.get(),
                          nullptr));
    framework_fd_.reset();
  }

  if (new_fd != -1) {
    struct epoll_event event;
    event.events = EPOLLIN;
    if (!output_queue_.empty()) {
      LOG(INFO) << "adbd_auth: marking framework writable";
      event.events |= EPOLLOUT;
    }
    event.data.u64 = kEpollConstFramework;
    CHECK_EQ(0,
             epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, new_fd.get(), &event));
    framework_fd_ = std::move(new_fd);
  }
}

void AdbdAuthContext::HandlePacket(std::string_view packet) EXCLUDES(mutex_) {
  LOG(INFO) << "adbd_auth: received packet: " << packet;

  received_packets_++;
  if (packet.size() < 2) {
    LOG(ERROR) << "adbd_auth: received packet of invalid length";
    std::lock_guard<std::mutex> lock(mutex_);
    ReplaceFrameworkFd(unique_fd());
  }

  bool handled_packet = false;
  for (size_t i = 0; i < framework_handlers_.size(); ++i) {
    if (android::base::ConsumePrefix(&packet, framework_handlers_[i].code)) {
      framework_handlers_[i].cb(packet);
      handled_packet = true;
      break;
    }
  }
  if (!handled_packet) {
    LOG(ERROR) << "adbd_auth: unhandled packet: " << packet;
  }
}

void AdbdAuthContext::AllowUsbDevice(std::string_view buf) EXCLUDES(mutex_) {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(buf.empty());

  if (dispatched_prompt_.has_value()) {
    // It's possible for the framework to send us a response without our having
    // sent a request to it: e.g. if adbd restarts while we have a pending
    // request.
    auto& [id, key, arg] = *dispatched_prompt_;
    keys_.emplace(id, std::move(key));

    callbacks_.key_authorized(arg, id);
    dispatched_prompt_ = std::nullopt;
  } else {
    LOG(WARNING)
        << "adbd_auth: received authorization for unknown prompt, ignoring";
  }

  // We need to dispatch pending prompts here upon success as well,
  // since we might have multiple queued prompts.
  DispatchPendingPrompt();
}

void AdbdAuthContext::DenyUsbDevice(std::string_view buf) EXCLUDES(mutex_) {
  std::lock_guard<std::mutex> lock(mutex_);
  CHECK(buf.empty());
  // TODO: Do we want a callback if the key is denied?
  dispatched_prompt_ = std::nullopt;
  DispatchPendingPrompt();
}

void AdbdAuthContext::KeyRemoved(std::string_view buf) EXCLUDES(mutex_) {
  CHECK(!buf.empty());
  callbacks_.key_removed(buf.data(), buf.size());
}

bool AdbdAuthContext::SendPacket() REQUIRES(mutex_) {
  if (output_queue_.empty()) {
    return false;
  }

  CHECK_NE(-1, framework_fd_.get());

  auto& packet = output_queue_.front();
  struct iovec iovs[3];
  int iovcnt = 2;

  if (auto* p = std::get_if<AdbdAuthPacketAuthenticated>(&packet)) {
    iovs[0].iov_base = const_cast<char*>("CK");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = p->public_key.data();
    iovs[1].iov_len = p->public_key.size();
  } else if (auto* p = std::get_if<AdbdAuthPacketDisconnected>(&packet)) {
    iovs[0].iov_base = const_cast<char*>("DC");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = p->public_key.data();
    iovs[1].iov_len = p->public_key.size();
  } else if (auto* p =
                 std::get_if<AdbdAuthPacketRequestAuthorization>(&packet)) {
    iovs[0].iov_base = const_cast<char*>("PK");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = p->public_key.data();
    iovs[1].iov_len = p->public_key.size();
  } else if (auto* p = std::get_if<AdbdPacketTlsDeviceConnected>(&packet)) {
    iovcnt = 3;
    iovs[0].iov_base = const_cast<char*>("WE");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = &p->transport_type;
    iovs[1].iov_len = 1;
    iovs[2].iov_base = p->public_key.data();
    iovs[2].iov_len = p->public_key.size();
  } else if (auto* p = std::get_if<AdbdPacketTlsDeviceDisconnected>(&packet)) {
    iovcnt = 3;
    iovs[0].iov_base = const_cast<char*>("WF");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = &p->transport_type;
    iovs[1].iov_len = 1;
    iovs[2].iov_base = p->public_key.data();
    iovs[2].iov_len = p->public_key.size();
  } else if (auto* p = std::get_if<AdbdPacketTlsServerPort>(&packet)) {
    iovcnt = 2;
    iovs[0].iov_base = const_cast<char*>("TP");
    iovs[0].iov_len = 2;
    iovs[1].iov_base = &p->port;
    iovs[1].iov_len = 2;
  } else {
    LOG(FATAL) << "adbd_auth: unhandled packet type?";
  }

  LOG(INFO) << "adbd_auth: sending packet: "
            << std::string((const char*)iovs[0].iov_base, 2);

  ssize_t rc = writev(framework_fd_.get(), iovs, iovcnt);
  output_queue_.pop_front();
  if (rc == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
    PLOG(ERROR) << "adbd_auth: failed to write to framework fd";
    ReplaceFrameworkFd(unique_fd());
    return false;
  }

  return true;
}

void AdbdAuthContext::Run() {
  if (sock_fd_ == -1) {
    LOG(ERROR) << "adbd_auth: socket unavailable, disabling user prompts";
  } else {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u64 = kEpollConstSocket;
    CHECK_EQ(0,
             epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, sock_fd_.get(), &event));
  }

  {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u64 = kEpollConstEventFd;
    CHECK_EQ(
        0, epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, interrupt_fd_.get(), &event));
  }

  running_ = true;

  while (running_) {
    struct epoll_event events[3];
    int rc = TEMP_FAILURE_RETRY(epoll_wait(epoll_fd_.get(), events, 3, -1));
    if (rc == -1) {
      PLOG(FATAL) << "adbd_auth: epoll_wait failed";
    } else if (rc == 0) {
      LOG(FATAL) << "adbd_auth: epoll_wait returned 0";
    }

    bool restart = false;
    for (int i = 0; i < rc; ++i) {
      if (restart) {
        break;
      }

      struct epoll_event& event = events[i];
      switch (event.data.u64) {
        case kEpollConstSocket: {
          unique_fd new_framework_fd(accept4(sock_fd_.get(), nullptr, nullptr,
                                             SOCK_CLOEXEC | SOCK_NONBLOCK));
          if (new_framework_fd == -1) {
            PLOG(FATAL) << "adbd_auth: failed to accept framework fd";
          }

          LOG(INFO) << "adbd_auth: received a new framework connection";
          std::lock_guard<std::mutex> lock(mutex_);
          ReplaceFrameworkFd(std::move(new_framework_fd));

          // Stop iterating over events: one of the later ones might be the old
          // framework fd.
          restart = false;
          break;
        }

        case kEpollConstEventFd: {
          // We were woken up to write something.
          uint64_t dummy;
          int rc =
              TEMP_FAILURE_RETRY(read(interrupt_fd_.get(), &dummy, sizeof(dummy)));
          if (rc != 8) {
            PLOG(FATAL) << "adbd_auth: failed to read from eventfd (rc = " << rc
                        << ")";
          }

          std::lock_guard<std::mutex> lock(mutex_);
          UpdateFrameworkWritable();
          break;
        }

        case kEpollConstFramework: {
          char buf[4096];
          if (event.events & EPOLLIN) {
            int rc =
                TEMP_FAILURE_RETRY(read(framework_fd_.get(), buf, sizeof(buf)));
            if (rc == -1) {
              PLOG(FATAL) << "adbd_auth: failed to read from framework fd";
            } else if (rc == 0) {
              LOG(INFO) << "adbd_auth: hit EOF on framework fd";
              std::lock_guard<std::mutex> lock(mutex_);
              ReplaceFrameworkFd(unique_fd());
            } else {
              HandlePacket(std::string_view(buf, rc));
            }
          }

          if (event.events & EPOLLOUT) {
            std::lock_guard<std::mutex> lock(mutex_);
            while (SendPacket()) {
              continue;
            }
            UpdateFrameworkWritable();
          }

          break;
        }
      }
    }
  }
}

void AdbdAuthContext::Stop() {
  running_ = false;
  Interrupt();
}

static constexpr std::pair<const char*, bool> key_paths[] = {
    {"/adb_keys", true /* follow symlinks */},
    {"/data/misc/adb/adb_keys", false /* don't follow symlinks */},
};
void AdbdAuthContext::IteratePublicKeys(bool (*callback)(void*, const char*,
                                                         size_t),
                                        void* opaque) {
  for (const auto& [path, follow_symlinks] : key_paths) {
    if (access(path, R_OK) == 0) {
      LOG(INFO) << "adbd_auth: loading keys from " << path;
      std::string content;
      if (!android::base::ReadFileToString(path, &content, follow_symlinks)) {
        PLOG(ERROR) << "adbd_auth: couldn't read " << path;
        continue;
      }
      for (const auto& line : android::base::Split(content, "\n")) {
        if (!callback(opaque, line.data(), line.size())) {
          return;
        }
      }
    }
  }
}

uint64_t AdbdAuthContext::PromptUser(std::string_view public_key, void* arg)
    EXCLUDES(mutex_) {
  uint64_t id = NextId();

  std::lock_guard<std::mutex> lock(mutex_);
  LOG(INFO) << "adbd_auth: sending prompt with id " << id;
  pending_prompts_.emplace_back(id, public_key, arg);
  DispatchPendingPrompt();
  return id;
}

uint64_t AdbdAuthContext::NotifyAuthenticated(std::string_view public_key)
    EXCLUDES(mutex_) {
  uint64_t id = NextId();
  std::lock_guard<std::mutex> lock(mutex_);
  keys_.emplace(id, public_key);
  output_queue_.emplace_back(
      AdbdAuthPacketAuthenticated{.public_key = std::string(public_key)});
  return id;
}

void AdbdAuthContext::NotifyDisconnected(uint64_t id) EXCLUDES(mutex_) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = keys_.find(id);
  if (it == keys_.end()) {
    LOG(DEBUG) << "adbd_auth: couldn't find public key to notify "
                  "disconnection, skipping";
    return;
  }
  output_queue_.emplace_back(
      AdbdAuthPacketDisconnected{.public_key = std::move(it->second)});
  keys_.erase(it);
}

uint64_t AdbdAuthContext::NotifyTlsDeviceConnected(AdbTransportType type,
                                                   std::string_view public_key)
    EXCLUDES(mutex_) {
  uint64_t id = NextId();
  std::lock_guard<std::mutex> lock(mutex_);
  keys_.emplace(id, public_key);
  output_queue_.emplace_back(
      AdbdPacketTlsDeviceConnected{.transport_type = static_cast<uint8_t>(type),
                                   .public_key = std::string(public_key)});
  Interrupt();
  return id;
}

void AdbdAuthContext::NotifyTlsDeviceDisconnected(AdbTransportType type,
                                                  uint64_t id)
    EXCLUDES(mutex_) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto it = keys_.find(id);
  if (it == keys_.end()) {
    LOG(DEBUG)
        << "adbd_auth: couldn't find public key to notify disconnection of tls "
           "device, skipping";
    return;
  }
  output_queue_.emplace_back(AdbdPacketTlsDeviceDisconnected{
      .transport_type = static_cast<uint8_t>(type),
      .public_key = std::move(it->second)});
  keys_.erase(it);
  Interrupt();
}

void AdbdAuthContext::SendTLSServerPort(uint16_t port) {
  std::lock_guard<std::mutex> lock(mutex_);
  output_queue_.emplace_back(AdbdPacketTlsServerPort{.port = port});
  Interrupt();
}

// Interrupt the worker thread to do some work.
void AdbdAuthContext::Interrupt() {
  uint64_t value = 1;
  ssize_t rc = write(interrupt_fd_.get(), &value, sizeof(value));
  if (rc == -1) {
    PLOG(FATAL) << "adbd_auth: write to interrupt_fd failed";
  } else if (rc != sizeof(value)) {
    LOG(FATAL) << "adbd_auth: write to interrupt_fd returned short (" << rc << ")";
  }
}

void AdbdAuthContext::InitFrameworkHandlers() {
  // Framework wants to disconnect from a secured wifi device
  framework_handlers_.emplace_back(
      FrameworkPktHandler{.code = "DD",
                          .cb = std::bind(&AdbdAuthContext::KeyRemoved, this,
                                          std::placeholders::_1)});
  // Framework allows USB debugging for the device
  framework_handlers_.emplace_back(
      FrameworkPktHandler{.code = "OK",
                          .cb = std::bind(&AdbdAuthContext::AllowUsbDevice,
                                          this, std::placeholders::_1)});
  // Framework denies USB debugging for the device
  framework_handlers_.emplace_back(
      FrameworkPktHandler{.code = "NO",
                          .cb = std::bind(&AdbdAuthContext::DenyUsbDevice, this,
                                          std::placeholders::_1)});
}

AdbdAuthContextV2::AdbdAuthContextV2(const AdbdAuthCallbacksV2* callbacks, std::optional<int> server_fd)
    : AdbdAuthContext(callbacks, server_fd), callbacks_v2_(*callbacks) {}

void AdbdAuthContextV2::InitFrameworkHandlers() {
  AdbdAuthContext::InitFrameworkHandlers();
  // Framework requires ADB Wifi to start
  framework_handlers_.emplace_back(
      FrameworkPktHandler{.code = "W1",
                          .cb = std::bind(&AdbdAuthContextV2::StartAdbWifi,
                                          this, std::placeholders::_1)});

  // Framework requires ADB Wifi to stop
  framework_handlers_.emplace_back(
      FrameworkPktHandler{.code = "W0",
                          .cb = std::bind(&AdbdAuthContextV2::StopAdbWifi, this,
                                          std::placeholders::_1)});
}

void AdbdAuthContextV2::StartAdbWifi(std::string_view buf) EXCLUDES(mutex_) {
  CHECK(buf.empty());
  callbacks_v2_.start_adbd_wifi();
}

void AdbdAuthContextV2::StopAdbWifi(std::string_view buf) EXCLUDES(mutex_) {
  CHECK(buf.empty());
  callbacks_v2_.stop_adbd_wifi();
}

AdbdAuthContext* adbd_auth_new(AdbdAuthCallbacks* callbacks) {
  switch (callbacks->version) {
    case 1: {
      AdbdAuthContext* ctx = new AdbdAuthContext(
          reinterpret_cast<AdbdAuthCallbacksV1*>(callbacks));
      ctx->InitFrameworkHandlers();
      return ctx;
    }
    case kAuthVersion: {
      AdbdAuthContextV2* ctx2 = new AdbdAuthContextV2(
          reinterpret_cast<AdbdAuthCallbacksV2*>(callbacks));
      ctx2->InitFrameworkHandlers();
      return ctx2;
    }
    default: {
      LOG(ERROR) << "adbd_auth: received unknown AdbdAuthCallbacks version "
                 << callbacks->version;
      return nullptr;
    }
  }
}

void adbd_auth_delete(AdbdAuthContext* ctx) { delete ctx; }

void adbd_auth_run(AdbdAuthContext* ctx) { return ctx->Run(); }

void adbd_auth_get_public_keys(AdbdAuthContext* ctx,
                               bool (*callback)(void* opaque,
                                                const char* public_key,
                                                size_t len),
                               void* opaque) {
  ctx->IteratePublicKeys(callback, opaque);
}

uint64_t adbd_auth_notify_auth(AdbdAuthContext* ctx, const char* public_key,
                               size_t len) {
  return ctx->NotifyAuthenticated(std::string_view(public_key, len));
}

void adbd_auth_notify_disconnect(AdbdAuthContext* ctx, uint64_t id) {
  return ctx->NotifyDisconnected(id);
}

void adbd_auth_prompt_user(AdbdAuthContext* ctx, const char* public_key,
                           size_t len, void* opaque) {
  adbd_auth_prompt_user_with_id(ctx, public_key, len, opaque);
}

uint64_t adbd_auth_prompt_user_with_id(AdbdAuthContext* ctx,
                                       const char* public_key, size_t len,
                                       void* opaque) {
  return ctx->PromptUser(std::string_view(public_key, len), opaque);
}

uint64_t adbd_auth_tls_device_connected(AdbdAuthContext* ctx,
                                        AdbTransportType type,
                                        const char* public_key, size_t len) {
  return ctx->NotifyTlsDeviceConnected(type, std::string_view(public_key, len));
}

void adbd_auth_tls_device_disconnected(AdbdAuthContext* ctx,
                                       AdbTransportType type, uint64_t id) {
  ctx->NotifyTlsDeviceDisconnected(type, id);
}

uint32_t adbd_auth_get_max_version() { return kAuthVersion; }

bool adbd_auth_supports_feature(AdbdAuthFeature f) {
  return supported_features.contains(f);
}

void adbd_auth_send_tls_server_port(AdbdAuthContext* ctx, uint16_t port) {
  ctx->SendTLSServerPort(port);
}
