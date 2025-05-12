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
#include <android/os/IServiceManager.h>
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

#ifdef LIBBINDER_ADDSERVICE_CACHE
constexpr bool kUseCacheInAddService = true;
#else
constexpr bool kUseCacheInAddService = false;
#endif

#ifdef LIBBINDER_REMOVE_CACHE_STATIC_LIST
constexpr bool kRemoveStaticList = true;
#else
constexpr bool kRemoveStaticList = false;
#endif

using AidlServiceManager = android::os::IServiceManager;
using android::os::IAccessor;
using binder::Status;

static const char* kStaticCachableList[] = {
        // go/keep-sorted start
        "accessibility",
        "account",
        "activity",
        "alarm",
        "android.frameworks.stats.IStats/default",
        "android.system.keystore2.IKeystoreService/default",
        "appops",
        "audio",
        "autofill",
        "batteryproperties",
        "batterystats",
        "biometic",
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
        "lock_settings",
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
        "processinfo",
        "role",
        "sensitive_content_protection_service",
        "sensorservice",
        "statscompanion",
        "telephony.registry",
        "thermalservice",
        "time_detector",
        "tracing.proxy",
        "trust",
        "uimode",
        "user",
        "vibrator",
        "virtualdevice",
        "virtualdevice_native",
        "webviewupdate",
        "window",
        // go/keep-sorted end
};

os::ServiceWithMetadata createServiceWithMetadata(const sp<IBinder>& service, bool isLazyService) {
    os::ServiceWithMetadata out = os::ServiceWithMetadata();
    out.service = service;
    out.isLazyService = isLazyService;
    return out;
}

bool BinderCacheWithInvalidation::isClientSideCachingEnabled(const std::string& serviceName) {
    sp<ProcessState> self = ProcessState::selfOrNull();
    if (!self || self->getThreadPoolMaxTotalThreadCount() <= 0) {
        ALOGW("Thread Pool max thread count is 0. Cannot cache binder as linkToDeath cannot be "
              "implemented. serviceName: %s",
              serviceName.c_str());
        return false;
    }
    if (kRemoveStaticList) return true;
    for (const char* name : kStaticCachableList) {
        if (name == serviceName) {
            return true;
        }
    }
    return false;
}

Status BackendUnifiedServiceManager::updateCache(const std::string& serviceName,
                                                 const os::Service& service) {
    if (!kUseCache) {
        return Status::ok();
    }

    if (service.getTag() == os::Service::Tag::serviceWithMetadata) {
        auto serviceWithMetadata = service.get<os::Service::Tag::serviceWithMetadata>();
        return updateCache(serviceName, serviceWithMetadata.service,
                           serviceWithMetadata.isLazyService);
    }
    return Status::ok();
}

Status BackendUnifiedServiceManager::updateCache(const std::string& serviceName,
                                                 const sp<IBinder>& binder, bool isServiceLazy) {
    std::string traceStr;
    // Don't cache if service is lazy
    if (kRemoveStaticList && isServiceLazy) {
        return Status::ok();
    }
    if (atrace_is_tag_enabled(ATRACE_TAG_AIDL)) {
        traceStr = "BinderCacheWithInvalidation::updateCache : " + serviceName;
    }
    binder::ScopedTrace aidlTrace(ATRACE_TAG_AIDL, traceStr.c_str());
    if (!binder) {
        binder::ScopedTrace
                aidlTrace(ATRACE_TAG_AIDL,
                          "BinderCacheWithInvalidation::updateCache failed: binder_null");
    } else if (!binder->isBinderAlive()) {
        binder::ScopedTrace aidlTrace(ATRACE_TAG_AIDL,
                                      "BinderCacheWithInvalidation::updateCache failed: "
                                      "isBinderAlive_false");
    }
    // If we reach here with kRemoveStaticList=true then we know service isn't lazy
    else if (mCacheForGetService->isClientSideCachingEnabled(serviceName)) {
        binder::ScopedTrace aidlTrace(ATRACE_TAG_AIDL,
                                      "BinderCacheWithInvalidation::updateCache successful");
        return mCacheForGetService->setItem(serviceName, binder);
    } else {
        binder::ScopedTrace aidlTrace(ATRACE_TAG_AIDL,
                                      "BinderCacheWithInvalidation::updateCache failed: "
                                      "caching_not_enabled");
    }
    return Status::ok();
}

bool BackendUnifiedServiceManager::returnIfCached(const std::string& serviceName,
                                                  os::Service* _out) {
    if (!kUseCache) {
        return false;
    }
    sp<IBinder> item = mCacheForGetService->getItem(serviceName);
    // TODO(b/363177618): Enable caching for binders which are always null.
    if (item != nullptr && item->isBinderAlive()) {
        *_out = createServiceWithMetadata(item, false);
        return true;
    }
    return false;
}

BackendUnifiedServiceManager::BackendUnifiedServiceManager(const sp<AidlServiceManager>& impl)
      : mTheRealServiceManager(impl) {
    mCacheForGetService = std::make_shared<BinderCacheWithInvalidation>();
}

Status BackendUnifiedServiceManager::getService(const ::std::string& name,
                                                sp<IBinder>* _aidl_return) {
    os::Service service;
    Status status = getService2(name, &service);
    *_aidl_return = service.get<os::Service::Tag::serviceWithMetadata>().service;
    return status;
}

