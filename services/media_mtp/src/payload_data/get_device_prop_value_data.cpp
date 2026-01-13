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
#include "payload_data/get_device_prop_value_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_operation_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
GetDevicePropValueData::GetDevicePropValueData(std::shared_ptr<MtpOperationContext> &context)
    :PayloadData(context)
{
}

GetDevicePropValueData::GetDevicePropValueData()
{
}

GetDevicePropValueData::~GetDevicePropValueData()
{
}

int GetDevicePropValueData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetDevicePropValueData::Maker(std::vector<uint8_t> &outBuffer)
{
    if (value_ == nullptr) {
        MEDIA_ERR_LOG("value is NULL");
        return MTP_ERROR_DEVICEPROP_NOT_SUPPORTED;
    }
    return WriteValue(outBuffer, type_, *value_);
}

uint32_t GetDevicePropValueData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;

    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return static_cast<uint32_t>(res);
    }

    uint32_t size = tmpVar.size();
    return size;
}

void GetDevicePropValueData::SetValue(uint16_t type, std::shared_ptr<Property::Value> &value)
{
    type_ = type;
    value_ = value;
}

int GetDevicePropValueData::WriteValue(std::vector<uint8_t> &buffer, uint16_t type, const Property::Value &value)
{
    MEDIA_INFO_LOG("WriteValue   value type %{public}d", type);
    switch (type) {
        case MTP_TYPE_INT8_CODE:
        case MTP_TYPE_AINT8_CODE:
            MtpPacketTool::PutUInt8(buffer, static_cast<uint8_t>(value.bin_.i8));
            break;
        case MTP_TYPE_UINT8_CODE:
        case MTP_TYPE_AUINT8_CODE:
            MtpPacketTool::PutUInt8(buffer, value.bin_.ui8);
            break;
        case MTP_TYPE_INT16_CODE:
        case MTP_TYPE_AINT16_CODE:
            MtpPacketTool::PutUInt16(buffer, static_cast<uint16_t>(value.bin_.i16));
            break;
        case MTP_TYPE_UINT16_CODE:
        case MTP_TYPE_AUINT16_CODE:
            MtpPacketTool::PutUInt16(buffer, value.bin_.ui16);
            break;
        case MTP_TYPE_INT32_CODE:
        case MTP_TYPE_AINT32_CODE:
            MtpPacketTool::PutUInt32(buffer, static_cast<uint32_t>(value.bin_.i32));
            break;
        case MTP_TYPE_UINT32_CODE:
        case MTP_TYPE_AUINT32_CODE:
            MtpPacketTool::PutUInt32(buffer, value.bin_.ui32);
            break;
        case MTP_TYPE_STRING_CODE:
            MEDIA_INFO_LOG("value type MTP_TYPE_STRING_CODE");
            if (value.str_ == nullptr) {
                MtpPacketTool::PutUInt8(buffer, 0);
            } else {
                MtpPacketTool::PutString(buffer, *(value.str_));
            }
            break;
        default: {
            MEDIA_ERR_LOG("value type not find");
            return MTP_ERROR_DEVICEPROP_NOT_SUPPORTED;
        }
    }
    return MTP_SUCCESS;
}
} // namespace Media
} // namespace OHOS