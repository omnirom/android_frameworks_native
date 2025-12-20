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
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ratio>
#include <source_location>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <aidl/android/hardware/graphics/common/DisplayHotplugEvent.h>
#include <aidl/android/hardware/graphics/common/Transform.h>
#include <aidl/android/hardware/graphics/composer3/Capability.h>
#include <aidl/android/hardware/graphics/composer3/ColorMode.h>
#include <aidl/android/hardware/graphics/composer3/CommandResultPayload.h>
#include <aidl/android/hardware/graphics/composer3/ContentType.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCommand.h>
#include <aidl/android/hardware/graphics/composer3/DisplayConnectionType.h>
#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerCallback.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <aidl/android/hardware/graphics/composer3/PerFrameMetadataKey.h>
#include <aidl/android/hardware/graphics/composer3/PowerMode.h>
#include <aidl/android/hardware/graphics/composer3/PresentOrValidate.h>
#include <aidl/android/hardware/graphics/composer3/RenderIntent.h>
#include <android-base/expected.h>
#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_interface_utils.h>
#include <android/binder_status.h>
#include <fmt/format.h>
#include <ftl/ignore.h>

#include "test_framework/core/AsyncFunction.h"
#include "test_framework/core/DisplayConfiguration.h"
#include "test_framework/core/EdidBuilder.h"
#include "test_framework/core/GuardedSharedState.h"
#include "test_framework/hwc3/DisplayVSyncEventService.h"
#include "test_framework/hwc3/FakeComposer.h"
#include "test_framework/hwc3/FakeDisplayConfiguration.h"
#include "test_framework/hwc3/SingleDisplayRefreshSchedule.h"
#include "test_framework/hwc3/delegators/Composer.h"
#include "test_framework/hwc3/delegators/ComposerClient.h"
#include "test_framework/hwc3/events/VSync.h"

namespace android::surfaceflinger::tests::end2end::test_framework::hwc3 {

using Capability = aidl::android::hardware::graphics::composer3::Capability;
using CommandError = aidl::android::hardware::graphics::composer3::CommandError;
using CommandResultPayload = aidl::android::hardware::graphics::composer3::CommandResultPayload;
using DisplayCommand = aidl::android::hardware::graphics::composer3::DisplayCommand;
using IComposer = aidl::android::hardware::graphics::composer3::IComposer;
using IComposerCallback = aidl::android::hardware::graphics::composer3::IComposerCallback;
using IComposerClient = aidl::android::hardware::graphics::composer3::IComposerClient;
using PresentOrValidate = aidl::android::hardware::graphics::composer3::PresentOrValidate;

class ValueName {
    template <auto Value>
    static consteval auto pretty() -> std::string_view {
        return __PRETTY_FUNCTION__;
    }

    static constexpr void exemplar();  // Intentionally declared but not defined.

    static consteval auto trim(std::string_view name) -> std::string_view {
        constexpr std::string_view expected = "&std::source_location::line";
        constexpr std::string_view ref = pretty<&std::source_location::line>();
        constexpr size_t prefixLength = ref.rfind(expected);
        static_assert(prefixLength != std::string_view::npos);
        constexpr size_t suffixLength = ref.size() - prefixLength - expected.size();
        static_assert(prefixLength + suffixLength + expected.size() == ref.size());

        name = name.substr(prefixLength, name.size() - suffixLength - prefixLength);
        if (name.starts_with('&')) {
            name = name.substr(1);
        }
        if (auto prefix = name.rfind("::"); prefix != std::string_view::npos) {
            name = name.substr(prefix + 2);
        }
        return name;
    }

    template <size_t... Indices>
    static consteval auto copy(std::string_view input, std::index_sequence<Indices...> sequence)
            -> std::array<char, 1 + sizeof...(Indices)> {
        ftl::ignore(sequence);
        return {input[Indices]..., 0};
    }

