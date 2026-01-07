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
#include "payload_data/get_object_info_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 1;

GetObjectInfoData::GetObjectInfoData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectInfoData::GetObjectInfoData()
{
}

GetObjectInfoData::~GetObjectInfoData()
{
}

int GetObjectInfoData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("GetObjectInfoData::parser null");
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectInfoData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_ERROR_PACKET_INCORRECT;
    }
    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectInfoData::Maker(std::vector<uint8_t> &outBuffer)
{
    auto objectInfo = GetObjectInfo();
    if (objectInfo == nullptr) {
        MEDIA_ERR_LOG("GetObjectInfoData::maker object info");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }

    MtpPacketTool::PutUInt32(outBuffer, objectInfo->storageID);
    MtpPacketTool::PutUInt16(outBuffer, objectInfo->format);
    MtpPacketTool::PutUInt16(outBuffer, objectInfo->protectionStatus);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->size);
    MtpPacketTool::PutUInt16(outBuffer, objectInfo->thumbFormat);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->thumbCompressedSize);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->thumbPixelWidth);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->thumbPixelHeight);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->imagePixelWidth);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->imagePixelHeight);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->imagePixelDepth);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->parent);
    MtpPacketTool::PutUInt16(outBuffer, objectInfo->associationType);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->associationDesc);
    MtpPacketTool::PutUInt32(outBuffer, objectInfo->sequenceNumber);
    MtpPacketTool::PutString(outBuffer, objectInfo->name);
    MtpPacketTool::PutString(outBuffer, MtpPacketTool::FormatDateTime(objectInfo->dateCreated));
    MtpPacketTool::PutString(outBuffer, MtpPacketTool::FormatDateTime(objectInfo->dateModified));
    MtpPacketTool::PutUInt8(outBuffer, 0);
    return MTP_SUCCESS;
}

uint32_t GetObjectInfoData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool GetObjectInfoData::SetObjectInfo(std::shared_ptr<ObjectInfo> &objectInfo)
{
    if (hasSetObjectInfo_) {
        return false;
    }
    hasSetObjectInfo_ = true;
    objectInfo_ = objectInfo;
    return true;
}

std::shared_ptr<ObjectInfo> GetObjectInfoData::GetObjectInfo()
{
    if (!hasSetObjectInfo_) {
        return nullptr;
    }
    return objectInfo_;
}
} // namespace Media
} // namespace OHOS