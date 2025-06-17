#libvulkan_test

This binary contains the unit tests for testing libvulkan (The Vulkan Loader).

These tests rely on the underlying GPU driver to be able to successfully create a valid
swapchain. These tests are design to run on an Android emulator to give us a consistent GPU
driver to test against. YMMV when running this on a physical device with an arbitrary GPU
driver.

To run these tests run:
```
atest libvulkan_test
```

If using an acloud device the full command list for the root of a freshly cloned repo would be:
```
source build/envsetup.sh
lunch aosp_cf_x86_64_phone-trunk_staging-eng
m
acloud create --local-image
atest libvulkan_test
```


