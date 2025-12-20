/*
 * Copyright 2022 The Android Open Source Project
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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <EventHub.h>
#include <InputDevice.h>
#include <ftl/flags.h>
#include <input/Input.h>
#include <input/PropertyMap.h>
#include <input/VirtualKeyMap.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>

#include "InputReaderTracer.h"

namespace android {

class FakeEventHub : public EventHubInterface {
    struct KeyInfo {
        int32_t keyCode;
        uint32_t flags;
    };

    struct SensorInfo {
        InputDeviceSensorType sensorType;
        int32_t sensorDataIndex;
    };

    struct Device {
        InputDeviceIdentifier identifier;
        ftl::Flags<InputDeviceClass> classes;
        PropertyMap configuration;
        KeyedVector<int, RawAbsoluteAxisInfo> absoluteAxes;
        KeyedVector<int, bool> relativeAxes;
        KeyedVector<int32_t, int32_t> keyCodeStates;
        KeyedVector<int32_t, int32_t> scanCodeStates;
        KeyedVector<int32_t, int32_t> switchStates;
        KeyedVector<int32_t, int32_t> absoluteAxisValue;
        KeyedVector<int32_t, KeyInfo> keysByScanCode;
        KeyedVector<int32_t, KeyInfo> keysByUsageCode;
        std::map<int32_t, int32_t> keyRemapping;
        KeyedVector<int32_t, bool> leds;
        // fake mapping which would normally come from keyCharacterMap
        std::unordered_map<int32_t, int32_t> keyCodeMapping;
        std::unordered_map<int32_t, SensorInfo> sensorsByAbsCode;
        BitArray<MSC_MAX> mscBitmask;
        std::vector<VirtualKeyDefinition> virtualKeys;
        bool enabled;
        std::optional<RawLayoutInfo> layoutInfo;
        std::string sysfsRootPath;
        std::unordered_map<int32_t, std::vector<int32_t>> mtSlotValues;

        status_t enable() {
            enabled = true;
            return OK;
        }

        status_t disable() {
            enabled = false;
            return OK;
        }

        explicit Device(ftl::Flags<InputDeviceClass> classes) : classes(classes), enabled(true) {}
    };

    std::mutex mLock;
    std::condition_variable mEventsCondition;

    KeyedVector<RawDeviceId, Device*> mDevices;
    std::vector<std::string> mExcludedDevices;
    std::vector<RawEvent> mEvents GUARDED_BY(mLock);
    std::unordered_map<RawDeviceId, std::vector<TouchVideoFrame>> mVideoFrames;
    std::vector<int32_t> mVibrators = {0, 1};
    std::unordered_map<int32_t, RawLightInfo> mRawLightInfos;
    // Simulates a device light brightness, from light id to light brightness.
    std::unordered_map<int32_t /* lightId */, int32_t /* brightness*/> mLightBrightness;
    // Simulates a device light intensities, from light id to light intensities map.
    std::unordered_map<int32_t /* lightId */, std::unordered_map<LightColor, int32_t>>
            mLightIntensities;
    // fake sysfs node path and value.
    std::unordered_map<RawDeviceId, bool /* wakeupNode*/> mKernelWakeup;

