/*
 * Copyright (C) 2007 The Android Open Source Project
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

// tag as surfaceflinger
#define LOG_TAG "SurfaceFlinger"

#include <android/gui/IDisplayEventConnection.h>
#include <android/gui/IRegionSamplingListener.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/ISurfaceComposer.h>
#include <gui/LayerState.h>
#include <gui/SchedulingPolicy.h>
#include <gui/TransactionState.h>
#include <private/gui/ParcelUtils.h>
#include <stdint.h>
#include <sys/types.h>
#include <system/graphics.h>
#include <ui/DisplayMode.h>
#include <ui/DisplayStatInfo.h>
#include <ui/DisplayState.h>
#include <ui/DynamicDisplayInfo.h>
#include <ui/HdrCapabilities.h>
#include <utils/Log.h>

// ---------------------------------------------------------------------------

using namespace aidl::android::hardware::graphics;

namespace android {

using gui::DisplayCaptureArgs;
using gui::IDisplayEventConnection;
using gui::IRegionSamplingListener;
using gui::IWindowInfosListener;
using gui::LayerCaptureArgs;
using ui::ColorMode;

class BpSurfaceComposer : public BpInterface<ISurfaceComposer>
{
public:
    explicit BpSurfaceComposer(const sp<IBinder>& impl)
        : BpInterface<ISurfaceComposer>(impl)
    {
    }

    virtual ~BpSurfaceComposer();

    status_t setTransactionState(TransactionState&& state) override {
        Parcel data, reply;
        data.writeInterfaceToken(ISurfaceComposer::getInterfaceDescriptor());
        SAFE_PARCEL(state.writeToParcel, &data);

        if (state.mFlags & ISurfaceComposer::eOneWay) {
            return remote()->transact(BnSurfaceComposer::SET_TRANSACTION_STATE,
                    data, &reply, IBinder::FLAG_ONEWAY);
        } else {
            return remote()->transact(BnSurfaceComposer::SET_TRANSACTION_STATE,
                    data, &reply);
        }
    }
};

// Out-of-line virtual method definition to trigger vtable emission in this
// translation unit (see clang warning -Wweak-vtables)
BpSurfaceComposer::~BpSurfaceComposer() {}

IMPLEMENT_META_INTERFACE(SurfaceComposer, "android.ui.ISurfaceComposer");

// ----------------------------------------------------------------------

status_t BnSurfaceComposer::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case SET_TRANSACTION_STATE: {
            CHECK_INTERFACE(ISurfaceComposer, data, reply);

            TransactionState state;
            SAFE_PARCEL(state.readFromParcel, &data);
            return setTransactionState(std::move(state));
        }
        case GET_SCHEDULING_POLICY: {
            gui::SchedulingPolicy policy;
            const auto status = gui::getSchedulingPolicy(&policy);
            if (!status.isOk()) {
                return status.exceptionCode();
            }

            SAFE_PARCEL(reply->writeInt32, policy.policy);
            SAFE_PARCEL(reply->writeInt32, policy.priority);
            return NO_ERROR;
        }

        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
}

} // namespace android
