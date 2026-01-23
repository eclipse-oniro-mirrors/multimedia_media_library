/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Cloud_Vo"

#include "on_modify_records_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool OnModifyRecord::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadString(this->path);
    parcel.ReadString(this->fileName);
    parcel.ReadInt32(this->fileId);
    parcel.ReadInt64(this->modifyTime);
    parcel.ReadInt64(this->metaDateModified);
    parcel.ReadInt64(this->version);
    parcel.ReadBool(this->isSuccess);
    int32_t copyRecordErrorType;
    parcel.ReadInt32(copyRecordErrorType);
    this->errorType = static_cast<ErrorType>(copyRecordErrorType);
    parcel.ReadInt32(this->serverErrorCode);
    IPC::ITypeMediaUtil::UnmarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    return true;
}

bool OnModifyRecord::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteString(this->path);
    parcel.WriteString(this->fileName);
    parcel.WriteInt32(this->fileId);
    parcel.WriteInt64(this->modifyTime);
    parcel.WriteInt64(this->metaDateModified);
    parcel.WriteInt64(this->version);
    parcel.WriteBool(this->isSuccess);
    parcel.WriteInt32(static_cast<int32_t>(this->errorType));
    parcel.WriteInt32(this->serverErrorCode);
    IPC::ITypeMediaUtil::MarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    return true;
}

std::string OnModifyRecord::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"fileId\": \"" << fileId << "\","
       << "\"modifyTime\": \"" << modifyTime << "\","
       << "\"metaDateModified\": \"" << metaDateModified << "\","
       << "\"path\": \"" << path << "\","
       << "\"isSuccess\": \"" << std::to_string(isSuccess) << "\","
       << "\"errorType\": \"" << static_cast<int32_t>(errorType) << "\","
       << "\"serverErrorCode\": " << serverErrorCode << ","
       << "[";
    for (uint32_t i = 0; i < errorDetails.size(); ++i) {
        ss << "{\"reason\": " << errorDetails[i].reason << "\","
           << "\"errorCode\": " << errorDetails[i].errorCode << "\","
           << "\"description\": " << errorDetails[i].description << "\","
           << "\"errorPos\": " << errorDetails[i].errorPos << "\","
           << "\"errorParam\": " << errorDetails[i].errorParam << "\","
           << "\"detailCode\": " << errorDetails[i].detailCode << "\""
           << "}";
        if (i != errorDetails.size() - 1) {
            ss << ",";
        }
    }
    ss << "]}";
    return ss.str();
}

bool OnModifyRecordsPhotosReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable(this->records, parcel);
}

bool OnModifyRecordsPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::MarshallingParcelable(this->records, parcel);
    return true;
}

int32_t OnModifyRecordsPhotosReqBody::AddModifyRecord(const OnModifyRecord &record)
{
    this->records.push_back(record);
    return E_OK;
}

std::vector<OnModifyRecord> OnModifyRecordsPhotosReqBody::GetModifyRecords()
{
    return records;
}

std::string OnModifyRecordsPhotosReqBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync