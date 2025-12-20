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
#include <android/os/IStatsBootstrapAtomService.h>

namespace android {
// Helper functions to create a StatsBootstrapAtomValue for a primitive
inline os::StatsBootstrapAtomValue createPrimitiveValue(bool value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::boolValue>(value);
    return atomValue;
}

inline os::StatsBootstrapAtomValue createPrimitiveValue(int32_t value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::intValue>(value);
    return atomValue;
}

inline os::StatsBootstrapAtomValue createPrimitiveValue(int64_t value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::longValue>(value);
    return atomValue;
}

inline os::StatsBootstrapAtomValue createPrimitiveValue(float value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::floatValue>(value);
    return atomValue;
}

inline os::StatsBootstrapAtomValue createPrimitiveValue(const std::string& value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::stringValue>(
            String16(value.c_str()));
    return atomValue;
}

inline os::StatsBootstrapAtomValue createPrimitiveValue(const String16& value) {
    os::StatsBootstrapAtomValue atomValue;
    atomValue.value.set<os::StatsBootstrapAtomValue::Primitive::stringValue>(value);
    return atomValue;
}

// Data for a monitored binder transaction.
struct BinderCallData {
    // TODO(b/299356196): Use the receiver binder object instead and resolve interface lazily
    String16 interfaceDescriptor;
    uint32_t transactionCode;
    int64_t startTimeNanos;
    int64_t endTimeNanos;
    uid_t senderUid;

    bool hasLatencyData() const { return endTimeNanos > startTimeNanos; }
};

} // namespace android
