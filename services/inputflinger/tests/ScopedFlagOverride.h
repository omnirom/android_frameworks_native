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

#pragma once

#include <com_android_input_flags.h>
#include <functional>

namespace android {

/**
 * Provide a local override for a flag value. The value is restored when the object of this class
 * goes out of scope.
 * This class is not intended to be used directly, because its usage is cumbersome.
 * Instead, a wrapper macro SCOPED_FLAG_OVERRIDE is provided.
 */
class ScopedFlagOverride {
public:
    ScopedFlagOverride(std::function<bool()> read, std::function<void(bool)> write, bool value)
          : mInitialValue(read()), mWriteValue(write) {
        mWriteValue(value);
    }
    ~ScopedFlagOverride() { mWriteValue(mInitialValue); }

private:
    const bool mInitialValue;
    std::function<void(bool)> mWriteValue;
};

typedef bool (*ReadFlagValueFunction)();
typedef void (*WriteFlagValueFunction)(bool);

/**
 * Use this macro to locally override a flag value.
 * Example usage:
 *    SCOPED_FLAG_OVERRIDE(enable_multi_device_same_window_stream, false);
 * Note: this works by creating a local variable in your current scope. Don't call this twice for
 * the same flag, because the variable names will clash!
 */
#define SCOPED_FLAG_OVERRIDE(NAME, VALUE)                                  \
    ReadFlagValueFunction read##NAME = com::android::input::flags::NAME;   \
    WriteFlagValueFunction write##NAME = com::android::input::flags::NAME; \
    ScopedFlagOverride override##NAME(read##NAME, write##NAME, (VALUE))

} // namespace android
