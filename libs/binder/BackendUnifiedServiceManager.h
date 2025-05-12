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
#pragma once

#include <android/os/BnServiceManager.h>
#include <android/os/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/Trace.h>
#include <map>
#include <memory>

namespace android {

class BinderCacheWithInvalidation
      : public std::enable_shared_from_this<BinderCacheWithInvalidation> {
    class BinderInvalidation : public IBinder::DeathRecipient {
    public:
        BinderInvalidation(std::weak_ptr<BinderCacheWithInvalidation> cache, const std::string& key)
              : mCache(cache), mKey(key) {}

        void binderDied(const wp<IBinder>& who) override {
            sp<IBinder> binder = who.promote();
            if (std::shared_ptr<BinderCacheWithInvalidation> cache = mCache.lock()) {
                cache->removeItem(mKey, binder);
            } else {
                ALOGI("Binder Cache pointer expired: %s", mKey.c_str());
            }
        }

    private:
        std::weak_ptr<BinderCacheWithInvalidation> mCache;
        std::string mKey;
    };
    struct Entry {
        sp<IBinder> service;
        sp<BinderInvalidation> deathRecipient;
    };

public:
    sp<IBinder> getItem(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mCacheMutex);

        if (auto it = mCache.find(key); it != mCache.end()) {
            return it->second.service;
        }
        return nullptr;
    }

    bool removeItem(const std::string& key, const sp<IBinder>& who) {
        std::string traceStr;
        uint64_t tag = ATRACE_TAG_AIDL;
        if (atrace_is_tag_enabled(tag)) {
            traceStr = "BinderCacheWithInvalidation::removeItem " + key;
        }
        binder::ScopedTrace aidlTrace(tag, traceStr.c_str());
        std::lock_guard<std::mutex> lock(mCacheMutex);
        if (auto it = mCache.find(key); it != mCache.end()) {
            if (it->second.service == who) {
                status_t result = who->unlinkToDeath(it->second.deathRecipient);
                if (result != DEAD_OBJECT) {
                    ALOGW("Unlinking to dead binder resulted in: %d", result);
                }
                mCache.erase(key);
                return true;
            }
        }
        return false;
    }

    binder::Status setItem(const std::string& key, const sp<IBinder>& item) {
        sp<BinderInvalidation> deathRecipient =
                sp<BinderInvalidation>::make(shared_from_this(), key);

        // linkToDeath if binder is a remote binder.
        if (item->localBinder() == nullptr) {
            status_t status = item->linkToDeath(deathRecipient);
            if (status != android::OK) {
                std::string traceStr;
                uint64_t tag = ATRACE_TAG_AIDL;
                if (atrace_is_tag_enabled(tag)) {
                    traceStr =
                            "BinderCacheWithInvalidation::setItem Failed LinkToDeath for service " +
                            key + " : " + std::to_string(status);
                }
                binder::ScopedTrace aidlTrace(tag, traceStr.c_str());

                ALOGE("Failed to linkToDeath binder for service %s. Error: %d", key.c_str(),
                      status);
                return binder::Status::fromStatusT(status);
            }
        }
        binder::ScopedTrace aidlTrace(ATRACE_TAG_AIDL,
                                      "BinderCacheWithInvalidation::setItem Successfully Cached");
        std::lock_guard<std::mutex> lock(mCacheMutex);
        mCache[key] = {.service = item, .deathRecipient = deathRecipient};
        return binder::Status::ok();
    }

    bool isClientSideCachingEnabled(const std::string& serviceName);

private:
    std::map<std::string, Entry> mCache;
    mutable std::mutex mCacheMutex;
};

class BackendUnifiedServiceManager : public android::os::BnServiceManager {
public:
    explicit BackendUnifiedServiceManager(const sp<os::IServiceManager>& impl);

    binder::Status getService(const ::std::string& name, sp<IBinder>* _aidl_return) override;
    binder::Status getService2(const ::std::string& name, os::Service* out) override;
    binder::Status checkService(const ::std::string& name, os::Service* out) override;
    binder::Status addService(const ::std::string& name, const sp<IBinder>& service,
                              bool allowIsolated, int32_t dumpPriority) override;
    binder::Status listServices(int32_t dumpPriority,
                                ::std::vector<::std::string>* _aidl_return) override;
    binder::Status registerForNotifications(const ::std::string& name,
                                            const sp<os::IServiceCallback>& callback) override;
    binder::Status unregisterForNotifications(const ::std::string& name,
                                              const sp<os::IServiceCallback>& callback) override;
    binder::Status isDeclared(const ::std::string& name, bool* _aidl_return) override;
    binder::Status getDeclaredInstances(const ::std::string& iface,
                                        ::std::vector<::std::string>* _aidl_return) override;
    binder::Status updatableViaApex(const ::std::string& name,
                                    ::std::optional<::std::string>* _aidl_return) override;
    binder::Status getUpdatableNames(const ::std::string& apexName,
                                     ::std::vector<::std::string>* _aidl_return) override;
    binder::Status getConnectionInfo(const ::std::string& name,
                                     ::std::optional<os::ConnectionInfo>* _aidl_return) override;
    binder::Status registerClientCallback(const ::std::string& name, const sp<IBinder>& service,
                                          const sp<os::IClientCallback>& callback) override;
    binder::Status tryUnregisterService(const ::std::string& name,
                                        const sp<IBinder>& service) override;
    binder::Status getServiceDebugInfo(::std::vector<os::ServiceDebugInfo>* _aidl_return) override;

    void enableAddServiceCache(bool value) { mEnableAddServiceCache = value; }
    // for legacy ABI
    const String16& getInterfaceDescriptor() const override {
        return mTheRealServiceManager->getInterfaceDescriptor();
    }

private:
    bool mEnableAddServiceCache = true;
    std::shared_ptr<BinderCacheWithInvalidation> mCacheForGetService;
    sp<os::IServiceManager> mTheRealServiceManager;
    binder::Status toBinderService(const ::std::string& name, const os::Service& in,
                                   os::Service* _out);
    binder::Status updateCache(const std::string& serviceName, const os::Service& service);
    binder::Status updateCache(const std::string& serviceName, const sp<IBinder>& binder,
                               bool isLazyService);
    bool returnIfCached(const std::string& serviceName, os::Service* _out);
};

sp<BackendUnifiedServiceManager> getBackendUnifiedServiceManager();

android::binder::Status getInjectedAccessor(const std::string& name, android::os::Service* service);

} // namespace android
