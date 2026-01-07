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
#include "payload_data/get_num_objects_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 3;

GetNumObjectsData::GetNumObjectsData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetNumObjectsData::GetNumObjectsData()
{
}

GetNumObjectsData::~GetNumObjectsData()
{
}

int GetNumObjectsData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("context_ is null");
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("paramCount=%{public}u, needCount=%{public}d", parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->storageID = MtpPacketTool::GetUInt32(buffer, offset);
    context_->format = MtpPacketTool::GetUInt32(buffer, offset);
    context_->parent = MtpPacketTool::GetUInt32(buffer, offset);

    if (!MtpStorageManager::GetInstance()->HasStorage(context_->storageID)) {
        MEDIA_ERR_LOG("no match storage");
        return MTP_ERROR_INVALID_STORAGE_ID;
    }
    return MTP_SUCCESS;
}

int GetNumObjectsData::Maker(std::vector<uint8_t> &outBuffer)
{
    auto count = GetNum();
    if (count >= 0) {
        MtpPacketTool::PutUInt32(outBuffer, count);
        return MTP_OK_CODE;
    } else {
        MtpPacketTool::PutUInt32(outBuffer, 0);
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    return MTP_SUCCESS;
}

uint32_t GetNumObjectsData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;

    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }

    uint32_t size = tmpVar.size();
    return size;
}

bool GetNumObjectsData::SetNum(int num)
{
    if (hasSetNum_) {
        return false;
    }

    hasSetNum_ = true;
    num_ = num;
    return true;
}

int GetNumObjectsData::GetNum()
{
    if (!hasSetNum_) {
        return -1;
    }

    return num_;
}
} // namespace Media
} // namespace OHOS