/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <android/binder_rpc.h>
#include <arpa/inet.h>
#include <binder/IServiceManager.h>
#include <linux/vm_sockets.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <variant>

#include "ibinder_internal.h"
#include "status_internal.h"

using ::android::defaultServiceManager;
using ::android::IBinder;
using ::android::IServiceManager;
using ::android::OK;
using ::android::sp;
using ::android::status_t;
using ::android::String16;
using ::android::String8;
using ::android::binder::Status;

#define LOG_ACCESSOR_DEBUG(...)
// #define LOG_ACCESSOR_DEBUG(...) ALOGW(__VA_ARGS__)

struct ABinderRpc_ConnectionInfo {
    std::variant<sockaddr_vm, sockaddr_un, sockaddr_in> addr;
};

struct ABinderRpc_Accessor final : public ::android::RefBase {
    static ABinderRpc_Accessor* make(const char* instance, const sp<IBinder>& binder) {
        LOG_ALWAYS_FATAL_IF(binder == nullptr, "ABinderRpc_Accessor requires a non-null binder");
        status_t status = android::validateAccessor(String16(instance), binder);
        if (status != OK) {
            ALOGE("The given binder is not a valid IAccessor for %s. Status: %s", instance,
                  android::statusToString(status).c_str());
            return nullptr;
        }
        return new ABinderRpc_Accessor(binder);
    }

    sp<IBinder> asBinder() { return mAccessorBinder; }

    ~ABinderRpc_Accessor() { LOG_ACCESSOR_DEBUG("ABinderRpc_Accessor dtor"); }

   private:
    ABinderRpc_Accessor(sp<IBinder> accessor) : mAccessorBinder(accessor) {}
    ABinderRpc_Accessor() = delete;
    sp<IBinder> mAccessorBinder;
};

struct ABinderRpc_AccessorProvider {
   public:
    static ABinderRpc_AccessorProvider* make(std::weak_ptr<android::AccessorProvider> cookie) {
        if (cookie.expired()) {
            ALOGE("Null AccessorProvider cookie from libbinder");
            return nullptr;
        }
        return new ABinderRpc_AccessorProvider(cookie);
    }
    std::weak_ptr<android::AccessorProvider> mProviderCookie;

   private:
    ABinderRpc_AccessorProvider() = delete;

    ABinderRpc_AccessorProvider(std::weak_ptr<android::AccessorProvider> provider)
        : mProviderCookie(provider) {}
};

struct OnDeleteProviderHolder {
    OnDeleteProviderHolder(void* data, ABinderRpc_AccessorProviderUserData_deleteCallback onDelete)
        : mData(data), mOnDelete(onDelete) {}
    ~OnDeleteProviderHolder() {
        if (mOnDelete) {
            mOnDelete(mData);
        }
    }
    void* mData;
    ABinderRpc_AccessorProviderUserData_deleteCallback mOnDelete;
    // needs to be copy-able for std::function, but we will never copy it
    OnDeleteProviderHolder(const OnDeleteProviderHolder&) {
        LOG_ALWAYS_FATAL("This object can't be copied!");
    }

   private:
    OnDeleteProviderHolder() = delete;
};

