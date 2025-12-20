/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <binder/Parcel.h>
#include <binder/Parcelable.h>
#include <format>
#include <math/HashCombine.h>
#include <math/vec2.h>
#include <ostream>
#include <string>
#include "ui/Transform.h"

#include <android/gui/CornerRadiiData.h>
#include <android/gui/Vec2.h>

namespace android::gui {

inline android::gui::Vec2 toAidlVec2(const ::android::vec2& v) {
    android::gui::Vec2 aidl_v;
    aidl_v.x = v.x;
    aidl_v.y = v.y;
    return aidl_v;
}

inline vec2 fromAidlVec2(const ::android::gui::Vec2& v) {
    return {v.x, v.y};
}

/**
 *  Utility class representing all four corner radii for a layer.
 */
class CornerRadii : public ::android::gui::CornerRadiiData {
public:
    CornerRadii() = default;

    explicit CornerRadii(float uniformRadius) {
        this->topLeft = toAidlVec2({uniformRadius, uniformRadius});
        this->topRight = toAidlVec2({uniformRadius, uniformRadius});
        this->bottomLeft = toAidlVec2({uniformRadius, uniformRadius});
        this->bottomRight = toAidlVec2({uniformRadius, uniformRadius});
    }

    CornerRadii(float tl, float tr, float bl, float br) {
        this->topLeft = toAidlVec2({tl, tl});
        this->topRight = toAidlVec2({tr, tr});
        this->bottomLeft = toAidlVec2({bl, bl});
        this->bottomRight = toAidlVec2({br, br});
    }

    /* Returns true if x and y are both set on any of the corners, otherwise false. */
    bool isEmpty() const {
        return !((topLeft.x > 0 && topLeft.y > 0) || (topRight.x > 0 && topRight.y > 0) ||
                 (bottomLeft.x > 0 && bottomLeft.y > 0) ||
                 (bottomRight.x > 0 && bottomRight.y > 0));
    }

    /* Scales the X and Y of each corner radius as per ui::Transform */
    void transform(ui::Transform t) {
        this->topLeft.x *= t.getScaleX();
        this->topLeft.y *= t.getScaleY();
        this->topRight.x *= t.getScaleX();
        this->topRight.y *= t.getScaleY();
        this->bottomLeft.x *= t.getScaleX();
        this->bottomLeft.y *= t.getScaleY();
        this->bottomRight.x *= t.getScaleX();
        this->bottomRight.y *= t.getScaleY();
    }

    std::string toString() const {
        return std::format(
            R"(CornerRadii(tl:{:.1f},{:.1f}, tr:{:.1f},{:.1f},)" "\n"
            R"( bl:{:.1f},{:.1f}, br:{:.1f},{:.1f}))",
            topLeft.x, topLeft.y, topRight.x, topRight.y,
            bottomLeft.x, bottomLeft.y, bottomRight.x, bottomRight.y
        );
    }
};

inline bool operator==(const CornerRadii& lhs, const CornerRadii& rhs) {
    return static_cast<const ::android::gui::CornerRadiiData&>(lhs) ==
            static_cast<const ::android::gui::CornerRadiiData&>(rhs);
}

inline bool operator>(const CornerRadii& lhs, const CornerRadii& rhs) {
    return fromAidlVec2(lhs.topLeft) > fromAidlVec2(rhs.topLeft) ||
            fromAidlVec2(lhs.topRight) > fromAidlVec2(rhs.topRight) ||
            fromAidlVec2(lhs.bottomLeft) > fromAidlVec2(rhs.bottomLeft) ||
            fromAidlVec2(lhs.bottomRight) > fromAidlVec2(rhs.bottomRight);
}

inline std::ostream& operator<<(std::ostream& os, const CornerRadii& cr) {
    // Print the values directly from the inherited AIDL-generated Vec2 members (topLeft.x, etc.).
    return os << "CornerRadius(tl:" << cr.topLeft.x << "," << cr.topLeft.y
              << ", tr:" << cr.topRight.x << "," << cr.topRight.y << ", bl:" << cr.bottomLeft.x
              << "," << cr.bottomLeft.y << ", br:" << cr.bottomRight.x << "," << cr.bottomRight.y
              << ")";
}

} // namespace android::gui

namespace std {

template <>
struct hash<android::gui::CornerRadii> {
    size_t operator()(const android::gui::CornerRadii& radii) const {
        return android::hashCombine(radii.topLeft, radii.topRight, radii.bottomLeft,
                                    radii.bottomRight);
    }
};

template <>
struct hash<android::gui::Vec2> {
    size_t operator()(const android::gui::Vec2& v) const { return android::hashCombine(v.x, v.y); }
};

} // namespace std