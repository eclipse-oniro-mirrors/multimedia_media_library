/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MEDIA_DEEP_OPTIMIZE_SPACE_CALLBACK_H
#define OHOS_MEDIA_DEEP_OPTIMIZE_SPACE_CALLBACK_H

#include <cstdint>

#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "media_log.h"
#include "message_option.h"
#include "message_parcel.h"

namespace OHOS::Media {
enum class DeepOptimizeSpaceState : int32_t {
    RUNNING = 0,
    COMPLETED = 1,
    FAILED = 2,
    STOPPED = 3,
    INTERRUPTED = 4,
};

enum class DeepOptimizeSpaceCallbackCode : uint32_t {
    ON_PROGRESS_UPDATE = 0,
};

class DeepOptimizeSpaceProgress {
public:
    DeepOptimizeSpaceState state = DeepOptimizeSpaceState::RUNNING;
    int32_t progress = 0;

    bool Marshalling(MessageParcel &parcel) const
    {
        CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(static_cast<int32_t>(state)), false, "Failed to write state");
        CHECK_AND_RETURN_RET_LOG(parcel.WriteInt32(progress), false, "Failed to write progress");
        return true;
    }

    bool Unmarshalling(MessageParcel &parcel)
    {
        int32_t stateValue = 0;
        CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(stateValue), false, "Failed to read stateValue");
        state = static_cast<DeepOptimizeSpaceState>(stateValue);
        CHECK_AND_RETURN_RET_LOG(parcel.ReadInt32(progress), false, "Failed to read progress");
        return true;
    }
};

class IDepOptimizeSpaceCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.Media.DeepOptimizeSpaceCallback");

    virtual int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) = 0;
};

class DeepOptimizeSpaceCallbackProxy : public IRemoteProxy<IDepOptimizeSpaceCallback> {
public:
    explicit DeepOptimizeSpaceCallbackProxy(const sptr<IRemoteObject> &impl)
        : IRemoteProxy<IDepOptimizeSpaceCallback>(impl)
    {
    }

    int32_t OnProgressUpdate(const DeepOptimizeSpaceProgress &progress) override
    {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option(MessageOption::TF_ASYNC);
        if (!data.WriteInterfaceToken(GetDescriptor())) {
            return IPC_STUB_INVALID_DATA_ERR;
        }
        if (!progress.Marshalling(data)) {
            return IPC_STUB_INVALID_DATA_ERR;
        }
        auto remote = Remote();
        if (remote == nullptr) {
            return IPC_STUB_INVALID_DATA_ERR;
        }
        return remote->SendRequest(static_cast<uint32_t>(DeepOptimizeSpaceCallbackCode::ON_PROGRESS_UPDATE),
            data, reply, option);
    }

private:
    static inline BrokerDelegator<DeepOptimizeSpaceCallbackProxy> delegator_;
};

class DeepOptimizeSpaceCallbackStub : public IRemoteStub<IDepOptimizeSpaceCallback> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        if (data.ReadInterfaceToken() != GetDescriptor()) {
            return ERR_UNKNOWN_TRANSACTION;
        }

        if (code != static_cast<uint32_t>(DeepOptimizeSpaceCallbackCode::ON_PROGRESS_UPDATE)) {
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }

        DeepOptimizeSpaceProgress progress;
        if (!progress.Unmarshalling(data)) {
            return IPC_STUB_INVALID_DATA_ERR;
        }
        return OnProgressUpdate(progress);
    }
};
} // namespace OHOS::Media

#endif // OHOS_MEDIA_DEEP_OPTIMIZE_SPACE_CALLBACK_H