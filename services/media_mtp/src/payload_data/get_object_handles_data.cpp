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
#include "payload_data/get_object_handles_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 3;

GetObjectHandlesData::GetObjectHandlesData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectHandlesData::GetObjectHandlesData()
{
}

GetObjectHandlesData::~GetObjectHandlesData()
{
}

int GetObjectHandlesData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("GetObjectHandlesData::parser context_ == nullptr");
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectHandlesData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_ERROR_PACKET_INCORRECT;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->storageID = MtpPacketTool::GetUInt32(buffer, offset);
    context_->format = MtpPacketTool::GetUInt32(buffer, offset);
    context_->parent = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectHandlesData::Maker(std::vector<uint8_t> &outBuffer)
{
    auto handles = GetObjectHandles();
    if (handles == nullptr) {
        MEDIA_ERR_LOG("GetObjectHandlesData::maker handles");
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }

    MtpPacketTool::PutAUInt32(outBuffer, handles->data(), handles->size());
    return MTP_SUCCESS;
}

uint32_t GetObjectHandlesData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }

    return tmpVar.size();
}

bool GetObjectHandlesData::SetObjectHandles(std::shared_ptr<UInt32List> &objectHandles)
{
    if (hasSetObjectHandles_) {
        return false;
    }

    hasSetObjectHandles_ = true;
    objectHandles_ = objectHandles;
    return true;
}

std::shared_ptr<UInt32List> GetObjectHandlesData::GetObjectHandles()
{
    if (!hasSetObjectHandles_) {
        return nullptr;
    }

    return objectHandles_;
}
} // namespace Media
} // namespace OHOS