public:
    static constexpr int32_t DEFAULT_BATTERY = 1;
    static constexpr int32_t BATTERY_STATUS = 4;
    static constexpr int32_t BATTERY_CAPACITY = 66;
    static const std::string BATTERY_DEVPATH;

    virtual ~FakeEventHub();
    FakeEventHub() {}

    void setTracer(std::shared_ptr<InputReaderTracer> tracer) {}

    void addDevice(RawDeviceId deviceId, const std::string& name,
                   ftl::Flags<InputDeviceClass> classes, int bus = 0);
    void removeDevice(RawDeviceId deviceId);

    bool isDeviceEnabled(RawDeviceId deviceId) const override;
    status_t enableDevice(RawDeviceId deviceId) override;
    status_t disableDevice(RawDeviceId deviceId) override;

    void addConfigurationProperty(RawDeviceId deviceId, const char* key, const char* value);

    void addAbsoluteAxis(RawDeviceId deviceId, int axis, int32_t minValue, int32_t maxValue,
                         int flat, int fuzz, int resolution = 0);
    void addRelativeAxis(RawDeviceId deviceId, int32_t axis);
    void setAbsoluteAxisValue(RawDeviceId deviceId, int32_t axis, int32_t value);

    void setRawLayoutInfo(RawDeviceId deviceId, RawLayoutInfo info);

    void setKeyCodeState(RawDeviceId deviceId, int32_t keyCode, int32_t state);
    void setScanCodeState(RawDeviceId deviceId, int32_t scanCode, int32_t state);
    void setSwitchState(RawDeviceId deviceId, int32_t switchCode, int32_t state);

    void addKey(RawDeviceId deviceId, int32_t scanCode, int32_t usageCode, int32_t keyCode,
                uint32_t flags);
    void addKeyCodeMapping(RawDeviceId deviceId, int32_t fromKeyCode, int32_t toKeyCode);
    void setKeyRemapping(RawDeviceId deviceId,
                         const std::map<int32_t, int32_t>& keyRemapping) const;
    void addVirtualKeyDefinition(RawDeviceId deviceId, const VirtualKeyDefinition& definition);

    void addSensorAxis(RawDeviceId deviceId, int32_t absCode, InputDeviceSensorType sensorType,
                       int32_t sensorDataIndex);

    void setMscEvent(RawDeviceId deviceId, int32_t mscEvent);

    void addLed(RawDeviceId deviceId, int32_t led, bool initialState);
    void addRawLightInfo(int32_t rawId, RawLightInfo&& info);
    void fakeLightBrightness(int32_t rawId, int32_t brightness);
    void fakeLightIntensities(int32_t rawId,
                              const std::unordered_map<LightColor, int32_t> intensities);
    bool getLedState(RawDeviceId deviceId, int32_t led);

    std::vector<std::string>& getExcludedDevices();

    void setVideoFrames(std::unordered_map<RawDeviceId, std::vector<TouchVideoFrame>> videoFrames);

    void enqueueEvent(nsecs_t when, nsecs_t readTime, RawDeviceId deviceId, int32_t type,
                      int32_t code, int32_t value);
    void assertQueueIsEmpty();
    void setSysfsRootPath(RawDeviceId deviceId, std::string sysfsRootPath) const;
    // Populate fake slot values to be returned by the getter, size of the values should be equal to
    // the slot count
    void setMtSlotValues(RawDeviceId deviceId, int32_t axis, const std::vector<int32_t>& values);
    base::Result<std::vector<int32_t>> getMtSlotValues(RawDeviceId deviceId, int32_t axis,
                                                       size_t slotCount) const override;
    bool setKernelWakeEnabled(RawDeviceId deviceId, bool enabled) override;
    bool fakeReadKernelWakeup(RawDeviceId deviceId) const;

