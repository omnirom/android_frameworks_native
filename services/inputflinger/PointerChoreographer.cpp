/*
 * Copyright 2023 The Android Open Source Project
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

#define LOG_TAG "PointerChoreographer"

#include <android-base/logging.h>
#include <android/configuration.h>
#include <com_android_input_flags.h>
#include <algorithm>
#if defined(__ANDROID__)
#include <gui/SurfaceComposerClient.h>
#endif
#include <input/InputFlags.h>
#include <input/Keyboard.h>
#include <input/PrintTools.h>
#include <unordered_set>

#include "PointerChoreographer.h"

#define INDENT "  "
#define INDENT2 "    "

namespace android {

namespace {

inline void notifyPointerDisplayChange(std::optional<std::tuple<ui::LogicalDisplayId, vec2>> change,
                                       PointerChoreographerPolicyInterface& policy) {
    if (!change) {
        return;
    }
    const auto& [displayId, cursorPosition] = *change;
    policy.notifyPointerDisplayIdChanged(displayId, cursorPosition);
}

void setIconForController(const std::variant<std::unique_ptr<SpriteIcon>, PointerIconStyle>& icon,
                          PointerControllerInterface& controller) {
    if (std::holds_alternative<std::unique_ptr<SpriteIcon>>(icon)) {
        if (std::get<std::unique_ptr<SpriteIcon>>(icon) == nullptr) {
            LOG(FATAL) << "SpriteIcon should not be null";
        }
        controller.setCustomPointerIcon(*std::get<std::unique_ptr<SpriteIcon>>(icon));
    } else {
        controller.updatePointerIcon(std::get<PointerIconStyle>(icon));
    }
}

// filters and returns a set of privacy sensitive displays that are currently visible.
std::unordered_set<ui::LogicalDisplayId> getPrivacySensitiveDisplaysFromWindowInfos(
        const std::vector<gui::WindowInfo>& windowInfos) {
    std::unordered_set<ui::LogicalDisplayId> privacySensitiveDisplays;
    for (const auto& windowInfo : windowInfos) {
        if (!windowInfo.inputConfig.test(gui::WindowInfo::InputConfig::NOT_VISIBLE) &&
            windowInfo.inputConfig.test(gui::WindowInfo::InputConfig::SENSITIVE_FOR_PRIVACY)) {
            privacySensitiveDisplays.insert(windowInfo.displayId);
        }
    }
    return privacySensitiveDisplays;
}

vec2 calculatePositionOnDestinationViewport(const DisplayViewport& destinationViewport,
                                            float pointerOffset,
                                            DisplayTopologyPosition sourceBoundary) {
    // destination is opposite of the source boundary
    switch (sourceBoundary) {
        case DisplayTopologyPosition::RIGHT:
            return {0, pointerOffset}; // left edge
        case DisplayTopologyPosition::TOP:
            return {pointerOffset, destinationViewport.logicalBottom}; // bottom edge
        case DisplayTopologyPosition::LEFT:
            return {destinationViewport.logicalRight, pointerOffset}; // right edge
        case DisplayTopologyPosition::BOTTOM:
            return {pointerOffset, 0}; // top edge
    }
}

// The standardised medium display density for which 1 px  = 1 dp
constexpr int32_t DENSITY_MEDIUM = ACONFIGURATION_DENSITY_MEDIUM;

inline float pxToDp(int px, int dpi) {
    return static_cast<float>(px * DENSITY_MEDIUM) / static_cast<float>(dpi);
}

inline int dpToPx(float dp, int dpi) {
    return static_cast<int>((dp * dpi) / DENSITY_MEDIUM);
}

} // namespace

// --- PointerChoreographer ---

PointerChoreographer::PointerChoreographer(InputListenerInterface& inputListener,
                                           PointerChoreographerPolicyInterface& policy)
      : PointerChoreographer(
                inputListener, policy,
                [](const sp<android::gui::WindowInfosListener>& listener) {
                    auto initialInfo = std::make_pair(std::vector<android::gui::WindowInfo>{},
                                                      std::vector<android::gui::DisplayInfo>{});
#if defined(__ANDROID__)
                    SurfaceComposerClient::getDefault()->addWindowInfosListener(listener,
                                                                                &initialInfo);
#endif
                    return initialInfo.first;
                },
                [](const sp<android::gui::WindowInfosListener>& listener) {
#if defined(__ANDROID__)
                    SurfaceComposerClient::getDefault()->removeWindowInfosListener(listener);
#endif
                }) {
}

PointerChoreographer::PointerChoreographer(
        android::InputListenerInterface& listener,
        android::PointerChoreographerPolicyInterface& policy,
        const android::PointerChoreographer::WindowListenerRegisterConsumer& registerListener,
        const android::PointerChoreographer::WindowListenerUnregisterConsumer& unregisterListener)
      : mTouchControllerConstructor([this]() {
            return mPolicy.createPointerController(
                    PointerControllerInterface::ControllerType::TOUCH);
        }),
        mNextListener(listener),
        mPolicy(policy),
        mCurrentMouseDisplayId(ui::LogicalDisplayId::INVALID),
        mNotifiedPointerDisplayId(ui::LogicalDisplayId::INVALID),
        mShowTouchesEnabled(false),
        mStylusPointerIconEnabled(false),
        mPointerMotionFilterEnabled(false),
        mCurrentFocusedDisplay(ui::LogicalDisplayId::DEFAULT),
        mIsWindowInfoListenerRegistered(false),
        mWindowInfoListener(sp<PointerChoreographerDisplayInfoListener>::make(this)),
        mRegisterListener(registerListener),
        mUnregisterListener(unregisterListener) {}

PointerChoreographer::~PointerChoreographer() {
    if (mIsWindowInfoListenerRegistered) {
        mUnregisterListener(mWindowInfoListener);
        mIsWindowInfoListenerRegistered = false;
    }
    mWindowInfoListener->onPointerChoreographerDestroyed();
}

void PointerChoreographer::notifyInputDevicesChanged(const NotifyInputDevicesChangedArgs& args) {
    PointerDisplayChange pointerDisplayChange;

    { // acquire lock
        std::scoped_lock _l(getLock());

        mInputDeviceInfos = args.inputDeviceInfos;
        pointerDisplayChange = updatePointerControllersLocked();
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
    mNextListener.notify(args);
}

void PointerChoreographer::notifyKey(const NotifyKeyArgs& args) {
    fadeMouseCursorOnKeyPress(args);
    mNextListener.notify(args);
}

void PointerChoreographer::notifyMotion(const NotifyMotionArgs& args) {
    NotifyMotionArgs newArgs = processMotion(args);

    mNextListener.notify(newArgs);
}

void PointerChoreographer::fadeMouseCursorOnKeyPress(const android::NotifyKeyArgs& args) {
    if (args.action == AKEY_EVENT_ACTION_UP || isMetaKey(args.keyCode)) {
        return;
    }
    // Meta state for these keys is ignored for dismissing cursor while typing
    constexpr static int32_t ALLOW_FADING_META_STATE_MASK = AMETA_CAPS_LOCK_ON | AMETA_NUM_LOCK_ON |
            AMETA_SCROLL_LOCK_ON | AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_RIGHT_ON | AMETA_SHIFT_ON;
    if (args.metaState & ~ALLOW_FADING_META_STATE_MASK) {
        // Do not fade if any other meta state is active
        return;
    }
    if (!mPolicy.isInputMethodConnectionActive()) {
        return;
    }

    std::scoped_lock _l(getLock());
    ui::LogicalDisplayId targetDisplay = args.displayId;
    if (targetDisplay == ui::LogicalDisplayId::INVALID) {
        targetDisplay = mCurrentFocusedDisplay;
    }
    auto it = mMousePointersByDisplay.find(targetDisplay);
    if (it != mMousePointersByDisplay.end()) {
        mPolicy.notifyMouseCursorFadedOnTyping();
        it->second->fade(PointerControllerInterface::Transition::GRADUAL);
    }
}

NotifyMotionArgs PointerChoreographer::processMotion(const NotifyMotionArgs& args) {
    NotifyMotionArgs newArgs(args);
    PointerDisplayChange pointerDisplayChange;
    { // acquire lock
        std::scoped_lock _l(getLock());
        if (isFromMouse(args.source, args.pointerProperties[0].toolType)) {
            newArgs = processMouseEventLocked(args);
            pointerDisplayChange = calculatePointerDisplayChangeToNotify();
        } else if (isFromTouchpad(args.source, args.pointerProperties[0].toolType)) {
            newArgs = processTouchpadEventLocked(args);
            pointerDisplayChange = calculatePointerDisplayChangeToNotify();
        } else if (isFromDrawingTablet(args.source, args.pointerProperties[0].toolType)) {
            processDrawingTabletEventLocked(args);
        } else if (mStylusPointerIconEnabled &&
                   isStylusHoverEvent(args.source, args.pointerProperties, args.action)) {
            processStylusHoverEventLocked(args);
        } else if (isFromSource(args.source, AINPUT_SOURCE_TOUCHSCREEN)) {
            processTouchscreenAndStylusEventLocked(args);
        }
    } // release lock

    if (pointerDisplayChange) {
        // pointer display may have changed if mouse crossed display boundary
        notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
    }
    return newArgs;
}

NotifyMotionArgs PointerChoreographer::processMouseEventLocked(const NotifyMotionArgs& args) {
    if (args.getPointerCount() != 1) {
        LOG(FATAL) << "Only mouse events with a single pointer are currently supported: "
                   << args.dump();
    }

    mMouseDevices.emplace(args.deviceId);
    auto [displayId, pc] = ensureMouseControllerLocked(args.displayId);
    NotifyMotionArgs newArgs(args);
    newArgs.displayId = displayId;

    if (MotionEvent::isValidCursorPosition(args.xCursorPosition, args.yCursorPosition)) {
        // This is an absolute mouse device that knows about the location of the cursor on the
        // display, so set the cursor position to the specified location.
        const auto position = pc.getPosition();
        const float deltaX = args.xCursorPosition - position.x;
        const float deltaY = args.yCursorPosition - position.y;
        newArgs.pointerCoords[0].setAxisValue(AMOTION_EVENT_AXIS_RELATIVE_X, deltaX);
        newArgs.pointerCoords[0].setAxisValue(AMOTION_EVENT_AXIS_RELATIVE_Y, deltaY);
        pc.setPosition(args.xCursorPosition, args.yCursorPosition);
    } else {
        // This is a relative mouse, so move the cursor by the specified amount.
        processPointerDeviceMotionEventLocked(/*byref*/ newArgs, /*byref*/ pc);
    }
    // Note displayId may have changed if the cursor moved to a different display
    if (canUnfadeOnDisplay(newArgs.displayId)) {
        pc.unfade(PointerControllerInterface::Transition::IMMEDIATE);
    }
    return newArgs;
}