ABinderRpc_AccessorProvider* ABinderRpc_registerAccessorProvider(
        ABinderRpc_AccessorProvider_getAccessorCallback provider,
        const char* const* const instances, size_t numInstances, void* data,
        ABinderRpc_AccessorProviderUserData_deleteCallback onDelete) {
    if (data && onDelete == nullptr) {
        ALOGE("If a non-null data ptr is passed to ABinderRpc_registerAccessorProvider, then a "
              "ABinderRpc_AccessorProviderUserData_deleteCallback must also be passed to delete "
              "the data object once the ABinderRpc_AccessorProvider is removed.");
        return nullptr;
    }
    // call the onDelete when the last reference of this goes away (when the
    // last reference to the generate std::function goes away).
    std::shared_ptr<OnDeleteProviderHolder> onDeleteHolder =
            std::make_shared<OnDeleteProviderHolder>(data, onDelete);
    if (provider == nullptr) {
        ALOGE("Null provider passed to ABinderRpc_registerAccessorProvider");
        return nullptr;
    }
    if (numInstances == 0 || instances == nullptr) {
        ALOGE("No instances passed to ABinderRpc_registerAccessorProvider. numInstances: %zu",
              numInstances);
        return nullptr;
    }
    std::set<std::string> instanceStrings;
    for (size_t i = 0; i < numInstances; i++) {
        instanceStrings.emplace(instances[i]);
    }
    android::RpcAccessorProvider generate = [provider,
                                             onDeleteHolder](const String16& name) -> sp<IBinder> {
        ABinderRpc_Accessor* accessor = provider(String8(name).c_str(), onDeleteHolder->mData);
        if (accessor == nullptr) {
            ALOGE("The supplied ABinderRpc_AccessorProvider_getAccessorCallback returned nullptr");
            return nullptr;
        }
        sp<IBinder> binder = accessor->asBinder();
        ABinderRpc_Accessor_delete(accessor);
        return binder;
    };

    std::weak_ptr<android::AccessorProvider> cookie =
            android::addAccessorProvider(std::move(instanceStrings), std::move(generate));
    return ABinderRpc_AccessorProvider::make(cookie);
}

void ABinderRpc_unregisterAccessorProvider(ABinderRpc_AccessorProvider* provider) {
    if (provider == nullptr) {
        LOG_ALWAYS_FATAL("Attempting to remove a null ABinderRpc_AccessorProvider");
    }

    status_t status = android::removeAccessorProvider(provider->mProviderCookie);
    // There shouldn't be a way to get here because the caller won't have a
    // ABinderRpc_AccessorProvider* without calling ABinderRpc_registerAccessorProvider
    LOG_ALWAYS_FATAL_IF(status == android::BAD_VALUE, "Provider (%p) is not valid. Status: %s",
                        provider, android::statusToString(status).c_str());
    LOG_ALWAYS_FATAL_IF(status == android::NAME_NOT_FOUND,
                        "Provider (%p) was already unregistered. Status: %s", provider,
                        android::statusToString(status).c_str());
    LOG_ALWAYS_FATAL_IF(status != OK,
                        "Unknown error when attempting to unregister ABinderRpc_AccessorProvider "
                        "(%p). Status: %s",
                        provider, android::statusToString(status).c_str());

    delete provider;
}

struct OnDeleteConnectionInfoHolder {
    OnDeleteConnectionInfoHolder(void* data,
                                 ABinderRpc_ConnectionInfoProviderUserData_delete onDelete)
        : mData(data), mOnDelete(onDelete) {}
    ~OnDeleteConnectionInfoHolder() {
        if (mOnDelete) {
            mOnDelete(mData);
        }
    }
    void* mData;
    ABinderRpc_ConnectionInfoProviderUserData_delete mOnDelete;
    // needs to be copy-able for std::function, but we will never copy it
    OnDeleteConnectionInfoHolder(const OnDeleteConnectionInfoHolder&) {
        LOG_ALWAYS_FATAL("This object can't be copied!");
    }

   private:
    OnDeleteConnectionInfoHolder() = delete;
};

