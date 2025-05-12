/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <android/display_luts.h>
#include <stdint.h>
#include <vector>
#include <utils/RefBase.h>

using namespace android;

__BEGIN_DECLS

struct ADisplayLutsEntry_buffer {
    std::vector<float> data;
};

struct ADisplayLutsEntry_properties {
    ADisplayLuts_Dimension dimension;
    int32_t size;
    ADisplayLuts_SamplingKey samplingKey;
};

struct ADisplayLutsEntry: public RefBase {
    struct ADisplayLutsEntry_buffer buffer;
    struct ADisplayLutsEntry_properties properties;
    ADisplayLutsEntry() {}

    // copy constructor
    ADisplayLutsEntry(const ADisplayLutsEntry& other) :
        buffer(other.buffer),
        properties(other.properties) {}

    // copy operator
    ADisplayLutsEntry& operator=(const ADisplayLutsEntry& other) {
        if (this != &other) { // Protect against self-assignment
            buffer = other.buffer;
            properties = other.properties;
        }
        return *this;
    }
};

struct ADisplayLuts: public RefBase {
    int32_t totalBufferSize;
    std::vector<int32_t> offsets;
    std::vector<sp<ADisplayLutsEntry>> entries;

    ADisplayLuts() : totalBufferSize(0) {}
};

__END_DECLS