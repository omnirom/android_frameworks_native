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

#include "VulkanInterface.h"

#include <include/gpu/GpuTypes.h>
#include <include/gpu/vk/VulkanBackendContext.h>

#include <android-base/stringprintf.h>
#include <log/log_main.h>
#include <utils/Timers.h>

#include <cinttypes>
#include <cstring>
#include <sstream>

namespace android {
namespace renderengine {
namespace skia {

using base::StringAppendF;

VulkanBackendContext VulkanInterface::createSkiaVulkanBackendContext() {
    VulkanBackendContext backendContext;
    backendContext.fInstance = mInstance;
    backendContext.fPhysicalDevice = mPhysicalDevice;
    backendContext.fDevice = mDevice;
    backendContext.fQueue = mQueue;
    backendContext.fGraphicsQueueIndex = mQueueIndex;
    backendContext.fMaxAPIVersion = mTargetApiVersion;
    backendContext.fVkExtensions = &mVulkanExtensions;
    backendContext.fDeviceFeatures2 = &mPhysicalDeviceFeatures2;
    backendContext.fGetProc = mGrGetProc;
    backendContext.fProtectedContext = mIsProtected ? Protected::kYes : Protected::kNo;
    backendContext.fDeviceLostContext = this; // VulkanInterface is long-lived
    backendContext.fDeviceLostProc = onVkDeviceFault;
    return backendContext;
};

VkSemaphore VulkanInterface::createExportableSemaphore() {
    VkExportSemaphoreCreateInfo exportInfo;
    exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
    exportInfo.pNext = nullptr;
    exportInfo.handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;

    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = &exportInfo;
    semaphoreInfo.flags = 0;

    VkSemaphore semaphore;
    VkResult err = mFuncs.vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &semaphore);
    if (VK_SUCCESS != err) {
        ALOGE("%s: failed to create semaphore. err %d\n", __func__, err);
        return VK_NULL_HANDLE;
    }

    return semaphore;
}

// syncFd cannot be <= 0
VkSemaphore VulkanInterface::importSemaphoreFromSyncFd(int syncFd) {
    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    VkSemaphore semaphore;
    VkResult err = mFuncs.vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &semaphore);
    if (VK_SUCCESS != err) {
        ALOGE("%s: failed to create import semaphore", __func__);
        return VK_NULL_HANDLE;
    }

    VkImportSemaphoreFdInfoKHR importInfo;
    importInfo.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
    importInfo.pNext = nullptr;
    importInfo.semaphore = semaphore;
    importInfo.flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
    importInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
    importInfo.fd = syncFd;

    err = mFuncs.vkImportSemaphoreFdKHR(mDevice, &importInfo);
    if (VK_SUCCESS != err) {
        mFuncs.vkDestroySemaphore(mDevice, semaphore, nullptr);
        ALOGE("%s: failed to import semaphore", __func__);
        return VK_NULL_HANDLE;
    }

