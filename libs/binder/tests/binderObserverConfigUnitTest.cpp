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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <codecvt>
#include <locale>
#include <map>

#include "../BinderObserverConfig.h"

namespace android {

using ::testing::_;
using ::testing::Return;

class BinderObserverConfigTest : public ::testing::Test {
protected:
    class MockEnvironment : public BinderObserverConfig::Environment {
    public:
        MOCK_METHOD(std::string, readFileLine, (const std::string& path), (override));
        MOCK_METHOD(uid_t, getUid, (), (override));
        MOCK_METHOD(std::string, getProcessName, (), (override));
        MOCK_METHOD(BinderObserverConfig::ShardingConfig, getSystemServerSharding, (), (override));
        MOCK_METHOD(BinderObserverConfig::ShardingConfig, getOtherProcessesSharding, (),
                    (override));
        MOCK_METHOD(size_t, hashString8, (const std::string& content), (override));
        MOCK_METHOD(size_t, hashString16, (const std::u16string_view& content), (override));
    };

    const std::string kBootIdPath = "/proc/sys/kernel/random/boot_id";
    const std::string kProcessOffsetToken = "16e12b27-2a84-4355"; // used for process offset
    const std::string kAidlOffsetToken = "-84cd-948348d6c998";    // used for aidl offset
    const std::string kOtherProcessName = "some_other_process";   // used for aidl offset

    static constexpr BinderObserverConfig::ShardingConfig kMonitorEverythingSharding = {
            .processMod = 1,
            .spamMod = 1,
            .callMod = 1,
    };

    static constexpr BinderObserverConfig::ShardingConfig kSometimesEverythingSharding = {
            .processMod = 2,
            .spamMod = 1,
            .callMod = 1,
    };

    static constexpr BinderObserverConfig::ShardingConfig kModerateSharding = {
            .processMod = 55,
            .spamMod = 11,
            .callMod = 22,
    };

    static constexpr BinderObserverConfig::ShardingConfig kOnlySpamSharding = {
            .processMod = 100,
            .spamMod = 33,
            .callMod = 0,
    };

    static constexpr BinderObserverConfig::ShardingConfig kMonitorNothingSharding = {
            .processMod = 0,
            .spamMod = 0,
            .callMod = 0,
    };

    static std::unique_ptr<BinderObserverConfig> createConfig(
            BinderObserverConfig::Environment* environment) {
        return BinderObserverConfig::createConfig(
                std::unique_ptr<BinderObserverConfig::Environment>(environment));
    }

    void SetUp() override {
        // Note: this should be owned (and destroyed) by the config class under test.
        mEnv = new MockEnvironment();
        ON_CALL(*mEnv, readFileLine(kBootIdPath))
                .WillByDefault(Return(kProcessOffsetToken + kAidlOffsetToken));
        ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(5678));
        ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(6789));
    }

    // Configure mEnv so that the created config will be enabled/disabled (if possible)
    void setUpForSharding(BinderObserverConfig::ShardingConfig sharding, bool enabled = true) {
        EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123));
        EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("other_process"));
        ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));

        // Make the process hash such that the total is divisible by processMod.
        size_t processHash = sharding.processMod != 0
                ? sharding.processMod - ((123 + 234) % sharding.processMod)
                : 345; // impossible, will be disabled
        if (!enabled) {
            // If we don't want it to be enabled, change it so that the mod is not zero
            ++processHash;
        }

        ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(processHash));

        EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(sharding));
    }

    MockEnvironment* mEnv;
};

