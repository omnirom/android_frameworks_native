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

#pragma once

#include <vector>
#include "MultiStateCounter.h"

namespace android {
namespace battery {

/**
 * Wrapper for an array of uint64's.
 */
class Uint64Array {
  protected:
    size_t mSize;

  public:
    Uint64Array() : Uint64Array(0) {}

    Uint64Array(size_t size) : mSize(size) {}

    virtual ~Uint64Array() {}

    size_t size() const { return mSize; }

    /**
     * Returns the wrapped array.
     *
     * Nullable! Null should be interpreted the same as an array of zeros
     */
    virtual const uint64_t *data() const { return nullptr; }

    friend std::ostream &operator<<(std::ostream &os, const Uint64Array &v);

    // Test API
    bool operator==(const Uint64Array &other) const;
};

/**
 * Mutable version of Uint64Array.
 */
class Uint64ArrayRW: public Uint64Array {
    uint64_t* mData;

public:
    Uint64ArrayRW() : Uint64ArrayRW(0) {}

    Uint64ArrayRW(size_t size) : Uint64Array(size), mData(nullptr) {}

    Uint64ArrayRW(const Uint64Array &copy);

    // Need an explicit copy constructor. In the initialization context C++ does not understand that
    // a Uint64ArrayRW is a Uint64Array.
    Uint64ArrayRW(const Uint64ArrayRW &copy) : Uint64ArrayRW((const Uint64Array &) copy) {}

    // Test API
    Uint64ArrayRW(std::initializer_list<uint64_t> init);

    ~Uint64ArrayRW() override { delete[] mData; }

    const uint64_t *data() const override { return mData; }

    // NonNull. Will initialize the wrapped array if it is null.
    uint64_t *dataRW();

    Uint64ArrayRW &operator=(const Uint64Array &t);
};

typedef MultiStateCounter<Uint64ArrayRW, Uint64Array> LongArrayMultiStateCounter;

} // namespace battery
} // namespace android
