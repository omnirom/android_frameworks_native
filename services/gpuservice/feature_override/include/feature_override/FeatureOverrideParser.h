/*
 * Copyright 2024 The Android Open Source Project
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

#ifndef FEATURE_OVERRIDE_PARSER_H_
#define FEATURE_OVERRIDE_PARSER_H_

#include <ctime>
#include <string>
#include <vector>

#include <graphicsenv/FeatureOverrides.h>

namespace android {

class FeatureOverrideParser {
public:
    FeatureOverrideParser() = default;
    FeatureOverrideParser(const FeatureOverrideParser &) = default;
    virtual ~FeatureOverrideParser() = default;

    FeatureOverrides getFeatureOverrides();
    void forceFileRead();

private:
    bool shouldReloadFeatureOverrides() const;
    void parseFeatureOverrides();
    // Allow FeatureOverrideParserMock to override with the unit test file's path.
    virtual std::string getFeatureOverrideFilePath() const;

    std::time_t mLastProtobufReadTime = 0;
    FeatureOverrides mFeatureOverrides;
};

} // namespace android

#endif  // FEATURE_OVERRIDE_PARSER_H_
