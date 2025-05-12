/*
 * Copyright 2024 The Android Open Source Project
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

#include "SensorInputMapper.h"

#include <cstdint>
#include <list>
#include <optional>
#include <utility>
#include <vector>

#include <EventHub.h>
#include <NotifyArgs.h>
#include <ftl/enum.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <input/Input.h>
#include <input/InputDevice.h>
#include <input/PrintTools.h>
#include <linux/input-event-codes.h>

#include "InputMapperTest.h"
#include "TestEventMatchers.h"

namespace android {

using testing::AllOf;
using testing::ElementsAre;
using testing::Return;
using testing::VariantWith;

namespace {

constexpr int32_t ACCEL_RAW_MIN = -32768;
constexpr int32_t ACCEL_RAW_MAX = 32768;
constexpr int32_t ACCEL_RAW_FUZZ = 16;
constexpr int32_t ACCEL_RAW_FLAT = 0;
constexpr int32_t ACCEL_RAW_RESOLUTION = 8192;

constexpr int32_t GYRO_RAW_MIN = -2097152;
constexpr int32_t GYRO_RAW_MAX = 2097152;
constexpr int32_t GYRO_RAW_FUZZ = 16;
constexpr int32_t GYRO_RAW_FLAT = 0;
constexpr int32_t GYRO_RAW_RESOLUTION = 1024;

constexpr float GRAVITY_MS2_UNIT = 9.80665f;
constexpr float DEGREE_RADIAN_UNIT = 0.0174533f;

} // namespace

class SensorInputMapperTest : public InputMapperUnitTest {
protected:
    void SetUp() override {
        InputMapperUnitTest::SetUp();
        EXPECT_CALL(mMockEventHub, getDeviceClasses(EVENTHUB_ID))
                .WillRepeatedly(Return(InputDeviceClass::SENSOR));
        // The mapper requests info on all ABS axes, including ones which aren't actually used, so
        // just return nullopt for all axes we don't explicitly set up.
        EXPECT_CALL(mMockEventHub, getAbsoluteAxisInfo(EVENTHUB_ID, testing::_))
                .WillRepeatedly(Return(std::nullopt));
    }

    void setupSensor(int32_t absCode, InputDeviceSensorType type, int32_t sensorDataIndex) {
        EXPECT_CALL(mMockEventHub, mapSensor(EVENTHUB_ID, absCode))
                .WillRepeatedly(Return(std::make_pair(type, sensorDataIndex)));
    }
};

TEST_F(SensorInputMapperTest, GetSources) {
    mMapper = createInputMapper<SensorInputMapper>(*mDeviceContext,
                                                   mFakePolicy->getReaderConfiguration());

    ASSERT_EQ(static_cast<uint32_t>(AINPUT_SOURCE_SENSOR), mMapper->getSources());
}

TEST_F(SensorInputMapperTest, ProcessAccelerometerSensor) {
    EXPECT_CALL(mMockEventHub, hasMscEvent(EVENTHUB_ID, MSC_TIMESTAMP))
            .WillRepeatedly(Return(true));
    setupSensor(ABS_X, InputDeviceSensorType::ACCELEROMETER, /*sensorDataIndex=*/0);
    setupSensor(ABS_Y, InputDeviceSensorType::ACCELEROMETER, /*sensorDataIndex=*/1);
    setupSensor(ABS_Z, InputDeviceSensorType::ACCELEROMETER, /*sensorDataIndex=*/2);
    setupAxis(ABS_X, /*valid=*/true, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_RESOLUTION,
              ACCEL_RAW_FLAT, ACCEL_RAW_FUZZ);
    setupAxis(ABS_Y, /*valid=*/true, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_RESOLUTION,
              ACCEL_RAW_FLAT, ACCEL_RAW_FUZZ);
    setupAxis(ABS_Z, /*valid=*/true, ACCEL_RAW_MIN, ACCEL_RAW_MAX, ACCEL_RAW_RESOLUTION,
              ACCEL_RAW_FLAT, ACCEL_RAW_FUZZ);
    mPropertyMap.addProperty("sensor.accelerometer.reportingMode", "0");
    mPropertyMap.addProperty("sensor.accelerometer.maxDelay", "100000");
    mPropertyMap.addProperty("sensor.accelerometer.minDelay", "5000");
    mPropertyMap.addProperty("sensor.accelerometer.power", "1.5");
    mMapper = createInputMapper<SensorInputMapper>(*mDeviceContext,
                                                   mFakePolicy->getReaderConfiguration());

    EXPECT_CALL(mMockEventHub, enableDevice(EVENTHUB_ID));
    ASSERT_TRUE(mMapper->enableSensor(InputDeviceSensorType::ACCELEROMETER,
                                      std::chrono::microseconds(10000),
                                      std::chrono::microseconds(0)));
    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_ABS, ABS_X, 20000);
    args += process(ARBITRARY_TIME, EV_ABS, ABS_Y, -20000);
    args += process(ARBITRARY_TIME, EV_ABS, ABS_Z, 40000);
    args += process(ARBITRARY_TIME, EV_MSC, MSC_TIMESTAMP, 1000);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);

    std::vector<float> values = {20000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT,
                                 -20000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT,
                                 40000.0f / ACCEL_RAW_RESOLUTION * GRAVITY_MS2_UNIT};

    ASSERT_EQ(args.size(), 1u);
    const NotifySensorArgs& arg = std::get<NotifySensorArgs>(args.front());
    ASSERT_EQ(arg.source, AINPUT_SOURCE_SENSOR);
    ASSERT_EQ(arg.deviceId, DEVICE_ID);
    ASSERT_EQ(arg.sensorType, InputDeviceSensorType::ACCELEROMETER);
    ASSERT_EQ(arg.accuracy, InputDeviceSensorAccuracy::HIGH);
    ASSERT_EQ(arg.hwTimestamp, ARBITRARY_TIME);
    ASSERT_EQ(arg.values, values);
    mMapper->flushSensor(InputDeviceSensorType::ACCELEROMETER);
}

