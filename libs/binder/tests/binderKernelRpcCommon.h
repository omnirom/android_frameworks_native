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

#pragma once

#include <gtest/gtest.h>
#include "binder/Binder.h"
#include "binder/Parcel.h"

namespace android {

inline void readFdsTest(Parcel& p, int fd) {
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeDupFileDescriptor(fd));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeDupFileDescriptor(fd));

    p.setDataPosition(0);

    uint8_t data[sizeof(int32_t) * 2];
    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t)));
    EXPECT_NE(-1, p.readFileDescriptor());
    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t) * 2));
    EXPECT_NE(-1, p.readFileDescriptor());
}

// Reading over FDs with the wrong APIs is not allowed
inline void readOverFdsTest(Parcel& p, int fd, size_t sizeofFd) {
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeDupFileDescriptor(fd));
    size_t dataPosBetweenFds = p.dataPosition();
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeDupFileDescriptor(fd));

    p.setDataPosition(0);

#ifdef __TRUSTY__
    // Trusty builds don't allow variable length arrays and don't have access
    // to kernel binder so always use int32_t + size of RPC FD object
    ASSERT_LE(sizeofFd, sizeof(int32_t) + sizeof(int32_t));
    uint8_t data[sizeof(int32_t) + sizeof(int32_t) + sizeof(int32_t)];
#else
    uint8_t data[sizeof(int32_t) + sizeofFd];
#endif // __TRUSTY__
    EXPECT_EQ(PERMISSION_DENIED, p.read((void*)data, sizeof(int32_t) + sizeofFd));

    p.setDataPosition(dataPosBetweenFds);

    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t) * 2));

    EXPECT_EQ(PERMISSION_DENIED, p.read((void*)data, sizeofFd));
}

// Using the read/writeBinder APIs is OK to read objects in parcels
inline void readBindersTest(Parcel& p, const sp<IBinder>& b) {
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeStrongBinder(b));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeStrongBinder(b));

    p.setDataPosition(0);

    uint8_t data[sizeof(int32_t) * 2];
    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t)));
    sp<IBinder> b2;
    EXPECT_EQ(OK, p.readStrongBinder(&b2));
    b2.clear();
    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t) * 2));
    EXPECT_EQ(OK, p.readStrongBinder(&b2));
}

// Reading over binder objects with the wrong APIs is not allowed
inline void readOverBindersTest(Parcel& p, const sp<IBinder>& b, size_t sizeofBinder) {
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeStrongBinder(b));
    size_t dataPosBetweenBinders = p.dataPosition();
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeInt32(1));
    EXPECT_EQ(OK, p.writeStrongBinder(b));

    p.setDataPosition(0);

#ifdef __TRUSTY__
    // Trusty builds don't allow variable length arrays and don't have access
    // to kernel binder so always use int32_t + size of RPC binder object
    ASSERT_LE(sizeofBinder, sizeof(int32_t) + sizeof(uint64_t));
    uint8_t data[sizeof(int32_t) + sizeof(int32_t) + sizeof(uint64_t)];
#else
    uint8_t data[sizeof(int32_t) + sizeofBinder];
#endif // __TRUSTY__
    EXPECT_EQ(PERMISSION_DENIED, p.read((void*)data, sizeof(int32_t) + sizeofBinder));

    p.setDataPosition(dataPosBetweenBinders);

    EXPECT_EQ(OK, p.read((void*)data, sizeof(int32_t) * 2));

    EXPECT_EQ(PERMISSION_DENIED, p.read((void*)data, sizeofBinder));
}

} // namespace android
