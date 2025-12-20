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

#include <android-base/logging.h>
#include <android/gui/ISurfaceComposer.h>
#include <android/gui/IWindowInfosListener.h>
#include <gui/AidlUtil.h>
#include <gui/WindowInfosListenerReporter.h>
#include "gui/WindowInfosUpdate.h"

namespace android {

using gui::DisplayInfo;
using gui::WindowInfo;
using gui::WindowInfosListener;
using gui::aidl_utils::statusTFromBinderStatus;

sp<WindowInfosListenerReporter> WindowInfosListenerReporter::getInstance() {
    static sp<WindowInfosListenerReporter> sInstance = sp<WindowInfosListenerReporter>::make();
    return sInstance;
}

android::base::Result<gui::WindowInfosUpdate> WindowInfosListenerReporter::addWindowInfosListener(
        sp<WindowInfosListener> windowInfosListener,
        const sp<gui::ISurfaceComposer>& surfaceComposer) {
    std::scoped_lock lock(mListenersMutex);
    if (mWindowInfosListeners.empty()) {
        gui::WindowInfosListenerInfo listenerInfo;
        binder::Status status =
                surfaceComposer
                        ->addWindowInfosListener(sp<WindowInfosListenerReporter>::fromExisting(
                                                         this),
                                                 &listenerInfo);
        LOG_IF(FATAL, !status.isOk()) << "Can't register window infos listener for pid " << getpid()
                                      << ". Device won't be usable";

        mWindowInfosPublisher = std::move(listenerInfo.windowInfosPublisher);
        mListenerId = listenerInfo.listenerId;
    }
    mWindowInfosListeners.emplace(std::move(windowInfosListener));

    return mLastUpdate;
}

status_t WindowInfosListenerReporter::removeWindowInfosListener(
        const sp<WindowInfosListener>& windowInfosListener,
        const sp<gui::ISurfaceComposer>& surfaceComposer) {
    std::scoped_lock lock(mListenersMutex);
    mWindowInfosListeners.erase(windowInfosListener);
    if (!mWindowInfosListeners.empty()) {
        return OK;
    }

    if (binder::Status status = surfaceComposer->removeWindowInfosListener(
                sp<WindowInfosListenerReporter>::fromExisting(this));
        !status.isOk()) {
        ALOGW("Failed to remove window infos listener from SurfaceFlinger");
        return statusTFromBinderStatus(status);
    }

    // Clear the last stored state since we're disabling updates and don't want to hold
    // stale values
    mLastUpdate = gui::WindowInfosUpdate();
    mWindowInfosPublisher.clear();
    mListenerId = UNASSIGNED_LISTENER_ID;

    return OK;
}

binder::Status WindowInfosListenerReporter::onWindowInfosChanged(
        const gui::WindowInfosUpdate& update) {
    ListenerSet listeners;
    int64_t id;
    sp<gui::IWindowInfosPublisher> publisher;
    {
        std::scoped_lock lock(mListenersMutex);
        listeners = mWindowInfosListeners;
        publisher = mWindowInfosPublisher;
        id = mListenerId;
        mLastUpdate = update;
    }
    // Publisher may be null if we've removed the last window infos listener before handling all
    // in-flight onWindowInfosChanged calls.
    if (!publisher) {
        return binder::Status::ok();
    }
    for (auto listener : listeners) {
        listener->onWindowInfosChanged(update);
    }
    publisher->ackWindowInfosReceived(update.vsyncId, id);
    return binder::Status::ok();
}

void WindowInfosListenerReporter::reconnect(const sp<gui::ISurfaceComposer>& composerService) {
    std::scoped_lock lock(mListenersMutex);
    if (!mWindowInfosListeners.empty()) {
        gui::WindowInfosListenerInfo listenerInfo;
        composerService->addWindowInfosListener(sp<gui::IWindowInfosListener>::fromExisting(this),
                                                &listenerInfo);
        mWindowInfosPublisher = std::move(listenerInfo.windowInfosPublisher);
        mListenerId = listenerInfo.listenerId;
    }
}

} // namespace android