    template <auto Value>
    static consteval auto operator()() -> std::string_view {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        constexpr auto name = trim(pretty<Value>());
#pragma clang diagnostic pop
        static constexpr auto copied = copy(name, std::make_index_sequence<name.size()>{});
        return copied.data();
    }

  public:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    template <auto Value>
    static constexpr auto value = operator()<Value>();
#pragma clang diagnostic pop
};

template <typename Interface, template <typename> typename Delegator>
class FatalIfUnhandled : public Delegator<FatalIfUnhandled<Interface, Delegator>> {
  public:
    // Attempts to extract just the base function name from a source_location::function_name()
    constexpr static auto terseFunctionName(std::string_view input) {
        auto suffix = input.find('(');
        if (suffix != std::string_view::npos) {
            auto remainder = input.substr(0, suffix);
            auto prefix = remainder.rfind("::");
            if (prefix != std::string_view::npos) {
                return remainder.substr(prefix + 2);
            }
        }
        return input;
    }

    [[nodiscard]] static auto makeIgnoredMessage(std::string_view functionName) -> std::string {
        return fmt::format("Ignoring {}.", functionName);
    }

    [[nodiscard]] static auto ignore(
            const std::string& message = makeIgnoredMessage(terseFunctionName(
                    std::source_location::current().function_name()))) -> ndk::ScopedAStatus {
        LOG(DEBUG) << message;
        return ndk::ScopedAStatus::ok();
    }

    [[nodiscard]] static auto makeUnsupportedMessage(std::string_view functionName) -> std::string {
        return fmt::format("{} is not implemented by the test fake.",
                           terseFunctionName(functionName));
    }

    [[nodiscard]] static auto unsupported(
            const std::string& message = makeUnsupportedMessage(terseFunctionName(
                    std::source_location::current().function_name()))) -> ndk::ScopedAStatus {
        LOG(DEBUG) << message;
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                                message.c_str());
    }

    [[nodiscard]] static auto unsupportedFatal(
            const std::string& message = makeUnsupportedMessage(
                    std::source_location::current().function_name())) -> ndk::ScopedAStatus {
        LOG(FATAL) << message;
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_UNSUPPORTED_OPERATION,
                                                                message.c_str());
    }

    template <auto MemberFn, typename... Args>
    auto operator()(Args&&... args) -> ndk::ScopedAStatus {
        ftl::ignore(std::forward<decltype(args)>(args)...);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        return unsupportedFatal(makeUnsupportedMessage(ValueName::value<MemberFn>));
#pragma clang diagnostic pop
    }
};

using DefaultComposerClientImpl = FatalIfUnhandled<IComposerClient, delegators::ComposerClient>;

using DefaultComposerImpl = FatalIfUnhandled<IComposer, delegators::Composer>;

// Local simplified fork of ComposerServiceWriter from
// android/hardware/graphics/composer3/ComposerServiceWriter.h
//
// Using that header library directly assumes the latest version of the AIDL interfaces, but that
// conflicts with the choice here to use an older but stable version of the interface.
struct ComposerServiceWriter {
    void setError(int32_t index, int32_t errorCode) {
        mCommandsResults.emplace_back(CommandError{.commandIndex = index, .errorCode = errorCode});
    }

    void setPresentOrValidateResult(int64_t display, PresentOrValidate::Result result) {
        mCommandsResults.emplace_back(PresentOrValidate{.display = display, .result = result});
    }

    auto getPendingCommandResults() -> std::vector<CommandResultPayload> {
        return std::move(mCommandsResults);
    }

  private:
    std::vector<CommandResultPayload> mCommandsResults;
};

struct ClientImpl final : public DefaultComposerClientImpl {
    struct Callbacks final {
        using OptDisplayConfiguration = std::optional<FakeDisplayConfiguration>;

        using GetDisplayAsyncConnector =
                core::AsyncFunctionStd<auto(int64_t displayId)->OptDisplayConfiguration>;
        using GetDisplayIdsAsyncConnector = core::AsyncFunctionStd<auto()->std::vector<int64_t>>;

