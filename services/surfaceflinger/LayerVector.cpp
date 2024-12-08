/*
 * Copyright (C) 2016 The Android Open Source Project
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

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include "LayerVector.h"
#include "Layer.h"

namespace android {

LayerVector::LayerVector(const StateSet stateSet) : mStateSet(stateSet) {}

LayerVector::LayerVector(const LayerVector& rhs, const StateSet stateSet)
      : SortedVector<sp<Layer>>(rhs), mStateSet(stateSet) {}

LayerVector::~LayerVector() = default;

// This operator override is needed to prevent mStateSet from getting copied over.
LayerVector& LayerVector::operator=(const LayerVector& rhs) {
    SortedVector::operator=(rhs);
    return *this;
}

int LayerVector::do_compare(const void* lhs, const void* rhs) const
{
    // sort layers per layer-stack, then by z-order and finally by sequence
    const auto& l = *reinterpret_cast<const sp<Layer>*>(lhs);
    const auto& r = *reinterpret_cast<const sp<Layer>*>(rhs);

    const auto& lState = l->getDrawingState();
    const auto& rState = r->getDrawingState();

    if (l->sequence == r->sequence)
        return 0;

    return (l->sequence > r->sequence) ? 1 : -1;
}

} // namespace android

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"
