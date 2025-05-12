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

#include <thread>

#include "utils.h"
#include "vibrator.h"

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

namespace android {
namespace idlcli {

class CommandVibrator;

namespace vibrator {

using aidl::VendorEffect;

class CommandPerformVendorEffect : public Command {
    std::string getDescription() const override { return "Perform vendor vibration effect."; }

    std::string getUsageSummary() const override { return "[options] <none>"; }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{
                {"-b", {"Block for duration of vibration."}},
                {"<none>", {"No valid input."}},
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

        return OK;
    }

    Status doMain(Args&& /*args*/) override { return UNAVAILABLE; }

    bool mBlocking;
    VendorEffect mEffect;
};

static const auto Command = CommandRegistry<CommandVibrator>::Register<CommandPerformVendorEffect>(
        "performVendorEffect");

} // namespace vibrator
} // namespace idlcli
} // namespace android
