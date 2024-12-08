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

namespace android {

class InputDeviceContext;
struct RawEvent;

/* Keeps track of cursor scrolling motions. */
class CursorScrollAccumulator {
public:
    CursorScrollAccumulator();
    void configure(InputDeviceContext& deviceContext);
    void reset(InputDeviceContext& deviceContext);

    void process(const RawEvent& rawEvent);
    void finishSync();

    inline bool haveRelativeVWheel() const { return mHaveRelWheel; }
    inline bool haveRelativeHWheel() const { return mHaveRelHWheel; }

    inline float getRelativeVWheel() const { return mRelWheel; }
    inline float getRelativeHWheel() const { return mRelHWheel; }

private:
    bool mHaveRelWheel;
    bool mHaveRelHWheel;
    bool mHaveRelWheelHighRes;
    bool mHaveRelHWheelHighRes;

    float mRelWheel;
    float mRelHWheel;

    void clearRelativeAxes();
};

} // namespace android