        // Invoked by the client to get the information for a display, in response to a SF query.
        GetDisplayAsyncConnector onGetDisplay;

        // Invoked when the client needs to get the connected displays, in response to a SF query.
        GetDisplayIdsAsyncConnector onGetDisplayIds;

        // Convenience function to reset all the callbacks to have no target/receiver.
        friend void clearAll(Callbacks& callbacks) {
            callbacks.onGetDisplay.clear()();
            callbacks.onGetDisplayIds.clear()();
        }
    };

    auto editCallbacks() -> Callbacks& { return mClientCallbacks; }

    auto getComposerClient() -> std::shared_ptr<IComposerClient> { return ref<ClientImpl>(); }

    void sendDisplayAdded(int64_t displayId) {
        mState.withSharedLock([displayId](const auto& state) {
            using aidl::android::hardware::graphics::common::DisplayHotplugEvent::CONNECTED;
            state.composerCallbacks->onHotplugEvent(displayId, CONNECTED);
        });
    }

    void sendDisplayRemoved(int64_t displayId) {
        mState.withSharedLock([displayId](const auto& state) {
            using aidl::android::hardware::graphics::common::DisplayHotplugEvent::DISCONNECTED;
            state.composerCallbacks->onHotplugEvent(displayId, DISCONNECTED);
        });
    }

    void sendVSync(events::VSync event) {
        mState.withSharedLock([event](const auto& state) {
            state.composerCallbacks->onVsync(
                    event.displayId, event.expectedAt.time_since_epoch().count(),
                    std::chrono::duration_cast<std::chrono::duration<int32_t, std::nano>>(
                            event.expectedPeriod)
                            .count());
        });
    }