Status BackendUnifiedServiceManager::getService2(const ::std::string& name, os::Service* _out) {
    if (returnIfCached(name, _out)) {
        return Status::ok();
    }
    os::Service service;
    Status status = mTheRealServiceManager->getService2(name, &service);

    if (status.isOk()) {
        status = toBinderService(name, service, _out);
        if (status.isOk()) {
            return updateCache(name, service);
        }
    }
    return status;
}

Status BackendUnifiedServiceManager::checkService(const ::std::string& name, os::Service* _out) {
    os::Service service;
    if (returnIfCached(name, _out)) {
        return Status::ok();
    }

    Status status = mTheRealServiceManager->checkService(name, &service);
    if (status.isOk()) {
        status = toBinderService(name, service, _out);
        if (status.isOk()) {
            return updateCache(name, service);
        }
    }
    return status;
}

Status BackendUnifiedServiceManager::toBinderService(const ::std::string& name,
                                                     const os::Service& in, os::Service* _out) {
    switch (in.getTag()) {
        case os::Service::Tag::serviceWithMetadata: {
            auto serviceWithMetadata = in.get<os::Service::Tag::serviceWithMetadata>();
            if (serviceWithMetadata.service == nullptr) {
                // failed to find a service. Check to see if we have any local
                // injected Accessors for this service.
                os::Service accessor;
                Status status = getInjectedAccessor(name, &accessor);
                if (!status.isOk()) {
                    *_out = os::Service::make<os::Service::Tag::serviceWithMetadata>(
                            createServiceWithMetadata(nullptr, false));
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
            return Status::ok();
        }
        case os::Service::Tag::accessor: {
            sp<IBinder> accessorBinder = in.get<os::Service::Tag::accessor>();
            sp<IAccessor> accessor = interface_cast<IAccessor>(accessorBinder);
            if (accessor == nullptr) {
                ALOGE("Service#accessor doesn't have accessor. VM is maybe starting...");
                *_out = os::Service::make<os::Service::Tag::serviceWithMetadata>(
                        createServiceWithMetadata(nullptr, false));
                return Status::ok();
            }
            auto request = [=] {
                os::ParcelFileDescriptor fd;
                Status ret = accessor->addConnection(&fd);
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
                return Status::fromStatusT(status);
            }
            session->setSessionSpecificRoot(accessorBinder);
            *_out = os::Service::make<os::Service::Tag::serviceWithMetadata>(
                    createServiceWithMetadata(session->getRootObject(), false));
            return Status::ok();
        }
        default: {
            LOG_ALWAYS_FATAL("Unknown service type: %d", in.getTag());
        }
    }
}

Status BackendUnifiedServiceManager::addService(const ::std::string& name,
                                                const sp<IBinder>& service, bool allowIsolated,
                                                int32_t dumpPriority) {
    Status status = mTheRealServiceManager->addService(name, service, allowIsolated, dumpPriority);
    // mEnableAddServiceCache is true by default.
    if (kUseCacheInAddService && mEnableAddServiceCache && status.isOk()) {
        return updateCache(name, service,
                           dumpPriority & android::os::IServiceManager::FLAG_IS_LAZY_SERVICE);
    }
    return status;
}
Status BackendUnifiedServiceManager::listServices(int32_t dumpPriority,
                                                  ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->listServices(dumpPriority, _aidl_return);
}
Status BackendUnifiedServiceManager::registerForNotifications(
        const ::std::string& name, const sp<os::IServiceCallback>& callback) {
    return mTheRealServiceManager->registerForNotifications(name, callback);
}
Status BackendUnifiedServiceManager::unregisterForNotifications(
        const ::std::string& name, const sp<os::IServiceCallback>& callback) {
    return mTheRealServiceManager->unregisterForNotifications(name, callback);
}
Status BackendUnifiedServiceManager::isDeclared(const ::std::string& name, bool* _aidl_return) {
    return mTheRealServiceManager->isDeclared(name, _aidl_return);
}
Status BackendUnifiedServiceManager::getDeclaredInstances(
        const ::std::string& iface, ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->getDeclaredInstances(iface, _aidl_return);
}
Status BackendUnifiedServiceManager::updatableViaApex(
        const ::std::string& name, ::std::optional<::std::string>* _aidl_return) {
    return mTheRealServiceManager->updatableViaApex(name, _aidl_return);
}
Status BackendUnifiedServiceManager::getUpdatableNames(const ::std::string& apexName,
                                                       ::std::vector<::std::string>* _aidl_return) {
    return mTheRealServiceManager->getUpdatableNames(apexName, _aidl_return);
}
Status BackendUnifiedServiceManager::getConnectionInfo(
        const ::std::string& name, ::std::optional<os::ConnectionInfo>* _aidl_return) {
    return mTheRealServiceManager->getConnectionInfo(name, _aidl_return);
}
Status BackendUnifiedServiceManager::registerClientCallback(
        const ::std::string& name, const sp<IBinder>& service,
        const sp<os::IClientCallback>& callback) {
    return mTheRealServiceManager->registerClientCallback(name, service, callback);
}
Status BackendUnifiedServiceManager::tryUnregisterService(const ::std::string& name,
                                                          const sp<IBinder>& service) {
    return mTheRealServiceManager->tryUnregisterService(name, service);
}
Status BackendUnifiedServiceManager::getServiceDebugInfo(
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
