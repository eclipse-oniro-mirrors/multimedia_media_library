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
#include "mtp_operation.h"
#include <algorithm>
#include "header_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"
#include "mtp_packet.h"
#include "mtp_packet_tools.h"
#include "mtp_operation_context.h"
#include "mtp_operation_utils.h"
#include "mtp_storage_manager.h"
#include "packet_payload_factory.h"
#include "payload_data/get_device_info_data.h"
#include "payload_data.h"
#include "payload_data/send_object_info_data.h"
#include "payload_data/set_object_prop_value_data.h"
#include "storage.h"

using namespace std;
namespace OHOS {
namespace Media {
MtpOperation::MtpOperation(void)
{
    Init();
}

void MtpOperation::Init()
{
    mtpContextPtr_ = make_shared<MtpOperationContext>();
    mtpContextPtr_->indata = false;

    mtpDriver_ = make_shared<MtpDriver>();
    mtpContextPtr_->mtpDriver = mtpDriver_;

    requestPacketPtr_ = make_shared<MtpPacket>(mtpContextPtr_, mtpDriver_);
    dataPacketPtr_ = make_shared<MtpPacket>(mtpContextPtr_, mtpDriver_);
    responsePacketPtr_ = make_shared<MtpPacket>(mtpContextPtr_, mtpDriver_);

    operationUtils_ = make_shared<MtpOperationUtils>(mtpContextPtr_);
    responseCode_ = MTP_UNDEFINED_CODE;
}

void MtpOperation::Execute()
{
    int errorCode;
    ResetOperation();
    ReceiveRequestPacket(errorCode);
    if (errorCode != MTP_SUCCESS) {
        SendMakeResponsePacket(errorCode);
        MEDIA_ERR_LOG("MtpOperation::Execute Out ReceiveRequestPacket fail err: %{public}d", errorCode);
        return;
    }

    DealRequest(mtpContextPtr_->operationCode, errorCode);
    if (errorCode != MTP_SUCCESS) {
        SendMakeResponsePacket(errorCode);
        MEDIA_ERR_LOG("MtpOperation::Execute Out DealRequest fail err: %{public}d", errorCode);
        return;
    }

    if (MtpPacket::IsNeedDataPhase(mtpContextPtr_->operationCode)) {
        if (MtpPacket::IsI2R(mtpContextPtr_->operationCode)) {
            ReceiveI2Rdata(errorCode);
        } else {
            SendR2Idata(errorCode);
        }
    }

    SendMakeResponsePacket(errorCode);
}

void MtpOperation::ReceiveRequestPacket(int &errorCode)
{
    shared_ptr<HeaderData> headerData = make_shared<HeaderData>(mtpContextPtr_);
    requestPacketPtr_->Init(headerData);
    errorCode = requestPacketPtr_->Read();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("requestPacket Read fail err: %{public}d", errorCode);
        return;
    }
    errorCode = requestPacketPtr_->Parser();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("requestPacket Parser fail err: %{public}d", errorCode);
        return;
    }
}

void MtpOperation::SendMakeResponsePacket(int &errorCode)
{
    responsePacketPtr_->Reset();
    GetPayloadData(mtpContextPtr_, dataPayloadData_, RESPONSE_CONTAINER_TYPE, errorCode);
    shared_ptr<HeaderData> responseHeaderData = make_shared<HeaderData>(
        RESPONSE_CONTAINER_TYPE, responseCode_, mtpContextPtr_->transactionID);

    responsePacketPtr_->Init(responseHeaderData, dataPayloadData_);
    errorCode = responsePacketPtr_->Maker(false);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("responsePacket Maker fail err: %{public}d", errorCode);
        return;
    }
    errorCode = responsePacketPtr_->Write();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("responsePacket Write fail err: %{public}d", errorCode);
        return;
    }
}

void MtpOperation::SendObjectData(int &errorCode)
{
    errorCode = operationUtils_->GetObjectDataDeal();
}

void MtpOperation::RecevieObjectData(int &errorCode)
{
    errorCode = operationUtils_->DoRecevieSendObject();
}