    // begin IComposerClient overrides
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)

    auto registerCallback(const std::shared_ptr<IComposerCallback>& in_callback)
            -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;

        mState.withExclusiveLock(
                [in_callback](auto& state) { state.composerCallbacks = in_callback; });

        const auto optDisplayIds = mClientCallbacks.onGetDisplayIds();
        CHECK(!!optDisplayIds);
        const auto& displayIds = *optDisplayIds;
        mState.withSharedLock([&displayIds](const auto& state) {
            using aidl::android::hardware::graphics::common::DisplayHotplugEvent::CONNECTED;
            for (auto displayId : displayIds) {
                state.composerCallbacks->onHotplugEvent(displayId, CONNECTED);
            }
        });

        return ndk::ScopedAStatus::ok();
    }

    auto executeCommands(const std::vector<DisplayCommand>& in_commands,
                         std::vector<CommandResultPayload>* out_results)
            -> ndk::ScopedAStatus override {
        LOG(VERBOSE) << __func__;

        ComposerServiceWriter writer;

        for (const auto& cmd : in_commands) {
            LOG(VERBOSE) << cmd.toString();

            // We expect cmd.presentOrValidateDisplay to be true and none of the other flags.
            if (cmd.validateDisplay || cmd.acceptDisplayChanges || cmd.presentDisplay ||
                !cmd.presentOrValidateDisplay) {
                // Assert since this is an unexpected command.
                return unsupportedFatal(
                        fmt::format("{}: Unexpected command. cmd={}.", __func__, cmd.toString()));
            }

            writer.setPresentOrValidateResult(cmd.display, PresentOrValidate::Result::Presented);
        }

        *out_results = writer.getPendingCommandResults();

        return ndk::ScopedAStatus::ok();
    }

    auto setClientTargetSlotCount(int64_t in_display,
                                  int32_t in_clientTargetSlotCount) -> ndk::ScopedAStatus override {
        return ignore(fmt::format("Ignoring {} for display {} with count {}", __func__, in_display,
                                  in_clientTargetSlotCount));
    }

    auto setColorMode(int64_t in_display,
                      aidl::android::hardware::graphics::composer3::ColorMode in_mode,
                      aidl::android::hardware::graphics::composer3::RenderIntent in_intent)
            -> ndk::ScopedAStatus override {
        return ignore(fmt::format("Ignoring {} for display {} with mode {} and intent {}", __func__,
                                  in_display, toString(in_mode), toString(in_intent)));
    }

    auto setPowerMode(int64_t in_display,
                      aidl::android::hardware::graphics::composer3::PowerMode in_mode)
            -> ndk::ScopedAStatus override {
        return ignore(fmt::format("Ignoring {} for display {} with mode {}", __func__, in_display,
                                  toString(in_mode)));
    }

    auto setVsyncEnabled(int64_t in_display, bool in_enabled) -> ndk::ScopedAStatus override {
        return ignore(fmt::format("Ignoring {} for display {} with enabled {}", __func__,
                                  in_display, in_enabled));
    }

    auto getActiveConfig(int64_t in_display, int32_t* out_config) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;
        *out_config = displayConfig.activeConfig;
        return ndk::ScopedAStatus::ok();
    }

    auto getColorModes(int64_t in_display,
                       std::vector<aidl::android::hardware::graphics::composer3::ColorMode>*
                               out_modes) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        out_modes->clear();
        out_modes->reserve(displayConfig.renderIntentsForColorMode.size());
        for (const auto& [mode, _] : displayConfig.renderIntentsForColorMode) {
            out_modes->push_back(mode);
        }
        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayCapabilities(
            int64_t in_display,
            std::vector<aidl::android::hardware::graphics::composer3::DisplayCapability>*
                    out_capabilities) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        *out_capabilities = displayConfig.capabilities;
        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayConfigs(int64_t in_display,
                           std::vector<int32_t>* out_configs) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        out_configs->clear();
        out_configs->reserve(displayConfig.displayConfigs.size());
        for (const auto& mode : displayConfig.displayConfigs) {
            out_configs->push_back(mode.configId);
        }
        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayConnectionType(
            int64_t in_display,
            aidl::android::hardware::graphics::composer3::DisplayConnectionType* out_type)
            -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;
        *out_type = displayConfig.connectionType;
        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayIdentificationData(
            int64_t in_display,
            aidl::android::hardware::graphics::composer3::DisplayIdentification* out_identification)
            -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;
        *out_identification = displayConfig.identification;
        return ndk::ScopedAStatus::ok();
    }
    auto getDisplayVsyncPeriod(int64_t in_display,
                               int32_t* out_period) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        const auto found = std::ranges::find_if(
                displayConfig.displayConfigs, [&displayConfig](const auto& config) {
                    return config.configId == displayConfig.activeConfig;
                });

        CHECK(found != displayConfig.displayConfigs.end())
                << "getDisplayVsyncPeriod internal error. Unable to find config for "
                   "activeConfig.";

        *out_period = found->vsyncPeriod;

        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayPhysicalOrientation(int64_t in_display,
                                       aidl::android::hardware::graphics::common::Transform*
                                               out_orientation) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        *out_orientation = displayConfig.transform;
        return ndk::ScopedAStatus::ok();
    }

    auto getHdrCapabilities(int64_t in_display,
                            aidl::android::hardware::graphics::composer3::HdrCapabilities*
                                    out_capabilities) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        *out_capabilities = displayConfig.hdrCapabilities;
        return ndk::ScopedAStatus::ok();
    }

    auto getPerFrameMetadataKeys(
            int64_t in_display,
            std::vector<aidl::android::hardware::graphics::composer3::PerFrameMetadataKey>*
                    out_metadataKeys) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        *out_metadataKeys = displayConfig.perFrameMetadataKeys;
        return ndk::ScopedAStatus::ok();
    }

    auto getRenderIntents(int64_t in_display,
                          aidl::android::hardware::graphics::composer3::ColorMode in_mode,
                          std::vector<aidl::android::hardware::graphics::composer3::RenderIntent>*
                                  out_intents) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        if (auto found = displayConfig.renderIntentsForColorMode.find(in_mode);
            found != displayConfig.renderIntentsForColorMode.end()) {
            *out_intents = found->second;
            return ndk::ScopedAStatus::ok();
        }
        return ndk::ScopedAStatus::fromServiceSpecificError(ClientImpl::EX_BAD_PARAMETER);
    }

    auto getSupportedContentTypes(
            int64_t in_display,
            std::vector<aidl::android::hardware::graphics::composer3::ContentType>* out_types)
            -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        *out_types = displayConfig.supportedContentTypes;
        return ndk::ScopedAStatus::ok();
    }

    auto getDisplayConfigurations(
            int64_t in_display, int32_t in_maxFrameIntervalNs,
            std::vector<aidl::android::hardware::graphics::composer3::DisplayConfiguration>*
                    out_configurations) -> ndk::ScopedAStatus override {
        auto result = lookupDisplay(in_display);
        if (!result) {
            return std::move(result.error());
        }
        const auto& displayConfig = *result;

        const auto maxFrameInterval = std::chrono::nanoseconds(in_maxFrameIntervalNs);
        out_configurations->clear();
        out_configurations->reserve(displayConfig.displayConfigs.size());
        for (const auto& mode : displayConfig.displayConfigs) {
            if (mode.vsyncPeriod <= maxFrameInterval.count()) {
                out_configurations->push_back(mode);
            }
        }

        return ndk::ScopedAStatus::ok();
    }

    auto getOverlaySupport(aidl::android::hardware::graphics::composer3::OverlayProperties*
                                   out_properties) -> ndk::ScopedAStatus override {
        ftl::ignore(out_properties);
        return unsupported();
    }

    auto getHdrConversionCapabilities(
            std::vector<aidl::android::hardware::graphics::common::HdrConversionCapability>*
                    out_capabilities) -> ndk::ScopedAStatus override {
        ftl::ignore(out_capabilities);
        return unsupported();
    }

    // NOLINTEND(bugprone-easily-swappable-parameters)
    // end IComposerClient overrides

  private:
    [[nodiscard]] auto lookupDisplay(
            int64_t display, std::source_location location = std::source_location::current()) const
            -> base::expected<FakeDisplayConfiguration, ndk::ScopedAStatus> {
        // Obtains the display configuration from the FakeComposerImpl instance.
        LOG(VERBOSE) << terseFunctionName(location.function_name()) << " looking up display "
                     << display;
        const auto optOptDisplayConfig = mClientCallbacks.onGetDisplay(display);

        // The first optional is nullopt when the callback isn't connected.
        CHECK(!!optOptDisplayConfig) << terseFunctionName(location.function_name())
                                     << ": Display lookup failed - no composer for client.";

        // The second optional is nullopt when the display id is invalid.
        const auto& optDisplayConfig = *optOptDisplayConfig;
        if (!optDisplayConfig) {
            LOG(INFO) << terseFunctionName(location.function_name())
                      << " was invoked with an unrecognized display id " << display << ".";
            return base::unexpected(
                    ndk::ScopedAStatus::fromServiceSpecificError(ClientImpl::EX_BAD_DISPLAY));
        }
        return *optDisplayConfig;
    }

    struct State {
        std::shared_ptr<IComposerCallback> composerCallbacks;
    };
    core::GuardedSharedState<State> mState;
    Callbacks mClientCallbacks;
};

