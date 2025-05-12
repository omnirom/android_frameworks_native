/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <stdint.h>
#include <array>
#include <string>

namespace android {

/**
 * An opaque value that uniquely identifies a picture profile, or a set of parameters, which
 * describes the configuration of a picture processing pipeline that is applied to a graphic buffer
 * to enhance its quality prior to rendering on the display.
 */
typedef int64_t PictureProfileId;

/**
 * A picture profile handle wraps the picture profile ID for type-safety, and represents an opaque
 * handle that doesn't have the performance drawbacks of Binders.
 */
class PictureProfileHandle {
public:
    // A profile that represents no picture processing.
    static const PictureProfileHandle NONE;

    PictureProfileHandle() { *this = NONE; }
    explicit PictureProfileHandle(PictureProfileId id) : mId(id) {}

    PictureProfileId const& getId() const { return mId; }

    inline bool operator==(const PictureProfileHandle& rhs) { return mId == rhs.mId; }
    inline bool operator!=(const PictureProfileHandle& rhs) { return !(*this == rhs); }

    // Is the picture profile effectively null, or not-specified?
    inline bool operator!() const { return mId == NONE.mId; }

    operator bool() const { return !!*this; }

    friend ::std::string toString(const PictureProfileHandle& handle);

private:
    PictureProfileId mId;
};

} // namespace android
