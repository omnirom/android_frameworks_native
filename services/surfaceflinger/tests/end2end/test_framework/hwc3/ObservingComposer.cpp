/*
 * Copyright 2025 The Android Open Source Project
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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCommand.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerCallback.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/LayerCommand.h>
#include <aidl/android/hardware/graphics/composer3/LayerLifecycleBatchCommandType.h>
#include <aidl/android/hardware/graphics/composer3/PowerMode.h>
#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android-base/thread_annotations.h>  // NOLINT(misc-include-cleaner)
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <fmt/format.h>
#include <ftl/finalizer.h>
#include <ftl/ignore.h>
#include <utils/Mutex.h>

#include "test_framework/core/BufferId.h"
#include "test_framework/core/ConstevaledTypeName.h"
#include "test_framework/hwc3/Hwc3Controller.h"
#include "test_framework/hwc3/ObservingComposer.h"
#include "test_framework/hwc3/delegators/Composer.h"
#include "test_framework/hwc3/delegators/ComposerCallback.h"
#include "test_framework/hwc3/delegators/ComposerClient.h"
#include "test_framework/hwc3/events/BufferPendingDisplay.h"
#include "test_framework/hwc3/events/BufferPendingRelease.h"
#include "test_framework/hwc3/events/ClientDestroyed.h"
#include "test_framework/hwc3/events/DisplayPresented.h"
#include "test_framework/hwc3/events/PowerMode.h"
#include "test_framework/hwc3/events/VSync.h"
#include "test_framework/hwc3/events/VSyncEnabled.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

using aidl::android::hardware::graphics::composer3::CommandResultPayload;
using aidl::android::hardware::graphics::composer3::DisplayCommand;
using aidl::android::hardware::graphics::composer3::IComposer;
using aidl::android::hardware::graphics::composer3::IComposerCallback;
using aidl::android::hardware::graphics::composer3::IComposerClient;
using aidl::android::hardware::graphics::composer3::LayerCommand;
using aidl::android::hardware::graphics::composer3::LayerLifecycleBatchCommandType;
using aidl::android::hardware::graphics::composer3::PowerMode;

namespace {

// Implements a generic forwarder for an interface, given also an matching delegator.
template <typename Interface, template <typename> typename Delegator, bool VersionMustMatch = true>
struct Forwarder : public Delegator<Forwarder<Interface, Delegator, VersionMustMatch>> {
    using Base = Delegator<Forwarder<Interface, Delegator, VersionMustMatch>>;

    auto init(std::shared_ptr<Interface> destination) -> base::expected<void, std::string> {
        int32_t destinationVersion = 0;
        if (!destination->getInterfaceVersion(&destinationVersion).isOk()) {
            return base::unexpected("Failed to get destination interface version");
        }

        if constexpr (VersionMustMatch) {
            if (destinationVersion != Interface::version) {
                return base::unexpected(fmt::format(
                        "{} forwarding interface version mismatch. Destination uses {}, "
                        "forwarder uses {}, and they must be the same.",
                        core::typeNameOf<Interface>(), destinationVersion, Interface::version));
            }
        } else {
            if (destinationVersion < Interface::version) {
                return base::unexpected(fmt::format(
                        "{} forwarding interface version mismatch. Destination uses {}, "
                        "forwarder uses {} and must not be newer.",
                        core::typeNameOf<Interface>(), destinationVersion, Interface::version));
            }
        }

        mDestination = std::move(destination);
        return {};
    }

    auto getComposerClient() -> std::shared_ptr<Interface> {
        return Base::template ref<Forwarder>();
    }

    template <auto MemberFn, typename... Args>
    auto operator()(Args&&... args) -> ::ndk::ScopedAStatus {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        // Uniformly forwards all calls through the appropriate member function.
        return (mDestination.get()->*MemberFn)(std::forward<decltype(args)>(args)...);
#pragma clang diagnostic pop
    }

  private:
    // All requests are forwarded through this interface.
    std::shared_ptr<Interface> mDestination;
};

// Each of these will by default forward calls for that interface to another target.
using ComposerForwarder = Forwarder<IComposer, delegators::Composer>;
using ComposerClientForwarder = Forwarder<IComposerClient, delegators::ComposerClient>;
// Note: SurfaceFlinger's ComposerCallback can be newer than the IComposerCallback used here.
using ComposerCallbackForwarder =
        Forwarder<IComposerCallback, delegators::ComposerCallback, /*VersionMustMatch=*/false>;

