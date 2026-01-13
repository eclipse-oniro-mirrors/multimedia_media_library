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
#include "payload_data/set_object_prop_value_data.h"
#include <cinttypes>
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 2;

SetObjectPropValueData::SetObjectPropValueData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

SetObjectPropValueData::~SetObjectPropValueData()
{
}

int SetObjectPropValueData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("SetObjectPropValueData::parser null or storage");
        return MTP_FAIL;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    if (!context_->indata) {
        int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
        if (parameterCount < PARSER_PARAM_SUM) {
            MEDIA_ERR_LOG("SetObjectPropValueData::parser paramCount=%{public}u, needCount=%{public}d",
                parameterCount, PARSER_PARAM_SUM);
            return MTP_INVALID_PARAMETER_CODE;
        }
        context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
        context_->property = MtpPacketTool::GetUInt32(buffer, offset);
        auto properType = MtpPacketTool::GetObjectPropTypeByPropCode(context_->property);
        if (properType == MTP_TYPE_UNDEFINED_CODE) {
            MEDIA_ERR_LOG("SetObjectPropValueData::parser unsupported type");
            return MTP_OBJECTPROP_NOT_SUPPORTED_CODE;
        }
        context_->properType = properType;
    } else {
        if (context_->properType == MTP_TYPE_STRING_CODE) {
            if (!MtpPacketTool::GetString(buffer, offset, context_->properStrValue)) {
                MEDIA_ERR_LOG("SetObjectPropValueData::parser invalid object prop string format");
                return MTP_GENERAL_ERROR_CODE;
            }
        } else {
            if (!ReadIntValue(buffer, offset, context_->properType, context_->properIntValue)) {
                MEDIA_ERR_LOG("SetObjectPropValueData::parser invalid object prop format");
                return MTP_INVALID_OBJECTPROP_FORMAT_CODE;
            }
        }
    }
    return MTP_SUCCESS;
}

int SetObjectPropValueData::Maker(std::vector<uint8_t> &outBuffer)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("SetObjectPropValueData::maker null or storage");
        return MTP_FAIL;
    }

    if (!hasSetResult_) {
        MEDIA_ERR_LOG("SetObjectPropValueData::maker set");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    return MTP_SUCCESS;
}

uint32_t SetObjectPropValueData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool SetObjectPropValueData::SetResult(uint16_t result)
{
    if (hasSetResult_) {
        return false;
    }
    hasSetResult_ = true;
    result_ = result;
    return true;
}

bool SetObjectPropValueData::ReadIntValue(const std::vector<uint8_t> &buffer, size_t &offset, int type,
    int64_t& int64Value)
{
    if ((type == MTP_TYPE_INT8_CODE) || (type == MTP_TYPE_UINT8_CODE)) {
        if (!ReadInt8Value(buffer, offset, type, int64Value)) {
            return false;
        }
        return true;
    }
    if ((type == MTP_TYPE_INT16_CODE) || (type == MTP_TYPE_UINT16_CODE)) {
        if (!ReadInt16Value(buffer, offset, type, int64Value)) {
            return false;
        }
        return true;
    }
    if ((type == MTP_TYPE_INT32_CODE) || (type == MTP_TYPE_UINT32_CODE)) {
        if (!ReadInt32Value(buffer, offset, type, int64Value)) {
            return false;
        }
        return true;
    }
    if ((type == MTP_TYPE_INT64_CODE) || (type == MTP_TYPE_UINT64_CODE)) {
        if (!ReadInt64Value(buffer, offset, type, int64Value)) {
            return false;
        }
        return true;
    }

    MEDIA_ERR_LOG("SetObjectPropValueData::ReadIntValue unsupported type");
    return false;
}

bool SetObjectPropValueData::ReadInt8Value(const std::vector<uint8_t> &buffer, size_t &offset, int type,
    int64_t& int64Value)
{
    if (type == MTP_TYPE_INT8_CODE) {
        int8_t tmpVar = 0;
        if (!MtpPacketTool::GetInt8(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }

    if (type == MTP_TYPE_UINT8_CODE) {
        uint8_t tmpVar = 0;
        if (!MtpPacketTool::GetUInt8(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }
    return false;
}

bool SetObjectPropValueData::ReadInt16Value(const std::vector<uint8_t> &buffer, size_t &offset, int type,
    int64_t& int64Value)
{
    if (type == MTP_TYPE_INT16_CODE) {
        int16_t tmpVar = 0;
        if (!MtpPacketTool::GetInt16(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }

    if (type == MTP_TYPE_UINT16_CODE) {
        uint16_t tmpVar = 0;
        if (!MtpPacketTool::GetUInt16(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }
    return false;
}

bool SetObjectPropValueData::ReadInt32Value(const std::vector<uint8_t> &buffer, size_t &offset, int type,
    int64_t& int64Value)
{
    if (type == MTP_TYPE_INT32_CODE) {
        int32_t tmpVar = 0;
        if (!MtpPacketTool::GetInt32(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }

    if (type == MTP_TYPE_UINT32_CODE) {
        uint32_t tmpVar = 0;
        if (!MtpPacketTool::GetUInt32(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }
    return false;
}

bool SetObjectPropValueData::ReadInt64Value(const std::vector<uint8_t> &buffer, size_t &offset, int type,
    int64_t& int64Value)
{
    if (type == MTP_TYPE_INT64_CODE) {
        int64_t tmpVar = 0;
        if (!MtpPacketTool::GetInt64(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = tmpVar;
        return true;
    }

    if (type == MTP_TYPE_UINT64_CODE) {
        uint64_t tmpVar = 0;
        if (!MtpPacketTool::GetUInt64(buffer, offset, tmpVar)) {
            return false;
        }
        int64Value = static_cast<int64_t>(tmpVar);
        return true;
    }
    return false;
}
} // namespace Media
} // namespace OHOS