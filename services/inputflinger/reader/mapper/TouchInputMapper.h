/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include <array>
#include <climits>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <input/DisplayViewport.h>
#include <input/Input.h>
#include <input/InputDevice.h>
#include <stdint.h>
#include <ui/Rect.h>
#include <ui/Rotation.h>
#include <ui/Size.h>
#include <ui/Transform.h>
#include <utils/BitSet.h>
#include <utils/Timers.h>

#include "CursorButtonAccumulator.h"
#include "EventHub.h"
#include "InputMapper.h"
#include "InputReaderBase.h"
#include "NotifyArgs.h"
#include "StylusState.h"
#include "TouchButtonAccumulator.h"

namespace android {

// Maximum amount of latency to add to touch events while waiting for data from an
// external stylus.
static constexpr nsecs_t EXTERNAL_STYLUS_DATA_TIMEOUT = ms2ns(72);

// Maximum amount of time to wait on touch data before pushing out new pressure data.
static constexpr nsecs_t TOUCH_DATA_TIMEOUT = ms2ns(20);

/* Raw axis information from the driver. */
struct RawPointerAxes {
    RawAbsoluteAxisInfo x{};
    RawAbsoluteAxisInfo y{};
    std::optional<RawAbsoluteAxisInfo> pressure{};
    std::optional<RawAbsoluteAxisInfo> touchMajor{};
    std::optional<RawAbsoluteAxisInfo> touchMinor{};
    std::optional<RawAbsoluteAxisInfo> toolMajor{};
    std::optional<RawAbsoluteAxisInfo> toolMinor{};
    std::optional<RawAbsoluteAxisInfo> orientation{};
    std::optional<RawAbsoluteAxisInfo> distance{};
    std::optional<RawAbsoluteAxisInfo> tiltX{};
    std::optional<RawAbsoluteAxisInfo> tiltY{};
    std::optional<RawAbsoluteAxisInfo> trackingId{};
    std::optional<RawAbsoluteAxisInfo> slot{};

    inline int32_t getRawWidth() const { return x.maxValue - x.minValue + 1; }
    inline int32_t getRawHeight() const { return y.maxValue - y.minValue + 1; }
    inline void clear() { *this = RawPointerAxes(); }
};

using PropertiesArray = std::array<PointerProperties, MAX_POINTERS>;
using CoordsArray = std::array<PointerCoords, MAX_POINTERS>;
using IdToIndexArray = std::array<uint32_t, MAX_POINTER_ID + 1>;

/* Raw data for a collection of pointers including a pointer id mapping table. */
struct RawPointerData {
    struct Pointer {
        uint32_t id{0xFFFFFFFF};
        int32_t x{};
        int32_t y{};
        int32_t pressure{};
        int32_t touchMajor{};
        int32_t touchMinor{};
        int32_t toolMajor{};
        int32_t toolMinor{};
        int32_t orientation{};
        int32_t distance{};
        int32_t tiltX{};
        int32_t tiltY{};
        // A fully decoded ToolType constant.
        ToolType toolType{ToolType::UNKNOWN};
        bool isHovering{false};
    };

    uint32_t pointerCount{};
    std::array<Pointer, MAX_POINTERS> pointers{};
    BitSet32 hoveringIdBits{}, touchingIdBits{}, canceledIdBits{};
    IdToIndexArray idToIndex{};

    inline void clear() { *this = RawPointerData(); }

    inline void markIdBit(uint32_t id, bool isHovering) {
        if (isHovering) {
            hoveringIdBits.markBit(id);
        } else {
            touchingIdBits.markBit(id);
        }
    }

    inline void clearIdBits() {
        hoveringIdBits.clear();
        touchingIdBits.clear();
        canceledIdBits.clear();
    }

    inline const Pointer& pointerForId(uint32_t id) const { return pointers[idToIndex[id]]; }

