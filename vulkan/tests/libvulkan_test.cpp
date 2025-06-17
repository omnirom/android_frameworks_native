/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include <android/log.h>
#include <driver.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <media/NdkImageReader.h>
#include <vulkan/vulkan.h>

#define LOGI(...) \
    __android_log_print(ANDROID_LOG_INFO, "libvulkan_test", __VA_ARGS__)
#define LOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, "libvulkan_test", __VA_ARGS__)

#define VK_CHECK(result) ASSERT_EQ(VK_SUCCESS, result)

namespace android {

namespace libvulkantest {

class AImageReaderVulkanSwapchainTest : public ::testing::Test {
   public:
    AImageReaderVulkanSwapchainTest() {}

    AImageReader* mReader = nullptr;
    ANativeWindow* mWindow = nullptr;
    VkInstance mVkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDev = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;
    uint32_t mPresentQueueFamily = UINT32_MAX;
    VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;

    void SetUp() override {}

    void TearDown() override {}

    // ------------------------------------------------------
    // Helper methods
    // ------------------------------------------------------

    void createVulkanInstance(std::vector<const char*>& layers) {
        const char* extensions[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
            VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        };

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "AImageReader Vulkan Swapchain Test";
        appInfo.applicationVersion = 1;
        appInfo.pEngineName = "TestEngine";
        appInfo.engineVersion = 1;
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instInfo{};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;
        instInfo.enabledExtensionCount =
            sizeof(extensions) / sizeof(extensions[0]);
        instInfo.ppEnabledExtensionNames = extensions;
        instInfo.enabledLayerCount = layers.size();
        instInfo.ppEnabledLayerNames = layers.data();
        VkResult res = vkCreateInstance(&instInfo, nullptr, &mVkInstance);
        VK_CHECK(res);
        LOGE("Vulkan instance created");
    }

    void createAImageReader(int width, int height, int format, int maxImages) {
        media_status_t status =
            AImageReader_new(width, height, format, maxImages, &mReader);
        ASSERT_EQ(AMEDIA_OK, status) << "Failed to create AImageReader";
        ASSERT_NE(nullptr, mReader) << "AImageReader is null";

        // Optionally set a listener
        AImageReader_ImageListener listener{};
        listener.context = this;
        listener.onImageAvailable =
            &AImageReaderVulkanSwapchainTest::onImageAvailable;
        AImageReader_setImageListener(mReader, &listener);

        LOGI("AImageReader created with %dx%d, format=%d", width, height,
             format);
    }

    void getANativeWindowFromReader() {
        ASSERT_NE(nullptr, mReader);

        media_status_t status = AImageReader_getWindow(mReader, &mWindow);
        ASSERT_EQ(AMEDIA_OK, status)
            << "Failed to get ANativeWindow from AImageReader";
        ASSERT_NE(nullptr, mWindow) << "ANativeWindow is null";
        LOGI("ANativeWindow obtained from AImageReader");
    }

    void createVulkanSurface() {
        ASSERT_NE((VkInstance)VK_NULL_HANDLE, mVkInstance);
        ASSERT_NE((ANativeWindow*)nullptr, mWindow);

        VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
        surfaceCreateInfo.sType =
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.window = mWindow;

        VkResult res = vkCreateAndroidSurfaceKHR(
            mVkInstance, &surfaceCreateInfo, nullptr, &mSurface);
        VK_CHECK(res);
        LOGI("Vulkan surface created from ANativeWindow");
    }

    void pickPhysicalDeviceAndQueueFamily() {
        ASSERT_NE((VkInstance)VK_NULL_HANDLE, mVkInstance);

        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, nullptr);
        ASSERT_GT(deviceCount, 0U) << "No Vulkan physical devices found!";

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mVkInstance, &deviceCount, devices.data());

