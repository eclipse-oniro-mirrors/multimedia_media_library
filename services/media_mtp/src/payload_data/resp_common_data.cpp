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

#include "payload_data/resp_common_data.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"
#include "mtp_packet_tools.h"
using namespace std;

namespace OHOS {
namespace Media {
const int DATA_SIZE = 4;
// these numbers are defined by protocol, have no exact meaning
const int PARAM_INDEX_1 = 1;
const int PARAM_INDEX_2 = 2;
const int PARAM_INDEX_3 = 3;
const int PARAM_INDEX_4 = 4;
const int PARAM_INDEX_5 = 5;
RespCommonData::RespCommonData(shared_ptr<MtpOperationContext> &context)
    :PayloadData(context)
{
}

RespCommonData::RespCommonData()
{
}

RespCommonData::~RespCommonData()
{
}

int RespCommonData::Parser(const vector<uint8_t> &buffer, int32_t readSize)
{
    return MTP_SUCCESS;
}

int RespCommonData::Maker(vector<uint8_t> &outBuffer)
{
    if (param1_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, param1_);
    } else {
        return MTP_SUCCESS;
    }

    if (param2_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, param2_);
    } else {
        return MTP_SUCCESS;
    }

    if (param3_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, param3_);
    } else {
        return MTP_SUCCESS;
    }

    if (param4_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, param4_);
    } else {
        return MTP_SUCCESS;
    }

    if (param5_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, param5_);
    } else {
        return MTP_SUCCESS;
    }
        
    return MTP_SUCCESS;
}

uint32_t RespCommonData::CalculateSize()
{
    int dataSize = 0;
    if (param1_ > 0) {
        dataSize += DATA_SIZE;
    }

    if (param2_ > 0) {
        dataSize += DATA_SIZE;
    }

    if (param3_ > 0) {
        dataSize += DATA_SIZE;
    }

    if (param4_ > 0) {
        dataSize += DATA_SIZE;
    }

    if (param5_ > 0) {
        dataSize += DATA_SIZE;
    }
    return dataSize;
}

void RespCommonData::SetParam(int paramIndex, uint32_t value)
{
    switch (paramIndex) {
        case PARAM_INDEX_1:
            param1_ = value;
            break;
        case PARAM_INDEX_2:
            param2_ = value;
            break;
        case PARAM_INDEX_3:
            param3_ = value;
            break;
        case PARAM_INDEX_4:
            param4_ = value;
            break;
        case PARAM_INDEX_5:
            param5_ = value;
            break;
        default:
            break;
    }
}
} // namespace Media
} // namespace OHOS