void MtpOperation::ReceiveI2Rdata(int &errorCode)
{
    if (mtpContextPtr_->operationCode == MTP_OPERATION_SEND_OBJECT_CODE) {
        MEDIA_INFO_LOG("ReceiveI2Rdata RecevieObjectData");
        RecevieObjectData(errorCode);
        return;
    }
    mtpContextPtr_->indata = true;

    errorCode = dataPacketPtr_->Read();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("dataPacket Read fail err: %{public}d", errorCode);
        return;
    }
    errorCode = dataPacketPtr_->Parser();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("dataPacket Parser fail err: %{public}d", errorCode);
        return;
    }

    if (mtpContextPtr_->operationCode == MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE) {
        MEDIA_INFO_LOG("ReceiveI2Rdata DoSetObjectPropValue");
        operationUtils_->DoSetObjectPropValue(errorCode);
    }
}

void MtpOperation::SendR2Idata(int &errorCode)
{
    if (mtpContextPtr_->operationCode == MTP_OPERATION_GET_OBJECT_CODE) {
        SendObjectData(errorCode);
        return;
    }

    responseCode_ = GetPayloadData(mtpContextPtr_, dataPayloadData_, DATA_CONTAINER_TYPE, errorCode);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("GetPayloadData fail err: %{public}d", errorCode);
        return;
    }
    shared_ptr<HeaderData> dataHeaderData = make_shared<HeaderData>(
        DATA_CONTAINER_TYPE, mtpContextPtr_->operationCode, mtpContextPtr_->transactionID);
    dataPacketPtr_->Init(dataHeaderData, dataPayloadData_);
    errorCode = dataPacketPtr_->Maker(true);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("dataPacket Maker fail err: %{public}d", errorCode);
        return;
    }
    errorCode = dataPacketPtr_->Write();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("dataPacket Write fail err: %{public}d", errorCode);
        return;
    }
}

void MtpOperation::DealRequest(uint16_t operationCode, int &errorCode)
{
    switch (operationCode) {
        case MTP_OPERATION_OPEN_SESSION_CODE:
            if (!mtpContextPtr_->sessionOpen) {
                mtpContextPtr_->sessionID = mtpContextPtr_->tempSessionID;
                errorCode = MTP_SUCCESS;
            } else {
                errorCode = MTP_ERROR_SESSION_ALREADY_OPEN;
            }
            break;
        default:
            errorCode = MTP_SUCCESS;
            break;
    }
}