        for (auto& dev : devices) {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                                     nullptr);
            std::vector<VkQueueFamilyProperties> queueProps(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                                     queueProps.data());

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                VkBool32 support = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, mSurface,
                                                     &support);
                if (support == VK_TRUE) {
                    // Found a queue family that can present
                    mPhysicalDev = dev;
                    mPresentQueueFamily = i;

                    LOGI(
                        "Physical device found with queue family %u supporting "
                        "present",
                        i);
                    return;
                }
            }
        }

        FAIL()
            << "No physical device found that supports present to the surface!";
    }

    void createDeviceAndGetQueue(std::vector<const char*>& layers,
                                 std::vector<const char*> inExtensions = {}) {
        ASSERT_NE((void*)VK_NULL_HANDLE, mPhysicalDev);
        ASSERT_NE(UINT32_MAX, mPresentQueueFamily);

        float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = mPresentQueueFamily;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledLayerCount = layers.size();
        deviceInfo.ppEnabledLayerNames = layers.data();

        std::vector<const char*> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        for (auto extension : inExtensions) {
            extensions.push_back(extension);
        }
        deviceInfo.enabledExtensionCount = extensions.size();
        deviceInfo.ppEnabledExtensionNames = extensions.data();

        VkResult res =
            vkCreateDevice(mPhysicalDev, &deviceInfo, nullptr, &mDevice);
        VK_CHECK(res);
        LOGI("Logical device created");

        vkGetDeviceQueue(mDevice, mPresentQueueFamily, 0, &mPresentQueue);
        ASSERT_NE((VkQueue)VK_NULL_HANDLE, mPresentQueue);
        LOGI("Acquired present-capable queue");
    }

    void createSwapchain() {
        ASSERT_NE((VkDevice)VK_NULL_HANDLE, mDevice);
        ASSERT_NE((VkSurfaceKHR)VK_NULL_HANDLE, mSurface);

        VkSurfaceCapabilitiesKHR surfaceCaps{};
        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            mPhysicalDev, mSurface, &surfaceCaps));

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface,
                                             &formatCount, nullptr);
        ASSERT_GT(formatCount, 0U);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface,
                                             &formatCount, formats.data());

        VkSurfaceFormatKHR chosenFormat = formats[0];
        LOGI("Chosen surface format: %d", chosenFormat.format);

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDev, mSurface,
                                                  &presentModeCount, nullptr);
        ASSERT_GT(presentModeCount, 0U);
        std::vector<VkPresentModeKHR> presentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            mPhysicalDev, mSurface, &presentModeCount, presentModes.data());

        VkPresentModeKHR chosenPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto mode : presentModes) {
            if (mode == VK_PRESENT_MODE_FIFO_KHR) {
                chosenPresentMode = mode;
                break;
            }
        }
        LOGI("Chosen present mode: %d", chosenPresentMode);

        VkExtent2D swapchainExtent{};
        if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
            swapchainExtent.width = 640;   // fallback
            swapchainExtent.height = 480;  // fallback
        } else {
            swapchainExtent = surfaceCaps.currentExtent;
        }
        LOGI("Swapchain extent: %d x %d", swapchainExtent.width,
             swapchainExtent.height);

        uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
        if (surfaceCaps.maxImageCount > 0 &&
            desiredImageCount > surfaceCaps.maxImageCount) {
            desiredImageCount = surfaceCaps.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = mSurface;
        swapchainInfo.minImageCount = desiredImageCount;
        swapchainInfo.imageFormat = chosenFormat.format;
        swapchainInfo.imageColorSpace = chosenFormat.colorSpace;
        swapchainInfo.imageExtent = swapchainExtent;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        swapchainInfo.preTransform = surfaceCaps.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        swapchainInfo.presentMode = chosenPresentMode;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

        uint32_t queueFamilyIndices[] = {mPresentQueueFamily};
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 1;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;

        VkResult res =
            vkCreateSwapchainKHR(mDevice, &swapchainInfo, nullptr, &mSwapchain);
        if (res == VK_SUCCESS) {
            LOGI("Swapchain created successfully");

            uint32_t swapchainImageCount = 0;
            vkGetSwapchainImagesKHR(mDevice, mSwapchain, &swapchainImageCount,
                                    nullptr);
            std::vector<VkImage> swapchainImages(swapchainImageCount);
            vkGetSwapchainImagesKHR(mDevice, mSwapchain, &swapchainImageCount,
                                    swapchainImages.data());

            LOGI("Swapchain has %u images", swapchainImageCount);
        } else {
            LOGI("Swapchain creation failed");
        }
    }

    // Image available callback (AImageReader)
    static void onImageAvailable(void*, AImageReader* reader) {
        LOGI("onImageAvailable callback triggered");
        AImage* image = nullptr;
        media_status_t status = AImageReader_acquireLatestImage(reader, &image);
        if (status != AMEDIA_OK || !image) {
            LOGE("Failed to acquire latest image");
            return;
        }
        AImage_delete(image);
        LOGI("Released acquired image");
    }

    void cleanUpSwapchainForTest() {
        if (mSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
            mSwapchain = VK_NULL_HANDLE;
        }
        if (mDevice != VK_NULL_HANDLE) {
            vkDestroyDevice(mDevice, nullptr);
            mDevice = VK_NULL_HANDLE;
        }
        if (mSurface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(mVkInstance, mSurface, nullptr);
            mSurface = VK_NULL_HANDLE;
        }
        if (mVkInstance != VK_NULL_HANDLE) {
            vkDestroyInstance(mVkInstance, nullptr);
            mVkInstance = VK_NULL_HANDLE;
        }
        if (mReader) {
            // AImageReader_delete(mReader);
            mReader = nullptr;
        }
        // Note: The ANativeWindow from AImageReader is implicitly
        // managed by the reader, so we don't explicitly delete it.
        mWindow = nullptr;
    }

    void buildSwapchianForTest(std::vector<const char*>& instanceLayers,
                               std::vector<const char*>& deviceLayers) {
        createVulkanInstance(instanceLayers);

        // the "atest libvulkan_test" command will execute this test as a binary
        // (not apk) on the device. Consequently we can't render to the screen
        // and need to work around this by using AImageReader*
        createAImageReader(640, 480, AIMAGE_FORMAT_PRIVATE, 3);
        getANativeWindowFromReader();
        createVulkanSurface();
        pickPhysicalDeviceAndQueueFamily();

        createDeviceAndGetQueue(deviceLayers);
        createSwapchain();
    }
};