ABinderRpc_Accessor* ABinderRpc_Accessor_new(
        const char* instance, ABinderRpc_ConnectionInfoProvider provider, void* data,
        ABinderRpc_ConnectionInfoProviderUserData_delete onDelete) {
    if (instance == nullptr) {
        ALOGE("Instance argument must be valid when calling ABinderRpc_Accessor_new");
        return nullptr;
    }
    if (data && onDelete == nullptr) {
        ALOGE("If a non-null data ptr is passed to ABinderRpc_Accessor_new, then a "
              "ABinderRpc_ConnectionInfoProviderUserData_delete callback must also be passed to "
              "delete "
              "the data object once the ABinderRpc_Accessor is deleted.");
        return nullptr;
    }
    std::shared_ptr<OnDeleteConnectionInfoHolder> onDeleteHolder =
            std::make_shared<OnDeleteConnectionInfoHolder>(data, onDelete);
    if (provider == nullptr) {
        ALOGE("Can't create a new ABinderRpc_Accessor without a ABinderRpc_ConnectionInfoProvider "
              "and it is "
              "null");
        return nullptr;
    }
    android::RpcSocketAddressProvider generate = [provider, onDeleteHolder](
                                                         const String16& name, sockaddr* outAddr,
                                                         size_t addrLen) -> status_t {
        std::unique_ptr<ABinderRpc_ConnectionInfo> info(
                provider(String8(name).c_str(), onDeleteHolder->mData));
        if (info == nullptr) {
            ALOGE("The supplied ABinderRpc_ConnectionInfoProvider returned nullptr");
            return android::NAME_NOT_FOUND;
        }
        if (auto addr = std::get_if<sockaddr_vm>(&info->addr)) {
            LOG_ALWAYS_FATAL_IF(addr->svm_family != AF_VSOCK,
                                "ABinderRpc_ConnectionInfo invalid family");
            if (addrLen < sizeof(sockaddr_vm)) {
                ALOGE("Provided outAddr is too small! Expecting %zu, got %zu", sizeof(sockaddr_vm),
                      addrLen);
                return android::BAD_VALUE;
            }
            LOG_ACCESSOR_DEBUG(
                    "Connection info provider found AF_VSOCK. family %d, port %d, cid %d",
                    addr->svm_family, addr->svm_port, addr->svm_cid);
            *reinterpret_cast<sockaddr_vm*>(outAddr) = *addr;
        } else if (auto addr = std::get_if<sockaddr_un>(&info->addr)) {
            LOG_ALWAYS_FATAL_IF(addr->sun_family != AF_UNIX,
                                "ABinderRpc_ConnectionInfo invalid family");
            if (addrLen < sizeof(sockaddr_un)) {
                ALOGE("Provided outAddr is too small! Expecting %zu, got %zu", sizeof(sockaddr_un),
                      addrLen);
                return android::BAD_VALUE;
            }
            *reinterpret_cast<sockaddr_un*>(outAddr) = *addr;
        } else if (auto addr = std::get_if<sockaddr_in>(&info->addr)) {
            LOG_ALWAYS_FATAL_IF(addr->sin_family != AF_INET,
                                "ABinderRpc_ConnectionInfo invalid family");
            if (addrLen < sizeof(sockaddr_in)) {
                ALOGE("Provided outAddr is too small! Expecting %zu, got %zu", sizeof(sockaddr_in),
                      addrLen);
                return android::BAD_VALUE;
            }
            *reinterpret_cast<sockaddr_in*>(outAddr) = *addr;
        } else {
            LOG_ALWAYS_FATAL(
                    "Unsupported address family type when trying to get ARpcConnection info. A "
                    "new variant was added to the ABinderRpc_ConnectionInfo and this needs to be "
                    "updated.");
        }
        return STATUS_OK;
    };
    sp<IBinder> accessorBinder = android::createAccessor(String16(instance), std::move(generate));
    if (accessorBinder == nullptr) {
        ALOGE("service manager did not get us an accessor");
        return nullptr;
    }
    LOG_ACCESSOR_DEBUG("service manager found an accessor, so returning one now from _new");
    return ABinderRpc_Accessor::make(instance, accessorBinder);
}

void ABinderRpc_Accessor_delete(ABinderRpc_Accessor* accessor) {
    delete accessor;
}

AIBinder* ABinderRpc_Accessor_asBinder(ABinderRpc_Accessor* accessor) {
    if (!accessor) {
        ALOGE("ABinderRpc_Accessor argument is null.");
        return nullptr;
    }

    sp<IBinder> binder = accessor->asBinder();
    sp<AIBinder> aBinder = ABpBinder::lookupOrCreateFromBinder(binder);
    AIBinder* ptr = aBinder.get();
    if (ptr == nullptr) {
        LOG_ALWAYS_FATAL("Failed to lookupOrCreateFromBinder");
    }
    ptr->incStrong(nullptr);
    return ptr;
}

ABinderRpc_Accessor* ABinderRpc_Accessor_fromBinder(const char* instance, AIBinder* binder) {
    if (!binder) {
        ALOGE("binder argument is null");
        return nullptr;
    }
    sp<IBinder> accessorBinder = binder->getBinder();
    if (accessorBinder) {
        return ABinderRpc_Accessor::make(instance, accessorBinder);
    } else {
        ALOGE("Attempting to get an ABinderRpc_Accessor for %s but AIBinder::getBinder returned "
              "null",
              instance);
        return nullptr;
    }
}