struct FakeComposer::ComposerImpl final : public DefaultComposerImpl {
    auto init() -> base::expected<void, std::string> {
        auto displayVsyncEventServiceResult = DisplayVSyncEventService::make();
        if (!displayVsyncEventServiceResult) {
            return base::unexpected(std::move(displayVsyncEventServiceResult).error());
        }

        mDisplayVSyncEventService = *std::move(displayVsyncEventServiceResult);
        mDisplayVSyncEventService->editCallbacks().onVSync.set(
                [this](auto event) { onVsyncEventGenerated(event); })();

        return {};
    }

    auto getComposer() -> std::shared_ptr<IComposer> { return ref<ComposerImpl>(); }

    void addDisplay(const core::DisplayConfiguration& display) {
        auto client = mState.withExclusiveLock([&display](auto& state) {
            if (!state.initialDisplayId) {
                state.initialDisplayId = display.id;
            }

            const std::string kMagicPrefix = "fake";
            const std::string encodedId = fmt::format("{}{}", kMagicPrefix, display.id);
            CHECK(encodedId.size() < core::EdidBuilder::kMaxProductNameStringLength);

            const auto timing = core::EdidBuilder::DigitalSeparateDetailedTimingDescriptor::
                    k1920x1080x60HzStandard;
            const auto edid = core::EdidBuilder()
                                      .set({.preferred = {.timing = timing}})
                                      .addDisplayProductNameStringDescriptor(encodedId)
                                      .build();

            state.configuredDisplays[display.id] =
                    hwc3::FakeDisplayConfiguration::Builder().setIdentification({
                            .port = 0,
                            .data = {edid.begin(), edid.end()},
                    })();

            return state.client;
        });

        if (client) {
            client->sendDisplayAdded(display.id);
        }
    }

