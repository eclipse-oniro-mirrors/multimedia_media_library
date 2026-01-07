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
const uint32_t BATCH_SIZE = 200000;

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
    CHECK_AND_RETURN_LOG(headerData != nullptr, "Init failed, headerData is nullptr");

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
    CHECK_AND_RETURN_LOG(headerData != nullptr, "Init failed, headerData is nullptr");
    CHECK_AND_RETURN_LOG(payloadData != nullptr, "Init failed, payloadData is nullptr");

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

void MtpPacket::Stop()
{
    CHECK_AND_RETURN_LOG(mtpDriver_ != nullptr, "mtpDriver_ is null");
    (void)mtpDriver_->CloseDriver();
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
        case MTP_OPERATION_GET_DEVICE_PROP_VALUE_CODE:
        case MTP_OPERATION_SET_DEVICE_PROP_VALUE_CODE:
        case MTP_OPERATION_GET_OBJECT_PROPS_SUPPORTED_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_DESC_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_VALUE_CODE:
        case MTP_OPERATION_SET_OBJECT_PROP_VALUE_CODE:
        case MTP_OPERATION_GET_OBJECT_PROP_LIST_CODE:
        case MTP_OPERATION_GET_OBJECT_REFERENCES_CODE:
        case MTP_OPERATION_GET_PARTIAL_OBJECT_CODE:
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
    CHECK_AND_RETURN_RET_LOG(mtpDriver_ != nullptr,
        MTP_ERROR_DRIVER_OPEN_FAILED, "Read failed, mtpDriver_ is nullptr");

    std::vector<uint8_t>().swap(readBuffer_);
    int errorCode = mtpDriver_->Read(readBuffer_, readSize_);
    return errorCode;
}

int MtpPacket::Write(int32_t &result)
{
    CHECK_AND_RETURN_RET_LOG(mtpDriver_ != nullptr,
        MTP_ERROR_DRIVER_OPEN_FAILED, "Write failed, mtpDriver_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_FAIL, "Write failed, headerData_ is nullptr");

    if (headerData_->GetContainerType() == EVENT_CONTAINER_TYPE) {
        EventMtp event;
        event.length = EVENT_LENGTH;
        event.data = writeBuffer_;
        result = mtpDriver_->WriteEvent(event);
        return MTP_SUCCESS;
    }
    // Due to the USB module using IPC for communication, and the maximum length supported by IPC is 248000,
    // when the write buffer is too large, it needs to be split into multiple calls.
    if (writeBuffer_.size() > BATCH_SIZE) {
        uint32_t total = writeBuffer_.size();
        for (uint32_t i = 0; i < writeBuffer_.size(); i += BATCH_SIZE) {
            uint32_t end = std::min(i + BATCH_SIZE, total);
            std::vector<uint8_t> batch(writeBuffer_.begin() + i, writeBuffer_.begin() + end);
            mtpDriver_->Write(batch, end, result);
            std::vector<uint8_t>().swap(batch);
        }
    } else {
        mtpDriver_->Write(writeBuffer_, writeSize_, result);
    }
    return MTP_SUCCESS;
}

int MtpPacket::Parser()
{
    int errorCode = ParserHead();
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "ParserHead fail err: %{public}d", errorCode);

    errorCode = ParserPayload();
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "ParserPayload fail err: %{public}d", errorCode);
    return MTP_SUCCESS;
}

int MtpPacket::Maker(bool isPayload)
{
    CHECK_AND_RETURN_RET_LOG(payloadData_ != nullptr, MTP_FAIL, "Maker failed, payloadData_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_FAIL, "Maker failed, headerData_ is nullptr");

    writeSize_ = payloadData_->CalculateSize() + PACKET_HEADER_LENGETH;
    headerData_->SetContainerLength(writeSize_);

    int errorCode = MakeHead();
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "MakeHead fail err: %{public}d", errorCode);

    errorCode = MakerPayload();
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "MakeHead fail err: %{public}d", errorCode);
    return MTP_SUCCESS;
}

int MtpPacket::ParserHead()
{
    CHECK_AND_RETURN_RET_LOG(readSize_ > 0, MTP_ERROR_PACKET_INCORRECT, "ParserHead fail readSize_ <= 0");
    if (headerData_ == nullptr) {
        headerData_ = make_shared<HeaderData>(context_);
    }
    int errorCode = headerData_->Parser(readBuffer_, readSize_);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "PacketHeader Parser fail err: %{public}d",
        errorCode);
    return MTP_SUCCESS;
}

int MtpPacket::ParserPayload()
{
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_ERROR_PACKET_INCORRECT,
        "ParserPayload failed, headerData_ is nullptr");

    CHECK_AND_RETURN_RET_LOG(readSize_ > 0, MTP_ERROR_PACKET_INCORRECT, "ParserPayload fail readSize_ <= 0");
    CHECK_AND_RETURN_RET_LOG(headerData_->GetCode() != 0, MTP_ERROR_PACKET_INCORRECT, "GetOperationCode fail");
    CHECK_AND_RETURN_RET_LOG(headerData_->GetContainerType() != 0, MTP_ERROR_PACKET_INCORRECT, "GetOperationCode fail");

    payloadData_ = PacketPayloadFactory::CreatePayload(context_,
        headerData_->GetCode(), headerData_->GetContainerType());
    CHECK_AND_RETURN_RET_LOG(payloadData_ != nullptr, MTP_FAIL, "payloadData_ is nullptr");

    int errorCode = payloadData_->Parser(readBuffer_, readSize_);
    CHECK_AND_PRINT_LOG(errorCode == MTP_SUCCESS, "PacketHeader Parser fail err: %{public}d", errorCode);
    return errorCode;
}

int MtpPacket::MakeHead()
{
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_SUCCESS, "headerData_ is null!");

    int errorCode = headerData_->Maker(writeBuffer_);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "HeaderData Make fail err: %{public}d", errorCode);
    return MTP_SUCCESS;
}

int MtpPacket::MakerPayload()
{
    CHECK_AND_RETURN_RET_LOG(payloadData_ != nullptr, MTP_SUCCESS, "payloadData_ is null!");

    int errorCode = payloadData_->Maker(writeBuffer_);
    CHECK_AND_RETURN_RET_LOG(errorCode == MTP_SUCCESS, errorCode, "PayloadData Make fail err: %{public}d", errorCode);
    return MTP_SUCCESS;
}

uint16_t MtpPacket::GetOperationCode()
{
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_FAIL, "GetOperationCode failed, headerData_ is nullptr");
    return headerData_->GetCode();
}

uint32_t MtpPacket::GetTransactionId()
{
    CHECK_AND_RETURN_RET_LOG(headerData_ != nullptr, MTP_FAIL, "GetTransactionId failed, headerData_ is nullptr");
    return headerData_->GetTransactionId();
}

uint32_t MtpPacket::GetSessionID()
{
    return 0;
}
} // namespace Media
} // namespace OHOS
