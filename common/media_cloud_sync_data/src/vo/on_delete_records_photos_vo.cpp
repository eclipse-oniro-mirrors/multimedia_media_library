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

#include "on_delete_records_photos_vo.h"

#include <sstream>

#include "media_itypes_utils.h"

namespace OHOS::Media::CloudSync {
bool OnDeleteRecordsPhoto::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadString(this->dkRecordId);
    parcel.ReadString(this->cloudId);
    parcel.ReadBool(this->isSuccess);
    return true;
}
bool OnDeleteRecordsPhoto::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteString(this->dkRecordId);
    parcel.WriteString(this->cloudId);
    parcel.WriteBool(this->isSuccess);
    return true;
}

std::string OnDeleteRecordsPhoto::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"dkRecordId\": \"" << dkRecordId << "\", "
       << "\"cloudId\": \"" << cloudId << "\""
       << "\"isSeccuss\": \"" << isSuccess << "\""
       << "}";
    return ss.str();
}

int32_t OnDeleteRecordsPhotosReqBody::AddDeleteRecord(const OnDeleteRecordsPhoto &record)
{
    this->records.push_back(record);
    return 0;
}

std::vector<OnDeleteRecordsPhoto> OnDeleteRecordsPhotosReqBody::GetDeleteRecords()
{
    return records;
}

bool OnDeleteRecordsPhotosReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<OnDeleteRecordsPhoto>(this->records, parcel);
}
bool OnDeleteRecordsPhotosReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<OnDeleteRecordsPhoto>(this->records, parcel);
}

std::string OnDeleteRecordsPhotosReqBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"records\": [";
    for (size_t i = 0; i < records.size(); i++) {
        ss << records[i].ToString();
        if (i != records.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}

bool OnDeleteRecordsPhotosRespBody::Unmarshalling(MessageParcel &parcel)
{
    parcel.ReadInt32(this->failSize);
    return true;
}

bool OnDeleteRecordsPhotosRespBody::Marshalling(MessageParcel &parcel) const
{
    parcel.WriteInt32(this->failSize);
    return true;
}

std::string OnDeleteRecordsPhotosRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"failSize\": " << failSize << "}";
    return ss.str();
}
}  // namespace OHOS::Media::CloudSync