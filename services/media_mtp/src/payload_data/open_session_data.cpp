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

#include "payload_data/open_session_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_constants.h"
#include "mtp_packet_tools.h"
using namespace std;

namespace OHOS {
namespace Media {
OpenSessionData::OpenSessionData(shared_ptr<MtpOperationContext> &context)
    :PayloadData(context)
{
}

OpenSessionData::~OpenSessionData()
{
}

int OpenSessionData::Parser(const vector<uint8_t> &buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        return MTP_FAIL;
    }
    if ((readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE < 1) { // param num < 1
        return MTP_FAIL;
    }
    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->tempSessionID = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int OpenSessionData::Maker(vector<uint8_t> &outBuffer)
{
    if (sessionID_ > 0) {
        MtpPacketTool::PutUInt32(outBuffer, sessionID_);
    }
    return MTP_SUCCESS;
}

uint32_t OpenSessionData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    if (sessionID_ > 0) {
        MtpPacketTool::PutUInt32(tmpVar, sessionID_);
    }
    return tmpVar.size();
}

void OpenSessionData::SetSessionId(uint32_t sessionID)
{
    sessionID_ = sessionID;
}
} // namespace Media
} // namespace OHOS