uint16_t MtpOperation::GetPayloadData(shared_ptr<MtpOperationContext> &context, shared_ptr<PayloadData> &data,
    uint16_t containerType, int &errorCode)
{
    responseCode_ = MTP_UNDEFINED_CODE;
    switch (context->operationCode) {
        case MTP_OPERATION_GET_DEVICE_INFO_CODE:
            responseCode_ = operationUtils_->GetDeviceInfo(data, containerType, errorCode);
            break;
        case MTP_OPERATION_OPEN_SESSION_CODE:
            responseCode_ = operationUtils_->GetOpenSession(data, errorCode);
            break;
        case MTP_OPERATION_RESET_DEVICE_CODE:
        case MTP_OPERATION_CLOSE_SESSION_CODE:
            responseCode_ = operationUtils_->GetCloseSession(data);
            break;
        case MTP_OPERATION_GET_STORAGE_IDS_CODE:
            responseCode_ = operationUtils_->GetStorageIDs(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_STORAGE_INFO_CODE:
            responseCode_ = operationUtils_->GetStorageInfo(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE:
            responseCode_ = operationUtils_->GetObjectPropsSupported(data);
            break;
        case MTP_OPERATION_GET_OBJECT_HANDLES_CODE:
            responseCode_ = operationUtils_->GetObjectHandles(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_NUM_OBJECTS_CODE:
            responseCode_ = operationUtils_->GetNumObjects(data);
            break;
        case MTP_OPERATION_GET_OBJECT_INFO_CODE:
            responseCode_ = operationUtils_->GetObjectInfo(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE:
            responseCode_ = operationUtils_->GetObjectPropDesc(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE:
            responseCode_ = operationUtils_->GetObjectPropValue(data, containerType, errorCode);
            break;
        case MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE:
            responseCode_ = operationUtils_->GetRespCommonData(data, errorCode);
            break;
        case MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE:
            responseCode_ = operationUtils_->GetObjectPropList(data, containerType, errorCode);
            break;
        default:
            responseCode_ = GetPayloadDataMore(context, data, containerType, errorCode);
            break;
    }
    return responseCode_;
}

uint16_t MtpOperation::GetPayloadDataMore(shared_ptr<MtpOperationContext> &context, shared_ptr<PayloadData> &data,
    uint16_t containerType, int &errorCode)
{
    responseCode_ = MTP_UNDEFINED_CODE;
    switch (context->operationCode) {
        case MTP_OPERATION_GET_OBJECT_REFERENCES_CODE:
            responseCode_ = operationUtils_->GetObjectReferences(data, containerType, errorCode);
            break;
        case MTP_OPERATION_SET_OBJECT_REFERENCES_CODE:
            responseCode_ = operationUtils_->SetObjectReferences(data);
            break;
        case MTP_OPERATION_DELETE_OBJECT_CODE:
            responseCode_ = operationUtils_->DeleteObject(data, errorCode);
            break;
        case MTP_OPERATION_MOVE_OBJECT_CODE:
            responseCode_ = operationUtils_->MoveObject(data, errorCode);
            break;
        case MTP_OPERATION_COPY_OBJECT_CODE:
            responseCode_ = operationUtils_->CopyObject(data, errorCode);
            break;
        case MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE:
            responseCode_ = operationUtils_->GetPropDesc(data, containerType, errorCode);
            break;
        case MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE:
            responseCode_ = operationUtils_->GetPropValue(data, containerType, errorCode);
            break;
        case MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE:
            responseCode_ = operationUtils_->SetDevicePropValueResp(data);
            break;
        case MTP_OPERATION_RESET_DEVICE_PROP_VALUE_CODE:
            responseCode_ = operationUtils_->ResetDevicePropResp(data);
            break;
        case MTP_OPERATION_GET_OBJECT_CODE:
            responseCode_ = operationUtils_->GetObject(data, errorCode);
            break;
        case MTP_OPERATION_SEND_OBJECT_CODE:
            responseCode_ = operationUtils_->GetRespCommonData(data, errorCode);
            break;
        case MTP_OPERATION_GET_THUMB_CODE:
            responseCode_ = operationUtils_->GetThumb(data, containerType, errorCode);
            break;
        case MTP_OPERATION_SEND_OBJECT_INFO_CODE:
            responseCode_ = operationUtils_->SendObjectInfo(data, errorCode);
            break;
        case MTP_OPERATION_GET_PARTIAL_OBJECT_CODE:
            responseCode_ = operationUtils_->GetPartialObject(data);
            break;
        default:
            responseCode_ = operationUtils_->GetRespCommonData(data, errorCode);
            break;
    }
    return responseCode_;
}

void MtpOperation::ResetOperation()
{
    if (requestPacketPtr_ != nullptr) {
        requestPacketPtr_->Reset();
    }
    if (dataPacketPtr_ != nullptr) {
        dataPacketPtr_->Reset();
    }
    if (responsePacketPtr_ != nullptr) {
        responsePacketPtr_->Reset();
    }
    if (dataPayloadData_!= nullptr) {
        dataPayloadData_ = nullptr;
    }
    if (mtpContextPtr_ != nullptr) {
        mtpContextPtr_->operationCode = 0;
        mtpContextPtr_->transactionID = 0;
    }

    responseCode_ = MTP_OK_CODE;
    mtpContextPtr_->indata = false;
}

void MtpOperation::AddStorage(shared_ptr<Storage> &storage)
{
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    if (mtpStorageManager != nullptr) {
        mtpStorageManager->AddStorage(storage);
    }
}

void MtpOperation::RemoveStorage(std::shared_ptr<Storage> &storage)
{
    auto mtpStorageManager = MtpStorageManager::GetInstance();
    if (mtpStorageManager != nullptr) {
        mtpStorageManager->RemoveStorage(storage);
    }
}
} // namespace Media
} // namespace OHOS