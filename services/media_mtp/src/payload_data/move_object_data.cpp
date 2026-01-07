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
#include "payload_data/move_object_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 3;

MoveObjectData::MoveObjectData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

MoveObjectData::~MoveObjectData()
{
}

int MoveObjectData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("MoveObjectData::parser null or storage");
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("MoveObjectData::parser paramCount=%{public}u, needCount=%{public}u",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_ERROR_PACKET_INCORRECT;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    context_->storageID = MtpPacketTool::GetUInt32(buffer, offset);
    context_->parent = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int MoveObjectData::Maker(std::vector<uint8_t> &outBuffer)
{
    return MTP_SUCCESS;
}

uint32_t MoveObjectData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}
} // namespace Media
} // namespace OHOS