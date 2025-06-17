/*
 * Copyright (C) 2020 The Android Open Source Project *
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

#include <future>

#include "utils.h"
#include "vibrator.h"

namespace android {
namespace idlcli {

class CommandVibrator;

namespace vibrator {

using aidl::CompositePrimitive;

class CommandGetPrimitiveDuration : public Command {
    std::string getDescription() const override {
        return "Retrieve effect primitive's duration in milliseconds.";
    }

    std::string getUsageSummary() const override { return "<primitive>"; }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{
                {"<primitive>", {"Primitive ID."}},
        };
        return details;
    }

    Status doArgs(Args &args) override {
        if (auto primitive = args.pop<decltype(mPrimitive)>()) {
            mPrimitive = *primitive;
            std::cout << "Primitive: " << toString(mPrimitive) << std::endl;
        } else {
            std::cerr << "Missing or Invalid Primitive!" << std::endl;
            return USAGE;
        }
        if (!args.empty()) {
            std::cerr << "Unexpected Arguments!" << std::endl;
            return USAGE;
        }
        return OK;
    }

    Status doMain(Args && /*args*/) override {
        auto hal = getHal();

        if (!hal) {
            return UNAVAILABLE;
        }

        int32_t duration;
        auto status = hal->getPrimitiveDuration(mPrimitive, &duration);

        std::cout << "Status: " << status.getDescription() << std::endl;
        std::cout << "Duration: " << duration << std::endl;

        return status.isOk() ? OK : ERROR;
    }

    CompositePrimitive mPrimitive;
};

static const auto Command = CommandRegistry<CommandVibrator>::Register<CommandGetPrimitiveDuration>(
        "getPrimitiveDuration");

} // namespace vibrator
} // namespace idlcli
} // namespace android
