/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _BINDER_MODULE_H_
#define _BINDER_MODULE_H_

/* obtain structures and constants from the kernel header */

// TODO(b/31559095): bionic on host
#ifndef __ANDROID__
#define __packed __attribute__((__packed__))
#endif

// TODO(b/31559095): bionic on host
#if defined(B_PACK_CHARS) && !defined(_UAPI_LINUX_BINDER_H)
#undef B_PACK_CHARS
#endif

#include <linux/android/binder.h>
#include <sys/ioctl.h>

struct binder_frozen_state_info {
    binder_uintptr_t cookie;
    __u32 is_frozen;
};

#ifndef BR_FROZEN_BINDER
// Temporary definition of BR_FROZEN_BINDER until UAPI binder.h includes it.
#define BR_FROZEN_BINDER _IOR('r', 21, struct binder_frozen_state_info)
#endif // BR_FROZEN_BINDER

#ifndef BR_CLEAR_FREEZE_NOTIFICATION_DONE
// Temporary definition of BR_CLEAR_FREEZE_NOTIFICATION_DONE until UAPI binder.h includes it.
#define BR_CLEAR_FREEZE_NOTIFICATION_DONE _IOR('r', 22, binder_uintptr_t)
#endif // BR_CLEAR_FREEZE_NOTIFICATION_DONE

#ifndef BC_REQUEST_FREEZE_NOTIFICATION
// Temporary definition of BC_REQUEST_FREEZE_NOTIFICATION until UAPI binder.h includes it.
#define BC_REQUEST_FREEZE_NOTIFICATION _IOW('c', 19, struct binder_handle_cookie)
#endif // BC_REQUEST_FREEZE_NOTIFICATION

#ifndef BC_CLEAR_FREEZE_NOTIFICATION
// Temporary definition of BC_CLEAR_FREEZE_NOTIFICATION until UAPI binder.h includes it.
#define BC_CLEAR_FREEZE_NOTIFICATION _IOW('c', 20, struct binder_handle_cookie)
#endif // BC_CLEAR_FREEZE_NOTIFICATION

#ifndef BC_FREEZE_NOTIFICATION_DONE
// Temporary definition of BC_FREEZE_NOTIFICATION_DONE until UAPI binder.h includes it.
#define BC_FREEZE_NOTIFICATION_DONE _IOW('c', 21, binder_uintptr_t)
#endif // BC_FREEZE_NOTIFICATION_DONE

#endif // _BINDER_MODULE_H_
