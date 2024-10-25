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
#include "mtp_packet.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"
#include "packet_payload_factory.h"
using namespace std;
namespace OHOS {
namespace Media {
const int EVENT_LENGTH = 16;
MtpPacket::MtpPacket(std::shared_ptr<MtpOperationContext> &context)
    : context_(context), readSize_(0), headerData_(nullptr), payloadData_(nullptr)
{
}

MtpPacket::MtpPacket(std::shared_ptr<MtpOperationContext> &context, const shared_ptr<MtpDriver> &mtpDriver)
    : context_(context), readSize_(0), headerData_(nullptr), payloadData_(nullptr), mtpDriver_(mtpDriver)
{
}

MtpPacket::~MtpPacket()
{
}

void MtpPacket::Init(std::shared_ptr<HeaderData> &headerData)
{
    readSize_ = 0;
    headerData_ = headerData;

    if (headerData->GetContainerType() == DATA_CONTAINER_TYPE) {
        readBufSize_ = READ_DATA_BUFFER_MAX_SIZE;
    } else {
        readBufSize_ = READ_BUFFER_MAX_SIZE;
    }
}

void MtpPacket::Init(std::shared_ptr<HeaderData> &headerData, std::shared_ptr<PayloadData> &payloadData)
{
    readSize_ = 0;
    headerData_ = headerData;
    payloadData_ = payloadData;

    if (headerData->GetContainerType() == DATA_CONTAINER_TYPE) {
        readBufSize_ = READ_DATA_BUFFER_MAX_SIZE;
    } else {
        readBufSize_ = READ_BUFFER_MAX_SIZE;
    }
}

void MtpPacket::Reset()
{
    readSize_ = 0;
    headerData_ = nullptr;
    payloadData_ = nullptr;
    std::vector<uint8_t>().swap(writeBuffer_);
}

bool MtpPacket::IsNeedDataPhase(uint16_t operationCode)
{
    switch (operationCode) {
        case MTP_OPERATION_GET_DEVICE_INFO_CODE:
        case MTP_OPERATION_GET_STORAGE_IDS_CODE:
        case MTP_OPERATION_GET_STORAGE_INFO_CODE:
        case MTP_OPERATION_GET_OBJECT_HANDLES_CODE:
        case MTP_OPERATION_GET_OBJECT_CODE:
        case MTP_OPERATION_GET_OBJECT_INFO_CODE:
        case MTP_OPERATION_GET_THUMB_CODE:
        case MTP_OPERATION_SEND_OBJECT_INFO_CODE:
        case MTP_OPERATION_SEND_OBJECT_CODE:
        case MTP_OPERATION_GET_DEVICE_PROP_DESC_CODE:
        case MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE:
        case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE:
        case MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE:
        case MTP_OPERATION_GET_OBJECT_REFERENCES_CODE:
            return true;
        default:
            break;
    }
    return false;
}

bool MtpPacket::IsI2R(uint16_t operationCode)
{
    switch (operationCode) {
        case MTP_OPERATION_SEND_OBJECT_INFO_CODE:
        case MTP_OPERATION_SEND_OBJECT_CODE:
        case MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE:
        case MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE:
            return true;
        default:
            break;
    }
    return false;
}

int MtpPacket::Read()
{
    std::vector<uint8_t>().swap(readBuffer_);
    int errorCode = mtpDriver_->Read(readBuffer_, readSize_);
    return errorCode;
}

int MtpPacket::Write()
{
    if (headerData_->GetContainerType() == EVENT_CONTAINER_TYPE) {
        EventMtp event;
        event.length = EVENT_LENGTH;
        event.data = writeBuffer_;
        mtpDriver_->WriteEvent(event);
        return MTP_SUCCESS;
    }
    mtpDriver_->Write(writeBuffer_, writeSize_);
    return MTP_SUCCESS;
}

int MtpPacket::Parser()
{
    int errorCode = ParserHead();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("ParserHead fail err: %{public}d", errorCode);
        return errorCode;
    }

    errorCode = ParserPayload();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("ParserPayload fail err: %{public}d", errorCode);
        return errorCode;
    }
    return MTP_SUCCESS;
}

int MtpPacket::Maker(bool isPayload)
{
    writeSize_ = payloadData_->CalculateSize() + PACKET_HEADER_LENGETH;
    headerData_->SetContainerLength(writeSize_);

    int errorCode = MakeHead();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("MakeHead fail err: %{public}d", errorCode);
        return errorCode;
    }

    errorCode = MakerPayload();
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("MakeHead fail err: %{public}d", errorCode);
        return errorCode;
    }
    return MTP_SUCCESS;
}

int MtpPacket::ParserHead()
{
    if (readSize_ <= 0) {
        MEDIA_ERR_LOG("ParserHead fail readSize_ <= 0");
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (headerData_ == nullptr) {
        headerData_ = make_shared<HeaderData>(context_);
    }
    int errorCode = headerData_->Parser(readBuffer_, readSize_);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("PacketHeader Parser fail err: %{public}d", errorCode);
        return errorCode;
    }
    return MTP_SUCCESS;
}

int MtpPacket::ParserPayload()
{
    if (readSize_ <= 0) {
        MEDIA_ERR_LOG("ParserPayload fail readSize_ <= 0");
        return MTP_ERROR_PACKET_INCORRECT;
    }
    if (headerData_->GetCode() == 0) {
        MEDIA_ERR_LOG("GetOperationCode fail");
        return MTP_ERROR_PACKET_INCORRECT;
    }

    if (headerData_->GetContainerType() == 0) {
        MEDIA_ERR_LOG("GetOperationCode fail");
        return MTP_ERROR_PACKET_INCORRECT;
    }

    payloadData_ = PacketPayloadFactory::CreatePayload(context_,
        headerData_->GetCode(), headerData_->GetContainerType());
    if (payloadData_ == nullptr) {
        MEDIA_ERR_LOG("payloadData_ is nullptr");
        return MTP_FAIL;
    }

    int errorCode = payloadData_->Parser(readBuffer_, readSize_);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("PacketHeader Parser fail err: %{public}d", errorCode);
    }
    return errorCode;
}

int MtpPacket::MakeHead()
{
    if (headerData_ == nullptr) {
        MEDIA_ERR_LOG("headerData_ is null!");
        return MTP_SUCCESS;
    }

    int errorCode = headerData_->Maker(writeBuffer_);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("HeaderData Make fail err: %{public}d", errorCode);
        return errorCode;
    }
    return MTP_SUCCESS;
}

int MtpPacket::MakerPayload()
{
    if (payloadData_ == nullptr) {
        MEDIA_ERR_LOG("payloadData_ is null!");
        return MTP_SUCCESS;
    }

    int errorCode = payloadData_->Maker(writeBuffer_);
    if (errorCode != MTP_SUCCESS) {
        MEDIA_ERR_LOG("PayloadData Make fail err: %{public}d", errorCode);
        return errorCode;
    }
    return MTP_SUCCESS;
}

uint16_t MtpPacket::GetOperationCode()
{
    return headerData_->GetCode();
}

uint32_t MtpPacket::GetTransactionId()
{
    return headerData_->GetTransactionId();
}

uint32_t MtpPacket::GetSessionID()
{
    return 0;
}
} // namespace Media
} // namespace OHOS