TEST_F(SensorInputMapperTest, ProcessGyroscopeSensor) {
    EXPECT_CALL(mMockEventHub, hasMscEvent(EVENTHUB_ID, MSC_TIMESTAMP))
            .WillRepeatedly(Return(true));
    setupSensor(ABS_RX, InputDeviceSensorType::GYROSCOPE, /*sensorDataIndex=*/0);
    setupSensor(ABS_RY, InputDeviceSensorType::GYROSCOPE, /*sensorDataIndex=*/1);
    setupSensor(ABS_RZ, InputDeviceSensorType::GYROSCOPE, /*sensorDataIndex=*/2);
    setupAxis(ABS_RX, /*valid=*/true, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_RESOLUTION,
              GYRO_RAW_FLAT, GYRO_RAW_FUZZ);
    setupAxis(ABS_RY, /*valid=*/true, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_RESOLUTION,
              GYRO_RAW_FLAT, GYRO_RAW_FUZZ);
    setupAxis(ABS_RZ, /*valid=*/true, GYRO_RAW_MIN, GYRO_RAW_MAX, GYRO_RAW_RESOLUTION,
              GYRO_RAW_FLAT, GYRO_RAW_FUZZ);
    mPropertyMap.addProperty("sensor.gyroscope.reportingMode", "0");
    mPropertyMap.addProperty("sensor.gyroscope.maxDelay", "100000");
    mPropertyMap.addProperty("sensor.gyroscope.minDelay", "5000");
    mPropertyMap.addProperty("sensor.gyroscope.power", "0.8");
    mMapper = createInputMapper<SensorInputMapper>(*mDeviceContext,
                                                   mFakePolicy->getReaderConfiguration());

    EXPECT_CALL(mMockEventHub, enableDevice(EVENTHUB_ID));
    ASSERT_TRUE(mMapper->enableSensor(InputDeviceSensorType::GYROSCOPE,
                                      std::chrono::microseconds(10000),
                                      std::chrono::microseconds(0)));
    std::list<NotifyArgs> args;
    args += process(ARBITRARY_TIME, EV_ABS, ABS_RX, 20000);
    args += process(ARBITRARY_TIME, EV_ABS, ABS_RY, -20000);
    args += process(ARBITRARY_TIME, EV_ABS, ABS_RZ, 40000);
    args += process(ARBITRARY_TIME, EV_MSC, MSC_TIMESTAMP, 1000);
    args += process(ARBITRARY_TIME, EV_SYN, SYN_REPORT, 0);

    std::vector<float> values = {20000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT,
                                 -20000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT,
                                 40000.0f / GYRO_RAW_RESOLUTION * DEGREE_RADIAN_UNIT};

    ASSERT_EQ(args.size(), 1u);
    const NotifySensorArgs& arg = std::get<NotifySensorArgs>(args.front());
    ASSERT_EQ(arg.source, AINPUT_SOURCE_SENSOR);
    ASSERT_EQ(arg.deviceId, DEVICE_ID);
    ASSERT_EQ(arg.sensorType, InputDeviceSensorType::GYROSCOPE);
    ASSERT_EQ(arg.accuracy, InputDeviceSensorAccuracy::HIGH);
    ASSERT_EQ(arg.hwTimestamp, ARBITRARY_TIME);
    ASSERT_EQ(arg.values, values);
    mMapper->flushSensor(InputDeviceSensorType::GYROSCOPE);
}

} // namespace android