// The observer only overrides the interface functions that need to be observed.
// The calls are otherwise forwarded, using the base forwarder class.
struct ComposerClientObserver final : public ComposerClientForwarder {
    using Base = ComposerClientForwarder;

  public:
    struct ComposerCallbackObserver final : public ComposerCallbackForwarder {
        using Base = ComposerCallbackForwarder;

        auto init(std::weak_ptr<const Hwc3Controller> controller,
                  std::shared_ptr<IComposerCallback> destination)
                -> base::expected<void, std::string> {
            if (auto result = Base::init(std::move(destination)); !result) {
                return base::unexpected(std::move(result).error());
            }

            mController = std::move(controller);
            return {};
        }

        // begin IComposerCallback overrides

        auto onVsync(int64_t in_display, int64_t in_timestamp, int32_t in_vsyncPeriodNanos)
                -> ::ndk::ScopedAStatus override {
            if (auto controller = mController.lock()) {
                controller->callbacks().onVSyncCallbackSent(events::VSync{
                        .displayId = in_display,
                        .expectedAt =
                                events::VSync::TimePoint(std::chrono::nanoseconds(in_timestamp)),
                        .expectedPeriod = std::chrono::nanoseconds(in_vsyncPeriodNanos),
                        .receivedAt = std::chrono::steady_clock::now(),
                });
            }

            return Base::onVsync(in_display, in_timestamp, in_vsyncPeriodNanos);
        }

        // end IComposerCallback overrides

      private:
        std::weak_ptr<const Hwc3Controller> mController;
    };

    auto init(std::weak_ptr<const Hwc3Controller> controller,
              std::shared_ptr<IComposerClient> destination) -> base::expected<void, std::string> {
        if (auto result = Base::init(std::move(destination)); !result) {
            return base::unexpected(std::move(result).error());
        }

        mController = std::move(controller);
        return {};
    }