    inline bool isHovering(uint32_t pointerIndex) { return pointers[pointerIndex].isHovering; }
};

/* Cooked data for a collection of pointers including a pointer id mapping table. */
struct CookedPointerData {
    uint32_t pointerCount{};
    PropertiesArray pointerProperties{};
    CoordsArray pointerCoords{};
    BitSet32 hoveringIdBits{}, touchingIdBits{}, canceledIdBits{}, validIdBits{};
    IdToIndexArray idToIndex{};

    inline void clear() { *this = CookedPointerData(); }

    inline const PointerCoords& pointerCoordsForId(uint32_t id) const {
        return pointerCoords[idToIndex[id]];
    }

    inline PointerCoords& editPointerCoordsWithId(uint32_t id) {
        return pointerCoords[idToIndex[id]];
    }

    inline PointerProperties& editPointerPropertiesWithId(uint32_t id) {
        return pointerProperties[idToIndex[id]];
    }

    inline bool isHovering(uint32_t pointerIndex) const {
        return hoveringIdBits.hasBit(pointerProperties[pointerIndex].id);
    }

    inline bool isTouching(uint32_t pointerIndex) const {
        return touchingIdBits.hasBit(pointerProperties[pointerIndex].id);
    }

    inline bool hasPointerCoordsForId(uint32_t id) const { return validIdBits.hasBit(id); }
};

class TouchInputMapper : public InputMapper {
public:
    ~TouchInputMapper() override;

    uint32_t getSources() const override;
    void populateDeviceInfo(InputDeviceInfo& deviceInfo) override;
    void dump(std::string& dump) override;
    [[nodiscard]] std::list<NotifyArgs> reconfigure(nsecs_t when,
                                                    const InputReaderConfiguration& config,
                                                    ConfigurationChanges changes) override;
    [[nodiscard]] std::list<NotifyArgs> reset(nsecs_t when) override;
    [[nodiscard]] std::list<NotifyArgs> process(const RawEvent& rawEvent) override;

    int32_t getKeyCodeState(uint32_t sourceMask, int32_t keyCode) override;
    int32_t getScanCodeState(uint32_t sourceMask, int32_t scanCode) override;
    bool markSupportedKeyCodes(uint32_t sourceMask, const std::vector<int32_t>& keyCodes,
                               uint8_t* outFlags) override;

    [[nodiscard]] std::list<NotifyArgs> cancelTouch(nsecs_t when, nsecs_t readTime) override;
    [[nodiscard]] std::list<NotifyArgs> timeoutExpired(nsecs_t when) override;
    [[nodiscard]] std::list<NotifyArgs> updateExternalStylusState(
            const StylusState& state) override;
    std::optional<ui::LogicalDisplayId> getAssociatedDisplayId() const override;

protected:
    CursorButtonAccumulator mCursorButtonAccumulator;
    TouchButtonAccumulator mTouchButtonAccumulator;

    struct VirtualKey {
        int32_t keyCode;
        int32_t scanCode;
        uint32_t flags;

        // computed hit box, specified in touch screen coords based on known display size
        int32_t hitLeft;
        int32_t hitTop;
        int32_t hitRight;
        int32_t hitBottom;

        inline bool isHit(int32_t x, int32_t y) const {
            return x >= hitLeft && x <= hitRight && y >= hitTop && y <= hitBottom;
        }
    };

    // Input sources and device mode.
    uint32_t mSource{0};

    enum class DeviceMode {
        DISABLED,   // input is disabled
        DIRECT,     // direct mapping (touchscreen)
        NAVIGATION, // unscaled mapping with assist gesture (touch navigation)
        POINTER,    // pointer mapping (e.g. absolute mouse, drawing tablet)

        ftl_last = POINTER
    };
    DeviceMode mDeviceMode{DeviceMode::DISABLED};

    // The reader's configuration.
    InputReaderConfiguration mConfig;

    // Immutable configuration parameters.
    struct Parameters {
        enum class DeviceType {
            TOUCH_SCREEN,
            TOUCH_NAVIGATION,
            POINTER,

            ftl_last = POINTER
        };

        // TouchInputMapper will configure devices with INPUT_PROP_DIRECT as
        // DeviceType::TOUCH_SCREEN, and will otherwise use DeviceType::POINTER by default.
        // This can be overridden by IDC files, using the `touch.deviceType` config.
        DeviceType deviceType;
        bool associatedDisplayIsExternal;
        bool orientationAware;

