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
#include "payload_data/get_device_prop_desc_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_operation_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
GetDevicePropDescData::GetDevicePropDescData(std::shared_ptr<MtpOperationContext> &context)
    :PayloadData(context)
{
}

GetDevicePropDescData::GetDevicePropDescData()
{
}

GetDevicePropDescData::~GetDevicePropDescData()
{
}

int GetDevicePropDescData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        return MTP_ERROR_CONTEXT_IS_NULL;
    }

    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->property = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetDevicePropDescData::Maker(std::vector<uint8_t> &outBuffer)
{
    if (result_ == nullptr) {
        MEDIA_ERR_LOG("property id NULL");
        return MTP_ERROR_DEVICEPROP_NOT_SUPPORTED;
    }
    result_->Write(outBuffer);

    return MTP_SUCCESS;
}

uint32_t GetDevicePropDescData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;

    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }

    uint32_t size = tmpVar.size();
    return size;
}

void GetDevicePropDescData::SetProperty(std::shared_ptr<Property> &property)
{
    result_ = property;
}
} // namespace Media
} // namespace OHOS