NotifyMotionArgs PointerChoreographer::processTouchpadEventLocked(const NotifyMotionArgs& args) {
    mMouseDevices.emplace(args.deviceId);
    auto [displayId, pc] = ensureMouseControllerLocked(args.displayId);

    NotifyMotionArgs newArgs(args);
    newArgs.displayId = displayId;
    if (args.getPointerCount() == 1 && args.classification == MotionClassification::NONE) {
        // This is a movement of the mouse pointer.
        processPointerDeviceMotionEventLocked(/*byref*/ newArgs, /*byref*/ pc);
    } else {
        // This is a trackpad gesture with fake finger(s) that should not move the mouse pointer.
        const auto position = pc.getPosition();
        for (uint32_t i = 0; i < newArgs.getPointerCount(); i++) {
            newArgs.pointerCoords[i].setAxisValue(AMOTION_EVENT_AXIS_X,
                                                  args.pointerCoords[i].getX() + position.x);
            newArgs.pointerCoords[i].setAxisValue(AMOTION_EVENT_AXIS_Y,
                                                  args.pointerCoords[i].getY() + position.y);
        }
        newArgs.xCursorPosition = position.x;
        newArgs.yCursorPosition = position.y;
    }

    // Note displayId may have changed if the cursor moved to a different display
    if (canUnfadeOnDisplay(newArgs.displayId)) {
        pc.unfade(PointerControllerInterface::Transition::IMMEDIATE);
    }
    return newArgs;
}