    return semaphore;
}

int VulkanInterface::exportSemaphoreSyncFd(VkSemaphore semaphore) {
    int res;

    VkSemaphoreGetFdInfoKHR getFdInfo;
    getFdInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
    getFdInfo.pNext = nullptr;
    getFdInfo.semaphore = semaphore;
    getFdInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
    VkResult err = mFuncs.vkGetSemaphoreFdKHR(mDevice, &getFdInfo, &res);
    if (VK_SUCCESS != err) {
        ALOGE("%s: failed to export semaphore, err: %d", __func__, err);
        return -1;
    }
    return res;
}

void VulkanInterface::destroySemaphore(VkSemaphore semaphore) {
    mFuncs.vkDestroySemaphore(mDevice, semaphore, nullptr);
}

void VulkanInterface::onVkDeviceFault(void* callbackContext, const std::string& description,
                                      const std::vector<VkDeviceFaultAddressInfoEXT>& addressInfos,
                                      const std::vector<VkDeviceFaultVendorInfoEXT>& vendorInfos,
                                      const std::vector<std::byte>& vendorBinaryData) {
    VulkanInterface* interface = static_cast<VulkanInterface*>(callbackContext);
    const std::string protectedStr = interface->mIsProtected ? "protected" : "non-protected";
    // The final crash string should contain as much differentiating info as possible, up to 1024
    // bytes. As this final message is constructed, the same information is also dumped to the logs
    // but in a more verbose format. Building the crash string is unsightly, so the clearer logging
    // statement is always placed first to give context.
    ALOGE("VK_ERROR_DEVICE_LOST (%s context): %s", protectedStr.c_str(), description.c_str());
    std::stringstream crashMsg;
    crashMsg << "VK_ERROR_DEVICE_LOST (" << protectedStr;

    if (!addressInfos.empty()) {
        ALOGE("%zu VkDeviceFaultAddressInfoEXT:", addressInfos.size());
        crashMsg << ", " << addressInfos.size() << " address info (";
        for (VkDeviceFaultAddressInfoEXT addressInfo : addressInfos) {
            ALOGE(" addressType:       %d", (int)addressInfo.addressType);
            ALOGE("  reportedAddress:  %" PRIu64, addressInfo.reportedAddress);
            ALOGE("  addressPrecision: %" PRIu64, addressInfo.addressPrecision);
            crashMsg << addressInfo.addressType << ":" << addressInfo.reportedAddress << ":"
                     << addressInfo.addressPrecision << ", ";
        }
        crashMsg.seekp(-2, crashMsg.cur); // Move back to overwrite trailing ", "
        crashMsg << ")";
    }

    if (!vendorInfos.empty()) {
        ALOGE("%zu VkDeviceFaultVendorInfoEXT:", vendorInfos.size());
        crashMsg << ", " << vendorInfos.size() << " vendor info (";
        for (VkDeviceFaultVendorInfoEXT vendorInfo : vendorInfos) {
            ALOGE(" description:      %s", vendorInfo.description);
            ALOGE("  vendorFaultCode: %" PRIu64, vendorInfo.vendorFaultCode);
            ALOGE("  vendorFaultData: %" PRIu64, vendorInfo.vendorFaultData);
            // Omit descriptions for individual vendor info structs in the crash string, as the
            // fault code and fault data fields should be enough for clustering, and the verbosity
            // isn't worth it. Additionally, vendors may just set the general description field of
            // the overall fault to the description of the first element in this list, and that
            // overall description will be placed at the end of the crash string.
            crashMsg << vendorInfo.vendorFaultCode << ":" << vendorInfo.vendorFaultData << ", ";
        }
        crashMsg.seekp(-2, crashMsg.cur); // Move back to overwrite trailing ", "
        crashMsg << ")";
    }

    if (!vendorBinaryData.empty()) {
        // TODO: b/322830575 - Log in base64, or dump directly to a file that gets put in bugreports
        ALOGE("%zu bytes of vendor-specific binary data (please notify Android's Core Graphics"
              " Stack team if you observe this message).",
              vendorBinaryData.size());
        crashMsg << ", " << vendorBinaryData.size() << " bytes binary";
    }

    crashMsg << "): " << description;
    LOG_ALWAYS_FATAL("%s", crashMsg.str().c_str());
};

static skgpu::VulkanGetProc sGetProc = [](const char* proc_name,
                                          VkInstance instance,
                                          VkDevice device) {
    if (device != VK_NULL_HANDLE) {
        return vkGetDeviceProcAddr(device, proc_name);
    }
    return vkGetInstanceProcAddr(instance, proc_name);
};

#define BAIL(fmt, ...)                                          \
    {                                                           \
        ALOGE("%s: " fmt ", bailing", __func__, ##__VA_ARGS__); \
        return;                                                 \
    }

#define CHECK_NONNULL(expr)       \
    if ((expr) == nullptr) {      \
        BAIL("[%s] null", #expr); \
    }

#define VK_CHECK(expr)                                    \
    if (VkResult result = (expr); result != VK_SUCCESS) { \
        BAIL("[%s] failed. err = %d", #expr, result);     \
        return;                                           \
    }

#define VK_GET_PROC(F)                                                           \
    PFN_vk##F vk##F = (PFN_vk##F)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vk" #F); \
    CHECK_NONNULL(vk##F)
#define VK_GET_INST_PROC(instance, F)                                      \
    PFN_vk##F vk##F = (PFN_vk##F)vkGetInstanceProcAddr(instance, "vk" #F); \
    CHECK_NONNULL(vk##F)
#define VK_GET_DEV_PROC(device, F)                                     \
    PFN_vk##F vk##F = (PFN_vk##F)vkGetDeviceProcAddr(device, "vk" #F); \
    CHECK_NONNULL(vk##F)

void VulkanInterface::init(bool protectedContent) {
    if (isInitialized()) {
        ALOGW("Called init on already initialized VulkanInterface");
        return;
    }

    const nsecs_t timeBefore = systemTime();

    VK_GET_PROC(EnumerateInstanceVersion);
    uint32_t instanceVersion;
    VK_CHECK(vkEnumerateInstanceVersion(&instanceVersion));

    if (instanceVersion < VK_API_VERSION_1_1) {
        BAIL("Vulkan instance API version %" PRIu32 ".%" PRIu32 ".%" PRIu32 " < 1.1.0",
             VK_VERSION_MAJOR(instanceVersion), VK_VERSION_MINOR(instanceVersion),
             VK_VERSION_PATCH(instanceVersion));
    }

    const VkApplicationInfo appInfo = {
            VK_STRUCTURE_TYPE_APPLICATION_INFO, nullptr, "surfaceflinger", 0, "android platform", 0,
            VK_API_VERSION_1_1,
    };

    VK_GET_PROC(EnumerateInstanceExtensionProperties);

    uint32_t extensionCount = 0;
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
    std::vector<VkExtensionProperties> instanceExtensions(extensionCount);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                    instanceExtensions.data()));
    std::vector<const char*> enabledInstanceExtensionNames;
    enabledInstanceExtensionNames.reserve(instanceExtensions.size());
    for (const auto& instExt : instanceExtensions) {
        enabledInstanceExtensionNames.push_back(instExt.extensionName);
    }

    // Let Skia enable instance extensions.
    mVulkanFeatures.init(appInfo.apiVersion);
    mVulkanFeatures.addToInstanceExtensions(instanceExtensions.data(), instanceExtensions.size(),
                                            enabledInstanceExtensionNames);

    mInstanceExtensionNames.reserve(enabledInstanceExtensionNames.size());
    for (const char* instExt : enabledInstanceExtensionNames) {
        mInstanceExtensionNames.push_back(instExt);
    }

    const VkInstanceCreateInfo instanceCreateInfo = {
            VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            nullptr,
            0,
            &appInfo,
            0,
            nullptr,
            (uint32_t)enabledInstanceExtensionNames.size(),
            enabledInstanceExtensionNames.data(),
    };

    VK_GET_PROC(CreateInstance);
    VkInstance instance;
    VK_CHECK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

    VK_GET_INST_PROC(instance, DestroyInstance);
    mFuncs.vkDestroyInstance = vkDestroyInstance;
    VK_GET_INST_PROC(instance, EnumeratePhysicalDevices);
    VK_GET_INST_PROC(instance, EnumerateDeviceExtensionProperties);
    VK_GET_INST_PROC(instance, GetPhysicalDeviceProperties2);
    VK_GET_INST_PROC(instance, GetPhysicalDeviceExternalSemaphoreProperties);
    VK_GET_INST_PROC(instance, GetPhysicalDeviceQueueFamilyProperties2);
    VK_GET_INST_PROC(instance, GetPhysicalDeviceFeatures2);
    VK_GET_INST_PROC(instance, CreateDevice);

    uint32_t physdevCount;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physdevCount, nullptr));
    if (physdevCount == 0) {
        BAIL("Could not find any physical devices");
    }

    physdevCount = 1;
    VkPhysicalDevice physicalDevice;
    VkResult enumeratePhysDevsErr =
            vkEnumeratePhysicalDevices(instance, &physdevCount, &physicalDevice);
    if (enumeratePhysDevsErr != VK_SUCCESS && VK_INCOMPLETE != enumeratePhysDevsErr) {
        BAIL("vkEnumeratePhysicalDevices failed with non-VK_INCOMPLETE error: %d",
             enumeratePhysDevsErr);
    }

    VkPhysicalDeviceProperties2 physDevProps = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            0,
            {},
    };
    VkPhysicalDeviceProtectedMemoryProperties protMemProps = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_PROPERTIES,
            0,
            {},
    };

    if (protectedContent) {
        physDevProps.pNext = &protMemProps;
    }

    vkGetPhysicalDeviceProperties2(physicalDevice, &physDevProps);
    mPhysicalDeviceProperties = physDevProps.properties;
    const uint32_t physicalDeviceApiVersion = physDevProps.properties.apiVersion;
    if (physicalDeviceApiVersion < VK_MAKE_VERSION(1, 1, 0)) {
        BAIL("Vulkan physical device API version %" PRIu32 ".%" PRIu32 ".%" PRIu32 " < 1.1.0",
             VK_VERSION_MAJOR(physicalDeviceApiVersion), VK_VERSION_MINOR(physicalDeviceApiVersion),
             VK_VERSION_PATCH(physicalDeviceApiVersion));
    }

    // Check for syncfd support. Bail if we cannot both import and export them.
    VkPhysicalDeviceExternalSemaphoreInfo semInfo = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
            nullptr,
            VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
    };
    VkExternalSemaphoreProperties semProps = {
            VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES, nullptr, 0, 0, 0,
    };
    vkGetPhysicalDeviceExternalSemaphoreProperties(physicalDevice, &semInfo, &semProps);

