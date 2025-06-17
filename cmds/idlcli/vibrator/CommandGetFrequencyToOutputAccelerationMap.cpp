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

#include "utils.h"
#include "vibrator.h"

namespace android {
namespace idlcli {

class CommandVibrator;

namespace vibrator {

using aidl::FrequencyAccelerationMapEntry;

class CommandGetFrequencyToOutputAccelerationMap : public Command {
    std::string getDescription() const override {
        return "Retrieves vibrator frequency to output acceleration map.";
    }

    std::string getUsageSummary() const override { return ""; }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{};
        return details;
    }

    Status doArgs(Args& args) override {
        if (!args.empty()) {
            std::cerr << "Unexpected Arguments!" << std::endl;
            return USAGE;
        }
        return OK;
    }

    Status doMain(Args&& /*args*/) override {
        auto hal = getHal();

        if (!hal) {
            return UNAVAILABLE;
        }

        std::vector<FrequencyAccelerationMapEntry> frequencyToOutputAccelerationMap;
        auto status = hal->getFrequencyToOutputAccelerationMap(&frequencyToOutputAccelerationMap);

        std::cout << "Status: " << status.getDescription() << std::endl;
        std::cout << "Frequency to Output Amplitude Map: " << std::endl;
        for (auto& entry : frequencyToOutputAccelerationMap) {
            std::cout << entry.frequencyHz << " " << entry.maxOutputAccelerationGs << std::endl;
        }

        return status.isOk() ? OK : ERROR;
    }
};

static const auto Command =
        CommandRegistry<CommandVibrator>::Register<CommandGetFrequencyToOutputAccelerationMap>(
                "getFrequencyToOutputAccelerationMap");

} // namespace vibrator
} // namespace idlcli
} // namespace android
