/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#define MLOG_TAG "MtpEvent"
#include "mtp_event.h"
#include <numeric>
#include <unistd.h>
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_dfx_reporter.h"
#include "mtp_packet.h"
#include "mtp_packet_tools.h"
#include "mtp_media_library.h"

using namespace std;
namespace OHOS {
namespace Media {
MtpEvent::MtpEvent(const std::shared_ptr<MtpOperationContext> &context)
{
    CHECK_AND_RETURN_LOG(context != nullptr, "MtpEvent failed, context is nullptr");

    if (context != nullptr) {
        mtpContextPtr_ = context;
    }
}

MtpEvent::~MtpEvent()
{
}

void MtpEvent::SendObjectAdded(const std::string &path)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendObjectAdded failed, mtpContextPtr_ is nullptr");

    handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    CHECK_AND_RETURN_LOG(handleptr_ != nullptr, "SendObjectAdded failed, handleptr_ is nullptr");

    uint32_t handle{0};
    int i{0};
    while (i < MTP_SEND_ADD_TIMES) {
        if (handleptr_->GetHandleByPaths(path, handle) == E_SUCCESS) {
            mtpContextPtr_->eventHandle = handle;
            SendEvent(MTP_EVENT_OBJECT_ADDED_CODE);
            return;
        }
        i++;
        usleep(MTP_SEND_ADD);
        MEDIA_DEBUG_LOG("MtpEvent::sendObjectAdded try %{public}d times", i);
    }
}

void MtpEvent::SendObjectRemoved(const std::string &path)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendObjectRemoved failed, mtpContextPtr_ is nullptr");

    handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    CHECK_AND_RETURN_LOG(handleptr_ != nullptr, "SendObjectRemoved failed, handleptr_ is nullptr");

    uint32_t handle{0};
    int i{0};
    while (i < MTP_SEND_ADD_TIMES) {
        if (handleptr_->GetHandleByPaths(path, handle) == E_SUCCESS) {
            mtpContextPtr_->eventHandle = handle;
            SendEvent(MTP_EVENT_OBJECT_REMOVED_CODE);
            MtpMediaLibrary::GetInstance()->ObserverDeletePathToMap(path);
            return;
        }
        i++;
        usleep(MTP_SEND_ADD);
        MEDIA_DEBUG_LOG("MtpEvent::sendObjectRemoved try %{public}d times", i);
    }
}

void MtpEvent::SendObjectRemovedByHandle(uint32_t handle)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendObjectRemovedByHandle failed, mtpContextPtr_ is nullptr");
    mtpContextPtr_->eventHandle = handle;
    SendEvent(MTP_EVENT_OBJECT_REMOVED_CODE);
}

void MtpEvent::SendObjectInfoChanged(const std::string &path)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendObjectInfoChanged failed, mtpContextPtr_ is nullptr");

    handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    CHECK_AND_RETURN_LOG(handleptr_ != nullptr, "SendObjectInfoChanged failed, handleptr_ is nullptr");

    uint32_t handle{0};
    if (handleptr_->GetHandleByPaths(path, handle) == E_SUCCESS) {
        mtpContextPtr_->eventHandle = handle;
        SendEvent(MTP_EVENT_OBJECT_INFO_CHANGED_CODE);
    }
}

void MtpEvent::SendDevicePropertyChanged()
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendDevicePropertyChanged failed, mtpContextPtr_ is nullptr");

    mtpContextPtr_->eventProperty = MTP_DEVICE_PROPERTY_BATTERY_LEVEL_CODE;
    SendEvent(MTP_EVENT_DEVICE_PROP_CHANGED_CODE);
}

void MtpEvent::SendStoreAdded(const std::string &fsUuid)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendStoreAdded mtpContextPtr_ is nullptr");
    handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    CHECK_AND_RETURN_LOG(handleptr_ != nullptr, "SendStoreAdded handleptr_ is nullptr");

    uint32_t storageId{0};
    CHECK_AND_RETURN_LOG(handleptr_->TryAddExternalStorage(fsUuid, storageId), "TryAddExternalStorage fail");
    MEDIA_INFO_LOG("SendStoreAdded storageId[%{public}d]", storageId);
    mtpContextPtr_->storageInfoID = storageId;
    SendEvent(MTP_EVENT_STORE_ADDED_CODE);
}