    bool sufficientSemaphoreSyncFdSupport = (semProps.exportFromImportedHandleTypes &
                                             VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT) &&
            (semProps.compatibleHandleTypes & VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT) &&
            (semProps.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT) &&
            (semProps.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT);

    if (!sufficientSemaphoreSyncFdSupport) {
        BAIL("Vulkan device does not support sufficient external semaphore sync fd features. "
             "exportFromImportedHandleTypes 0x%x (needed 0x%x) "
             "compatibleHandleTypes 0x%x (needed 0x%x) "
             "externalSemaphoreFeatures 0x%x (needed 0x%x) ",
             semProps.exportFromImportedHandleTypes, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
             semProps.compatibleHandleTypes, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
             semProps.externalSemaphoreFeatures,
             VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                     VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT);
    } else {
        ALOGD("Vulkan device supports sufficient external semaphore sync fd features. "
              "exportFromImportedHandleTypes 0x%x (needed 0x%x) "
              "compatibleHandleTypes 0x%x (needed 0x%x) "
              "externalSemaphoreFeatures 0x%x (needed 0x%x) ",
              semProps.exportFromImportedHandleTypes, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
              semProps.compatibleHandleTypes, VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
              semProps.externalSemaphoreFeatures,
              VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
                      VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT);
    }

