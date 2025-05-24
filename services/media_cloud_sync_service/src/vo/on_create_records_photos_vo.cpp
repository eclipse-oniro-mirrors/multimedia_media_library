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

#include "on_create_records_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {

bool OnCreateRecord::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadInt32(this->fileId);
    parcel.ReadInt32(this->localId);
    parcel.ReadInt32(this->rotation);
    parcel.ReadInt32(this->fileType);
    parcel.ReadInt64(this->size);
    parcel.ReadInt64(this->createTime);
    parcel.ReadInt64(this->modifiedTime);
    parcel.ReadInt64(this->editedTimeMs);
    parcel.ReadInt64(this->metaDateModified);
    parcel.ReadString(this->path);
    parcel.ReadString(this->fileName);
    parcel.ReadString(this->sourcePath);
    parcel.ReadString(this->livePhotoCachePath);
    parcel.ReadInt64(this->version);
    parcel.ReadInt32(this->serverErrorCode);
    parcel.ReadBool(this->isSuccess);
    IPC::ITypeMediaUtil::UnmarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    int32_t copyRecordErrorType;
    parcel.ReadInt32(copyRecordErrorType);
    this->errorType = static_cast<ErrorType>(copyRecordErrorType);
    return true;
}

bool OnCreateRecord::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteInt32(this->fileId);
    parcel.WriteInt32(this->localId);
    parcel.WriteInt32(this->rotation);
    parcel.WriteInt32(this->fileType);
    parcel.WriteInt64(this->size);
    parcel.WriteInt64(this->createTime);
    parcel.WriteInt64(this->modifiedTime);
    parcel.WriteInt64(this->editedTimeMs);
    parcel.WriteInt64(this->metaDateModified);
    parcel.WriteString(this->path);
    parcel.WriteString(this->fileName);
    parcel.WriteString(this->sourcePath);
    parcel.WriteString(this->livePhotoCachePath);
    parcel.WriteInt64(this->version);
    parcel.WriteInt32(this->serverErrorCode);
    parcel.WriteBool(this->isSuccess);
    IPC::ITypeMediaUtil::MarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    parcel.WriteInt32(static_cast<int32_t>(this->errorType));
    return true;
}

std::string OnCreateRecord::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"fileId\": " << fileId << ","
       << "\"localId\": \"" << localId << "\","
       << "\"rotation\": " << rotation << ","
       << "\"fileType\": " << fileType << ","
       << "\"size\": \"" << size << ","
       << "\"createTime\": " << createTime << ","
       << "\"modifiedTime\": " << modifiedTime << ","
       << "\"editedTimeMs\": " << editedTimeMs << ","
       << "\"metaDateModified\": " << metaDateModified << ","
       << "\"version\": " << version << ","
       << "\"serverErrorCode\": " << serverErrorCode << ","
       << "\"path\": \"" << path << "\","
       << "\"fileName\": \"" << fileName << "\","
       << "\"sourcePath\": \"" << sourcePath << "\","
       << "\"isSuccess\": \"" << isSuccess << "\","
       << "\"livePhotoCachePath\": \"" << livePhotoCachePath << "\","
       << "\"errorType\": \"" << static_cast<int32_t>(errorType) << "\","
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

bool OnCreateRecordsPhotosReqBody::Unmarshalling(MessageParcel &parcel)
{
    IPC::ITypeMediaUtil::UnmarshallingParcelable(this->records, parcel);
    return true;
}

bool OnCreateRecordsPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::MarshallingParcelable(this->records, parcel);
    return true;
}

int32_t OnCreateRecordsPhotosReqBody::AddRecord(const OnCreateRecord &record)
{
    this->records.push_back(record);
    return E_OK;
}

std::string OnCreateRecordsPhotosReqBody::ToString() const
{
    std::stringstream ss;
    ss << "[";
    for (uint32_t i = 0; i < records.size(); ++i) {
        ss << records[i].ToString();
        if (i != records.size() - 1) {
            ss << ",";
        }
    }
    ss << "]";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync