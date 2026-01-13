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
#include "payload_data/get_object_prop_value_data.h"
#include <cinttypes>
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 2;

GetObjectPropValueData::GetObjectPropValueData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectPropValueData::~GetObjectPropValueData()
{
}

int GetObjectPropValueData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("GetObjectPropValueData::parser null or storage");
        return MTP_FAIL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectPropValueData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectPropValueData::Maker(std::vector<uint8_t> &outBuffer)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("GetObjectPropValueData::maker null or storage");
        return MTP_FAIL;
    }

    if (!hasSetPropValue_) {
        MEDIA_ERR_LOG("GetObjectPropValueData::maker set");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }

    if ((type_ == MTP_TYPE_INT8_CODE) || (type_ == MTP_TYPE_UINT8_CODE)) {
        MtpPacketTool::PutUInt8(outBuffer, int64Value_);
    } else if ((type_ == MTP_TYPE_INT16_CODE) || (type_ == MTP_TYPE_UINT16_CODE)) {
        MtpPacketTool::PutUInt16(outBuffer, int64Value_);
    } else if ((type_ == MTP_TYPE_INT32_CODE) || (type_ == MTP_TYPE_UINT32_CODE)) {
        MtpPacketTool::PutUInt32(outBuffer, int64Value_);
    } else if ((type_ == MTP_TYPE_INT64_CODE) || (type_ == MTP_TYPE_UINT64_CODE)) {
        MtpPacketTool::PutUInt64(outBuffer, int64Value_);
    } else if ((type_ == MTP_TYPE_INT128_CODE) || (type_ == MTP_TYPE_UINT128_CODE)) {
        MtpPacketTool::PutUInt128(outBuffer, int128Value_);
    } else if (type_ == MTP_TYPE_STRING_CODE) {
        MtpPacketTool::PutString(outBuffer, strValue_);
    } else {
        MEDIA_ERR_LOG("GetObjectPropValueData::maker unsupported type");
        return MTP_INVALID_OBJECTPROP_FORMAT_CODE;
    }

    return MTP_SUCCESS;
}

uint32_t GetObjectPropValueData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool GetObjectPropValueData::SetPropValue(int type, uint64_t int64Value, const uint128_t int128Value,
    const std::string &strValue)
{
    if (hasSetPropValue_) {
        return false;
    }
    hasSetPropValue_ = true;
    type_ = type;
    int64Value_ = int64Value;
    if (int128Value != nullptr) {
        int128Value_[OFFSET_0] = int128Value[OFFSET_0];
        int128Value_[OFFSET_1] = int128Value[OFFSET_1];
        int128Value_[OFFSET_2] = int128Value[OFFSET_2];
        int128Value_[OFFSET_3] = int128Value[OFFSET_3];
    }
    strValue_ = strValue;
    return true;
}
} // namespace Media
} // namespace OHOS