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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <android/gui/ActivePicture.h>
#include <android/gui/IActivePictureListener.h>
#include <compositionengine/mock/CompositionEngine.h>
#include <mock/DisplayHardware/MockComposer.h>
#include <mock/MockLayer.h>
#include <renderengine/mock/RenderEngine.h>

#include "ActivePictureTracker.h"
#include "LayerFE.h"
#include "TestableSurfaceFlinger.h"

namespace android {

using android::compositionengine::LayerFECompositionState;
using android::gui::ActivePicture;
using android::gui::IActivePictureListener;
using android::mock::MockLayer;
using surfaceflinger::frontend::LayerSnapshot;
using testing::_;
using testing::NiceMock;
using testing::Return;
using testing::SizeIs;
using testing::StrictMock;

class TestableLayerFE : public LayerFE {
public:
    TestableLayerFE() : LayerFE("TestableLayerFE"), snapshot(*(new LayerSnapshot)) {
        mSnapshot = std::unique_ptr<LayerSnapshot>(&snapshot);
    }

    LayerSnapshot& snapshot;
};

class MockActivePictureListener : public gui::BnActivePictureListener {
public:
    operator ActivePictureTracker::Listeners const() {
        return {sp<IActivePictureListener>::fromExisting(this)};
    }

    MOCK_METHOD(binder::Status, onActivePicturesChanged, (const std::vector<ActivePicture>&),
                (override));
};

class ActivePictureTrackerTest : public testing::Test {
protected:
    const static ActivePictureTracker::Listeners NO_LISTENERS;

    SurfaceFlinger* flinger() {
        if (!mFlingerSetup) {
            mFlinger.setupMockScheduler();
            mFlinger.setupComposer(std::make_unique<Hwc2::mock::Composer>());
            mFlinger.setupRenderEngine(std::make_unique<renderengine::mock::RenderEngine>());
            mFlingerSetup = true;
        }
        return mFlinger.flinger();
    }

    sp<NiceMock<MockLayer>> createMockLayer(int layerId, int ownerUid) {
        auto layer = sp<NiceMock<MockLayer>>::make(flinger(), layerId);
        EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(ownerUid)));
        return layer;
    }

    sp<StrictMock<MockActivePictureListener>> createMockListener() {
        return sp<StrictMock<MockActivePictureListener>>::make();
    }

    ActivePictureTracker::Listeners mListenersToAdd;
    ActivePictureTracker::Listeners mListenersToRemove;

private:
    TestableSurfaceFlinger mFlinger;
    bool mFlingerSetup = false;
};

const ActivePictureTracker::Listeners ActivePictureTrackerTest::NO_LISTENERS;

// Hack to workaround initializer lists not working for parcelables because parcelables inherit from
// Parcelable, which has a virtual destructor.
auto UnorderedElementsAre(std::initializer_list<std::tuple<int32_t, int32_t, int64_t>> tuples) {
    std::vector<ActivePicture> activePictures;
    for (auto tuple : tuples) {
        ActivePicture ap;
        ap.layerId = std::get<0>(tuple);
        ap.ownerUid = std::get<1>(tuple);
        ap.pictureProfileId = std::get<2>(tuple);
        activePictures.push_back(ap);
    }
    return testing::UnorderedElementsAreArray(activePictures);
}

// Parcelables don't define this for matchers, which is unfortunate
void PrintTo(const ActivePicture& activePicture, std::ostream* os) {
    *os << activePicture.toString();
}

TEST_F(ActivePictureTrackerTest, whenListenerAdded_called) {
    ActivePictureTracker tracker;
    auto listener = createMockListener();
    EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
    tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
}

TEST_F(ActivePictureTrackerTest, whenListenerAdded_withListenerAlreadyAdded_notCalled) {
    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        EXPECT_CALL(*listener, onActivePicturesChanged(_)).Times(0);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenListenerAdded_withUncommittedProfile_calledWithNone) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
    {
        auto listener = createMockListener();
        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenListenerAdded_withCommittedProfile_calledWithActivePicture) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        auto listener = createMockListener();
        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenProfileAdded_calledWithActivePicture) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenContinuesUsingProfile_notCalled) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_)).Times(0);
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenProfileIsRemoved_calledWithNoActivePictures) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle::NONE;
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenProfileIsNotCommitted_calledWithNoActivePictures) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(0))).Times(1);
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenProfileChanges_calledWithDifferentProfile) {
    auto layer = createMockLayer(100, 10);
    TestableLayerFE layerFE;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1)))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1)))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 2}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenMultipleCommittedProfiles_calledWithMultipleActivePictures) {
    auto layer1 = createMockLayer(100, 10);
    TestableLayerFE layerFE1;

    auto layer2 = createMockLayer(200, 20);
    TestableLayerFE layerFE2;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(2)))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}, {200, 20, 2}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenNonCommittedProfileChanges_notCalled) {
    auto layer1 = createMockLayer(100, 10);
    TestableLayerFE layerFE1;

    auto layer2 = createMockLayer(200, 20);
    TestableLayerFE layerFE2;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_)).Times(0);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenDifferentLayerUsesSameProfile_called) {
    auto layer1 = createMockLayer(100, 10);
    TestableLayerFE layerFE1;

    auto layer2 = createMockLayer(200, 20);
    TestableLayerFE layerFE2;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}, {200, 20, 2}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 2}, {200, 20, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenSameUidDifferentLayerUsesSameProfile_called) {
    auto layer1 = createMockLayer(100, 10);
    TestableLayerFE layerFE1;

    auto layer2 = createMockLayer(200, 10);
    TestableLayerFE layerFE2;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}, {200, 10, 2}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 2}, {200, 10, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

TEST_F(ActivePictureTrackerTest, whenNewLayerUsesSameProfile_called) {
    auto layer1 = createMockLayer(100, 10);
    TestableLayerFE layerFE1;

    ActivePictureTracker tracker;
    auto listener = createMockListener();
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(SizeIs(1))).Times(1);
        tracker.updateAndNotifyListeners(*listener, NO_LISTENERS);
    }

    auto layer2 = createMockLayer(200, 10);
    TestableLayerFE layerFE2;
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        tracker.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        EXPECT_CALL(*listener, onActivePicturesChanged(_))
                .WillOnce([](const std::vector<gui::ActivePicture>& activePictures) {
                    EXPECT_THAT(activePictures, UnorderedElementsAre({{100, 10, 1}, {200, 10, 1}}));
                    return binder::Status::ok();
                });
        tracker.updateAndNotifyListeners(NO_LISTENERS, NO_LISTENERS);
    }
}

} // namespace android
