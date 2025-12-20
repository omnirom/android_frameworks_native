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

#include <gtest/gtest.h>

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <thread>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <android-base/unique_fd.h>

#include "adbd_auth.h"
#include "adbd_auth_internal.h"

void Log(const std::string& msg) {
  LOG(INFO) << "(" << gettid() << "): " << msg;
}

using namespace std::string_view_literals;
using namespace std::string_literals;
using namespace std::chrono_literals;

constexpr std::string_view kUdsName = "\0adb_auth_test_uds"sv;
static_assert(kUdsName.size() <= sizeof(reinterpret_cast<sockaddr_un*>(0)->sun_path));


// A convenient struct that will stop the context and wait for the context runner thread to
// return when it is destroyed.
struct ContextRunner {
 public:
  explicit ContextRunner(std::unique_ptr<AdbdAuthContext> context)  : context_(std::move(context)) {
    thread_ = std::thread([raw_context = context_.get()]() {
      raw_context->Run();
    });

    // Wait until the context has started running.
    while (!context_->IsRunning()) {
      std::this_thread::sleep_for(10ms);
    }
  }

  ~ContextRunner() {
    context_->Stop();
    thread_.join();
  }

  AdbdAuthContext* Context() {
    return context_.get();
  }
 private:
  std::thread thread_;
  std::unique_ptr<AdbdAuthContext> context_;
};

// Emulate android_get_control_socket which adbauth uses to get the Unix Domain Socket
// to communicate with Framework.
std::optional<int> CreateServerSocket() {
  int sockfd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if (sockfd == -1) {
    Log("Failed to create server socket");
    return {};
  }

  struct sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  memset(addr.sun_path, 0, sizeof(addr.sun_path));
  strncpy(addr.sun_path, kUdsName.data(), kUdsName.size());

  if (bind(sockfd, (struct sockaddr*) &addr, sizeof(sockaddr_un)) == -1) {
    Log("Failed to bind socket server to abstract namespace");
    return {};
  }

  if (listen(sockfd, 1) == -1) {
    Log("Failed to listen on socket server");
    return {};
  }

  return sockfd;
}

// Workaround to fail from anywhere. FAIL macro can only be called from a function
// returning void.
void fail(const std::string& msg) {
  FAIL() << msg;
}

// This class behaves like AdbDebuggingManager.
//   - Open the UDS created by our adb_auth emulation layer
//   - Allow to send messages
//   - Allow to recv messages
class Framework {
 public:
  Framework() {
    socket_ = Connect();
  }

  ~Framework() {
    close(socket_);
  }

  std::string Recv() const {
    char msg[256];
    auto num_bytes_read = read(socket_, msg, sizeof(msg));
    if (num_bytes_read < 0) {
      Log("Framework could not read: "s + strerror(errno));
      return "";
    }

    Log("Framework read "s + std::to_string(num_bytes_read) + " bytes");
    return std::string(msg, num_bytes_read);
  }

  int Send(const std::string& msg) const {
    return write(socket_, msg.data(), msg.size());
  }

  void SendAndWaitContext(const std::string& msg, ContextRunner* runner) {
    auto packet_id = runner->Context()->ReceivedPackets();
    Send(msg);

    while(runner->Context()->ReceivedPackets() == packet_id) {
      std::this_thread::sleep_for(10ms);
    }
  }

 private:
  int socket_;

  int Connect() {
    int fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd == -1) {
      fail("Cannot create client socket");
    }


    sockaddr_un addr = { .sun_family = AF_UNIX };
    strncpy(addr.sun_path, kUdsName.data(), kUdsName.size());

    auto res = connect(fd, (struct sockaddr*) &addr, sizeof(addr));
    if (res == -1) {
      fail("Cannot connect client socket");
    }

    return fd;
  }
};

std::unique_ptr<ContextRunner> CreateContextRunner(const AdbdAuthCallbacks& cb) {
  auto server_socket = CreateServerSocket();
  if (!server_socket.has_value()) {
    Log("Cannot create context");
    return {};
  }

  std::unique_ptr<AdbdAuthContext> context;
  switch (cb.version) {
    case 1: {
      context = std::make_unique<AdbdAuthContext>(&reinterpret_cast<const AdbdAuthCallbacksV1&>(cb),
                                            server_socket.value());
      break;
    }
    case 2: {
      context = std::make_unique<AdbdAuthContextV2>(&reinterpret_cast<const AdbdAuthCallbacksV2&>(cb),
                                              server_socket.value());
      break;
    }
    default: {
      fail("Unable to create AuthContext for version="s + std::to_string(cb.version));
    }
  }
  context->InitFrameworkHandlers();

  return std::make_unique<ContextRunner>(std::move(context));
}

TEST(AdbAuthTest, SendTcpPort) {
  AdbdAuthCallbacksV1 callbacks{};
  callbacks.version = 1;
  auto runner = CreateContextRunner(callbacks);
  Framework framework{};

  // Send TLS to framework
  const uint8_t port = 19;
  adbd_auth_send_tls_server_port(runner->Context(), port);

  // Check that Framework received it.
  std::string msg = framework.Recv();
  ASSERT_EQ(4, msg.size());
  ASSERT_EQ(msg[0], 'T');
  ASSERT_EQ(msg[1], 'P');
  ASSERT_EQ(msg[2], port);
  ASSERT_EQ(msg[3], 0);
}

// If user forget to set callbacks, adbauth should not crash. Instead, it should
// discard messages and issue a warning.
TEST(AdbAuthTest, UnsetCallbacks) {
    AdbdAuthCallbacksV2 callbacks{};
    callbacks.version = 2;
    auto runner= CreateContextRunner(callbacks);
    Framework framework{};

    // We did not set the callback "start ADB Wifi". This should not crash if
    // the message is properly dispatched.
    framework.Send("W1");
    // We did not set the callback "stop ADB Wifi". This should not crash if.
    // the message is properly dispatched.
    framework.Send("W0");
}

// Test Wifi lifecycle callbacks
TEST(AdbAuthTest, WifiLifeCycle) {
    AdbdAuthCallbacksV2 callbacks{};
    callbacks.version = 2;

    static bool start_message_received = false;
    callbacks.start_adbd_wifi = [] {
       start_message_received= true;
    };

    static bool stop_message_received = false;
    callbacks.stop_adbd_wifi = [] {
       stop_message_received= true;
    };


    auto runner= CreateContextRunner(callbacks);
    Framework framework{};

    framework.SendAndWaitContext("W1", runner.get());
    ASSERT_EQ(start_message_received,true);

    framework.SendAndWaitContext("W0", runner.get());
    ASSERT_EQ(stop_message_received,true);
}


TEST(AdbAuthTest, UnhandledPacket) {
    AdbdAuthCallbacksV2 callbacks{};
    callbacks.version = 2;
    auto runner= CreateContextRunner(callbacks);
    Framework framework{};

    uint16_t port = 19;
    adbd_auth_send_tls_server_port(runner->Context(), port);

    // Send an unhandled packet. This should not reset the stack.
    framework.SendAndWaitContext("XX", runner.get());

    // Check that libauth did not reset the socket.
    auto msg = framework.Recv();
    ASSERT_EQ(4, msg.size());
    ASSERT_EQ(msg[0], 'T');
    ASSERT_EQ(msg[1], 'P');
    ASSERT_EQ(msg[2], port);
    ASSERT_EQ(msg[3], 0);
}