    uint32_t queueCount;
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueCount, nullptr);
    if (queueCount == 0) {
        BAIL("Could not find queues for physical device");
    }

    std::vector<VkQueueFamilyProperties2> queueProps(queueCount);
    std::vector<VkQueueFamilyGlobalPriorityPropertiesEXT> queuePriorityProps(queueCount);
    VkQueueGlobalPriorityKHR queuePriority = VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR;
    // Even though we don't yet know if the VK_EXT_global_priority extension is available,
    // we can safely add the request to the pNext chain, and if the extension is not
    // available, it will be ignored.
    for (uint32_t i = 0; i < queueCount; ++i) {
        queuePriorityProps[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_EXT;
        queuePriorityProps[i].pNext = nullptr;
        queueProps[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        queueProps[i].pNext = &queuePriorityProps[i];
    }
    vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueCount, queueProps.data());

    int graphicsQueueIndex = -1;
    for (uint32_t i = 0; i < queueCount; ++i) {
        // Look at potential answers to the VK_EXT_global_priority query.  If answers were
        // provided, we may adjust the queuePriority.
        if (queueProps[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            for (uint32_t j = 0; j < queuePriorityProps[i].priorityCount; j++) {
                if (queuePriorityProps[i].priorities[j] > queuePriority) {
                    queuePriority = queuePriorityProps[i].priorities[j];
                }
            }
            if (queuePriority == VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR) {
                mIsRealtimePriority = true;
            }
            graphicsQueueIndex = i;
            break;
        }
    }

    if (graphicsQueueIndex == -1) {
        BAIL("Could not find a graphics queue family");
    }

    uint32_t deviceExtensionCount;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount,
                                                  nullptr));
    std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount,
                                                  deviceExtensions.data()));

    std::vector<const char*> enabledDeviceExtensionNames;
    enabledDeviceExtensionNames.reserve(deviceExtensions.size());
    for (const auto& devExt : deviceExtensions) {
        enabledDeviceExtensionNames.push_back(devExt.extensionName);
    }

    mPhysicalDeviceFeatures2 = {};
    mPhysicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    void** tailPnext = &mPhysicalDeviceFeatures2.pNext;

    if (protectedContent) {
        mProtectedMemoryFeatures = {};
        mProtectedMemoryFeatures.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROTECTED_MEMORY_FEATURES;
        *tailPnext = &mProtectedMemoryFeatures;
        tailPnext = &mProtectedMemoryFeatures.pNext;
    }

    for (const VkExtensionProperties& ext : deviceExtensions) {
        if (strcmp(ext.extensionName, VK_EXT_DEVICE_FAULT_EXTENSION_NAME) == 0) {
            mDeviceFaultFeatures = {};
            mDeviceFaultFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FAULT_FEATURES_EXT;
            *tailPnext = &mDeviceFaultFeatures;
            tailPnext = &mDeviceFaultFeatures.pNext;

            break;
        }
    }

    // Let Skia add features to be queried.
    mVulkanFeatures.addFeaturesToQuery(deviceExtensions.data(), deviceExtensions.size(),
                                       mPhysicalDeviceFeatures2);

    vkGetPhysicalDeviceFeatures2(physicalDevice, &mPhysicalDeviceFeatures2);
    // Robust buffer access can reduce performance on some platforms. It is not needed by
    // RenderEngine.
    mPhysicalDeviceFeatures2.features.robustBufferAccess = VK_FALSE;

    // Let Skia enable extensions and features.
    mVulkanFeatures.addFeaturesToEnable(enabledDeviceExtensionNames, mPhysicalDeviceFeatures2);

    mDeviceExtensionNames.reserve(enabledDeviceExtensionNames.size());
    for (const char* devExt : enabledDeviceExtensionNames) {
        mDeviceExtensionNames.push_back(devExt);
    }

    mVulkanExtensions.init(sGetProc, instance, physicalDevice, enabledInstanceExtensionNames.size(),
                           enabledInstanceExtensionNames.data(), enabledDeviceExtensionNames.size(),
                           enabledDeviceExtensionNames.data());

    if (!mVulkanExtensions.hasExtension(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, 1)) {
        BAIL("Vulkan driver doesn't support external semaphore fd");
    }

    if (protectedContent && !mProtectedMemoryFeatures.protectedMemory) {
        BAIL("Protected memory not supported");
    }

    float queuePriorities[1] = {0.0f};
    void* queueNextPtr = nullptr;

    VkDeviceQueueGlobalPriorityCreateInfoEXT queuePriorityCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_GLOBAL_PRIORITY_CREATE_INFO_EXT,
            nullptr,
            // If queue priority is supported, RE should always have realtime priority.
            queuePriority,
    };

    if (mVulkanExtensions.hasExtension(VK_EXT_GLOBAL_PRIORITY_EXTENSION_NAME, 2)) {
        queueNextPtr = &queuePriorityCreateInfo;
    }

    VkDeviceQueueCreateFlags deviceQueueCreateFlags =
            (VkDeviceQueueCreateFlags)(protectedContent ? VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT : 0);

    const VkDeviceQueueCreateInfo queueInfo = {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            queueNextPtr,
            deviceQueueCreateFlags,
            (uint32_t)graphicsQueueIndex,
            1,
            queuePriorities,
    };

    const VkDeviceCreateInfo deviceInfo = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            &mPhysicalDeviceFeatures2,
            0,
            1,
            &queueInfo,
            0,
            nullptr,
            (uint32_t)enabledDeviceExtensionNames.size(),
            enabledDeviceExtensionNames.data(),
            nullptr,
    };

    ALOGD("Trying to create Vk device with protectedContent=%d", protectedContent);
    VkDevice device;
    VK_CHECK(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device));
    ALOGD("Trying to create Vk device with protectedContent=%d (success)", protectedContent);

    VkQueue graphicsQueue;
    VK_GET_DEV_PROC(device, GetDeviceQueue2);
    const VkDeviceQueueInfo2 deviceQueueInfo2 = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2, nullptr,
                                                 deviceQueueCreateFlags,
                                                 (uint32_t)graphicsQueueIndex, 0};
    vkGetDeviceQueue2(device, &deviceQueueInfo2, &graphicsQueue);

    VK_GET_DEV_PROC(device, DeviceWaitIdle);
    VK_GET_DEV_PROC(device, DestroyDevice);
    mFuncs.vkDeviceWaitIdle = vkDeviceWaitIdle;
    mFuncs.vkDestroyDevice = vkDestroyDevice;

    VK_GET_DEV_PROC(device, CreateSemaphore);
    VK_GET_DEV_PROC(device, ImportSemaphoreFdKHR);
    VK_GET_DEV_PROC(device, GetSemaphoreFdKHR);
    VK_GET_DEV_PROC(device, DestroySemaphore);
    mFuncs.vkCreateSemaphore = vkCreateSemaphore;
    mFuncs.vkImportSemaphoreFdKHR = vkImportSemaphoreFdKHR;
    mFuncs.vkGetSemaphoreFdKHR = vkGetSemaphoreFdKHR;
    mFuncs.vkDestroySemaphore = vkDestroySemaphore;

    // At this point, everything's succeeded and we can continue
    mInitialized = true;
    mInstance = instance;
    mPhysicalDevice = physicalDevice;
    mDevice = device;
    mQueue = graphicsQueue;
    mQueueIndex = graphicsQueueIndex;
    mTargetApiVersion = std::min(physicalDeviceApiVersion, appInfo.apiVersion);
    // grExtensions already constructed
    // feature pointers already constructed
    mGrGetProc = sGetProc;
    mIsProtected = protectedContent;
    // mIsRealtimePriority already initialized by constructor
    // funcs already initialized

    const nsecs_t timeAfter = systemTime();
    const float initTimeMs = static_cast<float>(timeAfter - timeBefore) / 1.0E6;
    ALOGD("%s: Success init Vulkan interface in %f ms", __func__, initTimeMs);
}

