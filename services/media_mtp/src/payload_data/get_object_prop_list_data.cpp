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
#include "payload_data/get_object_prop_list_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 5;

GetObjectPropListData::GetObjectPropListData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

GetObjectPropListData::~GetObjectPropListData()
{
}

int GetObjectPropListData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("GetObjectPropListData::parser null or storage");
        return MTP_FAIL;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("GetObjectPropListData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_INVALID_PARAMETER_CODE;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;

    context_->handle = MtpPacketTool::GetUInt32(buffer, offset);
    context_->format = MtpPacketTool::GetUInt32(buffer, offset);
    context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    context_->groupCode = MtpPacketTool::GetUInt32(buffer, offset);
    context_->depth = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetObjectPropListData::Maker(std::vector<uint8_t> &outBuffer)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("GetObjectPropListData::maker null or storage");
        return MTP_FAIL;
    }
    if (!hasSetProps_) {
        MEDIA_ERR_LOG("GetObjectPropListData::maker set");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    size_t count = (props_ == nullptr) ? 0 : props_->size();

    MtpPacketTool::PutUInt32(outBuffer, count);
    for (size_t i = 0; i < count; i++) {
        Property &prop = (*props_)[i];
        WriteProperty(outBuffer, prop);
    }

    return MTP_SUCCESS;
}

uint32_t GetObjectPropListData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

bool GetObjectPropListData::SetProps(std::shared_ptr<std::vector<Property>> &props)
{
    if (hasSetProps_) {
        return false;
    }
    hasSetProps_ = true;
    props_ = props;
    return true;
}

void GetObjectPropListData::WriteProperty(std::vector<uint8_t> &outBuffer, const Property &prop)
{
    MtpPacketTool::PutUInt32(outBuffer, prop.handle_);
    MtpPacketTool::PutUInt16(outBuffer, prop.code_);
    MtpPacketTool::PutUInt16(outBuffer, prop.type_);

    if (prop.currentValue == nullptr) {
        MEDIA_ERR_LOG("GetObjectPropListData::WriteProperty bad value");
        return;
    }

    if (prop.type_ == MTP_TYPE_STRING_CODE) {
        WritePropertyStrValue(outBuffer, prop);
        return;
    }

    WritePropertyIntValue(outBuffer, prop);
}

void GetObjectPropListData::WritePropertyStrValue(std::vector<uint8_t> &outBuffer, const Property &prop)
{
    auto &value = prop.currentValue;
    if (prop.type_ == MTP_TYPE_STRING_CODE) {
        if (value->str_ == nullptr) {
            MtpPacketTool::PutUInt8(outBuffer, 0);
        } else {
            MtpPacketTool::PutString(outBuffer, *(value->str_));
        }
    }
}

void GetObjectPropListData::WritePropertyIntValue(std::vector<uint8_t> &outBuffer, const Property &prop)
{
    auto &value = prop.currentValue;
    switch (prop.type_) {
        case MTP_TYPE_INT8_CODE:
            MtpPacketTool::PutInt8(outBuffer, value->bin_.i8);
            break;
        case MTP_TYPE_UINT8_CODE:
            MtpPacketTool::PutUInt8(outBuffer, value->bin_.ui8);
            break;
        case MTP_TYPE_INT16_CODE:
            MtpPacketTool::PutInt16(outBuffer, value->bin_.i16);
            break;
        case MTP_TYPE_UINT16_CODE:
            MtpPacketTool::PutUInt16(outBuffer, value->bin_.ui16);
            break;
        case MTP_TYPE_INT32_CODE:
            MtpPacketTool::PutInt32(outBuffer, value->bin_.i32);
            break;
        case MTP_TYPE_UINT32_CODE:
            MtpPacketTool::PutUInt32(outBuffer, value->bin_.ui32);
            break;
        case MTP_TYPE_INT64_CODE:
            MtpPacketTool::PutInt64(outBuffer, value->bin_.i64);
            break;
        case MTP_TYPE_UINT64_CODE:
            MtpPacketTool::PutUInt64(outBuffer, value->bin_.ui64);
            break;
        case MTP_TYPE_INT128_CODE:
            MtpPacketTool::PutInt128(outBuffer, value->bin_.i128);
            break;
        case MTP_TYPE_UINT128_CODE:
            MtpPacketTool::PutUInt128(outBuffer, value->bin_.ui128);
            break;
        default:
            MEDIA_ERR_LOG("GetObjectPropListData::writeProperty bad or unsupported data type %{public}u", prop.type_);
            break;
    }
}
} // namespace Media
} // namespace OHOS