# `surfaceflinger_end2end_tests`

Tests to cover end to end testing of SurfaceFlinger.

In particular the test framework allows you to simulate various display
configurations, so the test can confirm displays are handled correctly.

## Quick Useful info

### Running the tests

At present the tests should run on any target, though the typical target would
be a [Cuttlefish](https://source.android.com/docs/devices/cuttlefish) VM
target such as `aosp_cf_x86_64_phone`.

At some future time the test may be rewritten to require
[`vkms`](https://dri.freedesktop.org/docs/drm/gpu/vkms.html) and
[`drm_hwcomposer`](https://gitlab.freedesktop.org/drm-hwcomposer/drm-hwcomposer)

```
atest surfaceflinger_end2end_tests
```

You can also run the google test binary directly. However you will also need
to run a few other set-up and tear-down commands that are part of the
AndroidTest.xml configuration, so that SurfaceFlinger can be used run isolation
from the rest of the system.

```
# Set-up
adb root
adb shell stop
adb shell setenforce 0
adb shell setprop debug.sf.nobootanimation 1

# Sync and run the test
adb sync data
adb shell data/nativetest64/surfaceflinger_end2end_tests/surfaceflinger_end2end_tests

# Tear-down
adb shell stop
adb shell setenforce 1
adb shell setprop debug.sf.nobootanimation 0
adb shell setprop debug.sf.hwc_service_name default
```

### Manual clang-tidy checks via Soong

At present Android does not run the clang-tidy checks as part of its
presubmit checks.

You can run them through the build system by using phony target that are
automatically created for each source subdirectory.

For the code under `frameworks/native/services/surfaceflinger/tests/end2end`,
you would build:

```
m tidy-frameworks-native-services-surfaceflinger-tests-end2end
```

For more information see the build documentation:

* <https://android.googlesource.com/platform/build/soong/+/main/docs/tidy.md#the-tidy_directory-targets>

### Seeing clang-tidy checks in your editor

If your editor supports using [`clangd`](https://clangd.llvm.org/) as a
C++ language server, you can build and export a compilation database using
Soong. With the local `.clangd` configuration file, you should see the same
checks in editor, along with all the other checks `clangd` runs.

See the build documentation for the compilation database instructions:

https://android.googlesource.com/platform/build/soong/+/main/docs/compdb.md
