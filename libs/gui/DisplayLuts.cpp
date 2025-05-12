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

#include "include/gui/DisplayLuts.h"
#include <gui/DisplayLuts.h>
#include <private/gui/ParcelUtils.h>

namespace android::gui {

status_t DisplayLuts::Entry::readFromParcel(const android::Parcel* parcel) {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }

    SAFE_PARCEL(parcel->readInt32, &dimension);
    SAFE_PARCEL(parcel->readInt32, &size);
    SAFE_PARCEL(parcel->readInt32, &samplingKey);

    return OK;
}

status_t DisplayLuts::Entry::writeToParcel(android::Parcel* parcel) const {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }

    SAFE_PARCEL(parcel->writeInt32, dimension);
    SAFE_PARCEL(parcel->writeInt32, size);
    SAFE_PARCEL(parcel->writeInt32, samplingKey);

    return OK;
}

status_t DisplayLuts::readFromParcel(const android::Parcel* parcel) {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }

    SAFE_PARCEL(parcel->readUniqueFileDescriptor, &fd);
    SAFE_PARCEL(parcel->readInt32Vector, &offsets);
    int32_t numLutProperties;
    SAFE_PARCEL(parcel->readInt32, &numLutProperties);
    lutProperties.reserve(numLutProperties);
    for (int32_t i = 0; i < numLutProperties; i++) {
        lutProperties.push_back({});
        SAFE_PARCEL(lutProperties.back().readFromParcel, parcel);
    }
    return OK;
}

status_t DisplayLuts::writeToParcel(android::Parcel* parcel) const {
    if (parcel == nullptr) {
        ALOGE("%s: Null parcel", __func__);
        return BAD_VALUE;
    }

    SAFE_PARCEL(parcel->writeUniqueFileDescriptor, fd);
    SAFE_PARCEL(parcel->writeInt32Vector, offsets);
    SAFE_PARCEL(parcel->writeInt32, static_cast<int32_t>(lutProperties.size()));
    for (auto& entry : lutProperties) {
        SAFE_PARCEL(entry.writeToParcel, parcel);
    }
    return OK;
}
} // namespace android::gui