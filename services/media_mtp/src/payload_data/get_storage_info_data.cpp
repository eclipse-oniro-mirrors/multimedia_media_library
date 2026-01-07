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
#include "payload_data/get_storage_info_data.h"
#include "media_log.h"
#include "media_mtp_utils.h"
#include "mtp_packet_tools.h"
using namespace std;
namespace OHOS {
namespace Media {
GetStorageInfoData::GetStorageInfoData(shared_ptr<MtpOperationContext>& context)
    : PayloadData(context)
{
}

GetStorageInfoData::GetStorageInfoData()
{
}

GetStorageInfoData::~GetStorageInfoData()
{
}

int GetStorageInfoData::Parser(const std::vector<uint8_t>& buffer, int32_t readSize)
{
    if (context_ == nullptr) {
        return MTP_ERROR_CONTEXT_IS_NULL;
    }
    if ((readSize - MTP_CONTAINER_HEADER_SIZE) / MTP_PARAMETER_SIZE < 1) { // param num < 1
        return MTP_ERROR_PACKET_INCORRECT;
    }
    size_t offset = MTP_CONTAINER_HEADER_SIZE;
    context_->storageInfoID = MtpPacketTool::GetUInt32(buffer, offset);
    return MTP_SUCCESS;
}

int GetStorageInfoData::Maker(std::vector<uint8_t>& outBuffer)
{
    if (storage_ != nullptr) {
        MEDIA_INFO_LOG("storageID_ = %{public}d", storage_->GetStorageID());
        MtpPacketTool::PutUInt16(outBuffer, storage_->GetStorageType());
        MtpPacketTool::PutUInt16(outBuffer, storage_->GetFilesystemType());
        MtpPacketTool::PutUInt16(outBuffer, storage_->GetAccessCapability());
        MtpPacketTool::PutUInt64(outBuffer, storage_->GetMaxCapacity());
        MtpPacketTool::PutUInt64(outBuffer, storage_->GetFreeSpaceInBytes());
        MtpPacketTool::PutUInt32(outBuffer, storage_->GetFreeSpaceInObjects());
        MtpPacketTool::PutString(outBuffer, storage_->GetStorageDescription());
        MtpPacketTool::PutString(outBuffer, storage_->GetVolumeIdentifier());
    }
    return MTP_SUCCESS;
}

uint32_t GetStorageInfoData::CalculateSize()
{
    std::vector<uint8_t> tmpVar;
    int res = Maker(tmpVar);
    if (res != MTP_SUCCESS) {
        return res;
    }
    return tmpVar.size();
}

void GetStorageInfoData::SetStorage(const std::shared_ptr<Storage> &storage)
{
    storage_ = storage;
}
} // namespace Media
} // namespace OHOS