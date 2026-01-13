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
#include "payload_data/get_object_references_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 1;

GetObjectReferencesData::GetObjectReferencesData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectReferencesData::~GetObjectReferencesData()
{
}

int GetObjectReferencesData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("GetObjectReferencesData::parser context_ is null");
        return MTP_ERROR_SESSION_NOT_OPEN;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectReferencesData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_ERROR_PACKET_INCORRECT;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectReferencesData::Maker(std::vector<uint8_t> &outBuffer)
{
    if ((context_ == nullptr) || (!context_->sessionOpen)) {
        MEDIA_ERR_LOG("GetObjectReferencesData::maker null or session");
        return MTP_SESSION_NOT_OPEN_CODE;
    }

    if (!MtpStorageManager::GetInstance()->HasStorage()) {
        MEDIA_ERR_LOG("GetObjectReferencesData::maker storage");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }

    auto handles = GetObjectHandles();
    if (handles == nullptr) {
        MtpPacketTool::PutUInt32(outBuffer, 0);
    } else  {
        MtpPacketTool::PutAUInt32(outBuffer, handles->data(), handles->size());
    }
    return MTP_SUCCESS;
}

uint32_t GetObjectReferencesData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool GetObjectReferencesData::SetObjectHandles(std::shared_ptr<UInt32List> &objectHandles)
{
    if (hasSetObjectHandles_) {
        return false;
    }
    hasSetObjectHandles_ = true;
    objectHandles_ = objectHandles;
    return true;
}

std::shared_ptr<UInt32List> GetObjectReferencesData::GetObjectHandles()
{
    if (!hasSetObjectHandles_) {
        return nullptr;
    }
    return objectHandles_;
}
} // namespace Media
} // namespace OHOS