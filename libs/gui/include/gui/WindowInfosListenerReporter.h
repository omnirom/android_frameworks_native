/*
 * Copyright 2021 The Android Open Source Project
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

#include <android-base/result.h>
#include <android/gui/BnWindowInfosListener.h>
#include <android/gui/ISurfaceComposer.h>
#include <android/gui/IWindowInfosPublisher.h>
#include <binder/IBinder.h>
#include <gui/SpHash.h>
#include <gui/WindowInfosListener.h>
#include <gui/WindowInfosUpdate.h>
#include <unordered_set>

namespace android {

class WindowInfosListenerReporter : public gui::BnWindowInfosListener {
public:
    static sp<WindowInfosListenerReporter> getInstance();
    binder::Status onWindowInfosChanged(const gui::WindowInfosUpdate& update) override;
    android::base::Result<gui::WindowInfosUpdate> addWindowInfosListener(
            sp<gui::WindowInfosListener> windowInfosListener, const sp<gui::ISurfaceComposer>&);
    status_t removeWindowInfosListener(const sp<gui::WindowInfosListener>& windowInfosListener,
                                       const sp<gui::ISurfaceComposer>& surfaceComposer);
    void reconnect(const sp<gui::ISurfaceComposer>&);

private:
    WindowInfosListenerReporter() = default;
    friend class sp<WindowInfosListenerReporter>;

    using ListenerSet =
            std::unordered_set<sp<gui::WindowInfosListener>, gui::SpHash<gui::WindowInfosListener>>;

    static constexpr int64_t UNASSIGNED_LISTENER_ID = -1;

    std::mutex mListenersMutex;
    ListenerSet mWindowInfosListeners GUARDED_BY(mListenersMutex);
    gui::WindowInfosUpdate mLastUpdate GUARDED_BY(mListenersMutex);
    sp<gui::IWindowInfosPublisher> mWindowInfosPublisher GUARDED_BY(mListenersMutex);
    int64_t mListenerId GUARDED_BY(mListenersMutex);
};
} // namespace android
