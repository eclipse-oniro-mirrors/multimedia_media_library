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

#include "on_modify_file_dirty_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool OnFileDirtyRecord::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->cloudId);
    parcel.ReadInt32(this->fileId);
    parcel.ReadInt32(this->rotation);
    parcel.ReadInt32(this->fileType);
    parcel.ReadInt64(this->size);
    parcel.ReadInt64(this->metaDateModified);
    parcel.ReadInt64(this->createTime);
    parcel.ReadInt64(this->modifyTime);
    parcel.ReadString(this->path);
    parcel.ReadString(this->fileName);
    parcel.ReadString(this->sourcePath);
    parcel.ReadInt64(this->version);
    parcel.ReadInt32(this->serverErrorCode);
    parcel.ReadBool(this->isSuccess);
    IPC::ITypeMediaUtil::UnmarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    int32_t copyRecordErrorType;
    parcel.ReadInt32(copyRecordErrorType);
    this->errorType = static_cast<ErrorType>(copyRecordErrorType);
    return true;
}

bool OnFileDirtyRecord::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->cloudId);
    parcel.WriteInt32(this->fileId);
    parcel.WriteInt32(this->rotation);
    parcel.WriteInt32(this->fileType);
    parcel.WriteInt64(this->size);
    parcel.WriteInt64(this->metaDateModified);
    parcel.WriteInt64(this->createTime);
    parcel.WriteInt64(this->modifyTime);
    parcel.WriteString(this->path);
    parcel.WriteString(this->fileName);
    parcel.WriteString(this->sourcePath);
    parcel.WriteInt64(this->version);
    parcel.WriteInt32(this->serverErrorCode);
    parcel.WriteBool(this->isSuccess);
    IPC::ITypeMediaUtil::MarshallingParcelable<CloudErrorDetail>(this->errorDetails, parcel);
    parcel.WriteInt32(static_cast<int32_t>(this->errorType));
    return true;
}

std::string OnFileDirtyRecord::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"cloudId\": \"" << cloudId << "\","
       << "\"fileId\": \"" << fileId << "\","
       << "\"rotation\": \"" << rotation << "\","
       << "\"fileType\": \"" << fileType << "\","
       << "\"size\": \"" << size << "\","
       << "\"path\": \"" << path << "\","
       << "\"createTime\": \"" << createTime << "\","
       << "\"modifyTime\": \"" << modifyTime << "\","
       << "\"version\": \"" << version << "\","
       << "\"serverErrorCode\": \"" << serverErrorCode << "\","
       << "\"isSuccess\": \"" << isSuccess << "\","
       << "\"errorType\": \"" << static_cast<int32_t>(errorType) << "\","
       << "\"errorDetails\": \"[";
    for (uint32_t i = 0; i < errorDetails.size(); ++i) {
        if (i != errorDetails.size() - 1) {
            ss << errorDetails[i].ToString() << ",";
            continue;
        }
        ss << errorDetails[i].ToString();
    }
    ss << "]}";
    return ss.str();
}

bool OnFileDirtyRecordsReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable(this->records, parcel);
}

bool OnFileDirtyRecordsReqBody::Marshalling(MessageParcel &parcel) const
{
    IPC::ITypeMediaUtil::MarshallingParcelable(this->records, parcel);
    return true;
}

int32_t OnFileDirtyRecordsReqBody::AddRecord(const OnFileDirtyRecord &record)
{
    this->records.push_back(record);
    return E_OK;
}

std::string OnFileDirtyRecordsReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{\"records\":[";
    for (uint32_t i = 0; i < records.size(); ++i) {
        if (i != records.size() - 1) {
            ss << records[i].ToString() << ",";
            continue;
        }
        ss << records[i].ToString();
    }
    ss << "]}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync