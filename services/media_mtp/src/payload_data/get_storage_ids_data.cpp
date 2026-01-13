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
#include "payload_data/get_storage_ids_data.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
#include "media_log.h"
using namespace std;

namespace OHOS {
namespace Media {
GetStorageIdsData::GetStorageIdsData(shared_ptr<MtpOperationContext> &context)
    :PayloadData(context)
{
}

GetStorageIdsData::GetStorageIdsData()
{
}

GetStorageIdsData::~GetStorageIdsData()
{
}

int GetStorageIdsData::Parser(const vector<uint8_t> &buffer, int32_t readSize)
{
    return MTP_SUCCESS;
}

int GetStorageIdsData::Maker(vector<uint8_t> &outBuffer)
{
    MtpPacketTool::PutUInt32(outBuffer, storages_.size());
    for (auto storage : storages_) {
        MtpPacketTool::PutUInt32(outBuffer, storage->GetStorageID());
    }
    return MTP_SUCCESS;
}

uint32_t GetStorageIdsData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }

    return tmpVar.size();
}

void GetStorageIdsData::SetStorages(const std::vector<std::shared_ptr<Storage>> &storages)
{
    storages_ = storages;
}
} // namespace Media
} // namespace OHOS