void MtpEvent::SendStoreRemoved(const std::string &fsUuid)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendStoreRemoved mtpContextPtr_ is nullptr");
    handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    CHECK_AND_RETURN_LOG(handleptr_ != nullptr, "SendStoreRemoved handleptr_ is nullptr");

    uint32_t storageId{0};
    CHECK_AND_RETURN_LOG(handleptr_->TryRemoveExternalStorage(fsUuid, storageId), "TryRemoveExternalStorage fail");
    MEDIA_INFO_LOG("SendStoreRemoved storageId[%{public}d]", storageId);
    mtpContextPtr_->storageInfoID = storageId;
    SendEvent(MTP_EVENT_STORE_REMOVED_CODE);
}

void MtpEvent::SendEvent(const int32_t &code)
{
    CHECK_AND_RETURN_LOG(mtpContextPtr_ != nullptr, "SendEvent failed, mtpContextPtr_ is nullptr");

    shared_ptr<PayloadData> eventPayloadData;

    uint16_t responseCode = EventPayloadData(code, eventPayloadData);
    if (responseCode == MTP_UNDEFINED_CODE) {
        MEDIA_DEBUG_LOG("Mtp Event GetPayloadData Error");
    }
    shared_ptr<HeaderData> eventHeaderData =
        make_shared<HeaderData>(EVENT_CONTAINER_TYPE, code, HeaderData::sTransactionID_);
    shared_ptr<MtpPacket> eventPacketPtr = std::make_shared<MtpPacket>(mtpContextPtr_, mtpContextPtr_->mtpDriver);
    CHECK_AND_RETURN_LOG(eventPacketPtr != nullptr, "SendEvent failed, eventPacketPtr is nullptr");

    eventPacketPtr->Init(eventHeaderData, eventPayloadData);
    int errorCode = eventPacketPtr->Maker(true);
    CHECK_AND_RETURN_LOG(errorCode == MTP_SUCCESS, "MtpEvent::SendEvent  responsePacket Maker err: %{public}d",
        errorCode);
    int32_t result = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    errorCode = eventPacketPtr->Write(result);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<uint16_t, std::milli> duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    MtpDfxReporter::GetInstance().DoSendResponseResultDfxReporter(mtpContextPtr_->operationCode, result,
        duration.count(), OperateMode::writemode);
    CHECK_AND_RETURN_LOG(errorCode == MTP_SUCCESS, "MtpEvent::SendEvent responsePacket Write err: %{public}d",
        errorCode);
}

uint16_t MtpEvent::EventPayloadData(const uint16_t code, shared_ptr<PayloadData> &data)
{
    uint16_t responseCode = MTP_UNDEFINED_CODE;
    CHECK_AND_RETURN_RET_LOG(mtpContextPtr_ != nullptr,
        responseCode, "EventPayloadData failed, mtpContextPtr_ is nullptr");

    if (handleptr_ == nullptr) {
        handleptr_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    }
    switch (code) {
        case MTP_EVENT_OBJECT_ADDED_CODE:
        case MTP_EVENT_OBJECT_REMOVED_CODE:
        case MTP_EVENT_OBJECT_INFO_CHANGED_CODE:
            responseCode = handleptr_->ObjectEvent(data, mtpContextPtr_->eventHandle);
            break;
        case MTP_EVENT_DEVICE_PROP_CHANGED_CODE:
            responseCode = handleptr_->ObjectEvent(data, mtpContextPtr_->eventProperty);
            break;
        case MTP_EVENT_STORE_ADDED_CODE:
        case MTP_EVENT_STORE_REMOVED_CODE:
            responseCode = handleptr_->ObjectEvent(data, mtpContextPtr_->storageInfoID);
            break;
        default:
            break;
    }
    return responseCode;
}
} // namespace Media
} // namespace OHOS