void PointerChoreographer::processPointerDeviceMotionEventLocked(NotifyMotionArgs& newArgs,
                                                                 PointerControllerInterface& pc) {
    const float deltaX = newArgs.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_RELATIVE_X);
    const float deltaY = newArgs.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_RELATIVE_Y);
    vec2 filteredDelta =
            filterPointerMotionForAccessibilityLocked(pc.getPosition(), vec2{deltaX, deltaY},
                                                      newArgs.displayId);
    vec2 unconsumedDelta = pc.move(filteredDelta.x, filteredDelta.y);
    if (InputFlags::connectedDisplaysCursorEnabled() &&
        (std::abs(unconsumedDelta.x) > 0 || std::abs(unconsumedDelta.y) > 0)) {
        handleUnconsumedDeltaLocked(pc, unconsumedDelta);
        // pointer may have moved to a different viewport
        newArgs.displayId = pc.getDisplayId();
    }

    const auto position = pc.getPosition();
    newArgs.pointerCoords[0].setAxisValue(AMOTION_EVENT_AXIS_X, position.x);
    newArgs.pointerCoords[0].setAxisValue(AMOTION_EVENT_AXIS_Y, position.y);
    newArgs.xCursorPosition = position.x;
    newArgs.yCursorPosition = position.y;
}

void PointerChoreographer::handleUnconsumedDeltaLocked(PointerControllerInterface& pc,
                                                       const vec2& unconsumedDelta) {
    // Display topology is in rotated coordinate space and Pointer controller returns and expects
    // values in the un-rotated coordinate space. So we need to transform delta and cursor position
    // back to the rotated coordinate space to lookup adjacent display in the display topology.
    const auto& sourceDisplayTransform = pc.getDisplayTransform();
    const vec2 rotatedUnconsumedDelta =
            transformWithoutTranslation(sourceDisplayTransform, unconsumedDelta);
    const vec2 cursorPosition = pc.getPosition();
    const vec2 rotatedCursorPosition = sourceDisplayTransform.transform(cursorPosition);

    // To find out the boundary that cursor is crossing we are checking delta in x and y direction
    // respectively. This prioritizes x direction over y.
    // In practise, majority of cases we only have non-zero values in either x or y coordinates,
    // except sometimes near the corners.
    // In these cases this behaviour is not noticeable. We also do not apply unconsumed delta on
    // the destination display for the same reason.
    DisplayTopologyPosition sourceBoundary;
    float cursorOffset = 0.0f;
    if (rotatedUnconsumedDelta.x > 0) {
        sourceBoundary = DisplayTopologyPosition::RIGHT;
        cursorOffset = rotatedCursorPosition.y;
    } else if (rotatedUnconsumedDelta.x < 0) {
        sourceBoundary = DisplayTopologyPosition::LEFT;
        cursorOffset = rotatedCursorPosition.y;
    } else if (rotatedUnconsumedDelta.y > 0) {
        sourceBoundary = DisplayTopologyPosition::BOTTOM;
        cursorOffset = rotatedCursorPosition.x;
    } else {
        sourceBoundary = DisplayTopologyPosition::TOP;
        cursorOffset = rotatedCursorPosition.x;
    }

    const ui::LogicalDisplayId sourceDisplayId = pc.getDisplayId();
    std::optional<std::pair<const DisplayViewport*, float /*offset*/>> destination =
            findDestinationDisplayLocked(sourceDisplayId, sourceBoundary, cursorOffset);
    if (!destination.has_value()) {
        // No matching adjacent display
        return;
    }

    const DisplayViewport& destinationViewport = *destination->first;
    const float destinationOffset = destination->second;
    if (mMousePointersByDisplay.find(destinationViewport.displayId) !=
        mMousePointersByDisplay.end()) {
        LOG(FATAL) << "A cursor already exists on destination display"
                   << destinationViewport.displayId;
    }
    mCurrentMouseDisplayId = destinationViewport.displayId;
    auto pcNode = mMousePointersByDisplay.extract(sourceDisplayId);
    pcNode.key() = destinationViewport.displayId;
    mMousePointersByDisplay.insert(std::move(pcNode));

    // Before updating the viewport and moving the cursor to appropriate location in the destination
    // viewport, we need to temporarily hide the cursor. This will prevent it from appearing at the
    // center of the display in any intermediate frames.
    pc.fade(PointerControllerInterface::Transition::IMMEDIATE);
    pc.setDisplayViewport(destinationViewport);
    vec2 destinationPosition =
            calculatePositionOnDestinationViewport(destinationViewport, destinationOffset,
                                                   sourceBoundary);

    // Transform position back to un-rotated coordinate space before sending it to controller
    destinationPosition = pc.getDisplayTransform().inverse().transform(destinationPosition.x,
                                                                       destinationPosition.y);
    pc.setPosition(destinationPosition.x, destinationPosition.y);
    pc.unfade(PointerControllerInterface::Transition::IMMEDIATE);
}

void PointerChoreographer::processDrawingTabletEventLocked(const android::NotifyMotionArgs& args) {
    if (args.displayId == ui::LogicalDisplayId::INVALID) {
        return;
    }

    if (args.getPointerCount() != 1) {
        LOG(WARNING) << "Only drawing tablet events with a single pointer are currently supported: "
                     << args.dump();
    }

    // Use a mouse pointer controller for drawing tablets, or create one if it doesn't exist.
    auto [it, controllerAdded] =
            mDrawingTabletPointersByDevice.try_emplace(args.deviceId,
                                                       getMouseControllerConstructor(
                                                               args.displayId));
    if (controllerAdded) {
        onControllerAddedOrRemovedLocked();
    }

    PointerControllerInterface& pc = *it->second;

    const float x = args.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X);
    const float y = args.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y);
    pc.setPosition(x, y);
    if (args.action == AMOTION_EVENT_ACTION_HOVER_EXIT) {
        // TODO(b/315815559): Do not fade and reset the icon if the hover exit will be followed
        //   immediately by a DOWN event.
        pc.fade(PointerControllerInterface::Transition::IMMEDIATE);
        pc.updatePointerIcon(PointerIconStyle::TYPE_NOT_SPECIFIED);
    } else if (canUnfadeOnDisplay(args.displayId)) {
        pc.unfade(PointerControllerInterface::Transition::IMMEDIATE);
    }
}

