/*
 * Copyright 2025 The Android Open Source Project
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

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

namespace android::surfaceflinger::tests::end2end::test_framework::core {

// Ensures all access to a state structure `SharedState` is done while holding an appropriate
// `std::shared_mutex` lock.
//
// This works around limitations with Clang thread safety analysis when using `std::shared_lock`, as
// there will be a compiler diagnostic despite acquiring the lock.
template <typename SharedState>
class GuardedSharedState final {
  public:
    // Allows shared read-only access to the state. The lambda is invoked with a `const
    // SharedState&` first argument referencing the state.
    template <typename F>
    auto withSharedLock(F&& continuation) const -> std::invoke_result_t<F, const SharedState&> {
        const std::shared_lock lock(mMutex);
        return std::invoke(std::forward<F>(continuation), mState);
    }

    // Allows exclusive read/write access to the state. The lambda is invoked with a `SharedState&`
    // first argument referencing the state.
    template <typename F>
    auto withExclusiveLock(F&& continuation) -> std::invoke_result_t<F, SharedState&> {
        const std::lock_guard lock(mMutex);
        return std::invoke(std::forward<F>(continuation), mState);
    }

  private:
    mutable std::shared_mutex mMutex;
    SharedState mState;
};

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
