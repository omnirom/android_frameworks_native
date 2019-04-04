/* Copyright (c) 2015, 2018, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "ExSurfaceFlinger.h"
#include <fstream>
#include <cutils/properties.h>
#include <ui/GraphicBufferAllocator.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace android {

bool ExSurfaceFlinger::sAllowHDRFallBack = false;
bool ExSurfaceFlinger::regionDump = false;

ExSurfaceFlinger::ExSurfaceFlinger() {
    char property[PROPERTY_VALUE_MAX] = {0};
    bool updateVSyncSourceOnDoze = false;

    mDebugLogs = false;
    if ((property_get("vendor.display.qdframework_logs", property, NULL) > 0) &&
        (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
         (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        mDebugLogs = true;
    }

    ALOGD_IF(isDebug(),"Creating custom SurfaceFlinger %s",__FUNCTION__);

    mDisableExtAnimation = false;
    if ((property_get("vendor.display.disable_ext_anim", property, "0") > 0) &&
        (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
         (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
        mDisableExtAnimation = true;
    }

    ALOGD_IF(isDebug(),"Animation on external is %s in %s",
             mDisableExtAnimation ? "disabled" : "not disabled", __FUNCTION__);

    if((property_get("vendor.display.hwc_disable_hdr", property, "0") > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
       sAllowHDRFallBack = true;
    }

    if((property_get("vendor.display.update_vsync_on_doze", property, "0") > 0) &&
       (!strncmp(property, "1", PROPERTY_VALUE_MAX ) ||
        (!strncasecmp(property,"true", PROPERTY_VALUE_MAX )))) {
       updateVSyncSourceOnDoze = true;
    }

    {
        using vendor::display::config::V1_2::IDisplayConfig;
        android::sp<IDisplayConfig> disp_config_v1_2 = IDisplayConfig::getService();
        if (disp_config_v1_2 != NULL) {
            disp_config_v1_2->setDisplayIndex(IDisplayConfig::DisplayTypeExt::DISPLAY_BUILTIN,
                     HWC_DISPLAY_BUILTIN_2, (HWC_DISPLAY_VIRTUAL - HWC_DISPLAY_BUILTIN_2));
            disp_config_v1_2->setDisplayIndex(IDisplayConfig::DisplayTypeExt::DISPLAY_PLUGGABLE,
                     HWC_DISPLAY_EXTERNAL, (HWC_DISPLAY_BUILTIN_2 - HWC_DISPLAY_EXTERNAL));
            disp_config_v1_2->setDisplayIndex(IDisplayConfig::DisplayTypeExt::DISPLAY_VIRTUAL,
                     HWC_DISPLAY_VIRTUAL, 1);
        }
    }

    {
        using vendor::display::config::V1_6::IDisplayConfig;
        android::sp<IDisplayConfig> disp_config_v1_6 = IDisplayConfig::getService();
        if (disp_config_v1_6 != NULL) {
            disp_config_v1_6->updateVSyncSourceOnPowerModeOff();
            if(updateVSyncSourceOnDoze) {
                disp_config_v1_6->updateVSyncSourceOnPowerModeDoze();
                mUpdateVSyncSourceOnDoze = true;
            }
        }
    }
}

ExSurfaceFlinger::~ExSurfaceFlinger() { }

void ExSurfaceFlinger::handleDPTransactionIfNeeded(
        const Vector<DisplayState>& displays) {
    /* Wait for one draw cycle before setting display projection only when the disable
     * external rotation animation feature is enabled
     */
    if (mDisableExtAnimation) {
        size_t count = displays.size();
        bool builtin_orient_changed = false;
        for (size_t i=0 ; i<count ; i++) {
            const DisplayState& s(displays[i]);
            sp<DisplayDevice> device(getDisplayDevice(s.token));
            if (device == nullptr) {
                continue;
            }
            int type = device->getDisplayType();
            if ((type > DisplayDevice::DISPLAY_ID_INVALID &&
                type < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES && mBuiltInBitmask.test(type)) &&
                !(s.orientation & DisplayState::eOrientationUnchanged)) {
                builtin_orient_changed = true;
                break;
            }
        }
        for (size_t i=0 ; i<count ; i++) {
            const DisplayState& s(displays[i]);
            sp<DisplayDevice> device(getDisplayDevice(s.token));
            if (device == nullptr) {
                continue;
            }
            int type = device->getDisplayType();
            if (type == DisplayDevice::DISPLAY_VIRTUAL ||
                (type > DisplayDevice::DISPLAY_ID_INVALID &&
                type < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES && !mBuiltInBitmask.test(type))) {
                const uint32_t what = s.what;
                /* Invalidate and wait on eDisplayProjectionChanged to trigger a draw cycle so that
                 * it can fix one incorrect frame on the External, when we
                 * disable external animation
                 */
                if (what & DisplayState::eDisplayProjectionChanged && builtin_orient_changed) {
                    Mutex::Autolock lock(mExtAnimationLock);
                    invalidateHwcGeometry();
                    android_atomic_or(1, &mRepaintEverything);
                    signalRefresh();
                    status_t err = mExtAnimationCond.waitRelative(mExtAnimationLock, 1000000000);
                    if (err == -ETIMEDOUT) {
                        ALOGW("External animation signal timed out!");
                    }
                }
            }
        }
    }
}

