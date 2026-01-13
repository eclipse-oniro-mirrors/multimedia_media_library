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
#define MLOG_TAG "MtpSetDevicePropValueData"
#include "payload_data/set_device_prop_value_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_operation_utils.h"
using namespace std;
namespace OHOS {
namespace Media {
static constexpr int32_t PARSER_PARAM_SUM = 1;

SetDevicePropValueData::SetDevicePropValueData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

SetDevicePropValueData::SetDevicePropValueData()
{
}

SetDevicePropValueData::~SetDevicePropValueData()
{
}

int SetDevicePropValueData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        MEDIA_ERR_LOG("SetDevicePropValueData::parser null");
        return MTP_ERROR_INVALID_OBJECTHANDLE;
    }

    int32_t parameterCount = (readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE;
    if (parameterCount < PARSER_PARAM_SUM) {
        MEDIA_ERR_LOG("SetDevicePropValueData::parser paramCount=%{public}u, needCount=%{public}d",
            parameterCount, PARSER_PARAM_SUM);
        return MTP_ERROR_PACKET_INCORRECT;
    }
    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    if (!context_->indata) {
        context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    } else {
        PaserPropValue(buffer, offset, context_->property);
    }
    return MTP_SUCCESS;
}

int SetDevicePropValueData::Maker(std::vector<uint8_t> &outBuffer)
{
    return MTP_SUCCESS;
}

uint32_t SetDevicePropValueData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    MtpPacketTool::PutInt16(tmpVar, MTP_OK_CODE);
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

void SetDevicePropValueData::PaserPropValue(const std::vector<uint8_t> &buffer, size_t &offset, uint32_t propertyCode)
{
    string value = MtpPacketTool::GetString(buffer, offset);

    switch (propertyCode) {
        case MTP_DEVICE_PROPERTY_DEVICE_FRIENDLY_NAME_CODE:
            if (!MtpOperationUtils::SetPropertyInner("const.product.name", value)) {
                MEDIA_ERR_LOG("PaserPropValue SetPropertyInner fail");
            }
            break;
        case MTP_DEVICE_PROPERTY_SYNCHRONIZATION_PARTNER_CODE:
            // This function will be completed later
            break;
        case MTP_DEVICE_PROPERTY_SESSION_INITIATOR_VERSION_INFO_CODE:
            MtpOperationUtils::SetIsDevicePropSet();
            break;
        default:
            MEDIA_INFO_LOG("property do not find");
            break;
    }
}
} // namespace Media
} // namespace OHOS