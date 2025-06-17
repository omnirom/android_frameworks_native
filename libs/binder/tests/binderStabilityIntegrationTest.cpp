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

#include <binder/Binder.h>
#include <binder/IServiceManager.h>
#include <binder/Stability.h>
#include <gtest/gtest.h>
#include <procpartition/procpartition.h>

using namespace android;
using android::internal::Stability; // for testing only!
using android::procpartition::getPartition;
using android::procpartition::Partition;

class BinderStabilityIntegrationTest : public testing::Test,
                                       public ::testing::WithParamInterface<String16> {
public:
    virtual ~BinderStabilityIntegrationTest() {}
};

TEST_P(BinderStabilityIntegrationTest, ExpectedStabilityForItsPartition) {
    const String16& serviceName = GetParam();

    sp<IBinder> binder = defaultServiceManager()->checkService(serviceName);
    if (!binder) GTEST_SKIP() << "Could not get service, may have gone away.";

    pid_t pid;
    status_t res = binder->getDebugPid(&pid);
    if (res != OK) {
        GTEST_SKIP() << "Could not talk to service to get PID, res: " << statusToString(res);
    }

    Partition partition = getPartition(pid);

    Stability::Level level = Stability::Level::UNDECLARED;
    switch (partition) {
        case Partition::PRODUCT:
        case Partition::SYSTEM:
        case Partition::SYSTEM_EXT:
            level = Stability::Level::SYSTEM;
            break;
        case Partition::VENDOR:
        case Partition::ODM:
            level = Stability::Level::VENDOR;
            break;
        case Partition::UNKNOWN:
            GTEST_SKIP() << "Not sure of partition of process.";
            return;
        default:
            ADD_FAILURE() << "Unrecognized partition for service: " << partition;
            return;
    }

    ASSERT_TRUE(Stability::check(Stability::getRepr(binder.get()), level))
            << "Binder hosted on partition " << partition
            << " should have corresponding stability set.";
}

std::string PrintTestParam(
        const testing::TestParamInfo<BinderStabilityIntegrationTest::ParamType>& info) {
    std::string name = String8(info.param).c_str();
    for (size_t i = 0; i < name.size(); i++) {
        bool alnum = false;
        alnum |= (name[i] >= 'a' && name[i] <= 'z');
        alnum |= (name[i] >= 'A' && name[i] <= 'Z');
        alnum |= (name[i] >= '0' && name[i] <= '9');
        alnum |= (name[i] == '_');
        if (!alnum) name[i] = '_';
    }

    // index for uniqueness
    return std::to_string(info.index) + "__" + name;
}

INSTANTIATE_TEST_CASE_P(RegisteredServices, BinderStabilityIntegrationTest,
                        ::testing::ValuesIn(defaultServiceManager()->listServices()),
                        PrintTestParam);
