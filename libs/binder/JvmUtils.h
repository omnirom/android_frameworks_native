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
#if defined(__ANDROID__) && !defined(__ANDROID_RECOVERY__)
#include <dlfcn.h>
#include <jni.h>
extern "C" JavaVM* AndroidRuntimeGetJavaVM();
#endif
namespace {
#if !defined(__ANDROID__) || defined(__ANDROID_RECOVERY__)
static void* getJavaVM() {
    return nullptr;
}
bool isThreadAttachedToJVM() {
    return false;
}
#else
static JavaVM* getJavaVM() {
    static auto fn = reinterpret_cast<decltype(&AndroidRuntimeGetJavaVM)>(
            dlsym(RTLD_DEFAULT, "AndroidRuntimeGetJavaVM"));
    if (fn == nullptr) return nullptr;
    return fn();
}

bool isThreadAttachedToJVM() {
    JNIEnv* env = nullptr;
    JavaVM* vm = getJavaVM();
    if (vm == nullptr || vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_4) < 0) {
        return false;
    }
    return env != nullptr;
}
#endif
} // namespace
