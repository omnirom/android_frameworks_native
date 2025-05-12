/*
 * Copyright (C) 2021 The Android Open Source Project
 * Android BPF library - public API
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

#include "LongArrayMultiStateCounter.h"
#include <log/log.h>

namespace android {
namespace battery {

Uint64ArrayRW::Uint64ArrayRW(const Uint64Array &copy) : Uint64Array(copy.size()) {
    if (mSize != 0 && copy.data() != nullptr) {
        mData = new uint64_t[mSize];
        memcpy(mData, copy.data(), mSize * sizeof(uint64_t));
    } else {
        mData = nullptr;
    }
}

uint64_t *Uint64ArrayRW::dataRW() {
    if (mData == nullptr) {
        mData = new uint64_t[mSize];
        memset(mData, 0, mSize * sizeof(uint64_t));
    }
    return mData;
}

Uint64ArrayRW &Uint64ArrayRW::operator=(const Uint64Array &t) {
    if (t.size() != mSize) {
        delete[] mData;
        mSize = t.size();
        mData = nullptr;
    }
    if (mSize != 0) {
        if (t.data() != nullptr) {
            if (mData == nullptr) {
                mData = new uint64_t[mSize];
            }
            memcpy(mData, t.data(), mSize * sizeof(uint64_t));
        } else {
            delete[] mData;
            mData = nullptr;
        }
    }
    return *this;
}

std::ostream &operator<<(std::ostream &os, const Uint64Array &v) {
    os << "{";
    const uint64_t *data = v.data();
    if (data != nullptr) {
        bool first = true;
        for (size_t i = 0; i < v.size(); i++) {
            if (!first) {
                os << ", ";
            }
            os << data[i];
            first = false;
        }
    }
    os << "}";
    return os;
}

// Convenience constructor for tests
Uint64ArrayRW::Uint64ArrayRW(std::initializer_list<uint64_t> init) : Uint64Array(init.size()) {
    mData = new uint64_t[mSize];
    memcpy(mData, init.begin(), mSize * sizeof(uint64_t));
}

// Used in tests only.
bool Uint64Array::operator==(const Uint64Array &other) const {
    if (size() != other.size()) {
        return false;
    }
    const uint64_t* thisData = data();
    const uint64_t* thatData = other.data();
    for (size_t i = 0; i < mSize; i++) {
        const uint64_t v1 = thisData != nullptr ? thisData[i] : 0;
        const uint64_t v2 = thatData != nullptr ? thatData[i] : 0;
        if (v1 != v2) {
            return false;
        }
    }
    return true;
}

template <>
void LongArrayMultiStateCounter::add(Uint64ArrayRW *value1, const Uint64Array &value2,
                                     const uint64_t numerator, const uint64_t denominator) const {
    const uint64_t* data2 = value2.data();
    if (data2 == nullptr) {
        return;
    }

    uint64_t* data1 = value1->dataRW();
    size_t size = value2.size();
    if (numerator != denominator) {
        for (size_t i = 0; i < size; i++) {
            // The caller ensures that denominator != 0
            data1[i] += data2[i] * numerator / denominator;
        }
    } else {
        for (size_t i = 0; i < size; i++) {
            data1[i] += data2[i];
        }
    }
}

template<>
bool LongArrayMultiStateCounter::delta(const Uint64ArrayRW &previousValue,
                                       const Uint64Array &newValue, Uint64ArrayRW *outValue) const {
    size_t size = previousValue.size();
    if (newValue.size() != size) {
        ALOGE("Incorrect array size: %d, should be %d", (int) newValue.size(), (int) size);
        return false;
    }
    if (outValue->size() != size) {
        ALOGE("Incorrect outValue size: %d, should be %d", (int) outValue->size(), (int) size);
        return false;
    }

    bool is_delta_valid = true;
    const uint64_t *prevData = previousValue.data();
    const uint64_t *newData = newValue.data();
    uint64_t *outData = outValue->dataRW();
    for (size_t i = 0; i < size; i++) {
        if (prevData == nullptr) {
            if (newData == nullptr) {
                outData[i] = 0;
            } else {
                outData[i] = newData[i];
            }
        } else if (newData == nullptr || newData[i] < prevData[i]) {
            outData[i] = 0;
            is_delta_valid = false;
        } else {
            outData[i] = newData[i] - prevData[i];
        }
    }
    return is_delta_valid;
}

} // namespace battery
} // namespace android