bool VulkanInterface::takeOwnership() {
    if (!isInitialized() || mIsOwned) {
        return false;
    }
    mIsOwned = true;
    return true;
}

void VulkanInterface::teardown() {
    // Core resources that must be destroyed using Vulkan functions.
    if (mDevice != VK_NULL_HANDLE) {
        mFuncs.vkDeviceWaitIdle(mDevice);
        mFuncs.vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    }
    if (mInstance != VK_NULL_HANDLE) {
        mFuncs.vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    }

    // Misc. fields that can be trivially reset without special deletion:
    mInitialized = false;
    mIsOwned = false;
    mPhysicalDevice = VK_NULL_HANDLE; // Implicitly destroyed by destroying mInstance.
    mQueue = VK_NULL_HANDLE;          // Implicitly destroyed by destroying mDevice.
    mQueueIndex = 0;
    mTargetApiVersion = 0;
    mVulkanExtensions = skgpu::VulkanExtensions();
    mGrGetProc = nullptr;
    mIsProtected = false;
    mIsRealtimePriority = false;

    mPhysicalDeviceFeatures2 = {};
    mProtectedMemoryFeatures = {};
    mDeviceFaultFeatures = {};

    mFuncs = VulkanFuncs();

    mInstanceExtensionNames.clear();
    mDeviceExtensionNames.clear();
}

