/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include "InputThread.h"

#include <functional>

#include <android-base/logging.h>
#include <com_android_input_flags.h>
#include <processgroup/processgroup.h>
#include "jni.h"

namespace android {

namespace input_flags = com::android::input::flags;

namespace {

bool applyInputEventProfile() {
    static constexpr pid_t CURRENT_THREAD = 0;
    return SetTaskProfiles(CURRENT_THREAD, {"InputPolicy"});
}

class JvmAttacher {
public:
    JvmAttacher(JNIEnv* env, const std::string& name) {
        if (env == nullptr) {
            LOG(INFO) << "env is nullptr for thread " << name;
            return;
        }
        env->GetJavaVM(&mVm);
        LOG_IF(FATAL, mVm == nullptr) << "Could not get JavaVM from provided JNIEnv";

        JavaVMAttachArgs args{
                .version = JNI_VERSION_1_6,
                .name = name.c_str(),
                .group = nullptr,
        };
        if (mVm->AttachCurrentThread(&env, &args) != JNI_OK) {
            LOG(FATAL) << "Cannot attach thread " << name << " to Java VM.";
        }
    }

    ~JvmAttacher() {
        if (mVm != nullptr && mVm->DetachCurrentThread() != JNI_OK) {
            LOG(FATAL) << "Cannot detach thread from Java VM.";
        }
    }

private:
    JavaVM* mVm = nullptr;
};

} // namespace

InputThread::InputThread(std::string name, std::function<void()> loop, std::function<void()> wake,
                         bool isInCriticalPath, JNIEnv* env)
      : mThreadWake(wake) {
    std::thread loopThread{[this, name, isInCriticalPath, loop, env] {
        if (input_flags::enable_input_policy_profile() && isInCriticalPath) {
            if (!applyInputEventProfile()) {
                LOG(ERROR) << "Couldn't apply input policy profile for " << name;
            }
        }

        JvmAttacher jvmAttacher(env, name);

        while (!mStopThread) {
            loop();
        }
    }};
    mThread = std::move(loopThread);
}

InputThread::~InputThread() {
    mStopThread = true;
    if (mThreadWake) {
        mThreadWake();
    }
    mThread.join();
}

bool InputThread::isCallingThread() {
    return std::this_thread::get_id() == mThread.get_id();
}

} // namespace android
