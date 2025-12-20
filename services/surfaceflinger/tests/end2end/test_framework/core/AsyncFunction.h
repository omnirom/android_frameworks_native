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

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <type_traits>
#include <utility>

#include <android-base/thread_annotations.h>
#include <ftl/finalizer.h>
#include <ftl/function.h>

namespace android::surfaceflinger::tests::end2end::test_framework::core {

// Define a function wrapper class that has some special features to make it async safe.
//
// 1) The contained function is only called on one thread at a time.
// 2) The contained function can be safely replaced at any time.
// 3) This wrapper helps ensure that after replacement, all calls to the replaced function are
//    complete by a well-defined synchronization point, which can be deferred to happen outside of
//    mutex locks that might otherwise cause a deadlock.
//
// To achieve the last feature, the `set` function to perform replacement returns a special
// `Finalizer` instance which is either automatically invoked on destruction, or on demand via
// `operator()` with no arguments (and returning no value). When invoked, the finalizer waits for
// any calls to the replaced function to complete, and as the finalizer can be moved if needed, this
// wait can be done without other mutexes being held that might cause a deadlock in the replaced
// function.
//
// Once the finalizer completes, any resources needed only by the previous function can be
// safely destroyed. The finalizer also destroys any captured state that was part of the
// previous function.
//
// Note that the target function that is called by this wrapper is allowed to replace the function
// this wrapper contains. When this happens there is no synchronization point, as waiting for the
// replaced function to complete as part of what is executed while it is invoked would be a
// deadlock. For this case, the returned finalizer instance is a no-op if invoked. Instead the
// capture for the prior function will be destroyed when the control returns back to the wrapper,
// before control returns to the code that invoked the wrapper.
//
// Instances of this class can be default constructed, and invoking the contained function in this
// state does nothing, and also requires no synchronization point when replaced by an actual target.
// After being set, this state can be entered again by using the `clear()` member function.
//
// If the contained function has a return type T other than void, the return type of the wrapper
// will be std::optional<T>. If there is no target set, invoking the wrapper will return a
// std::nullopt value, otherwise it will return an optional with the value set to the value returned
// by the contained function.
//
// Usage:
//
//    AsyncFunctionStd<void()> function = [this](){
//        std_lock_guard lock(mutex);
//        someMemberFunction();
//    };
//
//    function(); // Invokes someMemberFunction();
//
//    // May invoke someMemberFunction or otherMemberFunction (set below).
//    std::async(std::launch::async, function);
//
//    ftl::AsyncFunctionStd<void()>::Finalizer finalizer;
//    {
//        std_lock_guard lock(mutex);
//        finalizer = function.set([this](){
//            std_lock_guard lock(mutex);
//            otherMemberFunction();
//        });
//        // do not invoke the finalizer with locks held, unless there is no chance of a deadlock.
//    }
//    function()    // Invokes instance2->otherMemberFunction();
//    finalizer();  // Waits for calls to someMemberFunction to complete
//    // It is now safe to destroy resources that are used by someMemberFunction.
//
//    std::ignore = function.clear();  // Clear the function and implicitly invoke the returned
//    finalizer.
//    // It is now safe to destroy resource that used used by otherMemberFunction, including
//    // 'this' if desired.
//
template <typename Function>
class AsyncFunction final {
    struct SharedFunction;

    // Turns some return type `T` into `std::optional<T>`, unless `T` is `void`.
    template <typename T>
    using AddOptionalUnlessVoid = std::conditional_t<std::is_void_v<T>, T, std::optional<T>>;

  public:
    using Finalizer = ftl::FinalizerStd;

    // The return type from `operator()`.
    using result_type = AddOptionalUnlessVoid<typename Function::result_type>;

    // Default construct an empty state.
    AsyncFunction() = default;

    ~AsyncFunction() {
        // Ensure any outstanding calls complete before teardown by clearing the shared_ptr. Note
        // however if there are any, there would likely be other problems since the owning class is
        // in the process of being destroyed.
        setInternal(nullptr)();
    }

    // For simplicity, copying and moving are not possible.
    AsyncFunction(const AsyncFunction&) = delete;
    auto operator=(const AsyncFunction&) = delete;
    AsyncFunction(AsyncFunction&&) = delete;
    auto operator=(AsyncFunction&&) = delete;

    // Constructs an AsyncFunction from the function type.
    template <typename NewFunction>
        requires(!std::is_same_v<std::remove_cvref_t<NewFunction>, AsyncFunction> &&
                 std::is_constructible_v<Function, NewFunction>)
    // NOLINTNEXTLINE(google-explicit-constructor)
    explicit(false) AsyncFunction(NewFunction&& function)
        : mShared(std::make_shared<SharedFunction>(std::forward<NewFunction>(function))) {}

    // Replaces the contained function value with a new one.
    //
    // Returns a finalizer which when invoked waits for calls to the old function value
    // complete. This is done so that the caller can invoke the caller without locks held
    // that might block the call from completing,
    template <typename NewFunction>
        requires(std::is_constructible_v<Function, NewFunction>)
    [[nodiscard]] auto set(NewFunction&& function) -> Finalizer {
        return setInternal(std::make_shared<SharedFunction>(std::forward<NewFunction>(function)));
    }

