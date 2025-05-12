/*
 * Copyright (C) 2024 The Android Open Source Project *
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

#include <stdlib.h>

#include <charconv>

#include "utils.h"
#include "vibrator.h"

namespace android {
namespace idlcli {

class CommandVibrator;

namespace vibrator {

using aidl::CompositePwleV2;
using aidl::PwleV2Primitive;

class CommandComposePwleV2 : public Command {
    std::string getDescription() const override { return "Compose normalized PWLE vibration."; }

    std::string getUsageSummary() const override {
        return "[options] <time> <frequency> <amplitude> ...";
    }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{
                {"-b", {"Block for duration of vibration."}},
                {"<time>", {"Segment duration in milliseconds"}},
                {"<frequency>", {"Target frequency in Hz"}},
                {"<amplitude>", {"Target amplitude in [0.0, 1.0]"}},
                {"...", {"May repeat multiple times."}},
        };
        return details;
    }

    Status doArgs(Args& args) override {
        while (args.get<std::string>().value_or("").find("-") == 0) {
            auto opt = *args.pop<std::string>();
            if (opt == "--") {
                break;
            } else if (opt == "-b") {
                mBlocking = true;
            } else {
                std::cerr << "Invalid Option '" << opt << "'!" << std::endl;
                return USAGE;
            }
        }

        if (args.empty()) {
            std::cerr << "Missing arguments! Please see usage" << std::endl;
            return USAGE;
        }

        while (!args.empty()) {
            PwleV2Primitive segment;

            if (auto timeMs = args.pop<decltype(segment.timeMillis)>();
                timeMs && *timeMs >= 0 && *timeMs <= 0x7ffff) {
                segment.timeMillis = *timeMs;
                std::cout << "Time: " << segment.timeMillis << std::endl;
            } else {
                std::cerr << "Missing or Invalid Time!" << std::endl;
                return USAGE;
            }

            if (auto frequencyHz = args.pop<decltype(segment.frequencyHz)>();
                frequencyHz && *frequencyHz >= 30 && *frequencyHz <= 300) {
                segment.frequencyHz = *frequencyHz;
                std::cout << "Frequency: " << segment.frequencyHz << std::endl;
            } else {
                std::cerr << "Missing or Invalid Frequency!" << std::endl;
                return USAGE;
            }

            if (auto amplitude = args.pop<decltype(segment.amplitude)>();
                amplitude && *amplitude >= 0 && *amplitude <= 1.0) {
                segment.amplitude = *amplitude;
                std::cout << "Amplitude: " << segment.amplitude << std::endl;
            } else {
                std::cerr << "Missing or Invalid Amplitude!" << std::endl;
                return USAGE;
            }

            mCompositePwle.pwlePrimitives.emplace_back(std::move(segment));
        }

        if (!args.empty()) {
            std::cerr << "Unexpected Arguments!" << std::endl;
            return USAGE;
        }

        return OK;
    }

    Status doMain(Args&& /*args*/) override {
        auto hal = getHal<aidl::IVibrator>();

        if (!hal) {
            return UNAVAILABLE;
        }

        ABinderProcess_setThreadPoolMaxThreadCount(1);
        ABinderProcess_startThreadPool();

        std::shared_ptr<VibratorCallback> callback;

        if (mBlocking) {
            callback = ndk::SharedRefBase::make<VibratorCallback>();
        }

        auto status = hal->call(&aidl::IVibrator::composePwleV2, mCompositePwle, callback);

        if (status.isOk() && callback) {
            callback->waitForComplete();
        }

        std::cout << "Status: " << status.getDescription() << std::endl;

        return status.isOk() ? OK : ERROR;
    }

    bool mBlocking;
    CompositePwleV2 mCompositePwle;
};

static const auto Command =
        CommandRegistry<CommandVibrator>::Register<CommandComposePwleV2>("composePwleV2");

} // namespace vibrator
} // namespace idlcli
} // namespace android
