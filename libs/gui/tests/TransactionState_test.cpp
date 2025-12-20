/*
 * Copyright (C) 2025 The Android Open Source Project
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
#include <unordered_map>
#include "android/gui/FocusRequest.h"
#include "android/gui/TransactionBarrier.h"
#include "binder/Binder.h"
#include "binder/Parcel.h"
#include "gtest/gtest.h"
#include "gui/LayerState.h"
#include "gui/WindowInfo.h"

#include "gui/TransactionState.h"

namespace android {

using ::testing::Eq;

void sprintf(std::string& out, const char* format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int len = vsnprintf(nullptr, 0, format, arg_list);
    if (len < 0) {
        va_end(arg_list);
    }
    std::string line(len, '\0');
    int written = vsnprintf(line.data(), len + 1, format, arg_list);
    if (written != len) {
        va_end(arg_list);
    }
    line.pop_back();
    out += line;
    va_end(arg_list);
}

constexpr std::string dump_struct(auto& x) {
    std::string s;
#if __has_builtin(__builtin_dump_struct)
    __builtin_dump_struct(&x, sprintf, s);
#else
    (void)x;
#endif
    return s;
}

void PrintTo(const TransactionState& state, ::std::ostream* os) {
    *os << dump_struct(state);
    *os << state.mFrameTimelineInfo.toString();
    for (auto mergedId : state.mMergedTransactionIds) {
        *os << mergedId << ",";
    }
}

void PrintTo(const ComposerState& state, ::std::ostream* os) {
    *os << dump_struct(state.state);
    *os << state.state.getWindowInfo();
}

void CompareSimple(const TransactionState& s1, const TransactionState& s2) {
    EXPECT_EQ(s1.mId, s2.mId);
    EXPECT_EQ(s1.mFlags, s2.mFlags);
    EXPECT_EQ(s1.mDesiredPresentTime, s2.mDesiredPresentTime);
    EXPECT_EQ(s1.mIsAutoTimestamp, s2.mIsAutoTimestamp);
}

void Compare(const TransactionListenerCallbacks& s1, const TransactionListenerCallbacks& s2) {
    EXPECT_EQ(s1.mHasListenerCallbacks, s2.mHasListenerCallbacks);
    EXPECT_EQ(s1.mFlattenedListenerCallbacks.size(), s2.mFlattenedListenerCallbacks.size());
    EXPECT_EQ(s1.mFlattenedListenerCallbacks, s2.mFlattenedListenerCallbacks);
}

void Compare(const gui::TransactionBarrier& b1, const gui::TransactionBarrier& b2) {
    EXPECT_EQ(b1.barrierToken, b2.barrierToken);
    EXPECT_EQ(b1.kind, b2.kind);
}

void CompareComplex(const TransactionState& s1, const TransactionState& s2) {
    EXPECT_EQ(s1.mMergedTransactionIds, s2.mMergedTransactionIds);
    EXPECT_EQ(s1.mFrameTimelineInfo, s2.mFrameTimelineInfo);
    EXPECT_EQ(s1.mUncacheBuffers, s2.mUncacheBuffers);
    Compare(s1.mCallbacks, s2.mCallbacks);
    EXPECT_EQ(s1.mInputWindowCommands, s2.mInputWindowCommands);
    EXPECT_EQ(s1.mDisplayStates.size(), s2.mDisplayStates.size());
    for (size_t i = 0; i < s1.mDisplayStates.size(); ++i) {
        EXPECT_EQ(s1.mDisplayStates.at(i), s2.mDisplayStates.at(i));
    }
    EXPECT_EQ(s1.mComposerStates.size(), s2.mComposerStates.size());
    for (size_t i = 0; i < s1.mComposerStates.size(); ++i) {
        EXPECT_EQ(s1.mComposerStates.at(i), s2.mComposerStates.at(i));
    }

    EXPECT_EQ(s1.mBarriers.size(), s2.mBarriers.size());
    for (size_t i = 0; i < s1.mBarriers.size(); ++i) {
        Compare(s1.mBarriers.at(i), s2.mBarriers.at(i));
    }
}

// In case EXPECT_EQ fails, this function is useful to pinpoint exactly which
// field did not compare ==.
void Compare(const TransactionState& s1, const TransactionState& s2) {
    CompareSimple(s1, s2);
    CompareComplex(s1, s2);
}

std::unique_ptr<std::unordered_map<int, sp<BBinder>>> createTokenMap(size_t maxSize) {
    auto result = std::make_unique<std::unordered_map<int, sp<BBinder>>>();
    for (size_t i = 0; i < maxSize; ++i) {
        result->emplace(i, sp<BBinder>::make());
    }
    return result;
}

constexpr size_t kMaxComposerStates = 2;
ComposerState createComposerStateForTest(size_t i) {
    static const auto* const sLayerHandle = createTokenMap(kMaxComposerStates).release();

    ComposerState state;
    state.state.what = layer_state_t::eFlagsChanged;
    state.state.surface = sLayerHandle->at(i);
    state.state.layerId = i;
    state.state.flags = 20 * i;
    return state;
}

constexpr size_t kMaxDisplayStates = 5;
DisplayState createDisplayStateForTest(size_t i) {
    static const auto* const sDisplayTokens = createTokenMap(kMaxDisplayStates).release();

    DisplayState displayState;
    displayState.what = DisplayState::eFlagsChanged;
    displayState.token = sDisplayTokens->at(i);
    displayState.flags = 20 * i;
    return displayState;
}

TransactionState createSimpleTransactionStateForTest() {
    TransactionState state;
    state.mId = 123;
    state.mFlags = 1;
    state.mDesiredPresentTime = 11;
    state.mIsAutoTimestamp = true;
    return state;
}

TransactionState createTransactionStateForTest() {
    TransactionState state;
    state = createSimpleTransactionStateForTest();
    state.mMergedTransactionIds.push_back(15);
    state.mMergedTransactionIds.push_back(0);
    state.mFrameTimelineInfo.vsyncId = 14;
    static const auto* const sCacheToken = createTokenMap(5).release();
    for (int i = 0; i < 5; i++) {
        client_cache_t cache;
        cache.token = sCacheToken->at(i);
        cache.id = i;
        state.mUncacheBuffers.emplace_back(std::move(cache));
    }
    static const auto* const sListenerCallbacks = []() {
        auto* callbacks = new std::vector<ListenerCallbacks>();
        for (int i = 0; i < 5; i++) {
            callbacks->emplace_back(sp<BBinder>::make(),
                                    std::unordered_set<CallbackId, CallbackIdHash>{});
        }
        return callbacks;
    }();
    state.mCallbacks.mHasListenerCallbacks = true;
    state.mCallbacks.mFlattenedListenerCallbacks = *sListenerCallbacks;
    static const auto* const sFocusRequestTokens = createTokenMap(5).release();
    for (int i = 0; i < 5; i++) {
        gui::FocusRequest request;
        request.token = sFocusRequestTokens->at(i);
        request.timestamp = i;
        state.mInputWindowCommands.addFocusRequest(request);
    }

    for (size_t i = 0; i < kMaxDisplayStates; i++) {
        state.mDisplayStates.push_back(createDisplayStateForTest(i));
    }
    for (size_t i = 0; i < kMaxComposerStates; i++) {
        state.mComposerStates.push_back(createComposerStateForTest(i));
    }

    return state;
}

TransactionState createEmptyTransaction(uint64_t id) {
    TransactionState state;
    state.mId = id;
    return state;
}

gui::TransactionBarrier createTransactionBarrier(std::string_view token,
                                                 gui::TransactionBarrier::BarrierKind kind) {
    gui::TransactionBarrier barrier;
    barrier.barrierToken = String16(token.data(), token.size());
    barrier.kind = kind;
    return barrier;
}

gui::TransactionBarrier createSignalBarrier(std::string_view token) {
    return createTransactionBarrier(token, gui::TransactionBarrier::BarrierKind::KIND_SIGNAL);
}

gui::TransactionBarrier createWaitBarrier(std::string_view token) {
    return createTransactionBarrier(token, gui::TransactionBarrier::BarrierKind::KIND_WAIT);
}

TEST(SimpleTransactionStateTest, parcel) {
    TransactionState state = createSimpleTransactionStateForTest();
    Parcel p;
    EXPECT_EQ(state.writeToParcel(&p), NO_ERROR);
    p.setDataPosition(0);
    TransactionState parcelledState;
    EXPECT_EQ(parcelledState.readFromParcel(&p), NO_ERROR);
    EXPECT_EQ(state, parcelledState);
};

TEST(TransactionStateTest, parcel) {
    TransactionState state = createTransactionStateForTest();
    state.mBarriers.emplace_back(createSignalBarrier("sig"));
    Parcel p;
    state.writeToParcel(&p);
    p.setDataPosition(0);
    TransactionState parcelledState;
    parcelledState.readFromParcel(&p);
    EXPECT_EQ(state, parcelledState);
};

TEST(TransactionStateTest, parcelDisplayState) {
    DisplayState state = createDisplayStateForTest(0);
    Parcel p;
    state.write(p);
    p.setDataPosition(0);
    DisplayState parcelledState;
    parcelledState.read(p);
    EXPECT_EQ(state, parcelledState);
};

TEST(TransactionStateTest, parcelLayerState) {
    ComposerState state = createComposerStateForTest(0);
    Parcel p;
    state.write(p);
    p.setDataPosition(0);
    ComposerState parcelledState;
    parcelledState.read(p);
    EXPECT_EQ(state, parcelledState);
};

TEST(TransactionStateTest, parcelEmptyState) {
    TransactionState state;
    Parcel p;
    state.writeToParcel(&p);
    p.setDataPosition(0);
    TransactionState parcelledState;
    state.readFromParcel(&p);
    EXPECT_EQ(state, parcelledState);
};

TEST(TransactionStateTest, mergeLayerState) {
    ComposerState composerState = createComposerStateForTest(0);
    ComposerState update;
    update.state.surface = composerState.state.surface;
    update.state.layerId = 0;
    update.state.what = layer_state_t::eAlphaChanged;
    update.state.color.a = .42;
    composerState.state.merge(update.state);

    ComposerState expectedMergedState = createComposerStateForTest(0);
    expectedMergedState.state.what |= layer_state_t::eAlphaChanged;
    expectedMergedState.state.color.a = .42;
    EXPECT_EQ(composerState, expectedMergedState);
};

TEST(SimpleTransactionStateTest, merge) {
    // Setup.
    static constexpr uint64_t kUpdateTransactionId = 200;
    TransactionState state = createSimpleTransactionStateForTest();
    TransactionState update;
    update.mId = kUpdateTransactionId;
    update.mFlags = state.mFlags + 1;

    auto updateCopy = update;

    // Mutation.
    state.merge(std::move(update), [](const auto&) {});

    // Assertions.
    TransactionState expectedMergedState = createSimpleTransactionStateForTest();
    expectedMergedState.mFlags = state.mFlags | updateCopy.mFlags;

    EXPECT_EQ(state.mFlags, expectedMergedState.mFlags);

    // desired present time is not merged.
    expectedMergedState.mDesiredPresentTime = state.mDesiredPresentTime;
}

TEST(TransactionStateTest, merge) {
    // Setup.
    static constexpr uint64_t kUpdateTransactionId = 200;

    TransactionState state = createTransactionStateForTest();
    state.mBarriers.emplace_back(createSignalBarrier("sig"));

    TransactionState update;
    update.mId = kUpdateTransactionId;
    {
        ComposerState composerState;
        composerState.state.surface = state.mComposerStates[0].state.surface;
        composerState.state.what = layer_state_t::eAlphaChanged;
        composerState.state.color.a = .42;
        update.mComposerStates.push_back(composerState);
    }
    {
        ComposerState composerState;
        composerState.state.surface = state.mComposerStates[1].state.surface;
        composerState.state.what = layer_state_t::eBufferChanged;
        update.mComposerStates.push_back(composerState);
    }
    update.mBarriers.emplace_back(
            createTransactionBarrier("wait", gui::TransactionBarrier::BarrierKind::KIND_WAIT));
    int32_t overrwiteLayerId = -1;
    auto updateCopy = update;
    // Mutation.
    state.merge(std::move(update),
                [&overrwiteLayerId](layer_state_t ls) { overrwiteLayerId = ls.layerId; });
    // Assertions.
    EXPECT_EQ(1, overrwiteLayerId);

    TransactionState expectedMergedState = createTransactionStateForTest();
    expectedMergedState.mMergedTransactionIds
            .insert(expectedMergedState.mMergedTransactionIds.begin(), kUpdateTransactionId);
    expectedMergedState.mComposerStates.at(0).state.what |= layer_state_t::eAlphaChanged;
    expectedMergedState.mComposerStates.at(0).state.color.a = .42;
    expectedMergedState.mComposerStates.at(1).state.what |= layer_state_t::eBufferChanged;
    auto inputCommands = expectedMergedState.mInputWindowCommands;

    // desired present time is not merged.
    expectedMergedState.mDesiredPresentTime = state.mDesiredPresentTime;

    expectedMergedState.mBarriers.emplace_back(createSignalBarrier("sig"));
    expectedMergedState.mBarriers.emplace_back(createWaitBarrier("wait"));

    EXPECT_EQ(state.mComposerStates[0], expectedMergedState.mComposerStates[0]);
    EXPECT_EQ(state.mInputWindowCommands, expectedMergedState.mInputWindowCommands);
    EXPECT_EQ(state, expectedMergedState);
};

TEST(TransactionStateTest, mergeMoreThanMaxBarriers) {
    TransactionState state = createTransactionStateForTest();
    for (size_t i = 0; i < TransactionState::MAX_BARRIERS_LENGTH; ++i) {
        std::string token = std::format("sig{}", i);
        state.mBarriers.emplace_back(createSignalBarrier(token));
    }

    TransactionState update;
    update.mId = 200;
    update.mBarriers.emplace_back(createWaitBarrier("wait"));

    state.merge(std::move(update), [](layer_state_t) {});

    EXPECT_EQ(TransactionState::MAX_BARRIERS_LENGTH, state.mBarriers.size());
    EXPECT_THAT(state.mBarriers.front().toString(),
                Eq("TransactionBarrier{barrierToken: sig1, kind: KIND_SIGNAL}"));
    EXPECT_THAT(state.mBarriers.back().toString(),
                Eq("TransactionBarrier{barrierToken: wait, kind: KIND_WAIT}"));
}

TEST(TransactionStateTest, clear) {
    TransactionState state = createTransactionStateForTest();
    state.clear();
    TransactionState emptyState = createEmptyTransaction(state.getId());
    EXPECT_EQ(state, emptyState);
};

} // namespace android
