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

#undef LOG_TAG
#define LOG_TAG "gpuservice_unittest"

#include <android-base/file.h>
#include <log/log.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <com_android_graphics_graphicsenv_flags.h>
#include <feature_override/FeatureOverrideParser.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace android {
namespace {

std::string getTestBinarypbPath(const std::string &filename) {
    std::string path = android::base::GetExecutableDirectory();
    path.append("/");
    path.append(filename);

    return path;
}

class FeatureOverrideParserMock : public FeatureOverrideParser {
public:
    MOCK_METHOD(std::string, getFeatureOverrideFilePath, (), (const, override));
};

class FeatureOverrideParserTest : public testing::Test {
public:
    FeatureOverrideParserTest() {
        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("**** Setting up for %s.%s\n", test_info->test_case_name(), test_info->name());
    }

    ~FeatureOverrideParserTest() {
        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
        ALOGD("**** Tearing down after %s.%s\n", test_info->test_case_name(),
              test_info->name());
    }

    void SetUp() override {
        const std::string filename = "gpuservice_unittest_feature_config_vk.binarypb";

        EXPECT_CALL(mFeatureOverrideParser, getFeatureOverrideFilePath())
            .WillRepeatedly(Return(getTestBinarypbPath(filename)));
    }

    FeatureOverrideParserMock mFeatureOverrideParser;
};

testing::AssertionResult validateFeatureConfigTestTxtpbSizes(FeatureOverrides overrides) {
    size_t expectedGlobalFeaturesSize = 3;
    if (overrides.mGlobalFeatures.size() != expectedGlobalFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mGlobalFeatures.size(): " << overrides.mGlobalFeatures.size()
                << ", expected: " << expectedGlobalFeaturesSize;
    }

    size_t expectedPackageFeaturesSize = 3;
    if (overrides.mPackageFeatures.size() != expectedPackageFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures.size(): " << overrides.mPackageFeatures.size()
                << ", expected: " << expectedPackageFeaturesSize;
    }

    return testing::AssertionSuccess();
}

testing::AssertionResult validateFeatureConfigTestForceReadTxtpbSizes(FeatureOverrides overrides) {
    size_t expectedGlobalFeaturesSize = 1;
    if (overrides.mGlobalFeatures.size() != expectedGlobalFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mGlobalFeatures.size(): " << overrides.mGlobalFeatures.size()
                << ", expected: " << expectedGlobalFeaturesSize;
    }

    size_t expectedPackageFeaturesSize = 0;
    if (overrides.mPackageFeatures.size() != expectedPackageFeaturesSize) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures.size(): " << overrides.mPackageFeatures.size()
                << ", expected: " << expectedPackageFeaturesSize;
    }

    return testing::AssertionSuccess();
}

testing::AssertionResult validateGlobalOverrides1(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 0;
    const std::string expectedFeatureName = "globalOverrides1";
    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, globalOverrides1) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validateGlobalOverrides1(overrides));
}

testing::AssertionResult validateGlobalOverrides2(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 1;
    const std::string expectedFeatureName = "globalOverrides2";
    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = true;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    std::vector<uint32_t> expectedGpuVendorIDs = {
        0,      // GpuVendorID::VENDOR_ID_TEST
        0x13B5, // GpuVendorID::VENDOR_ID_ARM
    };
    if (cfg.mGpuVendorIDs.size() != expectedGpuVendorIDs.size()) {
        return testing::AssertionFailure()
                << "cfg.mGpuVendorIDs.size(): " << cfg.mGpuVendorIDs.size()
                << ", expected: " << expectedGpuVendorIDs.size();
    }
    for (int i = 0; i < expectedGpuVendorIDs.size(); i++) {
        if (cfg.mGpuVendorIDs[i] != expectedGpuVendorIDs[i]) {
            std::stringstream msg;
            msg << "cfg.mGpuVendorIDs[" << i << "]: 0x" << std::hex << cfg.mGpuVendorIDs[i]
                << ", expected: 0x" << std::hex << expectedGpuVendorIDs[i];
            return testing::AssertionFailure() << msg.str();
        }
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, globalOverrides2) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validateGlobalOverrides2(overrides));
}

