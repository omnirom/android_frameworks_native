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

#include "ActivePictureUpdater.h"
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

class TestableLayerFE : public LayerFE {
public:
    TestableLayerFE() : LayerFE("TestableLayerFE"), snapshot(*(new LayerSnapshot)) {
        mSnapshot = std::unique_ptr<LayerSnapshot>(&snapshot);
    }

    LayerSnapshot& snapshot;
};

class ActivePictureUpdaterTest : public testing::Test {
protected:
    SurfaceFlinger* flinger() {
        if (!mFlingerSetup) {
            mFlinger.setupMockScheduler();
            mFlinger.setupComposer(std::make_unique<Hwc2::mock::Composer>());
            mFlinger.setupRenderEngine(std::make_unique<renderengine::mock::RenderEngine>());
            mFlingerSetup = true;
        }
        return mFlinger.flinger();
    }

private:
    TestableSurfaceFlinger mFlinger;
    bool mFlingerSetup = false;
};

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

TEST_F(ActivePictureUpdaterTest, notCalledWithNoProfile) {
    sp<NiceMock<MockLayer>> layer = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE;
    EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle::NONE;
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_FALSE(updater.updateAndHasChanged());
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenLayerStartsUsingProfile) {
    sp<NiceMock<MockLayer>> layer = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE;
    EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle::NONE;
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_FALSE(updater.updateAndHasChanged());
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }
}

TEST_F(ActivePictureUpdaterTest, notCalledWhenLayerContinuesUsingProfile) {
    sp<NiceMock<MockLayer>> layer = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE;
    EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_FALSE(updater.updateAndHasChanged());
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenLayerStopsUsingProfile) {
    sp<NiceMock<MockLayer>> layer = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE;
    EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle::NONE;
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({}));
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenLayerChangesProfile) {
    sp<NiceMock<MockLayer>> layer = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE;
    EXPECT_CALL(*layer, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }
    {
        layerFE.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE.onPictureProfileCommitted();
        updater.onLayerComposed(*layer, layerFE, layerFE.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 2}}));
    }
}

TEST_F(ActivePictureUpdaterTest, notCalledWhenUncommittedLayerChangesProfile) {
    sp<NiceMock<MockLayer>> layer1 = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE1;
    EXPECT_CALL(*layer1, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    sp<NiceMock<MockLayer>> layer2 = sp<NiceMock<MockLayer>>::make(flinger(), 200);
    TestableLayerFE layerFE2;
    EXPECT_CALL(*layer2, getOwnerUid()).WillRepeatedly(Return(uid_t(20)));

    ActivePictureUpdater updater;
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_FALSE(updater.updateAndHasChanged());
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenDifferentLayerUsesSameProfile) {
    sp<NiceMock<MockLayer>> layer1 = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE1;
    EXPECT_CALL(*layer1, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    sp<NiceMock<MockLayer>> layer2 = sp<NiceMock<MockLayer>>::make(flinger(), 200);
    TestableLayerFE layerFE2;
    EXPECT_CALL(*layer2, getOwnerUid()).WillRepeatedly(Return(uid_t(20)));

    ActivePictureUpdater updater;
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE2.onPictureProfileCommitted();
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(),
                    UnorderedElementsAre({{100, 10, 1}, {200, 20, 2}}));
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(),
                    UnorderedElementsAre({{100, 10, 2}, {200, 20, 1}}));
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenSameUidUsesSameProfile) {
    sp<NiceMock<MockLayer>> layer1 = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE1;
    EXPECT_CALL(*layer1, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    sp<NiceMock<MockLayer>> layer2 = sp<NiceMock<MockLayer>>::make(flinger(), 200);
    TestableLayerFE layerFE2;
    EXPECT_CALL(*layer2, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE2.onPictureProfileCommitted();
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(),
                    UnorderedElementsAre({{100, 10, 1}, {200, 10, 2}}));
    }
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(2);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(),
                    UnorderedElementsAre({{100, 10, 2}, {200, 10, 1}}));
    }
}

TEST_F(ActivePictureUpdaterTest, calledWhenNewLayerUsesSameProfile) {
    sp<NiceMock<MockLayer>> layer1 = sp<NiceMock<MockLayer>>::make(flinger(), 100);
    TestableLayerFE layerFE1;
    EXPECT_CALL(*layer1, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    ActivePictureUpdater updater;
    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(), UnorderedElementsAre({{100, 10, 1}}));
    }

    sp<NiceMock<MockLayer>> layer2 = sp<NiceMock<MockLayer>>::make(flinger(), 200);
    TestableLayerFE layerFE2;
    EXPECT_CALL(*layer2, getOwnerUid()).WillRepeatedly(Return(uid_t(10)));

    {
        layerFE1.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE1.onPictureProfileCommitted();
        updater.onLayerComposed(*layer1, layerFE1, layerFE1.stealCompositionResult());

        layerFE2.snapshot.pictureProfileHandle = PictureProfileHandle(1);
        layerFE2.onPictureProfileCommitted();
        updater.onLayerComposed(*layer2, layerFE2, layerFE2.stealCompositionResult());

        ASSERT_TRUE(updater.updateAndHasChanged());
        EXPECT_THAT(updater.getActivePictures(),
                    UnorderedElementsAre({{100, 10, 1}, {200, 10, 1}}));
    }
}

} // namespace android