        ui::Rotation orientation;

        std::string uniqueDisplayId;

        bool wake;

        // The Universal Stylus Initiative (USI) protocol version supported by this device.
        std::optional<InputDeviceUsiVersion> usiVersion;

        // Allows touches while the display is off.
        bool enableForInactiveViewport;
    } mParameters;

    // Immutable calibration parameters in parsed form.
    struct Calibration {
        // Size
        enum class SizeCalibration {
            DEFAULT,
            NONE,
            GEOMETRIC,
            DIAMETER,
            BOX,
            AREA,
            ftl_last = AREA
        };

        SizeCalibration sizeCalibration;

        std::optional<float> sizeScale;
        std::optional<float> sizeBias;
        std::optional<bool> sizeIsSummed;

        // Pressure
        enum class PressureCalibration {
            DEFAULT,
            NONE,
            PHYSICAL,
            AMPLITUDE,
        };

        PressureCalibration pressureCalibration;
        std::optional<float> pressureScale;

        // Orientation
        enum class OrientationCalibration {
            DEFAULT,
            NONE,
            INTERPOLATED,
            VECTOR,
        };

        OrientationCalibration orientationCalibration;

        // Distance
        enum class DistanceCalibration {
            DEFAULT,
            NONE,
            SCALED,
        };

        DistanceCalibration distanceCalibration;
        std::optional<float> distanceScale;

        inline void applySizeScaleAndBias(float& outSize) const {
            if (sizeScale) {
                outSize *= *sizeScale;
            }
            if (sizeBias) {
                outSize += *sizeBias;
            }
            if (outSize < 0) {
                outSize = 0;
            }
        }
    } mCalibration;

    // Affine location transformation/calibration
    struct TouchAffineTransformation mAffineTransform;

    RawPointerAxes mRawPointerAxes;

    struct RawState {
        nsecs_t when{std::numeric_limits<nsecs_t>::min()};
        nsecs_t readTime{};

        // Raw pointer sample data.
        RawPointerData rawPointerData{};

        int32_t buttonState{};

        inline void clear() { *this = RawState(); }
    };

    friend std::ostream& operator<<(std::ostream& out, const RawState& state);

    struct CookedState {
        // Cooked pointer sample data.
        CookedPointerData cookedPointerData{};

        int32_t buttonState{};

        inline void clear() { *this = CookedState(); }
    };

    std::vector<RawState> mRawStatesPending;
    RawState mCurrentRawState;
    CookedState mCurrentCookedState;
    RawState mLastRawState;
    CookedState mLastCookedState;

    enum class ExternalStylusPresence {
        // No external stylus connected.
        NONE,
        // An external stylus that can report touch/pressure that can be fused with the touchscreen.
        TOUCH_FUSION,
        // An external stylus that can only report buttons.
        BUTTON_FUSION,
        ftl_last = BUTTON_FUSION,
    };
    ExternalStylusPresence mExternalStylusPresence{ExternalStylusPresence::NONE};
    // State provided by an external stylus
    StylusState mExternalStylusState;
    // If an external stylus is capable of reporting pointer-specific data like pressure, we will
    // attempt to fuse the pointer data reported by the stylus to the first touch pointer. This is
    // the id of the pointer to which the external stylus data is fused.
    std::optional<uint32_t> mFusedStylusPointerId;
    nsecs_t mExternalStylusFusionTimeout;
    bool mExternalStylusDataPending;
    // A subset of the buttons in mCurrentRawState that came from an external stylus.
    int32_t mExternalStylusButtonsApplied{0};
    // True if the current cooked pointer data was modified due to the state of an external stylus.
    bool mCurrentStreamModifiedByExternalStylus{false};

    // True if we sent a HOVER_ENTER event.
    bool mSentHoverEnter{false};

    // Have we assigned pointer IDs for this stream
    bool mHavePointerIds{false};

    // Is the current stream of direct touch events aborted
    bool mCurrentMotionAborted{false};

