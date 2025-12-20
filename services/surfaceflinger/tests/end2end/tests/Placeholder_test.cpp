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

#include <chrono>
#include <cstddef>
#include <memory>
#include <thread>
#include <utility>

#include <android-base/logging.h>
#include <fmt/format.h>
#include <gtest/gtest.h>

#include "test_framework/core/TestService.h"
#include "test_framework/hwc3/Hwc3Controller.h"
#include "test_framework/hwc3/ObservingComposer.h"
#include "test_framework/hwc3/events/BufferPendingDisplay.h"
#include "test_framework/hwc3/events/BufferPendingRelease.h"
#include "test_framework/hwc3/events/DisplayPresented.h"
#include "test_framework/hwc3/events/VSyncEnabled.h"
#include "test_framework/surfaceflinger/DisplayEventReceiver.h"
#include "test_framework/surfaceflinger/SFController.h"
#include "test_framework/surfaceflinger/Surface.h"
#include "test_framework/surfaceflinger/events/BufferReleased.h"
#include "test_framework/surfaceflinger/events/Hotplug.h"
#include "test_framework/surfaceflinger/events/TransactionCommitted.h"
#include "test_framework/surfaceflinger/events/TransactionCompleted.h"
#include "test_framework/surfaceflinger/events/TransactionInitiated.h"
#include "test_framework/surfaceflinger/events/VSyncTiming.h"

#define TRY_OR_ASSERT(expr)                       \
    ({                                            \
        auto result_ = (expr);                    \
        if (!result_) {                           \
            FAIL() << std::move(result_).error(); \
            return;                               \
        }                                         \
        *std::move(result_);                      \
    })

namespace android::surfaceflinger::tests::end2end {
namespace {

using Timestamp = std::chrono::steady_clock::time_point;

struct Placeholder : public ::testing::Test {};

TEST_F(Placeholder, Bringup) {
    auto serviceResult = test_framework::core::TestService::startWithDisplays({
            {.id = 123},
    });
    if (!serviceResult) {
        LOG(WARNING) << "End2End service not available. " << serviceResult.error();
        GTEST_SKIP() << "End2End service not available. " << serviceResult.error();
    }

    auto service = *std::move(serviceResult);

    service->hwc().editCallbacks().onVsyncEnabledChanged.set(
            [&](test_framework::hwc3::events::VSyncEnabled event) {
                LOG(INFO) << fmt::format("onVsyncEnabledChanged {}", event);
            })();
    service->hwc().editCallbacks().onDisplayPresented.set(
            [&](test_framework::hwc3::events::DisplayPresented event) {
                LOG(INFO) << fmt::format("onDisplayPresented {}", event);
            })();
    service->hwc().editCallbacks().onBufferPendingDisplay.set(
            [&](test_framework::hwc3::events::BufferPendingDisplay event) {
                LOG(INFO) << fmt::format("onBufferPendingDisplay {}", event);
            })();

    service->hwc().editCallbacks().onBufferPendingRelease.set(
            [&](test_framework::hwc3::events::BufferPendingRelease event) {
                LOG(INFO) << fmt::format("onPendingBufferSwap {}", event);
            })();

    auto displayEvents = TRY_OR_ASSERT(service->flinger().makeDisplayEventReceiver());
    displayEvents->setVsyncRate(1);

    displayEvents->editCallbacks().onHotplug.set(
            [](test_framework::surfaceflinger::events::Hotplug event) {
                LOG(INFO) << fmt::format("onHotplug {}", event);
            })();
    size_t vsyncCount = 0;
    displayEvents->editCallbacks().onVSyncTiming.set(
            [&](test_framework::surfaceflinger::events::VSyncTiming event) {
                ++vsyncCount;
                LOG(INFO) << fmt::format("onVSyncTiming {}", event);
            })();

    auto surface =
            TRY_OR_ASSERT(service->flinger().makeSurface({.name = "test", .size = {100, 100}}));

    size_t bufferReleaseCount = 0;
    surface->editCallbacks().onBufferReleased.set(
            [&](test_framework::surfaceflinger::events::BufferReleased event) {
                ++bufferReleaseCount;
                LOG(INFO) << fmt::format("onBufferReleased {}", event);
            })();

    size_t transactionInitiatedCount = 0;
    surface->editCallbacks().onTransactionInitiated.set(
            [&](test_framework::surfaceflinger::events::TransactionInitiated event) {
                ++transactionInitiatedCount;
                LOG(INFO) << fmt::format("onTransactionInitiated {}", event);
            })();
    size_t transactionCommitCount = 0;
    surface->editCallbacks().onTransactionCommitted.set(
            [&](test_framework::surfaceflinger::events::TransactionCommitted event) {
                ++transactionCommitCount;
                LOG(INFO) << fmt::format("onTransactionCommitted {}", event);
                // Commit a new frame now that the previous one has completed.
                // This is ONE possible way an app can continue to submit new frames.
                auto nextFrameNumber = surface->commitNextBuffer();
            })();
    size_t transactionCompleteCount = 0;
    surface->editCallbacks().onTransactionCompleted.set(
            [&](test_framework::surfaceflinger::events::TransactionCompleted event) {
                ++transactionCompleteCount;
                LOG(INFO) << fmt::format("onTransactionCompleted {}", event);
            })();

    // Trigger the first commit to get things started.
    auto firstFrameNumber = surface->commitNextBuffer();
    LOG(INFO) << "firstFrameNumber " << firstFrameNumber;

    // This sleep should allow multiple frames to be generated, though we don't know how many.
    constexpr auto kSleepTime = std::chrono::milliseconds(100);
    LOG(INFO) << "Waiting for " << kSleepTime << "....";
    std::this_thread::sleep_for(kSleepTime);
    LOG(INFO) << "Done waiting for " << kSleepTime << "....";

    EXPECT_GT(vsyncCount, 0) << "Expected at least one vsync timing callback. Zero observed.";
    EXPECT_GT(bufferReleaseCount, 0)
            << "Expected at least one buffer release callback. Zero observed.";
    EXPECT_GT(transactionInitiatedCount, 0)
            << "Expected at least one transaction initiated callback. Zero observed.";
    EXPECT_GT(transactionCommitCount, 0)
            << "Expected at least one transaction commit callback. Zero observed.";
    EXPECT_GT(transactionCompleteCount, 0)
            << "Expected at least one transaction complete callback. Zero observed.";
}

}  // namespace
}  // namespace android::surfaceflinger::tests::end2end