/**
 * When screen is touched, fade the mouse pointer on that display. We only call fade for
 * ACTION_DOWN events.This would allow both mouse and touch to be used at the same time if the
 * mouse device keeps moving and unfades the cursor.
 * For touch events, we do not need to populate the cursor position.
 */
void PointerChoreographer::processTouchscreenAndStylusEventLocked(const NotifyMotionArgs& args) {
    if (!args.displayId.isValid()) {
        return;
    }

    if (const auto it = mMousePointersByDisplay.find(args.displayId);
        it != mMousePointersByDisplay.end() && args.action == AMOTION_EVENT_ACTION_DOWN) {
        it->second->fade(PointerControllerInterface::Transition::GRADUAL);
    }

    if (!mShowTouchesEnabled) {
        return;
    }

    // Get the touch pointer controller for the device, or create one if it doesn't exist.
    auto [it, controllerAdded] =
            mTouchPointersByDevice.try_emplace(args.deviceId, mTouchControllerConstructor);
    if (controllerAdded) {
        onControllerAddedOrRemovedLocked();
    }

    PointerControllerInterface& pc = *it->second;

    const PointerCoords* coords = args.pointerCoords.data();
    const int32_t maskedAction = MotionEvent::getActionMasked(args.action);
    const uint8_t actionIndex = MotionEvent::getActionIndex(args.action);
    std::array<uint32_t, MAX_POINTER_ID + 1> idToIndex;
    BitSet32 idBits;
    if (maskedAction != AMOTION_EVENT_ACTION_UP && maskedAction != AMOTION_EVENT_ACTION_CANCEL &&
        maskedAction != AMOTION_EVENT_ACTION_HOVER_EXIT) {
        for (size_t i = 0; i < args.getPointerCount(); i++) {
            if (maskedAction == AMOTION_EVENT_ACTION_POINTER_UP && actionIndex == i) {
                continue;
            }
            uint32_t id = args.pointerProperties[i].id;
            idToIndex[id] = i;
            idBits.markBit(id);
        }
    }
    // The PointerController already handles setting spots per-display, so
    // we do not need to manually manage display changes for touch spots for now.
    pc.setSpots(coords, idToIndex.cbegin(), idBits, args.displayId);
}

void PointerChoreographer::processStylusHoverEventLocked(const NotifyMotionArgs& args) {
    if (!args.displayId.isValid()) {
        return;
    }

    if (args.getPointerCount() != 1) {
        LOG(WARNING) << "Only stylus hover events with a single pointer are currently supported: "
                     << args.dump();
    }

    // Fade the mouse pointer on the display if there is one when the stylus starts hovering.
    if (args.action == AMOTION_EVENT_ACTION_HOVER_ENTER) {
        if (const auto it = mMousePointersByDisplay.find(args.displayId);
            it != mMousePointersByDisplay.end()) {
            it->second->fade(PointerControllerInterface::Transition::GRADUAL);
        }
    }

    // Get the stylus pointer controller for the device, or create one if it doesn't exist.
    auto [it, controllerAdded] =
            mStylusPointersByDevice.try_emplace(args.deviceId,
                                                getStylusControllerConstructor(args.displayId));
    if (controllerAdded) {
        onControllerAddedOrRemovedLocked();
    }

    PointerControllerInterface& pc = *it->second;

    const float x = args.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X);
    const float y = args.pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y);
    pc.setPosition(x, y);
    if (args.action == AMOTION_EVENT_ACTION_HOVER_EXIT) {
        // TODO(b/315815559): Do not fade and reset the icon if the hover exit will be followed
        //   immediately by a DOWN event.
        pc.fade(PointerControllerInterface::Transition::IMMEDIATE);
        pc.updatePointerIcon(mShowTouchesEnabled ? PointerIconStyle::TYPE_SPOT_HOVER
                                                 : PointerIconStyle::TYPE_NOT_SPECIFIED);
    } else if (canUnfadeOnDisplay(args.displayId)) {
        pc.unfade(PointerControllerInterface::Transition::IMMEDIATE);
    }
}

void PointerChoreographer::notifySwitch(const NotifySwitchArgs& args) {
    mNextListener.notify(args);
}

void PointerChoreographer::notifySensor(const NotifySensorArgs& args) {
    mNextListener.notify(args);
}

void PointerChoreographer::notifyVibratorState(const NotifyVibratorStateArgs& args) {
    mNextListener.notify(args);
}

void PointerChoreographer::notifyDeviceReset(const NotifyDeviceResetArgs& args) {
    processDeviceReset(args);

    mNextListener.notify(args);
}

void PointerChoreographer::processDeviceReset(const NotifyDeviceResetArgs& args) {
    std::scoped_lock _l(getLock());
    mTouchPointersByDevice.erase(args.deviceId);
    mStylusPointersByDevice.erase(args.deviceId);
    mDrawingTabletPointersByDevice.erase(args.deviceId);
    onControllerAddedOrRemovedLocked();
}

void PointerChoreographer::onControllerAddedOrRemovedLocked() {
    bool requireListener = !mTouchPointersByDevice.empty() || !mMousePointersByDisplay.empty() ||
            !mDrawingTabletPointersByDevice.empty() || !mStylusPointersByDevice.empty();

    // PointerChoreographer uses Listener's lock which is already held by caller
    base::ScopedLockAssertion assumeLocked(mWindowInfoListener->mLock);

    if (requireListener && !mIsWindowInfoListenerRegistered) {
        mIsWindowInfoListenerRegistered = true;
        mWindowInfoListener->setInitialDisplayInfosLocked(mRegisterListener(mWindowInfoListener));
        onPrivacySensitiveDisplaysChangedLocked(
                mWindowInfoListener->getPrivacySensitiveDisplaysLocked());
    } else if (!requireListener && mIsWindowInfoListenerRegistered) {
        mIsWindowInfoListenerRegistered = false;
        mUnregisterListener(mWindowInfoListener);
    } else if (requireListener) {
        // controller may have been added to an existing privacy sensitive display, we need to
        // update all controllers again
        onPrivacySensitiveDisplaysChangedLocked(
                mWindowInfoListener->getPrivacySensitiveDisplaysLocked());
    }
}