    // The time the primary pointer last went down.
    nsecs_t mDownTime{0};

    std::vector<VirtualKey> mVirtualKeys;

    explicit TouchInputMapper(InputDeviceContext& deviceContext,
                              const InputReaderConfiguration& readerConfig);

    virtual void dumpParameters(std::string& dump);
    virtual void configureRawPointerAxes();
    virtual void dumpRawPointerAxes(std::string& dump);
    virtual void configureInputDevice(nsecs_t when, bool* outResetNeeded);
    virtual void dumpDisplay(std::string& dump);
    virtual void configureVirtualKeys();
    virtual void dumpVirtualKeys(std::string& dump);
    virtual void parseCalibration();
    virtual void resolveCalibration();
    virtual void dumpCalibration(std::string& dump);
    virtual void updateAffineTransformation();
    virtual void dumpAffineTransformation(std::string& dump);
    virtual void resolveExternalStylusPresence();
    virtual bool hasStylus() const = 0;
    virtual bool hasExternalStylus() const;

    virtual void syncTouch(nsecs_t when, RawState* outState) = 0;

private:
    // The current viewport.
    // The components of the viewport are specified in the display's rotated orientation.
    DisplayViewport mViewport;

    // We refer to the display as being in the "natural orientation" when there is no rotation
    // applied. The display size obtained from the viewport in the natural orientation.
    // Always starts at (0, 0).
    ui::Size mDisplayBounds{ui::kInvalidSize};

    // The physical frame is the rectangle in the rotated display's coordinate space that maps to
    // the logical display frame.
    Rect mPhysicalFrameInRotatedDisplay{Rect::INVALID_RECT};

    // The orientation of the input device relative to that of the display panel. It specifies
    // the rotation of the input device coordinates required to produce the display panel
    // orientation, so it will depend on whether the device is orientation aware.
    ui::Rotation mInputDeviceOrientation{ui::ROTATION_0};

    // The transform that maps the input device's raw coordinate space to the un-rotated display's
    // coordinate space. InputReader generates events in the un-rotated display's coordinate space.
    ui::Transform mRawToDisplay;

    // The transform that maps the input device's raw coordinate space to the rotated display's
    // coordinate space. This used to perform hit-testing of raw events with the physical frame in
    // the rotated coordinate space. See mPhysicalFrameInRotatedDisplay.
    ui::Transform mRawToRotatedDisplay;

    // The transform used for non-planar raw axes, such as orientation and tilt.
    ui::Transform mRawRotation;

    float mGeometricScale;

    float mPressureScale;

    float mSizeScale;

    float mOrientationScale;

    float mDistanceScale;

    bool mHaveTilt;
    float mTiltXCenter;
    float mTiltXScale;
    float mTiltYCenter;
    float mTiltYScale;

    // Oriented motion ranges for input device info.
    struct OrientedRanges {
        InputDeviceInfo::MotionRange x;
        InputDeviceInfo::MotionRange y;
        InputDeviceInfo::MotionRange pressure;

        std::optional<InputDeviceInfo::MotionRange> size;

        std::optional<InputDeviceInfo::MotionRange> touchMajor;
        std::optional<InputDeviceInfo::MotionRange> touchMinor;

        std::optional<InputDeviceInfo::MotionRange> toolMajor;
        std::optional<InputDeviceInfo::MotionRange> toolMinor;

        std::optional<InputDeviceInfo::MotionRange> orientation;

        std::optional<InputDeviceInfo::MotionRange> distance;

        std::optional<InputDeviceInfo::MotionRange> tilt;

        void clear() {
            size = std::nullopt;
            touchMajor = std::nullopt;
            touchMinor = std::nullopt;
            toolMajor = std::nullopt;
            toolMinor = std::nullopt;
            orientation = std::nullopt;
            distance = std::nullopt;
            tilt = std::nullopt;
        }
    } mOrientedRanges;

    // Oriented dimensions and precision.
    float mOrientedXPrecision;
    float mOrientedYPrecision;