    // Clears the contained function value.
    //
    // Returns a finalizer which when invoked waits for calls to the old function value
    // complete. This is done so that the caller can invoke the caller without locks held
    // that might block the call from completing,
    [[nodiscard]] auto clear() -> Finalizer { return setInternal(nullptr); }

    // Invoke the contained function, if set.
    template <typename... Args>
        requires(std::is_invocable_v<Function, Args...>)
    auto operator()(Args&&... args) const -> result_type {
        // We might need to retry the process to forward the call if we happen to obtain a zombie
        // shared_ptr. We try at least once.
        bool retry = true;
        while (std::exchange(retry, false)) {
            // To avoid deadlocks, the call to the contained function must be made on a copy of the
            // shared_ptr without the internal locks on the source shared_ptr member data.
            const auto shared = copy();

            // Confirm we got a non-null pointer before continuing. The pointer can be null if no
            // target is set.
            if (shared == nullptr) {
                break;
            }

            // We must hold shared->callingMutex before accessing the other fields it contains.
            std::lock_guard lock(shared->callingMutex);

            // Now that the calling mutex is held, confirm it is valid to use.
            if (!shared->valid) [[unlikely]] {
                // If the pointer isn't valid, we must retry to get a new copy.
                // It indicates another thread set a new target by setting a new pointer after we
                // made our copy, but before we acquired `callingMutex`. Our pointer is effectively
                // a zombie, and must not be used.
                retry = true;
                continue;
            }

            // If `Function` can be (possibly explicitly) converted to bool, it is used as a check
            // at runtime that the function is safe to invoke.
            if constexpr (std::is_constructible_v<bool, Function>) {
                if (!shared->function) {
                    break;
                }
            }

            // Forward the call. Note that callingMutex must be held for the duration of the call.
            return std::invoke(shared->function, std::forward<Args>(args)...);
        }

        // If we reached this point, we had no target to invoke.
        if constexpr (!std::is_void_v<std::invoke_result_t<Function, Args...>>) {
            // If the function was supposed to return a value, we explicitly return `std::nullopt`
            // rather than manufacturing a value of some arbitrary type (perhaps by default
            // construction).
            return std::nullopt;
        }
    }

  private:
    struct SharedFunction final {
        SharedFunction() = default;

        template <typename NewFunction>
            requires(!std::is_same_v<NewFunction, SharedFunction>)
        explicit SharedFunction(NewFunction&& newFunction)
            : function(std::forward<NewFunction>(newFunction)) {}

        // NOLINTBEGIN(misc-non-private-member-variables-in-classes)

        // `callingMutex` must be held for the duration that `function` is invoked.
        mutable std::recursive_mutex callingMutex;
        const Function function;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
        // If valid is false, it means the shared pointer was exchanged to point at a new value, and
        // the function here is no longer safe to invoke.
        bool valid = true;  // GUARDED_BY(callingMutex)

        // NOLINTEND(misc-non-private-member-variables-in-classes)
    };

    [[nodiscard]] auto setInternal(std::shared_ptr<SharedFunction> newShared) -> Finalizer {
        // To avoid deadlocks, the old instance MUST be destroyed outside of all locks, including
        // locks held by the caller. It is the caller's responsibility to invoke the returned
        // finalizer outside of any locks being holds.
        std::shared_ptr<SharedFunction> prior = exchange(std::move(newShared));

        return Finalizer([prior = std::move(prior)]() {
            if (prior) {
                // Wait for any call to complete.
                // Note that callingMutex is a recursive_mutex so that we won't deadlock if the
                // current thread is already holding the same lock.
                std::lock_guard lock(prior->callingMutex);
                // Mark the function as no longer valid to call, on the off chance another thread
                // obtained a copy just before this thread did the exchange.
                prior->valid = false;
            }
        });
    }

    [[nodiscard]] auto exchange(std::shared_ptr<SharedFunction>&& newShared)
            -> std::shared_ptr<SharedFunction> {
        std::lock_guard lock(mMutex);
        return std::exchange(mShared, std::move(newShared));
    }

    [[nodiscard]] auto copy() const -> std::shared_ptr<SharedFunction> {
        std::lock_guard lock(mMutex);
        return mShared;
    }

    // In the future, maybe this can become `std::atomic<std::shared_ptr<Function>>`.
    mutable std::mutex mMutex;
    std::shared_ptr<SharedFunction> mShared GUARDED_BY(mMutex);
};

template <typename Function>
AsyncFunction(Function&&) -> AsyncFunction<std::decay_t<Function>>;

template <typename Signature>
using AsyncFunctionStd = AsyncFunction<std::function<Signature>>;

template <typename Signature>
using AsyncFunctionFtl = AsyncFunction<ftl::Function<Signature>>;

template <typename Signature>
using AsyncFunctionFtl1 = AsyncFunction<ftl::Function<Signature, 1>>;

template <typename Signature>
using AsyncFunctionFtl2 = AsyncFunction<ftl::Function<Signature, 2>>;

template <typename Signature>
using AsyncFunctionFtl3 = AsyncFunction<ftl::Function<Signature, 3>>;

}  // namespace android::surfaceflinger::tests::end2end::test_framework::core
