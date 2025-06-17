/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef ANDROID_GUI_BUFFERQUEUECOREDEFS_H
#define ANDROID_GUI_BUFFERQUEUECOREDEFS_H

#include <com_android_graphics_libgui_flags.h>
#include <gui/BufferSlot.h>
#include <ui/BufferQueueDefs.h>

namespace android {
    class BufferQueueCore;

    namespace BufferQueueDefs {
#if COM_ANDROID_GRAPHICS_LIBGUI_FLAGS(WB_UNLIMITED_SLOTS)
    typedef std::vector<BufferSlot> SlotsType;
#else
    typedef BufferSlot SlotsType[NUM_BUFFER_SLOTS];
#endif
    } // namespace BufferQueueDefs
} // namespace android

#endif