testing::AssertionResult validateGlobalOverrides3(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 2;
    const std::string expectedFeatureName = "globalOverrides3";
    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = true;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    std::vector<uint32_t> expectedGpuVendorIDs = {
            0,      // GpuVendorID::VENDOR_ID_TEST
            0x8086, // GpuVendorID::VENDOR_ID_INTEL
    };
    if (cfg.mGpuVendorIDs.size() != expectedGpuVendorIDs.size()) {
        return testing::AssertionFailure()
                << "cfg.mGpuVendorIDs.size(): " << cfg.mGpuVendorIDs.size()
                << ", expected: " << expectedGpuVendorIDs.size();
    }
    for (int i = 0; i < expectedGpuVendorIDs.size(); i++) {
        if (cfg.mGpuVendorIDs[i] != expectedGpuVendorIDs[i]) {
            std::stringstream msg;
            msg << "cfg.mGpuVendorIDs[" << i << "]: 0x" << std::hex << cfg.mGpuVendorIDs[i]
                << ", expected: 0x" << std::hex << expectedGpuVendorIDs[i];
            return testing::AssertionFailure() << msg.str();
        }
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, globalOverrides3) {
FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

EXPECT_TRUE(validateGlobalOverrides3(overrides));
}

testing::AssertionResult validatePackageOverrides1(FeatureOverrides overrides) {
    const std::string expectedTestPackageName = "com.gpuservice_unittest.packageOverrides1";

    if (!overrides.mPackageFeatures.count(expectedTestPackageName)) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures missing expected package: "
                << expectedTestPackageName;
    }

    const std::vector<FeatureConfig>& features =
            overrides.mPackageFeatures[expectedTestPackageName];

    size_t expectedFeaturesSize = 1;
    if (features.size() != expectedFeaturesSize) {
        return testing::AssertionFailure()
                << "features.size(): " << features.size()
                << ", expectedFeaturesSize: " << expectedFeaturesSize;
    }

    const std::string expectedFeatureName = "packageOverrides1";
    const FeatureConfig &cfg = features[0];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = true;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, packageOverrides1) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validatePackageOverrides1(overrides));
}

testing::AssertionResult validateForceFileRead(FeatureOverrides overrides) {
    const int kTestFeatureIndex = 0;
    const std::string expectedFeatureName = "forceFileRead";

    const FeatureConfig &cfg = overrides.mGlobalFeatures[kTestFeatureIndex];
    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    return testing::AssertionSuccess();
}

testing::AssertionResult validatePackageOverrides2(FeatureOverrides overrides) {
    const std::string expectedPackageName = "com.gpuservice_unittest.packageOverrides2";

    if (!overrides.mPackageFeatures.count(expectedPackageName)) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures missing expected package: " << expectedPackageName;
    }

    const std::vector<FeatureConfig>& features = overrides.mPackageFeatures[expectedPackageName];

    size_t expectedFeaturesSize = 1;
    if (features.size() != expectedFeaturesSize) {
        return testing::AssertionFailure()
                << "features.size(): " << features.size()
                << ", expectedFeaturesSize: " << expectedFeaturesSize;
    }

    const std::string expectedFeatureName = "packageOverrides2";
    const FeatureConfig &cfg = features[0];

    if (cfg.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg.mEnabled
                << ", expected: " << expectedEnabled;
    }

    std::vector<uint32_t> expectedGpuVendorIDs = {
            0,      // GpuVendorID::VENDOR_ID_TEST
            0x8086, // GpuVendorID::VENDOR_ID_INTEL
    };
    if (cfg.mGpuVendorIDs.size() != expectedGpuVendorIDs.size()) {
        return testing::AssertionFailure()
                << "cfg.mGpuVendorIDs.size(): " << cfg.mGpuVendorIDs.size()
                << ", expected: " << expectedGpuVendorIDs.size();
    }
    for (int i = 0; i < expectedGpuVendorIDs.size(); i++) {
        if (cfg.mGpuVendorIDs[i] != expectedGpuVendorIDs[i]) {
            std::stringstream msg;
            msg << "cfg.mGpuVendorIDs[" << i << "]: 0x" << std::hex << cfg.mGpuVendorIDs[i]
                << ", expected: 0x" << std::hex << expectedGpuVendorIDs[i];
            return testing::AssertionFailure() << msg.str();
        }
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, packageOverrides2) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    EXPECT_TRUE(validatePackageOverrides2(overrides));
}