    struct CurrentVirtualKeyState {
        bool down;
        bool ignored;
        nsecs_t downTime;
        int32_t keyCode;
        int32_t scanCode;
    } mCurrentVirtualKey;

    std::optional<DisplayViewport> findViewport();

    void resetExternalStylus();
    void clearStylusDataPendingFlags();

    int32_t clampResolution(const char* axisName, int32_t resolution) const;
    void initializeOrientedRanges();
    void initializeSizeRanges();

    [[nodiscard]] std::list<NotifyArgs> sync(nsecs_t when, nsecs_t readTime);

    [[nodiscard]] std::list<NotifyArgs> consumeRawTouches(nsecs_t when, nsecs_t readTime,
                                                          uint32_t policyFlags, bool& outConsumed);
    [[nodiscard]] std::list<NotifyArgs> processRawTouches(bool timeout);
    [[nodiscard]] std::list<NotifyArgs> cookAndDispatch(nsecs_t when, nsecs_t readTime);
    [[nodiscard]] NotifyKeyArgs dispatchVirtualKey(nsecs_t when, nsecs_t readTime,
                                                   uint32_t policyFlags, int32_t keyEventAction,
                                                   int32_t keyEventFlags);

    [[nodiscard]] std::list<NotifyArgs> dispatchTouches(nsecs_t when, nsecs_t readTime,
                                                        uint32_t policyFlags);
    [[nodiscard]] std::list<NotifyArgs> dispatchHoverExit(nsecs_t when, nsecs_t readTime,
                                                          uint32_t policyFlags);
    [[nodiscard]] std::list<NotifyArgs> dispatchHoverEnterAndMove(nsecs_t when, nsecs_t readTime,
                                                                  uint32_t policyFlags);
    [[nodiscard]] std::list<NotifyArgs> dispatchButtonRelease(nsecs_t when, nsecs_t readTime,
                                                              uint32_t policyFlags);
    [[nodiscard]] std::list<NotifyArgs> dispatchButtonPress(nsecs_t when, nsecs_t readTime,
                                                            uint32_t policyFlags);
    const BitSet32& findActiveIdBits(const CookedPointerData& cookedPointerData);
    void cookPointerData();
    [[nodiscard]] std::list<NotifyArgs> abortTouches(
            nsecs_t when, nsecs_t readTime, uint32_t policyFlags,
            std::optional<ui::LogicalDisplayId> gestureDisplayId);

    // Attempts to assign a pointer id to the external stylus. Returns true if the state should be
    // withheld from further processing while waiting for data from the stylus.
    bool assignExternalStylusId(const RawState& state, bool timeout);
    void applyExternalStylusButtonState(nsecs_t when);
    void applyExternalStylusTouchState(nsecs_t when);

    // Dispatches a motion event.
    // If the changedId is >= 0 and the action is POINTER_DOWN or POINTER_UP, the
    // method will take care of setting the index and transmuting the action to DOWN or UP
    // it is the first / last pointer to go down / up.
    [[nodiscard]] NotifyMotionArgs dispatchMotion(
            nsecs_t when, nsecs_t readTime, uint32_t policyFlags, ui::LogicalDisplayId displayId,
            int32_t action, int32_t actionButton, int32_t flags, int32_t metaState,
            int32_t buttonState, const PropertiesArray& properties, const CoordsArray& coords,
            const IdToIndexArray& idToIndex, BitSet32 idBits, int32_t changedId) const;

    // Returns if this touch device is a touch screen with an associated display.
    bool isTouchScreen();

    ui::LogicalDisplayId resolveDisplayId() const;

    bool isPointInsidePhysicalFrame(int32_t x, int32_t y) const;
    const VirtualKey* findVirtualKeyHit(int32_t x, int32_t y);

    static void assignPointerIds(const RawState& last, RawState& current);

    // Compute input transforms for DIRECT and POINTER modes.
    void computeInputTransforms();
    static Parameters::DeviceType computeDeviceType(const InputDeviceContext& deviceContext);
    static Parameters computeParameters(const InputDeviceContext& deviceContext);
};

} // namespace android