    // begin IComposerClient overrides
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)

    auto destroyLayer(int64_t in_display, int64_t in_layer) -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;
        handleLayerDestroyed(in_display, in_layer);
        return Base::destroyLayer(in_display, in_layer);
    }

    auto executeCommands(const std::vector<DisplayCommand>& in_commands,
                         std::vector<CommandResultPayload>* out_results)
            -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;
        handleExecuteCommands(in_commands);
        return Base::executeCommands(in_commands, out_results);
    }

    auto setPowerMode(int64_t in_display, PowerMode in_mode) -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;
        if (auto observer = mController.lock()) {
            observer->callbacks().onPowerModeChanged(
                    events::PowerMode{.displayId = in_display, .mode = in_mode});
        }

        return Base::setPowerMode(in_display, in_mode);
    }

    auto setVsyncEnabled(int64_t in_display, bool in_enabled) -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;
        if (auto observer = mController.lock()) {
            observer->callbacks().onVsyncEnabledChanged(
                    events::VSyncEnabled{.displayId = in_display, .enabled = in_enabled});
        }
        return Base::setVsyncEnabled(in_display, in_enabled);
    }

    auto registerCallback(const std::shared_ptr<IComposerCallback>& in_callback)
            -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;
        const std::lock_guard lock(mMutex);

        // Set up an observer to intercept the callback events sent from the client implementation.
        auto composerCallback = ndk::SharedRefBase::make<ComposerCallbackObserver>();
        if (auto result = composerCallback->init(mController, in_callback); !result) {
            LOG(ERROR) << "Failed to create a ComposerCallbackObserver instance: "
                       << result.error();
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IComposer::EX_NO_RESOURCES, "Callback interception failed.");
        }
        mComposerCallback = composerCallback;

        // Pass our callback observer interface to the implementation, rather than the original
        // interface.
        return Base::registerCallback(composerCallback);
    }

    // NOLINTEND(bugprone-easily-swappable-parameters)
    // end IComposerClient overrides

  private:
    struct LayerState {
        std::optional<core::BufferId> current;
        std::unordered_map<int32_t, core::BufferId> cache;
    };
    struct DisplayState {
        std::unordered_map<uint64_t, LayerState> layers;
    };
    using DisplaysState = std::unordered_map<uint64_t, DisplayState>;

    void handleBufferChanges(const LayerCommand& cmd, events::DisplayPresented eventTemplate)
            REQUIRES(mMutex) {
        if (!cmd.buffer) {
            return;
        }

        const auto layerId = cmd.layer;
        const auto& buffer = *cmd.buffer;
        const auto slot = buffer.slot;
        auto& layerState = mDisplaysState[eventTemplate.displayId].layers[layerId];

        if (buffer.handle) {
            auto bufferId = core::toBufferId(*buffer.handle);

            LOG(VERBOSE) << "layer " << layerId << " buffer slot " << slot << " set to "
                         << toString(bufferId);
            layerState.cache.emplace(slot, bufferId);

        } else {
            LOG(VERBOSE) << "layer " << layerId << " buffer slot " << slot;
        }

        const auto displayed = layerState.cache[slot];
        const auto released = std::exchange(layerState.current, displayed);

        if (auto observer = mController.lock()) {
            observer->onBufferPendingDisplay(events::BufferPendingDisplay{
                    .displayId = eventTemplate.displayId,
                    .layerId = layerId,
                    .bufferId = displayed,
                    .expectedPresentTime = eventTemplate.expectedPresentTime,
                    .receivedAt = eventTemplate.receivedAt,
            });

            if (released) {
                observer->onBufferPendingRelease(events::BufferPendingRelease{
                        .displayId = eventTemplate.displayId,
                        .layerId = layerId,
                        .bufferId = *released,
                        .expectedPresentTime = eventTemplate.expectedPresentTime,
                        .receivedAt = eventTemplate.receivedAt,
                });
            }
        }

        LOG(VERBOSE) << "Displaying buffer " << toString(displayed);
        LOG_IF(VERBOSE, released.has_value()) << "Releasing buffer " << toString(*released);
    }

    void handleLayerDestroyedCommon(int64_t layerId, events::DisplayPresented eventTemplate)
            REQUIRES(mMutex) {
        auto& layerState = mDisplaysState[eventTemplate.displayId].layers[layerId];
        constexpr auto displayed = std::nullopt;
        const auto released = std::exchange(layerState.current, displayed);

        if (released) {
            if (auto observer = mController.lock()) {
                observer->onBufferPendingRelease(events::BufferPendingRelease{
                        .displayId = eventTemplate.displayId,
                        .layerId = layerId,
                        .bufferId = *released,
                        .expectedPresentTime = eventTemplate.expectedPresentTime,
                        .receivedAt = eventTemplate.receivedAt,
                });
            }
        }

        LOG_IF(VERBOSE, released.has_value()) << "Releasing buffer " << toString(*released);
        LOG(VERBOSE) << "layer " << layerId << " destroy";
        mDisplaysState[eventTemplate.displayId].layers.erase(layerId);
    }

    void handleLayerDestroyed(int64_t in_display, int64_t in_layer) {
        const std::lock_guard lock(mMutex);

        handleLayerDestroyedCommon(
                in_layer,
                events::DisplayPresented{
                        .displayId = in_display,
                        .expectedPresentTime = std::chrono::steady_clock::time_point::min()});
    };

    void executeLayerCommand(const LayerCommand& cmd, events::DisplayPresented eventTemplate)
            REQUIRES(mMutex) {
        LOG(VERBOSE) << "LayerCommand:" << toString(cmd.layerLifecycleBatchCommandType) << " layer "
                     << cmd.layer;

        handleBufferChanges(cmd, eventTemplate);

        if (cmd.layerLifecycleBatchCommandType == LayerLifecycleBatchCommandType::DESTROY) {
            handleLayerDestroyedCommon(cmd.layer, eventTemplate);
        }
    }

    void handleExecuteCommands(const std::vector<DisplayCommand>& in_commands) {
        const std::lock_guard lock(mMutex);

        const auto receivedTime = std::chrono::steady_clock::now();
        for (const auto& cmd : in_commands) {
            LOG(VERBOSE) << cmd.toString();

            using Timestamp = std::chrono::steady_clock::time_point;

            const auto expectedPresentTime =
                    cmd.expectedPresentTime
                            .transform([](auto value) {
                                return Timestamp(std::chrono::nanoseconds(value.timestampNanos));
                            })
                            .value_or(Timestamp::min());

            for (const auto& layerCommand : cmd.layers) {
                executeLayerCommand(layerCommand,
                                    events::DisplayPresented{
                                            .displayId = cmd.display,
                                            .expectedPresentTime = expectedPresentTime,
                                            .receivedAt = receivedTime,
                                    });
            }

            if (cmd.presentOrValidateDisplay) {
                if (auto observer = mController.lock()) {
                    observer->callbacks().onDisplayPresented(
                            events::DisplayPresented{.displayId = cmd.display,
                                                     .expectedPresentTime = expectedPresentTime,
                                                     .receivedAt = receivedTime});
                }
            }
        }
    }

    std::weak_ptr<const Hwc3Controller> mController;

    mutable std::mutex mMutex;
    std::shared_ptr<IComposerCallback> mComposerCallback GUARDED_BY(mMutex);
    DisplaysState mDisplaysState GUARDED_BY(mMutex);
};

}  // namespace