    void removeDisplay(core::DisplayConfiguration::Id displayId) {
        mDisplayVSyncEventService->removeDisplay(displayId);

        auto client = mState.withExclusiveLock([displayId](auto& state) {
            if (state.initialDisplayId == displayId) {
                state.initialDisplayId.reset();
            }
            state.configuredDisplays.erase(displayId);
            return state.client;
        });

        if (client) {
            client->sendDisplayRemoved(displayId);
        }
    }

  private:
    // begin IComposer overrides

    auto dump(int dumpFd, const char** args, uint32_t num_args) -> binder_status_t override {
        ftl::ignore(dumpFd, args, num_args);
        ftl::ignore = unsupported();
        return STATUS_INVALID_OPERATION;
    }

    auto createClient(std::shared_ptr<IComposerClient>* out_client) -> ndk::ScopedAStatus override {
        const bool expired =
                mState.withSharedLock([](auto& state) { return state.client == nullptr; });

        if (!expired) {
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IComposer::EX_NO_RESOURCES, "Client already created and valid");
        }

        auto client = ndk::SharedRefBase::make<ClientImpl>();
        if (!client) {
            LOG(ERROR) << "Failed to allocate a FakeClient instance.";
            return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(
                    IComposer::EX_NO_RESOURCES, "Client failed to initialize");
        }

        auto& callbacks = client->editCallbacks();
        using Self = ComposerImpl;
        callbacks.onGetDisplay.set(std::bind_front(&Self::getDisplay, this))();
        callbacks.onGetDisplayIds.set(std::bind_front(&Self::getDisplayIds, this))();

        *out_client = client->getComposerClient();
        mState.withExclusiveLock(
                [client = std::move(client)](auto& state) { state.client = client; });