private:
    Device* getDevice(RawDeviceId deviceId) const;

    ftl::Flags<InputDeviceClass> getDeviceClasses(RawDeviceId deviceId) const override;
    InputDeviceIdentifier getDeviceIdentifier(RawDeviceId deviceId) const override;
    int32_t getDeviceControllerNumber(RawDeviceId) const override;
    std::optional<PropertyMap> getConfiguration(RawDeviceId deviceId) const override;
    std::optional<RawAbsoluteAxisInfo> getAbsoluteAxisInfo(RawDeviceId deviceId,
                                                           int axis) const override;
    bool hasRelativeAxis(RawDeviceId deviceId, int axis) const override;
    bool hasInputProperty(RawDeviceId, int) const override;
    bool hasMscEvent(RawDeviceId deviceId, int mscEvent) const override final;
    status_t mapKey(RawDeviceId deviceId, int32_t scanCode, int32_t usageCode, int32_t metaState,
                    int32_t* outKeycode, int32_t* outMetaState, uint32_t* outFlags) const override;
    const KeyInfo* getKey(Device* device, int32_t scanCode, int32_t usageCode) const;

    status_t mapAxis(RawDeviceId, int32_t, AxisInfo*) const override;
    base::Result<std::pair<InputDeviceSensorType, int32_t>> mapSensor(
            RawDeviceId deviceId, int32_t absCode) const override;
    void setExcludedDevices(const std::vector<std::string>& devices) override;
    std::vector<RawEvent> getEvents(int) override;
    std::vector<TouchVideoFrame> getVideoFrames(RawDeviceId deviceId) override;
    int32_t getScanCodeState(RawDeviceId deviceId, int32_t scanCode) const override;
    std::optional<RawLayoutInfo> getRawLayoutInfo(RawDeviceId deviceId) const override;
    int32_t getKeyCodeState(RawDeviceId deviceId, int32_t keyCode) const override;
    int32_t getSwitchState(RawDeviceId deviceId, int32_t sw) const override;
    std::optional<int32_t> getAbsoluteAxisValue(RawDeviceId deviceId, int32_t axis) const override;
    int32_t getKeyCodeForKeyLocation(RawDeviceId deviceId, int32_t locationKeyCode) const override;

    // Return true if the device has non-empty key layout.
    bool markSupportedKeyCodes(RawDeviceId deviceId, const std::vector<int32_t>& keyCodes,
                               uint8_t* outFlags) const override;
    bool hasScanCode(RawDeviceId deviceId, int32_t scanCode) const override;
    bool hasKeyCode(RawDeviceId deviceId, int32_t keyCode) const override;
    bool hasLed(RawDeviceId deviceId, int32_t led) const override;
    void setLedState(RawDeviceId deviceId, int32_t led, bool on) override;
    void getVirtualKeyDefinitions(RawDeviceId deviceId,
                                  std::vector<VirtualKeyDefinition>& outVirtualKeys) const override;
    const std::shared_ptr<KeyCharacterMap> getKeyCharacterMap(RawDeviceId) const override;
    bool setKeyboardLayoutOverlay(RawDeviceId, std::shared_ptr<KeyCharacterMap>) override;

    void vibrate(RawDeviceId, const VibrationElement&) override {}
    void cancelVibrate(RawDeviceId) override {}
    std::vector<int32_t> getVibratorIds(RawDeviceId deviceId) const override;

    std::optional<int32_t> getBatteryCapacity(RawDeviceId, int32_t) const override;
    std::optional<int32_t> getBatteryStatus(RawDeviceId, int32_t) const override;
    std::vector<int32_t> getRawBatteryIds(RawDeviceId deviceId) const override;
    std::optional<RawBatteryInfo> getRawBatteryInfo(RawDeviceId deviceId,
                                                    int32_t batteryId) const override;

    std::vector<int32_t> getRawLightIds(RawDeviceId deviceId) const override;
    std::optional<RawLightInfo> getRawLightInfo(RawDeviceId deviceId,
                                                int32_t lightId) const override;
    void setLightBrightness(RawDeviceId deviceId, int32_t lightId, int32_t brightness) override;
    void setLightIntensities(RawDeviceId deviceId, int32_t lightId,
                             std::unordered_map<LightColor, int32_t> intensities) override;
    std::optional<int32_t> getLightBrightness(RawDeviceId deviceId, int32_t lightId) const override;
    std::optional<std::unordered_map<LightColor, int32_t>> getLightIntensities(
            RawDeviceId deviceId, int32_t lightId) const override;
    std::filesystem::path getSysfsRootPath(RawDeviceId deviceId) const override;
    void sysfsNodeChanged(const std::string& sysfsNodePath) override;
    void dump(std::string&) const override {}
    void monitor() const override {}
    void requestReopenDevices() override {}
    void wake() override {}
};

} // namespace android