void PointerChoreographer::onPrivacySensitiveDisplaysChangedLocked(
        const std::unordered_set<ui::LogicalDisplayId>& privacySensitiveDisplays) {
    for (auto& [_, pc] : mTouchPointersByDevice) {
        pc->clearSkipScreenshotFlags();
        for (auto displayId : privacySensitiveDisplays) {
            pc->setSkipScreenshotFlagForDisplay(displayId);
        }
    }

    for (auto& [displayId, pc] : mMousePointersByDisplay) {
        if (privacySensitiveDisplays.find(displayId) != privacySensitiveDisplays.end()) {
            pc->setSkipScreenshotFlagForDisplay(displayId);
        } else {
            pc->clearSkipScreenshotFlags();
        }
    }

    for (auto* pointerControllerByDevice :
         {&mDrawingTabletPointersByDevice, &mStylusPointersByDevice}) {
        for (auto& [_, pc] : *pointerControllerByDevice) {
            auto displayId = pc->getDisplayId();
            if (privacySensitiveDisplays.find(displayId) != privacySensitiveDisplays.end()) {
                pc->setSkipScreenshotFlagForDisplay(displayId);
            } else {
                pc->clearSkipScreenshotFlags();
            }
        }
    }
}

void PointerChoreographer::notifyPointerCaptureChanged(
        const NotifyPointerCaptureChangedArgs& args) {
    if (args.request.isEnable()) {
        std::scoped_lock _l(getLock());
        for (const auto& [_, mousePointerController] : mMousePointersByDisplay) {
            mousePointerController->fade(PointerControllerInterface::Transition::IMMEDIATE);
        }
    }
    mNextListener.notify(args);
}