TEST_F(AImageReaderVulkanSwapchainTest, TestHelperMethods) {
    // Verify that the basic plumbing/helper functions of these tests is
    // working. This doesn't directly test any of the layer code. It only
    // verifies that we can successfully create a swapchain with an AImageReader

    std::vector<const char*> instanceLayers;
    std::vector<const char*> deviceLayers;
    buildSwapchianForTest(deviceLayers, instanceLayers);

    ASSERT_NE(mVkInstance, (VkInstance)VK_NULL_HANDLE);
    ASSERT_NE(mPhysicalDev, (VkPhysicalDevice)VK_NULL_HANDLE);
    ASSERT_NE(mDevice, (VkDevice)VK_NULL_HANDLE);
    ASSERT_NE(mSurface, (VkSurfaceKHR)VK_NULL_HANDLE);
    ASSERT_NE(mSwapchain, (VkSwapchainKHR)VK_NULL_HANDLE);
    cleanUpSwapchainForTest();
}

// Passing state in these tests requires global state. Wrap each test in an
// anonymous namespace to prevent conflicting names.
namespace {

VKAPI_ATTR VkResult VKAPI_CALL hookedGetPhysicalDeviceImageFormatProperties2KHR(
    VkPhysicalDevice,
    const VkPhysicalDeviceImageFormatInfo2*,
    VkImageFormatProperties2*) {
    return VK_ERROR_SURFACE_LOST_KHR;
}

static PFN_vkGetSwapchainGrallocUsage2ANDROID
    pfnNextGetSwapchainGrallocUsage2ANDROID = nullptr;

static bool g_grallocCalled = false;

VKAPI_ATTR VkResult VKAPI_CALL hookGetSwapchainGrallocUsage2ANDROID(
    VkDevice device,
    VkFormat format,
    VkImageUsageFlags imageUsage,
    VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
    uint64_t* grallocConsumerUsage,
    uint64_t* grallocProducerUsage) {
    g_grallocCalled = true;
    if (pfnNextGetSwapchainGrallocUsage2ANDROID) {
        return pfnNextGetSwapchainGrallocUsage2ANDROID(
            device, format, imageUsage, swapchainImageUsage,
            grallocConsumerUsage, grallocProducerUsage);
    }

    return VK_ERROR_INITIALIZATION_FAILED;
}

TEST_F(AImageReaderVulkanSwapchainTest, getProducerUsageFallbackTest1) {
    // BUG: 379230826
    // Verify that getProducerUsage falls back to
    // GetSwapchainGrallocUsage*ANDROID if GPDIFP2 fails
    std::vector<const char*> instanceLayers = {};
    std::vector<const char*> deviceLayers = {};
    createVulkanInstance(instanceLayers);

    createAImageReader(640, 480, AIMAGE_FORMAT_PRIVATE, 3);
    getANativeWindowFromReader();
    createVulkanSurface();
    pickPhysicalDeviceAndQueueFamily();

    createDeviceAndGetQueue(deviceLayers);
    auto& pdev = vulkan::driver::GetData(mDevice).driver_physical_device;
    auto& pdevDispatchTable = vulkan::driver::GetData(pdev).driver;
    auto& deviceDispatchTable = vulkan::driver::GetData(mDevice).driver;

    ASSERT_NE(deviceDispatchTable.GetSwapchainGrallocUsage2ANDROID, nullptr);

    pdevDispatchTable.GetPhysicalDeviceImageFormatProperties2 =
        hookedGetPhysicalDeviceImageFormatProperties2KHR;
    deviceDispatchTable.GetSwapchainGrallocUsage2ANDROID =
        hookGetSwapchainGrallocUsage2ANDROID;

    ASSERT_FALSE(g_grallocCalled);

    createSwapchain();

    ASSERT_TRUE(g_grallocCalled);

    ASSERT_NE(mVkInstance, (VkInstance)VK_NULL_HANDLE);
    ASSERT_NE(mPhysicalDev, (VkPhysicalDevice)VK_NULL_HANDLE);
    ASSERT_NE(mDevice, (VkDevice)VK_NULL_HANDLE);
    ASSERT_NE(mSurface, (VkSurfaceKHR)VK_NULL_HANDLE);
    cleanUpSwapchainForTest();
}

}  // namespace