TEST_F(BinderObserverConfigTest, CreateConfigOtherProcessMonitorEverything) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("other_process"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(345)); // process hash
    EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(kMonitorEverythingSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // Since processMod 1 it should be enabled
    EXPECT_TRUE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigOtherProcessMonitorNothing) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("other_process"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(345)); // process hash
    EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(kMonitorNothingSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // Since processMod is 0 it should be disabled
    EXPECT_FALSE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigOtherProcessEnabledByHash) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("other_process"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(303)); // process hash
    EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(kModerateSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // the resulting token should be (123 + 234 + 303) % 55 = 0, so enabled
    EXPECT_TRUE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigOtherProcessDisabledByHash) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("other_process"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(345)); // process hash
    EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(kModerateSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // the resulting token should be (123 + 234 + 345) % 55 = 42, so not enabled
    EXPECT_FALSE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigSystemServerEnabledByHash) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(1000));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("system_server"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("system_server")).WillByDefault(Return(306)); // process hash
    EXPECT_CALL(*mEnv, getSystemServerSharding()).WillOnce(Return(kModerateSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // the resulting token should be (1000 + 234 + 306) % 55 = 0, so enabled
    EXPECT_TRUE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigSystemServerDisabledByHash) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(1000));
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("system_server"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("system_server")).WillByDefault(Return(345)); // process hash
    EXPECT_CALL(*mEnv, getSystemServerSharding()).WillOnce(Return(kModerateSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // the resulting token should be (1000 + 234 + 345) % 55 = 39, so not enabled
    EXPECT_FALSE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, CreateConfigFakeSystemServer) {
    EXPECT_CALL(*mEnv, getUid()).WillOnce(Return(123)); // not the right uid
    EXPECT_CALL(*mEnv, getProcessName()).WillOnce(Return("system_server"));
    ON_CALL(*mEnv, hashString8(kProcessOffsetToken)).WillByDefault(Return(234));
    ON_CALL(*mEnv, hashString8("other_process")).WillByDefault(Return(345)); // process hash
    ON_CALL(*mEnv, getSystemServerSharding()).WillByDefault(Return(kMonitorEverythingSharding));
    EXPECT_CALL(*mEnv, getOtherProcessesSharding()).WillOnce(Return(kMonitorNothingSharding));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // Not enabled because it is not the real system_server.
    EXPECT_FALSE(config->isEnabled());
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoModerateShardingNotTracked) {
    setUpForSharding(kModerateSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2345));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have ensured we have enabled config.
    EXPECT_TRUE(config->isEnabled());

    // spam: (1234 + 2345 + 21) % 11 = 3
    // calls: (1234 + 2345 + 21) % 22 = 14

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = false, .trackLatency = false};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoModerateShardingTrackSpamOnly) {
    setUpForSharding(kModerateSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2342));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have ensured we have enabled config.
    EXPECT_TRUE(config->isEnabled());

    // spam: (1234 + 2342 + 21) % 11 = 0
    // calls: (1234 + 2342 + 21) % 22 = 11
    BinderObserverConfig::TrackingInfo expected = {.trackSpam = true, .trackLatency = false};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoModerateShardingTrackSpamAndCalls) {
    setUpForSharding(kModerateSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2331));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have ensured we have enabled config.
    EXPECT_TRUE(config->isEnabled());

    // spam: (1234 + 2331 + 21) % 11 = 0
    // calls: (1234 + 2331 + 21) % 22 = 0

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = true, .trackLatency = true};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoMonitorEverythingShardingTrackSpamAndCalls) {
    setUpForSharding(kMonitorEverythingSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2345));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have ensured we have enabled config.
    EXPECT_TRUE(config->isEnabled());

    // The numbers should not matter, everything should be monitored

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = true, .trackLatency = true};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoMonitorNothingShardingTrackNothing) {
    setUpForSharding(kMonitorNothingSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2345));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have ensured we have disiabled config.
    EXPECT_FALSE(config->isEnabled());

    // The numbers should not matter, nothing should be monitored

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = false, .trackLatency = false};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoSometimesEverythingShardingTrackNothing) {
    setUpForSharding(kSometimesEverythingSharding, false);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2345));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have created a disabled config
    EXPECT_FALSE(config->isEnabled());

    // The numbers should not matter, nothing should be tracked because we are disabled

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = false, .trackLatency = false};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

TEST_F(BinderObserverConfigTest, GetTrackingInfoOnlySpamShardingTrackSpam) {
    setUpForSharding(kOnlySpamSharding);

    ON_CALL(*mEnv, hashString8(kAidlOffsetToken)).WillByDefault(Return(1234));
    ON_CALL(*mEnv, hashString16(std::u16string_view(u"IContentProvider")))
            .WillByDefault(Return(2342));

    std::unique_ptr<BinderObserverConfig> config = createConfig(mEnv);

    // setUpForSharding should have created an enabled config
    EXPECT_TRUE(config->isEnabled());

    // spam: (1234 + 2342 + 21) % 33 = 0
    // calls: the numbers don't matter since callMod is 0

    BinderObserverConfig::TrackingInfo expected = {.trackSpam = true, .trackLatency = false};
    EXPECT_EQ(config->getTrackingInfo(u"IContentProvider", 21), expected);
}

} // namespace android