void PointerChoreographer::setDisplayTopology(const DisplayTopologyGraph& displayTopologyGraph) {
    PointerDisplayChange pointerDisplayChange;
    { // acquire lock
        std::scoped_lock _l(getLock());
        mTopology = displayTopologyGraph;

        // make primary display default mouse display, if it was not set or
        // the existing display was removed
        if (mCurrentMouseDisplayId == ui::LogicalDisplayId::INVALID ||
            mTopology.graph.find(mCurrentMouseDisplayId) == mTopology.graph.end()) {
            mCurrentMouseDisplayId = mTopology.primaryDisplayId;
            pointerDisplayChange = updatePointerControllersLocked();
        }
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
}

void PointerChoreographer::dump(std::string& dump) {
    std::scoped_lock _l(getLock());

    dump += "PointerChoreographer:\n";
    dump += StringPrintf(INDENT "Show Touches Enabled: %s\n",
                         mShowTouchesEnabled ? "true" : "false");
    dump += StringPrintf(INDENT "Stylus PointerIcon Enabled: %s\n",
                         mStylusPointerIconEnabled ? "true" : "false");
    dump += StringPrintf(INDENT "Accessibility Pointer Motion Filter Enabled: %s\n",
                         mPointerMotionFilterEnabled ? "true" : "false");

    dump += INDENT "MousePointerControllers:\n";
    for (const auto& [displayId, mousePointerController] : mMousePointersByDisplay) {
        std::string pointerControllerDump = addLinePrefix(mousePointerController->dump(), INDENT);
        dump += INDENT + displayId.toString() + " : " + pointerControllerDump;
    }
    dump += INDENT "TouchPointerControllers:\n";
    for (const auto& [deviceId, touchPointerController] : mTouchPointersByDevice) {
        std::string pointerControllerDump = addLinePrefix(touchPointerController->dump(), INDENT);
        dump += INDENT + std::to_string(deviceId) + " : " + pointerControllerDump;
    }
    dump += INDENT "StylusPointerControllers:\n";
    for (const auto& [deviceId, stylusPointerController] : mStylusPointersByDevice) {
        std::string pointerControllerDump = addLinePrefix(stylusPointerController->dump(), INDENT);
        dump += INDENT + std::to_string(deviceId) + " : " + pointerControllerDump;
    }
    dump += INDENT "DrawingTabletControllers:\n";
    for (const auto& [deviceId, drawingTabletController] : mDrawingTabletPointersByDevice) {
        std::string pointerControllerDump = addLinePrefix(drawingTabletController->dump(), INDENT);
        dump += INDENT + std::to_string(deviceId) + " : " + pointerControllerDump;
    }
    dump += INDENT "DisplayTopologyGraph:\n";
    dump += addLinePrefix(mTopology.dump(), INDENT2);
    dump += "\n";
}

const DisplayViewport* PointerChoreographer::findViewportByIdLocked(
        ui::LogicalDisplayId displayId) const {
    for (auto& viewport : mViewports) {
        if (viewport.displayId == displayId) {
            return &viewport;
        }
    }
    return nullptr;
}

ui::LogicalDisplayId PointerChoreographer::getTargetMouseDisplayLocked(
        ui::LogicalDisplayId associatedDisplayId) const {
    if (!InputFlags::connectedDisplaysCursorAndAssociatedDisplayCursorBugfixEnabled()) {
        if (associatedDisplayId.isValid()) {
            return associatedDisplayId;
        }
        return mCurrentMouseDisplayId.isValid() ? mCurrentMouseDisplayId
                                                : ui::LogicalDisplayId::DEFAULT;
    }
    // Associated display is not included in the topology, return this associated display.
    if (associatedDisplayId.isValid() &&
        mTopology.graph.find(associatedDisplayId) == mTopology.graph.end()) {
        return associatedDisplayId;
    }
    if (mCurrentMouseDisplayId.isValid()) {
        return mCurrentMouseDisplayId;
    }
    if (mTopology.primaryDisplayId.isValid()) {
        return mTopology.primaryDisplayId;
    }
    return ui::LogicalDisplayId::DEFAULT;
}

std::pair<ui::LogicalDisplayId, PointerControllerInterface&>
PointerChoreographer::ensureMouseControllerLocked(ui::LogicalDisplayId associatedDisplayId) {
    const ui::LogicalDisplayId displayId = getTargetMouseDisplayLocked(associatedDisplayId);

    auto it = mMousePointersByDisplay.find(displayId);
    if (it == mMousePointersByDisplay.end()) {
        it = mMousePointersByDisplay.emplace(displayId, getMouseControllerConstructor(displayId))
                     .first;
        onControllerAddedOrRemovedLocked();
    }

    return {displayId, *it->second};
}

InputDeviceInfo* PointerChoreographer::findInputDeviceLocked(DeviceId deviceId) {
    auto it = std::find_if(mInputDeviceInfos.begin(), mInputDeviceInfos.end(),
                           [deviceId](const auto& info) { return info.getId() == deviceId; });
    return it != mInputDeviceInfos.end() ? &(*it) : nullptr;
}

bool PointerChoreographer::canUnfadeOnDisplay(ui::LogicalDisplayId displayId) {
    return mDisplaysWithPointersHidden.find(displayId) == mDisplaysWithPointersHidden.end();
}

std::mutex& PointerChoreographer::getLock() const {
    return mWindowInfoListener->mLock;
}

PointerChoreographer::PointerDisplayChange PointerChoreographer::updatePointerControllersLocked() {
    std::set<ui::LogicalDisplayId /*displayId*/> mouseDisplaysToKeep;
    std::set<DeviceId> touchDevicesToKeep;
    std::set<DeviceId> stylusDevicesToKeep;
    std::set<DeviceId> drawingTabletDevicesToKeep;

    // Mark the displayIds or deviceIds of PointerControllers currently needed, and create
    // new PointerControllers if necessary.
    for (const auto& info : mInputDeviceInfos) {
        if (!info.isEnabled()) {
            // If device is disabled, we should not keep it, and should not show pointer for
            // disabled mouse device.
            continue;
        }
        const uint32_t sources = info.getSources();
        const bool isKnownMouse = mMouseDevices.count(info.getId()) != 0;

        if (isMouseOrTouchpad(sources) || isKnownMouse) {
            const ui::LogicalDisplayId displayId =
                    getTargetMouseDisplayLocked(info.getAssociatedDisplayId());
            mouseDisplaysToKeep.insert(displayId);
            // For mice, show the cursor immediately when the device is first connected or
            // when it moves to a new display.
            auto [mousePointerIt, isNewMousePointer] =
                    mMousePointersByDisplay.try_emplace(displayId,
                                                        getMouseControllerConstructor(displayId));
            if (isNewMousePointer) {
                onControllerAddedOrRemovedLocked();
            }

            mMouseDevices.emplace(info.getId());
            if ((!isKnownMouse || isNewMousePointer) && canUnfadeOnDisplay(displayId)) {
                mousePointerIt->second->unfade(PointerControllerInterface::Transition::IMMEDIATE);
            }
        }
        if (isFromSource(sources, AINPUT_SOURCE_TOUCHSCREEN) && mShowTouchesEnabled &&
            info.getAssociatedDisplayId().isValid()) {
            touchDevicesToKeep.insert(info.getId());
        }
        if (isFromSource(sources, AINPUT_SOURCE_STYLUS) && mStylusPointerIconEnabled &&
            info.getAssociatedDisplayId().isValid()) {
            stylusDevicesToKeep.insert(info.getId());
        }
        if (isFromSource(sources, AINPUT_SOURCE_STYLUS | AINPUT_SOURCE_MOUSE) &&
            info.getAssociatedDisplayId().isValid()) {
            drawingTabletDevicesToKeep.insert(info.getId());
        }
    }

    // Remove PointerControllers no longer needed.
    std::erase_if(mMousePointersByDisplay, [&mouseDisplaysToKeep](const auto& pair) {
        return mouseDisplaysToKeep.find(pair.first) == mouseDisplaysToKeep.end();
    });
    std::erase_if(mTouchPointersByDevice, [&touchDevicesToKeep](const auto& pair) {
        return touchDevicesToKeep.find(pair.first) == touchDevicesToKeep.end();
    });
    std::erase_if(mStylusPointersByDevice, [&stylusDevicesToKeep](const auto& pair) {
        return stylusDevicesToKeep.find(pair.first) == stylusDevicesToKeep.end();
    });
    std::erase_if(mDrawingTabletPointersByDevice, [&drawingTabletDevicesToKeep](const auto& pair) {
        return drawingTabletDevicesToKeep.find(pair.first) == drawingTabletDevicesToKeep.end();
    });
    std::erase_if(mMouseDevices, [&](DeviceId id) REQUIRES(getLock()) {
        return std::find_if(mInputDeviceInfos.begin(), mInputDeviceInfos.end(),
                            [id](const auto& info) { return info.getId() == id; }) ==
                mInputDeviceInfos.end();
    });

    onControllerAddedOrRemovedLocked();

    // Check if we need to notify the policy if there's a change on the pointer display ID.
    return calculatePointerDisplayChangeToNotify();
}

PointerChoreographer::PointerDisplayChange
PointerChoreographer::calculatePointerDisplayChangeToNotify() {
    ui::LogicalDisplayId displayIdToNotify = ui::LogicalDisplayId::INVALID;
    vec2 cursorPosition = {0, 0};
    if (const auto it =
                mMousePointersByDisplay.find(getTargetMouseDisplayLocked(mCurrentMouseDisplayId));
        it != mMousePointersByDisplay.end()) {
        const auto& pointerController = it->second;
        // Use the displayId from the pointerController, because it accurately reflects whether
        // the viewport has been added for that display. Otherwise, we would have to check if
        // the viewport exists separately.
        displayIdToNotify = pointerController->getDisplayId();
        cursorPosition = pointerController->getPosition();
    }
    if (mNotifiedPointerDisplayId == displayIdToNotify) {
        return {};
    }
    mNotifiedPointerDisplayId = displayIdToNotify;
    return {{displayIdToNotify, cursorPosition}};
}

void PointerChoreographer::setDefaultMouseDisplayId(ui::LogicalDisplayId displayId) {
    if (InputFlags::connectedDisplaysCursorEnabled()) {
        // In connected displays scenario, default mouse display will only be updated from topology.
        return;
    }
    PointerDisplayChange pointerDisplayChange;

    { // acquire lock
        std::scoped_lock _l(getLock());

        mCurrentMouseDisplayId = displayId;
        pointerDisplayChange = updatePointerControllersLocked();
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
}

void PointerChoreographer::setDisplayViewports(const std::vector<DisplayViewport>& viewports) {
    PointerDisplayChange pointerDisplayChange;

    { // acquire lock
        std::scoped_lock _l(getLock());
        for (const auto& viewport : viewports) {
            const ui::LogicalDisplayId displayId = viewport.displayId;
            if (const auto it = mMousePointersByDisplay.find(displayId);
                it != mMousePointersByDisplay.end()) {
                it->second->setDisplayViewport(viewport);
            }
            for (const auto& [deviceId, stylusPointerController] : mStylusPointersByDevice) {
                const InputDeviceInfo* info = findInputDeviceLocked(deviceId);
                if (info && info->getAssociatedDisplayId() == displayId) {
                    stylusPointerController->setDisplayViewport(viewport);
                }
            }
            for (const auto& [deviceId, drawingTabletController] : mDrawingTabletPointersByDevice) {
                const InputDeviceInfo* info = findInputDeviceLocked(deviceId);
                if (info && info->getAssociatedDisplayId() == displayId) {
                    drawingTabletController->setDisplayViewport(viewport);
                }
            }
        }
        mViewports = viewports;
        pointerDisplayChange = calculatePointerDisplayChangeToNotify();
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
}

std::optional<DisplayViewport> PointerChoreographer::getViewportForPointerDevice(
        ui::LogicalDisplayId associatedDisplayId) {
    std::scoped_lock _l(getLock());
    const ui::LogicalDisplayId resolvedDisplayId = getTargetMouseDisplayLocked(associatedDisplayId);
    if (const auto viewport = findViewportByIdLocked(resolvedDisplayId); viewport) {
        return *viewport;
    }
    return std::nullopt;
}

vec2 PointerChoreographer::getMouseCursorPosition(ui::LogicalDisplayId displayId) {
    std::scoped_lock _l(getLock());
    const ui::LogicalDisplayId resolvedDisplayId = getTargetMouseDisplayLocked(displayId);
    if (auto it = mMousePointersByDisplay.find(resolvedDisplayId);
        it != mMousePointersByDisplay.end()) {
        return it->second->getPosition();
    }
    return {AMOTION_EVENT_INVALID_CURSOR_POSITION, AMOTION_EVENT_INVALID_CURSOR_POSITION};
}

void PointerChoreographer::setShowTouchesEnabled(bool enabled) {
    PointerDisplayChange pointerDisplayChange;

    { // acquire lock
        std::scoped_lock _l(getLock());
        if (mShowTouchesEnabled == enabled) {
            return;
        }
        mShowTouchesEnabled = enabled;
        pointerDisplayChange = updatePointerControllersLocked();
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
}

void PointerChoreographer::setStylusPointerIconEnabled(bool enabled) {
    PointerDisplayChange pointerDisplayChange;

    { // acquire lock
        std::scoped_lock _l(getLock());
        if (mStylusPointerIconEnabled == enabled) {
            return;
        }
        mStylusPointerIconEnabled = enabled;
        pointerDisplayChange = updatePointerControllersLocked();
    } // release lock

    notifyPointerDisplayChange(pointerDisplayChange, mPolicy);
}

bool PointerChoreographer::setPointerIcon(
        std::variant<std::unique_ptr<SpriteIcon>, PointerIconStyle> icon,
        ui::LogicalDisplayId displayId, DeviceId deviceId) {
    std::scoped_lock _l(getLock());
    if (deviceId < 0) {
        LOG(WARNING) << "Invalid device id " << deviceId << ". Cannot set pointer icon.";
        return false;
    }
    const InputDeviceInfo* info = findInputDeviceLocked(deviceId);
    if (!info) {
        LOG(WARNING) << "No input device info found for id " << deviceId
                     << ". Cannot set pointer icon.";
        return false;
    }
    const uint32_t sources = info->getSources();

    if (isFromSource(sources, AINPUT_SOURCE_STYLUS | AINPUT_SOURCE_MOUSE)) {
        auto it = mDrawingTabletPointersByDevice.find(deviceId);
        if (it != mDrawingTabletPointersByDevice.end()) {
            setIconForController(icon, *it->second);
            return true;
        }
    }
    if (isFromSource(sources, AINPUT_SOURCE_STYLUS)) {
        auto it = mStylusPointersByDevice.find(deviceId);
        if (it != mStylusPointersByDevice.end()) {
            if (mShowTouchesEnabled) {
                // If an app doesn't override the icon for the hovering stylus, show the hover icon.
                auto* style = std::get_if<PointerIconStyle>(&icon);
                if (style != nullptr && *style == PointerIconStyle::TYPE_NOT_SPECIFIED) {
                    *style = PointerIconStyle::TYPE_SPOT_HOVER;
                }
            }
            setIconForController(icon, *it->second);
            return true;
        }
    }
    if (isFromSource(sources, AINPUT_SOURCE_MOUSE)) {
        auto it = mMousePointersByDisplay.find(displayId);
        if (it != mMousePointersByDisplay.end()) {
            setIconForController(icon, *it->second);
            return true;
        } else {
            LOG(WARNING) << "No mouse pointer controller found for display " << displayId
                         << ", device " << deviceId << ".";
            return false;
        }
    }
    LOG(WARNING) << "Cannot set pointer icon for display " << displayId << ", device " << deviceId
                 << ".";
    return false;
}

void PointerChoreographer::setPointerIconVisibility(ui::LogicalDisplayId displayId, bool visible) {
    std::scoped_lock lock(getLock());
    if (visible) {
        mDisplaysWithPointersHidden.erase(displayId);
        // We do not unfade the icons here, because we don't know when the last event happened.
        return;
    }

    mDisplaysWithPointersHidden.emplace(displayId);

    // Hide any icons that are currently visible on the display.
    if (auto it = mMousePointersByDisplay.find(displayId); it != mMousePointersByDisplay.end()) {
        const auto& [_, controller] = *it;
        controller->fade(PointerControllerInterface::Transition::IMMEDIATE);
    }
    for (const auto& [_, controller] : mStylusPointersByDevice) {
        if (controller->getDisplayId() == displayId) {
            controller->fade(PointerControllerInterface::Transition::IMMEDIATE);
        }
    }
}

void PointerChoreographer::setFocusedDisplay(ui::LogicalDisplayId displayId) {
    std::scoped_lock lock(getLock());
    mCurrentFocusedDisplay = displayId;
}

void PointerChoreographer::setAccessibilityPointerMotionFilterEnabled(bool enabled) {
    std::scoped_lock _l(getLock());
    mPointerMotionFilterEnabled = enabled;
}

PointerChoreographer::ControllerConstructor PointerChoreographer::getMouseControllerConstructor(
        ui::LogicalDisplayId displayId) {
    std::function<std::shared_ptr<PointerControllerInterface>()> ctor =
            [this, displayId]() REQUIRES(getLock()) {
                auto pc = mPolicy.createPointerController(
                        PointerControllerInterface::ControllerType::MOUSE);
                if (const auto viewport = findViewportByIdLocked(displayId); viewport) {
                    pc->setDisplayViewport(*viewport);
                }
                return pc;
            };
    return ConstructorDelegate(std::move(ctor));
}

PointerChoreographer::ControllerConstructor PointerChoreographer::getStylusControllerConstructor(
        ui::LogicalDisplayId displayId) {
    std::function<std::shared_ptr<PointerControllerInterface>()> ctor =
            [this, displayId]() REQUIRES(getLock()) {
                auto pc = mPolicy.createPointerController(
                        PointerControllerInterface::ControllerType::STYLUS);
                if (const auto viewport = findViewportByIdLocked(displayId); viewport) {
                    pc->setDisplayViewport(*viewport);
                }
                return pc;
            };
    return ConstructorDelegate(std::move(ctor));
}

std::optional<std::pair<const DisplayViewport*, float /*offsetPx*/>>
PointerChoreographer::findDestinationDisplayLocked(const ui::LogicalDisplayId sourceDisplayId,
                                                   const DisplayTopologyPosition sourceBoundary,
                                                   int32_t sourceCursorOffsetPx) const {
    const auto& sourceNode = mTopology.graph.find(sourceDisplayId);
    if (sourceNode == mTopology.graph.end()) {
        // Topology is likely out of sync with viewport info, wait for it to be updated
        LOG(WARNING) << "Source display missing from topology " << sourceDisplayId;
        return std::nullopt;
    }
    for (const DisplayTopologyAdjacentDisplay& adjacentDisplay : sourceNode->second) {
        if (adjacentDisplay.position != sourceBoundary) {
            continue;
        }
        const DisplayViewport* adjacentViewport = findViewportByIdLocked(adjacentDisplay.displayId);
        if (adjacentViewport == nullptr) {
            // Topology is likely out of sync with viewport info, wait for them to be updated
            LOG(WARNING) << "Cannot find viewport for adjacent display "
                         << adjacentDisplay.displayId << "of source display " << sourceDisplayId;
            continue;
        }
        // As displays can have different densities we need to do all calculations in
        // density-independent-pixels a.k.a. dp values.
        const int sourceDensity = mTopology.displaysDensity.at(sourceDisplayId);
        const int adjacentDisplayDensity = mTopology.displaysDensity.at(adjacentDisplay.displayId);
        const float sourceCursorOffsetDp = pxToDp(sourceCursorOffsetPx, sourceDensity);
        const int32_t edgeSizePx = sourceBoundary == DisplayTopologyPosition::TOP ||
                        sourceBoundary == DisplayTopologyPosition::BOTTOM
                ? (adjacentViewport->logicalRight - adjacentViewport->logicalLeft)
                : (adjacentViewport->logicalBottom - adjacentViewport->logicalTop);
        const float adjacentEdgeSizeDp = pxToDp(edgeSizePx, adjacentDisplayDensity);
        // Target position must be within target display boundary.
        // Cursor should also be able to cross displays when only display corners are touching and
        // there may be zero overlapping pixels. To accommodate this we have margin of one pixel
        // around the end of the overlapping edge.
        if (sourceCursorOffsetDp >= adjacentDisplay.offsetDp &&
            sourceCursorOffsetDp <= adjacentDisplay.offsetDp + adjacentEdgeSizeDp) {
            const int destinationOffsetPx =
                    dpToPx(sourceCursorOffsetDp - adjacentDisplay.offsetDp, adjacentDisplayDensity);
            return std::make_pair(adjacentViewport, destinationOffsetPx);
        }
    }
    return std::nullopt;
}

vec2 PointerChoreographer::filterPointerMotionForAccessibilityLocked(
        const vec2& current, const vec2& delta, const ui::LogicalDisplayId& displayId) {
    if (!mPointerMotionFilterEnabled) {
        return delta;
    }
    std::optional<vec2> filterResult =
            mPolicy.filterPointerMotionForAccessibility(current, delta, displayId);
    if (!filterResult.has_value()) {
        // Disable filter when there's any error.
        mPointerMotionFilterEnabled = false;
        return delta;
    }
    return *filterResult;
}

// --- PointerChoreographer::PointerChoreographerDisplayInfoListener ---

void PointerChoreographer::PointerChoreographerDisplayInfoListener::onWindowInfosChanged(
        const gui::WindowInfosUpdate& windowInfosUpdate) {
    std::scoped_lock _l(mLock);
    if (mPointerChoreographer == nullptr) {
        return;
    }
    auto newPrivacySensitiveDisplays =
            getPrivacySensitiveDisplaysFromWindowInfos(windowInfosUpdate.windowInfos);

    // PointerChoreographer uses Listener's lock.
    base::ScopedLockAssertion assumeLocked(mPointerChoreographer->getLock());
    if (newPrivacySensitiveDisplays != mPrivacySensitiveDisplays) {
        mPrivacySensitiveDisplays = std::move(newPrivacySensitiveDisplays);
        mPointerChoreographer->onPrivacySensitiveDisplaysChangedLocked(mPrivacySensitiveDisplays);
    }
}

void PointerChoreographer::PointerChoreographerDisplayInfoListener::setInitialDisplayInfosLocked(
        const std::vector<gui::WindowInfo>& windowInfos) {
    mPrivacySensitiveDisplays = getPrivacySensitiveDisplaysFromWindowInfos(windowInfos);
}

std::unordered_set<ui::LogicalDisplayId /*displayId*/>
PointerChoreographer::PointerChoreographerDisplayInfoListener::getPrivacySensitiveDisplaysLocked() {
    return mPrivacySensitiveDisplays;
}

void PointerChoreographer::PointerChoreographerDisplayInfoListener::
        onPointerChoreographerDestroyed() {
    std::scoped_lock _l(mLock);
    mPointerChoreographer = nullptr;
}

} // namespace android