// Passing state in these tests requires global state. Wrap each test in an
// anonymous namespace to prevent conflicting names.
namespace {

static bool g_returnNotSupportedOnce = true;

VKAPI_ATTR VkResult VKAPI_CALL
Hook_GetPhysicalDeviceImageFormatProperties2_NotSupportedOnce(
    VkPhysicalDevice /*physicalDevice*/,
    const VkPhysicalDeviceImageFormatInfo2* /*pImageFormatInfo*/,
    VkImageFormatProperties2* /*pImageFormatProperties*/) {
    if (g_returnNotSupportedOnce) {
        g_returnNotSupportedOnce = false;
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }
    return VK_SUCCESS;
}

TEST_F(AImageReaderVulkanSwapchainTest, SurfaceFormats2KHR_IgnoreNotSupported) {
    // BUG: 357903074
    // Verify that vkGetPhysicalDeviceSurfaceFormats2KHR properly
    // ignores VK_ERROR_FORMAT_NOT_SUPPORTED and continues enumerating formats.
    std::vector<const char*> instanceLayers;
    createVulkanInstance(instanceLayers);
    createAImageReader(640, 480, AIMAGE_FORMAT_PRIVATE, 3);
    getANativeWindowFromReader();
    createVulkanSurface();
    pickPhysicalDeviceAndQueueFamily();

    auto& pdevDispatchTable = vulkan::driver::GetData(mPhysicalDev).driver;
    pdevDispatchTable.GetPhysicalDeviceImageFormatProperties2 =
        Hook_GetPhysicalDeviceImageFormatProperties2_NotSupportedOnce;

    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR
        pfnGetPhysicalDeviceSurfaceFormats2KHR =
            reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormats2KHR>(
                vkGetInstanceProcAddr(mVkInstance,
                                      "vkGetPhysicalDeviceSurfaceFormats2KHR"));
    ASSERT_NE(nullptr, pfnGetPhysicalDeviceSurfaceFormats2KHR)
        << "Could not get pointer to vkGetPhysicalDeviceSurfaceFormats2KHR";

    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo2{};
    surfaceInfo2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surfaceInfo2.pNext = nullptr;
    surfaceInfo2.surface = mSurface;

    uint32_t formatCount = 0;
    VkResult res = pfnGetPhysicalDeviceSurfaceFormats2KHR(
        mPhysicalDev, &surfaceInfo2, &formatCount, nullptr);

    // If the loader never tries a second format, it might fail or 0-out the
    // formatCount. The patch ensures it continues to the next format rather
    // than bailing out on the first NOT_SUPPORTED.
    ASSERT_EQ(VK_SUCCESS, res)
        << "vkGetPhysicalDeviceSurfaceFormats2KHR failed unexpectedly";
    ASSERT_GT(formatCount, 0U)
        << "No surface formats found; the loader may have bailed early.";

    std::vector<VkSurfaceFormat2KHR> formats(formatCount);
    for (auto& f : formats) {
        f.sType = VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR;
        f.pNext = nullptr;
    }
    res = pfnGetPhysicalDeviceSurfaceFormats2KHR(mPhysicalDev, &surfaceInfo2,
                                                 &formatCount, formats.data());
    ASSERT_EQ(VK_SUCCESS, res) << "Failed to retrieve surface formats";

    LOGI(
        "SurfaceFormats2KHR_IgnoreNotSupported test: found %u formats after "
        "ignoring NOT_SUPPORTED",
        formatCount);

    cleanUpSwapchainForTest();
}

}  // namespace

