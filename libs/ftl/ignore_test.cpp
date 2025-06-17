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

#include <string>

#include <ftl/ignore.h>
#include <gtest/gtest.h>

namespace android::test {
namespace {

// Keep in sync with the example usage in the header file.

void ftl_ignore_multiple(int arg1, const char* arg2, std::string arg3) {
  // When invoked, all the arguments are ignored.
  ftl::ignore(arg1, arg2, arg3);
}

void ftl_ignore_single(int arg) {
  // It can be used like std::ignore to ignore a single value
  ftl::ignore = arg;
}

}  // namespace

TEST(Ignore, Example) {
  // The real example test is that there are no compiler warnings for unused arguments above.

  // Use the example functions to avoid a compiler warning about unused functions.
  ftl_ignore_multiple(0, "a", "b");
  ftl_ignore_single(0);
}

}  // namespace android::test