void VulkanInterface::appendVulkanInfoToDump(std::string& result) const {
    StringAppendF(&result, "Device properties: [\n");
    StringAppendF(&result, "  Name: %s\n", mPhysicalDeviceProperties.deviceName);
    StringAppendF(&result, "  Type: %" PRIu32 "\n", mPhysicalDeviceProperties.deviceType);
    StringAppendF(&result, "  Driver version: %" PRIu32 "\n",
                  mPhysicalDeviceProperties.driverVersion);
    StringAppendF(&result, "  Supported API version: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
                  VK_VERSION_MAJOR(mPhysicalDeviceProperties.apiVersion),
                  VK_VERSION_MINOR(mPhysicalDeviceProperties.apiVersion),
                  VK_VERSION_PATCH(mPhysicalDeviceProperties.apiVersion));
    StringAppendF(&result, "]\n");

    StringAppendF(&result, "Instance extensions: [\n");
    for (const auto& name : mInstanceExtensionNames) {
        StringAppendF(&result, "  %s\n", name.c_str());
    }
    StringAppendF(&result, "]\n");

    StringAppendF(&result, "Device extensions: [\n");
    for (const auto& name : mDeviceExtensionNames) {
        StringAppendF(&result, "  %s\n", name.c_str());
    }
    StringAppendF(&result, "]\n");
}

} // namespace skia
} // namespace renderengine
} // namespace android