struct ObservingComposer::ObservingComposerImpl final : public ComposerForwarder {
    using Base = ComposerForwarder;

    auto init(std::weak_ptr<const Hwc3Controller> controller,
              std::shared_ptr<IComposer> destination) -> base::expected<void, std::string> {
        if (auto result = Base::init(std::move(destination)); !result) {
            return base::unexpected(std::move(result).error());
        }

        mController = std::move(controller);
        return {};
    }

    // begin IComposer overrides

    auto createClient(std::shared_ptr<IComposerClient>* out_client) -> ndk::ScopedAStatus override {
        auto status = Base::createClient(out_client);
        if (!status.isOk()) {
            return status;
        }

        auto client = ndk::SharedRefBase::make<ComposerClientObserver>();
        if (auto result = client->init(mController, *out_client); !result) {
            *out_client = nullptr;
            LOG(ERROR) << "Failed to create a ComposerClientObserver instance: " << result.error();
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IComposer::EX_NO_RESOURCES, "Client failed to initialize");
        }

        const std::lock_guard lock(mMutex);
        mComposerClient = client;
        *out_client = std::move(client);
        return ndk::ScopedAStatus::ok();
    }

    // end IComposer overrides

  private:
    std::weak_ptr<const Hwc3Controller> mController;

    mutable std::mutex mMutex;
    std::shared_ptr<IComposerClient> mComposerClient GUARDED_BY(mMutex);

    // Finalizers should be last so their destructors are invoked first.
    ftl::FinalizerFtl1 mCleanup{[this] {
        if (auto controller = mController.lock()) {
            controller->callbacks().onClientDestroyed(events::ClientDestroyed{});
        }
    }};
};

struct ObservingComposer::Passkey final {};

auto ObservingComposer::getServiceName(std::string_view baseServiceName) -> std::string {
    return ObservingComposerImpl::makeServiceName(baseServiceName);
}

auto ObservingComposer::make(std::weak_ptr<const Hwc3Controller> controller,
                             std::shared_ptr<IComposer> destination)
        -> base::expected<std::shared_ptr<ObservingComposer>, std::string> {
    using namespace std::string_literals;

    auto composer = std::make_shared<ObservingComposer>(Passkey{});
    if (composer == nullptr) {
        return base::unexpected("Failed to construct the ObservingComposer instance."s);
    }

    if (auto result = composer->init(std::move(controller), std::move(destination)); !result) {
        return base::unexpected("Failed to init the ObservingComposer instance: "s +
                                result.error());
    }

    return composer;
}

ObservingComposer::ObservingComposer(Passkey passkey) {
    ftl::ignore(passkey);
}

auto ObservingComposer::init(std::weak_ptr<const Hwc3Controller> controller,
                             std::shared_ptr<IComposer> destination)
        -> base::expected<void, std::string> {
    using namespace std::string_literals;

    auto impl = ndk::SharedRefBase::make<ObservingComposerImpl>();
    if (!impl) {
        return base::unexpected("Failed to construct the Hwc3ComposerImpl instance."s);
    }

    if (auto result = impl->init(std::move(controller), std::move(destination)); !result) {
        return base::unexpected(std::move(result).error());
    }

    mImpl = std::move(impl);

    return {};
}

auto ObservingComposer::getComposer() -> std::shared_ptr<IComposer> {
    CHECK(mImpl != nullptr);
    return mImpl;
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