binder_status_t ABinderRpc_Accessor_delegateAccessor(const char* instance, AIBinder* accessor,
                                                     AIBinder** outDelegator) {
    LOG_ALWAYS_FATAL_IF(outDelegator == nullptr, "The outDelegator argument is null");
    if (instance == nullptr || accessor == nullptr) {
        ALOGW("instance or accessor arguments to ABinderRpc_Accessor_delegateBinder are null");
        *outDelegator = nullptr;
        return STATUS_UNEXPECTED_NULL;
    }
    sp<IBinder> accessorBinder = accessor->getBinder();

    sp<IBinder> delegator;
    status_t status = android::delegateAccessor(String16(instance), accessorBinder, &delegator);
    if (status != OK) {
        return PruneStatusT(status);
    }
    sp<AIBinder> binder = ABpBinder::lookupOrCreateFromBinder(delegator);
    // This AIBinder needs a strong ref to pass ownership to the caller
    binder->incStrong(nullptr);
    *outDelegator = binder.get();
    return STATUS_OK;
}

ABinderRpc_ConnectionInfo* ABinderRpc_ConnectionInfo_new(const sockaddr* addr, socklen_t len) {
    if (addr == nullptr || len < 0 || static_cast<size_t>(len) < sizeof(sa_family_t)) {
        ALOGE("Invalid arguments in ABinderRpc_Connection_new");
        return nullptr;
    }
    // socklen_t was int32_t on 32-bit and uint32_t on 64 bit.
    size_t socklen = len < 0 || static_cast<uintmax_t>(len) > SIZE_MAX ? 0 : len;

    if (addr->sa_family == AF_VSOCK) {
        if (len != sizeof(sockaddr_vm)) {
            ALOGE("Incorrect size of %zu for AF_VSOCK sockaddr_vm. Expecting %zu", socklen,
                  sizeof(sockaddr_vm));
            return nullptr;
        }
        sockaddr_vm vm = *reinterpret_cast<const sockaddr_vm*>(addr);
        LOG_ACCESSOR_DEBUG(
                "ABinderRpc_ConnectionInfo_new found AF_VSOCK. family %d, port %d, cid %d",
                vm.svm_family, vm.svm_port, vm.svm_cid);
        return new ABinderRpc_ConnectionInfo(vm);
    } else if (addr->sa_family == AF_UNIX) {
        if (len != sizeof(sockaddr_un)) {
            ALOGE("Incorrect size of %zu for AF_UNIX sockaddr_un. Expecting %zu", socklen,
                  sizeof(sockaddr_un));
            return nullptr;
        }
        sockaddr_un un = *reinterpret_cast<const sockaddr_un*>(addr);
        LOG_ACCESSOR_DEBUG("ABinderRpc_ConnectionInfo_new found AF_UNIX. family %d, path %s",
                           un.sun_family, un.sun_path);
        return new ABinderRpc_ConnectionInfo(un);
    } else if (addr->sa_family == AF_INET) {
        if (len != sizeof(sockaddr_in)) {
            ALOGE("Incorrect size of %zu for AF_INET sockaddr_in. Expecting %zu", socklen,
                  sizeof(sockaddr_in));
            return nullptr;
        }
        sockaddr_in in = *reinterpret_cast<const sockaddr_in*>(addr);
        LOG_ACCESSOR_DEBUG(
                "ABinderRpc_ConnectionInfo_new found AF_INET. family %d, address %s, port %d",
                in.sin_family, inet_ntoa(in.sin_addr), ntohs(in.sin_port));
        return new ABinderRpc_ConnectionInfo(in);
    }

    ALOGE("ABinderRpc APIs only support AF_VSOCK right now but the supplied sockaddr::sa_family "
          "is: %hu",
          addr->sa_family);
    return nullptr;
}

void ABinderRpc_ConnectionInfo_delete(ABinderRpc_ConnectionInfo* info) {
    delete info;
}