namespace {

TEST_F(AImageReaderVulkanSwapchainTest, MutableFormatSwapchainTest) {
    // Test swapchain with mutable format extension
    std::vector<const char*> instanceLayers;
    std::vector<const char*> deviceLayers;
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME};

    createVulkanInstance(instanceLayers);
    createAImageReader(640, 480, AIMAGE_FORMAT_PRIVATE, 3);
    getANativeWindowFromReader();
    createVulkanSurface();
    pickPhysicalDeviceAndQueueFamily();
    createDeviceAndGetQueue(deviceLayers, deviceExtensions);

    ASSERT_NE((VkDevice)VK_NULL_HANDLE, mDevice);
    ASSERT_NE((VkSurfaceKHR)VK_NULL_HANDLE, mSurface);

    VkSurfaceCapabilitiesKHR surfaceCaps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDev, mSurface,
                                                       &surfaceCaps));

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface, &formatCount,
                                         nullptr);
    ASSERT_GT(formatCount, 0U);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDev, mSurface, &formatCount,
                                         formats.data());

    VkFormat viewFormats[2] = {formats[0].format, formats[0].format};
    if (formatCount > 1) {
        viewFormats[1] = formats[1].format;
    }

    VkImageFormatListCreateInfoKHR formatList{};
    formatList.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
    formatList.viewFormatCount = 2;
    formatList.pViewFormats = viewFormats;

    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = &formatList;
    swapchainInfo.surface = mSurface;
    swapchainInfo.minImageCount = surfaceCaps.minImageCount + 1;
    swapchainInfo.imageFormat = formats[0].format;
    swapchainInfo.imageColorSpace = formats[0].colorSpace;
    swapchainInfo.imageExtent = surfaceCaps.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.preTransform = surfaceCaps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainInfo.clipped = VK_TRUE;

    swapchainInfo.flags = VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR;

    uint32_t queueFamilyIndices[] = {mPresentQueueFamily};
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;

    VkResult res =
        vkCreateSwapchainKHR(mDevice, &swapchainInfo, nullptr, &mSwapchain);
    if (res == VK_SUCCESS) {
        LOGI("Mutable format swapchain created successfully");

        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(mDevice, mSwapchain, &imageCount, nullptr);
        ASSERT_GT(imageCount, 0U);
    } else {
        LOGI(
            "Mutable format swapchain creation failed (extension may not be "
            "supported)");
    }

    cleanUpSwapchainForTest();
}

}  // namespace

}  // namespace libvulkantest

}  // namespace android
