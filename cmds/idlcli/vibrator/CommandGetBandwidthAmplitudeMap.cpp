/*
 * Copyright (C) 2021 The Android Open Source Project *
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

class CommandGetBandwidthAmplitudeMap : public Command {
    std::string getDescription() const override {
        return "Retrieves vibrator bandwidth amplitude map.";
    }

    std::string getUsageSummary() const override { return ""; }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{};
        return details;
    }

    Status doArgs(Args &args) override {
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

        std::vector<float> bandwidthAmplitude;
        float frequencyMinimumHz;
        float frequencyResolutionHz;

        auto status = hal->getBandwidthAmplitudeMap(&bandwidthAmplitude);

        if (!status.isOk()) {
            std::cout << "Status: " << status.getDescription() << std::endl;
            return ERROR;
        }

        status = hal->getFrequencyMinimum(&frequencyMinimumHz);

        if (!status.isOk()) {
            std::cout << "Status: " << status.getDescription() << std::endl;
            return ERROR;
        }

        status = hal->getFrequencyResolution(&frequencyResolutionHz);

        if (!status.isOk()) {
            std::cout << "Status: " << status.getDescription() << std::endl;
            return ERROR;
        }

        std::cout << "Status: " << status.getDescription() << std::endl;
        std::cout << "Bandwidth Amplitude Map: " << std::endl;
        float frequency = frequencyMinimumHz;
        for (auto &e : bandwidthAmplitude) {
            std::cout << frequency << ":" << e << std::endl;
            frequency += frequencyResolutionHz;
        }

        return OK;
    }
};

static const auto Command =
    CommandRegistry<CommandVibrator>::Register<CommandGetBandwidthAmplitudeMap>(
        "getBandwidthAmplitudeMap");

}  // namespace vibrator
}  // namespace idlcli
}  // namespace android