testing::AssertionResult validatePackageOverrides3(FeatureOverrides overrides) {
    const std::string expectedPackageName = "com.gpuservice_unittest.packageOverrides3";

    if (!overrides.mPackageFeatures.count(expectedPackageName)) {
        return testing::AssertionFailure()
                << "overrides.mPackageFeatures missing expected package: " << expectedPackageName;
    }

    const std::vector<FeatureConfig>& features = overrides.mPackageFeatures[expectedPackageName];

    size_t expectedFeaturesSize = 2;
    if (features.size() != expectedFeaturesSize) {
        return testing::AssertionFailure()
                << "features.size(): " << features.size()
                << ", expectedFeaturesSize: " << expectedFeaturesSize;
    }

    std::string expectedFeatureName = "packageOverrides3_1";
    const FeatureConfig &cfg_1 = features[0];

    if (cfg_1.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg_1.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    bool expectedEnabled = false;
    if (cfg_1.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg_1.mEnabled
                << ", expected: " << expectedEnabled;
    }

    std::vector<uint32_t> expectedGpuVendorIDs = {
            0,      // GpuVendorID::VENDOR_ID_TEST
            0x13B5, // GpuVendorID::VENDOR_ID_ARM
    };
    if (cfg_1.mGpuVendorIDs.size() != expectedGpuVendorIDs.size()) {
        return testing::AssertionFailure()
                << "cfg.mGpuVendorIDs.size(): " << cfg_1.mGpuVendorIDs.size()
                << ", expected: " << expectedGpuVendorIDs.size();
    }
    for (int i = 0; i < expectedGpuVendorIDs.size(); i++) {
        if (cfg_1.mGpuVendorIDs[i] != expectedGpuVendorIDs[i]) {
            std::stringstream msg;
            msg << "cfg.mGpuVendorIDs[" << i << "]: 0x" << std::hex << cfg_1.mGpuVendorIDs[i]
                << ", expected: 0x" << std::hex << expectedGpuVendorIDs[i];
            return testing::AssertionFailure() << msg.str();
        }
    }

    expectedFeatureName = "packageOverrides3_2";
    const FeatureConfig &cfg_2 = features[1];

    if (cfg_2.mFeatureName != expectedFeatureName) {
        return testing::AssertionFailure()
                << "cfg.mFeatureName: " << cfg_2.mFeatureName
                << ", expected: " << expectedFeatureName;
    }

    expectedEnabled = true;
    if (cfg_2.mEnabled != expectedEnabled) {
        return testing::AssertionFailure()
                << "cfg.mEnabled: " << cfg_2.mEnabled
                << ", expected: " << expectedEnabled;
    }

    expectedGpuVendorIDs = {
            0,      // GpuVendorID::VENDOR_ID_TEST
            0x8086, // GpuVendorID::VENDOR_ID_INTEL
    };
    if (cfg_2.mGpuVendorIDs.size() != expectedGpuVendorIDs.size()) {
        return testing::AssertionFailure()
                << "cfg.mGpuVendorIDs.size(): " << cfg_2.mGpuVendorIDs.size()
                << ", expected: " << expectedGpuVendorIDs.size();
    }
    for (int i = 0; i < expectedGpuVendorIDs.size(); i++) {
        if (cfg_2.mGpuVendorIDs[i] != expectedGpuVendorIDs[i]) {
            std::stringstream msg;
            msg << "cfg.mGpuVendorIDs[" << i << "]: 0x" << std::hex << cfg_2.mGpuVendorIDs[i]
                << ", expected: 0x" << std::hex << expectedGpuVendorIDs[i];
            return testing::AssertionFailure() << msg.str();
        }
    }

    return testing::AssertionSuccess();
}

TEST_F(FeatureOverrideParserTest, packageOverrides3) {
FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

EXPECT_TRUE(validatePackageOverrides3(overrides));
}

TEST_F(FeatureOverrideParserTest, forceFileRead) {
    FeatureOverrides overrides = mFeatureOverrideParser.getFeatureOverrides();

    // Validate the "original" contents are present.
    EXPECT_TRUE(validateFeatureConfigTestTxtpbSizes(overrides));
    EXPECT_TRUE(validateGlobalOverrides1(overrides));

    // "Update" the config file.
    const std::string filename = "gpuservice_unittest_feature_config_vk_force_read.binarypb";
    EXPECT_CALL(mFeatureOverrideParser, getFeatureOverrideFilePath())
        .WillRepeatedly(Return(getTestBinarypbPath(filename)));

    mFeatureOverrideParser.forceFileRead();

    overrides = mFeatureOverrideParser.getFeatureOverrides();

    // Validate the new file contents were read and parsed.
    EXPECT_TRUE(validateFeatureConfigTestForceReadTxtpbSizes(overrides));
    EXPECT_TRUE(validateForceFileRead(overrides));
}

} // namespace
} // namespace android
