/*
 * Copyright (C) 2019 The Android Open Source Project *
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

class CommandOn : public Command {
    std::string getDescription() const override { return "Turn on vibrator."; }

    std::string getUsageSummary() const override { return "[options] <duration>"; }

    UsageDetails getUsageDetails() const override {
        UsageDetails details{
                {"-b", {"Block for duration of vibration."}},
                {"<duration>", {"In milliseconds."}},
        };
        return details;
    }

    Status doArgs(Args &args) override {
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
        if (auto duration = args.pop<decltype(mDuration)>()) {
            mDuration = *duration;
        } else {
            std::cerr << "Missing or Invalid Duration!" << std::endl;
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

        std::shared_ptr<VibratorCallback> callback;

        ABinderProcess_setThreadPoolMaxThreadCount(1);
        ABinderProcess_startThreadPool();

        int32_t cap;
        hal->getCapabilities(&cap);

        if (mBlocking && (cap & aidl::IVibrator::CAP_ON_CALLBACK)) {
            callback = ndk::SharedRefBase::make<VibratorCallback>();
        }

        auto status = hal->on(mDuration, callback);

        if (status.isOk() && mBlocking) {
            if (callback) {
                callback->waitForComplete();
            } else {
                sleep_for(milliseconds(mDuration));
            }
        }

        std::cout << "Status: " << status.getDescription() << std::endl;

        return status.isOk() ? OK : ERROR;
    }

    bool mBlocking;
    uint32_t mDuration;
};

static const auto Command = CommandRegistry<CommandVibrator>::Register<CommandOn>("on");

} // namespace vibrator
} // namespace idlcli
} // namespace android