        return ndk::ScopedAStatus::ok();
    }

    auto getCapabilities(std::vector<Capability>* out_capabilities) -> ndk::ScopedAStatus override {
        out_capabilities->assign({
                Capability::PRESENT_FENCE_IS_NOT_RELIABLE,
                Capability::LAYER_LIFECYCLE_BATCH_COMMAND,
        });
        return ndk::ScopedAStatus::ok();
    }

    // end IComposer overrides

    auto getDisplay(int64_t displayId) const -> std::optional<FakeDisplayConfiguration> {
        return mState.withSharedLock([displayId](auto& state) {
            const auto found = state.configuredDisplays.find(displayId);
            return (found != state.configuredDisplays.end()) ? std::make_optional(found->second)
                                                             : std::nullopt;
        });
    }

    auto getDisplayIds() const -> std::vector<int64_t> {
        return mState.withSharedLock([&](auto& state) {
            std::vector<int64_t> ids;
            ids.reserve(state.configuredDisplays.size());
            if (state.initialDisplayId) {
                ids.push_back(*state.initialDisplayId);
            }
            for (const auto& [id, config] : state.configuredDisplays) {
                if (id != state.initialDisplayId) {
                    ids.push_back(id);
                }
            }
            return ids;
        });
    }

    void onVsyncEventGenerated(events::VSync event) {
        if (auto client = mState.withSharedLock([](const auto& state) { return state.client; })) {
            client->sendVSync(event);
        }
    }

    void onVsyncEnabledChanged(int64_t displayId, bool enable) const {
        LOG(VERBOSE) << __func__;
        mState.withSharedLock([this, displayId, enable](const auto& state) {
            LOG(INFO) << "displayId: " << displayId << " enable: " << enable;
            if (!enable) {
                mDisplayVSyncEventService->removeDisplay(displayId);
                return;
            }

            auto found = state.configuredDisplays.find(displayId);
            if (found == state.configuredDisplays.end()) {
                LOG(WARNING) << "No display " << displayId << " to configure vsync generation.";
                return;
            }
            const auto& display = found->second;
            auto foundConfig = std::ranges::find_if(
                    display.displayConfigs, [configId = display.activeConfig](const auto& config) {
                        return config.configId == configId;
                    });
            if (foundConfig == display.displayConfigs.end()) {
                LOG(WARNING) << "No config " << display.activeConfig << " for display " << displayId
                             << " to configure vsync generation.";
                return;
            }
            const auto& config = *foundConfig;

            using TimePoint = std::chrono::steady_clock::time_point;
            using Duration = std::chrono::steady_clock::duration;

            LOG(WARNING) << "Assuming a base of zero for vsync timing!";
            const TimePoint base{Duration(0)};
            const auto period = Duration(config.vsyncPeriod);
            mDisplayVSyncEventService->addDisplay(
                    displayId, SingleDisplayRefreshSchedule{.base = base, .period = period});
        });
    }

    std::shared_ptr<DisplayVSyncEventService> mDisplayVSyncEventService;

    struct State {
        std::shared_ptr<ClientImpl> client;
        std::optional<int64_t> initialDisplayId;
        std::unordered_map<int64_t, FakeDisplayConfiguration> configuredDisplays;
    };
    core::GuardedSharedState<State> mState;
};

struct FakeComposer::Passkey final {};

auto FakeComposer::getServiceName(std::string_view baseServiceName) -> std::string {
    return ComposerImpl::makeServiceName(baseServiceName);
}

auto FakeComposer::make() -> base::expected<std::shared_ptr<FakeComposer>, std::string> {
    using namespace std::string_literals;

    auto composer = std::make_shared<FakeComposer>(Passkey{});
    if (composer == nullptr) {
        return base::unexpected("Failed to construct the FakeComposer instance."s);
    }

    if (auto result = composer->init(); !result) {
        return base::unexpected("Failed to init the FakeComposer instance: "s + result.error());
    }

    return composer;
}

FakeComposer::FakeComposer(FakeComposer::Passkey passkey) {
    ftl::ignore(passkey);
}

auto FakeComposer::init() -> base::expected<void, std::string> {
    using namespace std::string_literals;

    auto impl = ndk::SharedRefBase::make<ComposerImpl>();
    if (!impl) {
        return base::unexpected("Failed to construct the ComposerImpl instance."s);
    }

    if (auto result = impl->init(); !result) {
        return base::unexpected("Failed to init the ComposerImpl instance: "s + result.error());
    }

    mImpl = std::move(impl);

    return {};
}

auto FakeComposer::getComposer() -> std::shared_ptr<IComposer> {
    CHECK(mImpl != nullptr);
    return mImpl;
}

void FakeComposer::addDisplay(const core::DisplayConfiguration& display) {
    CHECK(mImpl != nullptr);
    mImpl->addDisplay(display);
}

void FakeComposer::removeDisplay(core::DisplayConfiguration::Id displayId) {
    CHECK(mImpl != nullptr);
    mImpl->removeDisplay(displayId);
}

}  // namespace android::surfaceflinger::tests::end2end::test_framework::hwc3
