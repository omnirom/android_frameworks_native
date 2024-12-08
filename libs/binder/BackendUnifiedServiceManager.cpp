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
#include "BackendUnifiedServiceManager.h"

#include <android/os/IAccessor.h>
#include <binder/RpcSession.h>

#if defined(__BIONIC__) && !defined(__ANDROID_VNDK__)
#include <android-base/properties.h>
#endif

namespace android {

#ifdef LIBBINDER_CLIENT_CACHE
constexpr bool kUseCache = true;
#else
constexpr bool kUseCache = false;
#endif

using AidlServiceManager = android::os::IServiceManager;
using IAccessor = android::os::IAccessor;

static const char* kStaticCachableList[] = {
        // go/keep-sorted start
        "accessibility",
        "account",
        "activity",
        "alarm",
        "android.system.keystore2.IKeystoreService/default",
        "appops",
        "audio",
        "batterystats",
        "carrier_config",
        "connectivity",
        "content",
        "content_capture",
        "device_policy",
        "display",
        "dropbox",
        "econtroller",
        "graphicsstats",
        "input",
        "input_method",
        "isub",
        "jobscheduler",
        "legacy_permission",
        "location",
        "media.extractor",
        "media.metrics",
        "media.player",
        "media.resource_manager",
        "media_resource_monitor",
        "mount",
        "netd_listener",
        "netstats",
        "network_management",
        "nfc",
        "notification",
        "package",
        "package_native",
        "performance_hint",
        "permission",
        "permission_checker",
        "permissionmgr",
        "phone",
        "platform_compat",
        "power",
        "role",
        "sensorservice",
        "statscompanion",
        "telephony.registry",
        "thermalservice",
        "time_detector",
        "trust",
        "uimode",
        "user",
        "virtualdevice",
        "virtualdevice_native",
        "webviewupdate",
        "window",
        // go/keep-sorted end
};

bool BinderCacheWithInvalidation::isClientSideCachingEnabled(const std::string& serviceName) {
    if (ProcessState::self()->getThreadPoolMaxTotalThreadCount() <= 0) {
        ALOGW("Thread Pool max thread count is 0. Cannot cache binder as linkToDeath cannot be "
              "implemented. serviceName: %s",
              serviceName.c_str());
        return false;
    }
    for (const char* name : kStaticCachableList) {
        if (name == serviceName) {
            return true;
        }
    }
    return false;
}

binder::Status BackendUnifiedServiceManager::updateCache(const std::string& serviceName,
                                                         const os::Service& service) {
    if (!kUseCache) {
        return binder::Status::ok();
    }
    if (service.getTag() == os::Service::Tag::binder) {
        sp<IBinder> binder = service.get<os::Service::Tag::binder>();
        if (binder && mCacheForGetService->isClientSideCachingEnabled(serviceName) &&
            binder->isBinderAlive()) {
            return mCacheForGetService->setItem(serviceName, binder);
        }
    }
    return binder::Status::ok();
}

bool BackendUnifiedServiceManager::returnIfCached(const std::string& serviceName,
                                                  os::Service* _out) {
    if (!kUseCache) {
        return false;
    }
    sp<IBinder> item = mCacheForGetService->getItem(serviceName);
    // TODO(b/363177618): Enable caching for binders which are always null.
    if (item != nullptr && item->isBinderAlive()) {
        *_out = os::Service::make<os::Service::Tag::binder>(item);
        return true;
    }
    return false;
}

BackendUnifiedServiceManager::BackendUnifiedServiceManager(const sp<AidlServiceManager>& impl)
      : mTheRealServiceManager(impl) {
    mCacheForGetService = std::make_shared<BinderCacheWithInvalidation>();
}

sp<AidlServiceManager> BackendUnifiedServiceManager::getImpl() {
    return mTheRealServiceManager;
}

binder::Status BackendUnifiedServiceManager::getService(const ::std::string& name,
                                                        sp<IBinder>* _aidl_return) {
    os::Service service;
    binder::Status status = getService2(name, &service);
    *_aidl_return = service.get<os::Service::Tag::binder>();
    return status;
}

binder::Status BackendUnifiedServiceManager::getService2(const ::std::string& name,
                                                         os::Service* _out) {
    if (returnIfCached(name, _out)) {
        return binder::Status::ok();
    }
    os::Service service;
    binder::Status status = mTheRealServiceManager->getService2(name, &service);

    if (status.isOk()) {
        status = toBinderService(name, service, _out);
        if (status.isOk()) {
            return updateCache(name, service);
        }
    }
    return status;
}

binder::Status BackendUnifiedServiceManager::checkService(const ::std::string& name,
                                                          os::Service* _out) {
    os::Service service;
    if (returnIfCached(name, _out)) {
        return binder::Status::ok();
    }

    binder::Status status = mTheRealServiceManager->checkService(name, &service);
    if (status.isOk()) {
        status = toBinderService(name, service, _out);
        if (status.isOk()) {
            return updateCache(name, service);
        }
    }
    return status;
}

binder::Status BackendUnifiedServiceManager::toBinderService(const ::std::string& name,
                                                             const os::Service& in,
                                                             os::Service* _out) {
    switch (in.getTag()) {
        case os::Service::Tag::binder: {
            if (in.get<os::Service::Tag::binder>() == nullptr) {
                // failed to find a service. Check to see if we have any local
                // injected Accessors for this service.
                os::Service accessor;
                binder::Status status = getInjectedAccessor(name, &accessor);
                if (!status.isOk()) {
                    *_out = os::Service::make<os::Service::Tag::binder>(nullptr);
                    return status;
                }
                if (accessor.getTag() == os::Service::Tag::accessor &&
                    accessor.get<os::Service::Tag::accessor>() != nullptr) {
                    ALOGI("Found local injected service for %s, will attempt to create connection",
                          name.c_str());
                    // Call this again using the accessor Service to get the real
                    // service's binder into _out
                    return toBinderService(name, accessor, _out);
                }
            }

            *_out = in;
            return binder::Status::ok();
        }
        case os::Service::Tag::accessor: {
            sp<IBinder> accessorBinder = in.get<os::Service::Tag::accessor>();
            sp<IAccessor> accessor = interface_cast<IAccessor>(accessorBinder);
            if (accessor == nullptr) {
                ALOGE("Service#accessor doesn't have accessor. VM is maybe starting...");
                *_out = os::Service::make<os::Service::Tag::binder>(nullptr);
                return binder::Status::ok();
            }
            auto request = [=] {
                os::ParcelFileDescriptor fd;
                binder::Status ret = accessor->addConnection(&fd);
                if (ret.isOk()) {
                    return base::unique_fd(fd.release());
                } else {
                    ALOGE("Failed to connect to RpcSession: %s", ret.toString8().c_str());
                    return base::unique_fd(-1);
                }
            };
            auto session = RpcSession::make();
            status_t status = session->setupPreconnectedClient(base::unique_fd{}, request);
            if (status != OK) {
                ALOGE("Failed to set up preconnected binder RPC client: %s",
                      statusToString(status).c_str());
                return binder::Status::fromStatusT(status);
            }
            session->setSessionSpecificRoot(accessorBinder);
            *_out = os::Service::make<os::Service::Tag::binder>(session->getRootObject());
            return binder::Status::ok();
        }
        default: {
            LOG_ALWAYS_FATAL("Unknown service type: %d", in.getTag());
        }
    }
}

binder::Status BackendUnifiedServiceManager::addService(const ::std::string& name,
                                                        const sp<IBinder>& service,
                                                        bool allowIsolated, int32_t dumpPriority) {
    return mTheRealServiceManager->addService(name, service, allowIsolated, dumpPriority);
}
binder::Status BackendUnifiedServiceManager::listServices(
        int32_t dumpPriority, ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->listServices(dumpPriority, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::registerForNotifications(
        const ::std::string& name, const sp<os::IServiceCallback>& callback) {
    return mTheRealServiceManager->registerForNotifications(name, callback);
}
binder::Status BackendUnifiedServiceManager::unregisterForNotifications(
        const ::std::string& name, const sp<os::IServiceCallback>& callback) {
    return mTheRealServiceManager->unregisterForNotifications(name, callback);
}
binder::Status BackendUnifiedServiceManager::isDeclared(const ::std::string& name,
                                                        bool* _aidl_return) {
    return mTheRealServiceManager->isDeclared(name, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::getDeclaredInstances(
        const ::std::string& iface, ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->getDeclaredInstances(iface, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::updatableViaApex(
        const ::std::string& name, ::std::optional<::std::string>* _aidl_return) {
    return mTheRealServiceManager->updatableViaApex(name, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::getUpdatableNames(
        const ::std::string& apexName, ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->getUpdatableNames(apexName, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::getConnectionInfo(
        const ::std::string& name, ::std::optional<os::ConnectionInfo>* _aidl_return) {
    return mTheRealServiceManager->getConnectionInfo(name, _aidl_return);
}
binder::Status BackendUnifiedServiceManager::registerClientCallback(
        const ::std::string& name, const sp<IBinder>& service,
        const sp<os::IClientCallback>& callback) {
    return mTheRealServiceManager->registerClientCallback(name, service, callback);
}
binder::Status BackendUnifiedServiceManager::tryUnregisterService(const ::std::string& name,
                                                                  const sp<IBinder>& service) {
    return mTheRealServiceManager->tryUnregisterService(name, service);
}
binder::Status BackendUnifiedServiceManager::getServiceDebugInfo(
        ::std::vector<os::ServiceDebugInfo>* _aidl_return) {
    return mTheRealServiceManager->getServiceDebugInfo(_aidl_return);
}

[[clang::no_destroy]] static std::once_flag gUSmOnce;
[[clang::no_destroy]] static sp<BackendUnifiedServiceManager> gUnifiedServiceManager;

sp<BackendUnifiedServiceManager> getBackendUnifiedServiceManager() {
    std::call_once(gUSmOnce, []() {
#if defined(__BIONIC__) && !defined(__ANDROID_VNDK__)
        /* wait for service manager */ {
            using std::literals::chrono_literals::operator""s;
            using android::base::WaitForProperty;
            while (!WaitForProperty("servicemanager.ready", "true", 1s)) {
                ALOGE("Waited for servicemanager.ready for a second, waiting another...");
            }
        }
#endif

        sp<AidlServiceManager> sm = nullptr;
        while (sm == nullptr) {
            sm = interface_cast<AidlServiceManager>(
                    ProcessState::self()->getContextObject(nullptr));
            if (sm == nullptr) {
                ALOGE("Waiting 1s on context object on %s.",
                      ProcessState::self()->getDriverName().c_str());
                sleep(1);
            }
        }

        gUnifiedServiceManager = sp<BackendUnifiedServiceManager>::make(sm);
    });

    return gUnifiedServiceManager;
}

} // namespace android
