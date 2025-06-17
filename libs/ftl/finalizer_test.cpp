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

#include <memory>
#include <type_traits>
#include <utility>

#include <ftl/finalizer.h>
#include <gtest/gtest.h>

namespace android::test {

namespace {

struct Counter {
  constexpr auto increment_fn() {
    return [this] { ++value_; };
  }

  auto increment_finalizer() {
    return ftl::Finalizer([this] { ++value_; });
  }

  [[nodiscard]] constexpr auto value() const -> int { return value_; }

 private:
  int value_ = 0;
};

struct CounterPair {
  constexpr auto increment_first_fn() { return first.increment_fn(); }
  constexpr auto increment_second_fn() { return second.increment_fn(); }
  [[nodiscard]] constexpr auto values() const -> std::pair<int, int> {
    return {first.value(), second.value()};
  }

 private:
  Counter first;
  Counter second;
};

}  // namespace

TEST(Finalizer, DefaultConstructionAndNoOpDestructionWhenPolymorphicType) {
  ftl::FinalizerStd finalizer1;
  ftl::FinalizerFtl finalizer2;
  ftl::FinalizerFtl1 finalizer3;
  ftl::FinalizerFtl2 finalizer4;
  ftl::FinalizerFtl3 finalizer5;
}

TEST(Finalizer, InvokesTheFunctionOnDestruction) {
  Counter counter;
  {
    const auto finalizer = counter.increment_finalizer();
    EXPECT_EQ(counter.value(), 0);
  }
  EXPECT_EQ(counter.value(), 1);
}

TEST(Finalizer, InvocationCanBeCanceled) {
  Counter counter;
  {
    auto finalizer = counter.increment_finalizer();
    EXPECT_EQ(counter.value(), 0);
    finalizer.cancel();
    EXPECT_EQ(counter.value(), 0);
  }
  EXPECT_EQ(counter.value(), 0);
}

TEST(Finalizer, InvokesTheFunctionOnce) {
  Counter counter;
  {
    auto finalizer = counter.increment_finalizer();
    EXPECT_EQ(counter.value(), 0);
    finalizer();
    EXPECT_EQ(counter.value(), 1);
    finalizer();
    EXPECT_EQ(counter.value(), 1);
  }
  EXPECT_EQ(counter.value(), 1);
}

TEST(Finalizer, SelfInvocationIsAllowedAndANoOp) {
  Counter counter;
  ftl::FinalizerStd finalizer;
  finalizer = ftl::Finalizer([&]() {
    counter.increment_fn()();
    finalizer();  // recursive invocation should do nothing.
  });
  EXPECT_EQ(counter.value(), 0);
  finalizer();
  EXPECT_EQ(counter.value(), 1);
}

TEST(Finalizer, MoveConstruction) {
  Counter counter;
  {
    ftl::FinalizerStd outer_finalizer = counter.increment_finalizer();
    EXPECT_EQ(counter.value(), 0);
    {
      ftl::FinalizerStd inner_finalizer = std::move(outer_finalizer);
      static_assert(std::is_same_v<decltype(inner_finalizer), decltype(outer_finalizer)>);
      EXPECT_EQ(counter.value(), 0);
    }
    EXPECT_EQ(counter.value(), 1);
  }
  EXPECT_EQ(counter.value(), 1);
}

TEST(Finalizer, MoveConstructionWithImplicitConversion) {
  Counter counter;
  {
    auto outer_finalizer = counter.increment_finalizer();
    EXPECT_EQ(counter.value(), 0);
    {
      ftl::FinalizerStd inner_finalizer = std::move(outer_finalizer);
      static_assert(!std::is_same_v<decltype(inner_finalizer), decltype(outer_finalizer)>);
      EXPECT_EQ(counter.value(), 0);
    }
    EXPECT_EQ(counter.value(), 1);
  }
  EXPECT_EQ(counter.value(), 1);
}

TEST(Finalizer, MoveAssignment) {
  CounterPair pair;
  {
    ftl::FinalizerStd outer_finalizer = ftl::Finalizer(pair.increment_first_fn());
    EXPECT_EQ(pair.values(), std::make_pair(0, 0));

    {
      ftl::FinalizerStd inner_finalizer = ftl::Finalizer(pair.increment_second_fn());
      static_assert(std::is_same_v<decltype(inner_finalizer), decltype(outer_finalizer)>);
      EXPECT_EQ(pair.values(), std::make_pair(0, 0));
      inner_finalizer = std::move(outer_finalizer);
      EXPECT_EQ(pair.values(), std::make_pair(0, 1));
    }
    EXPECT_EQ(pair.values(), std::make_pair(1, 1));
  }
  EXPECT_EQ(pair.values(), std::make_pair(1, 1));
}

TEST(Finalizer, MoveAssignmentWithImplicitConversion) {
  CounterPair pair;
  {
    auto outer_finalizer = ftl::Finalizer(pair.increment_first_fn());
    EXPECT_EQ(pair.values(), std::make_pair(0, 0));

    {
      ftl::FinalizerStd inner_finalizer = ftl::Finalizer(pair.increment_second_fn());
      static_assert(!std::is_same_v<decltype(inner_finalizer), decltype(outer_finalizer)>);
      EXPECT_EQ(pair.values(), std::make_pair(0, 0));
      inner_finalizer = std::move(outer_finalizer);
      EXPECT_EQ(pair.values(), std::make_pair(0, 1));
    }
    EXPECT_EQ(pair.values(), std::make_pair(1, 1));
  }
  EXPECT_EQ(pair.values(), std::make_pair(1, 1));
}

TEST(Finalizer, NullifiesTheFunctionWhenInvokedIfPossible) {
  auto shared = std::make_shared<int>(0);
  std::weak_ptr<int> weak = shared;

  int count = 0;
  {
    auto lambda = [capture = std::move(shared)]() {};
    auto finalizer = ftl::Finalizer(std::move(lambda));
    EXPECT_FALSE(weak.expired());

    // A lambda is not nullable. Invoking the finalizer cannot destroy it to destroy the lambda's
    // capture.
    finalizer();
    EXPECT_FALSE(weak.expired());
  }
  // The lambda is only destroyed when the finalizer instance is destroyed.
  EXPECT_TRUE(weak.expired());

  shared = std::make_shared<int>(0);
  weak = shared;

  {
    auto lambda = [capture = std::move(shared)]() {};
    auto finalizer = ftl::FinalizerStd(std::move(lambda));
    EXPECT_FALSE(weak.expired());

    // Since std::function is used, and is nullable, invoking the finalizer will destroy the
    // contained function, which will destroy the lambda's capture.
    finalizer();
    EXPECT_TRUE(weak.expired());
  }
}

}  // namespace android::test
