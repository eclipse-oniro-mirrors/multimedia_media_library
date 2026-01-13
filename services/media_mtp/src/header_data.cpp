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
#include "header_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
uint32_t HeaderData::sTransactionID_ = 0;

HeaderData::HeaderData(std::shared_ptr<MtpOperationContext> &context)
{
    context_ = context;
}

HeaderData::HeaderData(uint16_t containerType, uint16_t code, uint32_t transactionID)
    : containerType_(containerType), code_(code), transactionID_(transactionID)
{
}

HeaderData::~HeaderData()
{
}

int HeaderData::Parser(vector<uint8_t> &buffer, uint32_t readSize)
{
    CHECK_AND_RETURN_RET_LOG(readSize >= PACKET_HEADER_LENGETH, MTP_ERROR_PACKET_INCORRECT,
        "readSize incorrect : < PACKET_HEADER_LENGETH!");
    CHECK_AND_RETURN_RET_LOG(buffer.size() >= PACKET_HEADER_LENGETH, MTP_ERROR_PACKET_INCORRECT,
        "buffer size incorrect : < PACKET_HEADER_LENGETH!");

    int offset = 0;
    containerLength_ = MtpPacketTool::GetUInt32(buffer[offset], buffer[offset + OFFSET_1],
        buffer[offset + OFFSET_2], buffer[offset + OFFSET_3]);
    containerType_ = MtpPacketTool::GetUInt16(buffer[offset + OFFSET_4], buffer[offset + OFFSET_5]);
    code_ = MtpPacketTool::GetUInt16(buffer[offset + OFFSET_6], buffer[offset + OFFSET_7]);
    transactionID_ = MtpPacketTool::GetUInt32(buffer[offset + OFFSET_8], buffer[offset + OFFSET_9],
        buffer[offset + OFFSET_10], buffer[offset + OFFSET_11]);
    CHECK_AND_RETURN_RET_LOG(context_ != nullptr, MTP_ERROR_CONTEXT_IS_NULL,
        "Parser error: MTP_ERROR_CONTEXT_IS_NULL");

    context_->operationCode = code_;
    context_->transactionID = transactionID_;
    HeaderData::sTransactionID_ = transactionID_;
    MEDIA_DEBUG_LOG("Parser operationCode 0x%{public}x, transactionID %{public}d, containerType %{public}d",
        context_->operationCode, context_->transactionID, containerType_);
    if (containerType_ == CONTAINER_TYPE_2) {
        context_->indata = true;
    } else {
        context_->indata = false;
    }
    return MTP_SUCCESS;
}

int HeaderData::Maker(std::vector<uint8_t> &outBuffer)
{
    MtpPacketTool::PutUInt32(outBuffer, containerLength_);
    MtpPacketTool::PutUInt16(outBuffer, containerType_);
    MtpPacketTool::PutUInt16(outBuffer, code_);
    MtpPacketTool::PutUInt32(outBuffer, transactionID_);
    return MTP_SUCCESS;
}

uint16_t HeaderData::GetCode() const
{
    return code_;
}

void HeaderData::SetCode(uint16_t code)
{
    code_ = code;
}

uint32_t HeaderData::GetContainerLength() const
{
    return containerLength_;
}

void HeaderData::SetContainerLength(uint32_t containerLength)
{
    containerLength_ = containerLength;
}

uint16_t HeaderData::GetContainerType() const
{
    return containerType_;
}

void HeaderData::SetContainerType(uint16_t containerType)
{
    containerType_ = containerType;
}

uint32_t HeaderData::GetTransactionId() const
{
    return transactionID_;
}

void HeaderData::SetTransactionId(uint32_t transactionId)
{
    transactionID_ = transactionId;
}

void HeaderData::Reset()
{
    containerLength_ = 0;
    containerType_ = 0;
    code_ = 0;
    transactionID_ = 0;
}
} // namespace Media
} // namespace OHOS