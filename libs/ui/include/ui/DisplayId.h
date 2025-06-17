/*
 * Copyright 2020 The Android Open Source Project
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

#include <cstdint>
#include <ostream>
#include <string>

#include <ftl/match.h>
#include <ftl/optional.h>

namespace android {

// ID of a physical or a virtual display. This class acts as a type safe wrapper around uint64_t.
// The encoding of the ID is type-specific for bits 0 to 61.
struct DisplayId {
    // Flag indicating that the display is virtual.
    static constexpr uint64_t FLAG_VIRTUAL = 1ULL << 63;

    // TODO: b/162612135 - Remove default constructor.
    DisplayId() = default;
    constexpr DisplayId(const DisplayId&) = default;
    DisplayId& operator=(const DisplayId&) = default;

    static constexpr DisplayId fromValue(uint64_t value) { return DisplayId(value); }

    uint64_t value;

protected:
    explicit constexpr DisplayId(uint64_t id) : value(id) {}
};

inline bool operator==(DisplayId lhs, DisplayId rhs) {
    return lhs.value == rhs.value;
}

inline bool operator!=(DisplayId lhs, DisplayId rhs) {
    return !(lhs == rhs);
}

inline std::string to_string(DisplayId displayId) {
    return std::to_string(displayId.value);
}

// For tests.
inline std::ostream& operator<<(std::ostream& stream, DisplayId displayId) {
    return stream << "DisplayId{" << displayId.value << '}';
}

// DisplayId of a physical display, such as the internal display or externally connected display.
struct PhysicalDisplayId : DisplayId {
    // TODO: b/162612135 - Remove default constructor.
    PhysicalDisplayId() = default;

    // Returns a stable ID based on EDID and port information.
    static constexpr PhysicalDisplayId fromEdid(uint8_t port, uint16_t manufacturerId,
                                                uint32_t modelHash) {
        return PhysicalDisplayId(FLAG_STABLE, port, manufacturerId, modelHash);
    }

    // Returns an unstable ID. If EDID is available using "fromEdid" is preferred.
    static constexpr PhysicalDisplayId fromPort(uint8_t port) {
        constexpr uint16_t kManufacturerId = 0;
        constexpr uint32_t kModelHash = 0;
        return PhysicalDisplayId(0, port, kManufacturerId, kModelHash);
    }

    static constexpr PhysicalDisplayId fromValue(uint64_t value) {
        return PhysicalDisplayId(value);
    }

private:
    // Flag indicating that the ID is stable across reboots.
    static constexpr uint64_t FLAG_STABLE = 1ULL << 62;

    using DisplayId::DisplayId;

    constexpr PhysicalDisplayId(uint64_t flags, uint8_t port, uint16_t manufacturerId,
                                uint32_t modelHash)
          : DisplayId(flags | (static_cast<uint64_t>(manufacturerId) << 40) |
                      (static_cast<uint64_t>(modelHash) << 8) | port) {}

    explicit constexpr PhysicalDisplayId(DisplayId other) : DisplayId(other) {}
};

struct VirtualDisplayId : DisplayId {
    using BaseId = uint32_t;

    // Flag indicating that this virtual display is backed by the GPU.
    static constexpr uint64_t FLAG_GPU = 1ULL << 61;

    static constexpr VirtualDisplayId fromValue(uint64_t value) {
        return VirtualDisplayId(SkipVirtualFlag{}, value);
    }

protected:
    struct SkipVirtualFlag {};
    constexpr VirtualDisplayId(SkipVirtualFlag, uint64_t value) : DisplayId(value) {}
    explicit constexpr VirtualDisplayId(uint64_t value) : DisplayId(FLAG_VIRTUAL | value) {}

    explicit constexpr VirtualDisplayId(DisplayId other) : DisplayId(other) {}
};

struct HalVirtualDisplayId : VirtualDisplayId {
    explicit constexpr HalVirtualDisplayId(BaseId baseId) : VirtualDisplayId(baseId) {}

    static constexpr HalVirtualDisplayId fromValue(uint64_t value) {
        return HalVirtualDisplayId(SkipVirtualFlag{}, value);
    }

private:
    using VirtualDisplayId::VirtualDisplayId;
};

struct GpuVirtualDisplayId : VirtualDisplayId {
    explicit constexpr GpuVirtualDisplayId(BaseId baseId) : VirtualDisplayId(FLAG_GPU | baseId) {}

    static constexpr GpuVirtualDisplayId fromValue(uint64_t value) {
        return GpuVirtualDisplayId(SkipVirtualFlag{}, value);
    }

private:
    using VirtualDisplayId::VirtualDisplayId;
};

// HalDisplayId is the ID of a display which is managed by HWC.
// PhysicalDisplayId and HalVirtualDisplayId are implicitly convertible to HalDisplayId.
struct HalDisplayId : DisplayId {
    constexpr HalDisplayId(HalVirtualDisplayId other) : DisplayId(other) {}
    constexpr HalDisplayId(PhysicalDisplayId other) : DisplayId(other) {}
    static constexpr HalDisplayId fromValue(uint64_t value) { return HalDisplayId(value); }

private:
    using DisplayId::DisplayId;
    explicit constexpr HalDisplayId(DisplayId other) : DisplayId(other) {}
};

using DisplayIdVariant = std::variant<PhysicalDisplayId, GpuVirtualDisplayId, HalVirtualDisplayId>;
using VirtualDisplayIdVariant = std::variant<GpuVirtualDisplayId, HalVirtualDisplayId>;

template <typename DisplayIdType>
inline auto asDisplayIdOfType(DisplayIdVariant variant) -> ftl::Optional<DisplayIdType> {
    return ftl::match(
            variant,
            [](DisplayIdType id) -> ftl::Optional<DisplayIdType> { return ftl::Optional(id); },
            [](auto) -> ftl::Optional<DisplayIdType> { return std::nullopt; });
}

template <typename Variant>
inline auto asHalDisplayId(Variant variant) -> ftl::Optional<HalDisplayId> {
    return ftl::match(
            variant,
            [](GpuVirtualDisplayId) -> ftl::Optional<HalDisplayId> { return std::nullopt; },
            [](auto id) -> ftl::Optional<HalDisplayId> {
                return ftl::Optional(static_cast<HalDisplayId>(id));
            });
}

inline auto asPhysicalDisplayId(DisplayIdVariant variant) -> ftl::Optional<PhysicalDisplayId> {
    return asDisplayIdOfType<PhysicalDisplayId>(variant);
}

inline auto asVirtualDisplayId(DisplayIdVariant variant) -> ftl::Optional<VirtualDisplayId> {
    return ftl::match(
            variant,
            [](GpuVirtualDisplayId id) -> ftl::Optional<VirtualDisplayId> {
                return ftl::Optional(static_cast<VirtualDisplayId>(id));
            },
            [](HalVirtualDisplayId id) -> ftl::Optional<VirtualDisplayId> {
                return ftl::Optional(static_cast<VirtualDisplayId>(id));
            },
            [](auto) -> ftl::Optional<VirtualDisplayId> { return std::nullopt; });
}

inline auto asDisplayId(DisplayIdVariant variant) -> DisplayId {
    return ftl::match(variant, [](auto id) -> DisplayId { return static_cast<DisplayId>(id); });
}

static_assert(sizeof(DisplayId) == sizeof(uint64_t));
static_assert(sizeof(HalDisplayId) == sizeof(uint64_t));
static_assert(sizeof(VirtualDisplayId) == sizeof(uint64_t));

static_assert(sizeof(PhysicalDisplayId) == sizeof(uint64_t));
static_assert(sizeof(HalVirtualDisplayId) == sizeof(uint64_t));
static_assert(sizeof(GpuVirtualDisplayId) == sizeof(uint64_t));

} // namespace android

namespace std {

template <>
struct hash<android::DisplayId> {
    size_t operator()(android::DisplayId displayId) const {
        return hash<uint64_t>()(displayId.value);
    }
};

template <>
struct hash<android::PhysicalDisplayId> : hash<android::DisplayId> {};

template <>
struct hash<android::HalVirtualDisplayId> : hash<android::DisplayId> {};

template <>
struct hash<android::GpuVirtualDisplayId> : hash<android::DisplayId> {};

template <>
struct hash<android::HalDisplayId> : hash<android::DisplayId> {};

} // namespace std
