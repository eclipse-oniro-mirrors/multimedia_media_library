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
#include "payload_data/get_object_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 1;

GetObjectData::GetObjectData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectData::GetObjectData()
{
}

GetObjectData::~GetObjectData()
{
}

int GetObjectData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("GetObjectData::parser null or storage");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    context_->offset = 0;
    context_->length = 0;
    return MTP_SUCCESS;
}

int GetObjectData::Maker(std::vector<uint8_t> &outBuffer)
{
    return MTP_SUCCESS;
}

uint32_t GetObjectData::CalculateSize()
{
    return 0;
}

bool GetObjectData::SetResult(uint32_t result)
{
    if (hasSetResult_) {
        return false;
    }

    hasSetResult_ = true;
    result_ = result;
    return true;
}
} // namespace Media
} // namespace OHOS