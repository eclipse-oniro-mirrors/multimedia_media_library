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
#include "payload_data/send_object_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "mtp_storage_manager.h"
using namespace std;
namespace OHOS {
namespace Media {
SendObjectData::SendObjectData(std::shared_ptr<MtpOperationContext> &context)
    : PayloadData(context)
{
}

SendObjectData::~SendObjectData()
{
}

int SendObjectData::Parser(const std::vector<uint8_t> &buffer, int32_t readSize)
{
    if ((context_ == nullptr) || (!MtpStorageManager::GetInstance()->HasStorage())) {
        MEDIA_ERR_LOG("SendObjectData::parser null or storage");
        return MTP_INVALID_OBJECTHANDLE_CODE;
    }
    return MTP_SUCCESS;
}

int SendObjectData::Maker(std::vector<uint8_t> &outBuffer)
{
    return MTP_SUCCESS;
}

uint32_t SendObjectData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}
} // namespace Media
} // namespace OHOS