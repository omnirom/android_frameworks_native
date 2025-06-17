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

#include "ActivePictureTracker.h"

#include <algorithm>

#include "Layer.h"
#include "LayerFE.h"

namespace android {

using gui::ActivePicture;
using gui::IActivePictureListener;

void ActivePictureTracker::onLayerComposed(const Layer& layer, const LayerFE& layerFE,
                                           const CompositionResult& result) {
    if (result.wasPictureProfileCommitted) {
        gui::ActivePicture picture;
        picture.layerId = int32_t(layer.sequence);
        picture.ownerUid = int32_t(layer.getOwnerUid());
        // TODO(b/337330263): Why does LayerFE coming from SF have a null composition state?
        if (layerFE.getCompositionState()) {
            picture.pictureProfileId = layerFE.getCompositionState()->pictureProfileHandle.getId();
        } else {
            picture.pictureProfileId = result.pictureProfileHandle.getId();
        }
        mNewActivePictures.push_back(picture);
    }
}

void ActivePictureTracker::updateAndNotifyListeners(const Listeners& listenersToAdd,
                                                    const Listeners& listenersToRemove) {
    Listeners newListeners = updateListeners(listenersToAdd, listenersToRemove);
    if (updateAndHasChanged()) {
        for (auto listener : mListeners) {
            listener->onActivePicturesChanged(getActivePictures());
        }
    } else {
        for (auto listener : newListeners) {
            listener->onActivePicturesChanged(getActivePictures());
        }
    }
}

ActivePictureTracker::Listeners ActivePictureTracker::updateListeners(
        const Listeners& listenersToAdd, const Listeners& listenersToRemove) {
    Listeners newListeners;
    for (auto listener : listenersToRemove) {
        std::erase_if(mListeners, [listener](const sp<IActivePictureListener>& otherListener) {
            return IInterface::asBinder(listener) == IInterface::asBinder(otherListener);
        });
    }
    for (auto listener : listenersToAdd) {
        if (std::find_if(mListeners.begin(), mListeners.end(),
                         [listener](const sp<IActivePictureListener>& otherListener) {
                             return IInterface::asBinder(listener) ==
                                     IInterface::asBinder(otherListener);
                         }) == mListeners.end()) {
            newListeners.push_back(listener);
        }
    }
    for (auto listener : newListeners) {
        mListeners.push_back(listener);
    }
    return newListeners;
}

bool ActivePictureTracker::updateAndHasChanged() {
    bool hasChanged = true;
    if (mNewActivePictures.size() == mOldActivePictures.size()) {
        auto compare = [](const ActivePicture& lhs, const ActivePicture& rhs) -> int {
            if (lhs.layerId == rhs.layerId) {
                return lhs.pictureProfileId < rhs.pictureProfileId;
            }
            return lhs.layerId < rhs.layerId;
        };
        std::sort(mNewActivePictures.begin(), mNewActivePictures.end(), compare);
        if (std::equal(mNewActivePictures.begin(), mNewActivePictures.end(),
                       mOldActivePictures.begin())) {
            hasChanged = false;
        }
    }
    std::swap(mOldActivePictures, mNewActivePictures);
    mNewActivePictures.resize(0);
    return hasChanged;
}

const std::vector<ActivePicture>& ActivePictureTracker::getActivePictures() const {
    return mOldActivePictures;
}

} // namespace android