void ExSurfaceFlinger::setDisplayAnimating(const sp<const DisplayDevice>& hw __unused) {
    static android::sp<vendor::display::config::V1_1::IDisplayConfig> disp_config_v1_1 =
                                        vendor::display::config::V1_1::IDisplayConfig::getService();

    int32_t dpy = hw->getDisplayType();
    if (disp_config_v1_1 == NULL || !mDisableExtAnimation ||
        ((dpy > DisplayDevice::DISPLAY_ID_INVALID &&
        dpy < DisplayDevice::NUM_BUILTIN_DISPLAY_TYPES) && mBuiltInBitmask.test(dpy))) {
        return;
    }

    bool hasScreenshot = false;
    mDrawingState.traverseInZOrder([&](Layer* layer) {
      if (layer->getLayerStack() == hw->getLayerStack()) {
          if (layer->isScreenshot()) {
              hasScreenshot = true;
          }
      }
    });

    if (hasScreenshot == mAnimating) {
        return;
    }

    disp_config_v1_1->setDisplayAnimating(dpy, hasScreenshot);
    mAnimating = hasScreenshot;
}

status_t ExSurfaceFlinger::doDump(int fd, const Vector<String16>& args, bool asProto) {
    // Format: adb shell dumpsys SurfaceFlinger --file --no-limit
    size_t numArgs = args.size();
    status_t err = NO_ERROR;

    if (!numArgs || ((args[0] != String16("--file")) &&
        (args[0] != String16("--allocated_buffers")))) {
        return SurfaceFlinger::doDump(fd, args, asProto);
    }

    if (args[0] == String16("--allocated_buffers")) {
        String8 dumpsys;
        GraphicBufferAllocator& alloc(GraphicBufferAllocator::get());
        alloc.dump(dumpsys);
        write(fd, dumpsys.string(), dumpsys.size());
        return NO_ERROR;
    }

    if (numArgs >= 3 && (args[2] == String16("--region-dump"))){
        regionDump = true;
    }

    Mutex::Autolock _l(mFileDump.lock);

    // Same command is used to start and end dump.
    mFileDump.running = !mFileDump.running;

    if (mFileDump.running) {
        std::ofstream ofs;
        ofs.open(mFileDump.name, std::ofstream::out | std::ofstream::trunc);
        if (!ofs) {
            mFileDump.running = false;
            err = UNKNOWN_ERROR;
        } else {
            ofs.close();
            mFileDump.position = 0;
            if (numArgs >= 2 && (args[1] == String16("--no-limit"))) {
            mFileDump.noLimit = true;
        } else {
            mFileDump.noLimit = false;
        }
      }
    }

    String8 result;
    result += mFileDump.running ? "Start" : "End";
    result += mFileDump.noLimit ? " unlimited" : " fixed limit";
    result += " dumpsys to file : ";
    result += mFileDump.name;
    result += "\n";

    write(fd, result.string(), result.size());

    return NO_ERROR;
}

void ExSurfaceFlinger::dumpDrawCycle(bool prePrepare) {
    Mutex::Autolock _l(mFileDump.lock);

    // User might stop dump collection in middle of prepare & commit.
    // Collect dumpsys again after commit and replace.
    if (!mFileDump.running && !mFileDump.replaceAfterCommit) {
        regionDump = false;
        return;
    }

    Vector<String16> args;
    size_t index = 0;
    String8 dumpsys;

    {
        Mutex::Autolock lock(mStateLock);
        dumpAllLocked(args, index, dumpsys, regionDump);
    }

    char timeStamp[32];
    char dataSize[32];
    char hms[32];
    long millis;
    struct timeval tv;
    struct tm *ptm;

    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);
    strftime (hms, sizeof (hms), "%H:%M:%S", ptm);
    millis = tv.tv_usec / 1000;
    snprintf(timeStamp, sizeof(timeStamp), "Timestamp: %s.%03ld", hms, millis);
    snprintf(dataSize, sizeof(dataSize), "Size: %8zu", dumpsys.size());

    std::fstream fs;
    fs.open(mFileDump.name, std::ios::app);
    if (!fs) {
        ALOGE("Failed to open %s file for dumpsys", mFileDump.name);
        return;
    }

    // Format:
    //    | start code | after commit? | time stamp | dump size | dump data |
    fs.seekp(mFileDump.position, std::ios::beg);

    fs << "#@#@-- DUMPSYS START --@#@#" << std::endl;
    fs << "PostCommit: " << ( prePrepare ? "false" : "true" ) << std::endl;
    fs << timeStamp << std::endl;
    fs << dataSize << std::endl;
    fs << dumpsys << std::endl;

    if (prePrepare) {
        mFileDump.replaceAfterCommit = true;
    } else {
        mFileDump.replaceAfterCommit = false;
        // Reposition only after commit.
        // Keep file size to appx 20 MB limit by default, wrap around if exceeds.
        mFileDump.position = fs.tellp();
        if (!mFileDump.noLimit && (mFileDump.position > (20 * 1024 * 1024))) {
            mFileDump.position = 0;
        }
    }

    fs.close();
}

void ExSurfaceFlinger::handleMessageRefresh() {
    SurfaceFlinger::handleMessageRefresh();
    if (mDisableExtAnimation && mAnimating) {
        Mutex::Autolock lock(mExtAnimationLock);
        mExtAnimationCond.signal();
    }
}

void ExSurfaceFlinger::setLayerAsMask(const int32_t& dispId, const uint64_t& layerId) {
    using vendor::display::config::V1_7::IDisplayConfig;
    android::sp<IDisplayConfig> disp_config_v1_7 = IDisplayConfig::getService();

    if (!disp_config_v1_7 || (dispId < 0)) {
      ALOGI("disp_config_v1_7 not found or dispId %d", dispId);
      return;
    }

    disp_config_v1_7->setLayerAsMask(dispId, layerId);
    ALOGI("setLayerAsMask dispId %d layerId %d", dispId, (uint32_t)layerId);
}
}; // namespace android
