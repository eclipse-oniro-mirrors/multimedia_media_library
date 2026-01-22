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

#include "on_dentry_file_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "media_log.h"
#include "message_parcel.h"

namespace OHOS::Media::CloudSync {
bool OnDentryFileReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<OnFetchPhotosVo>(this->records, parcel);
}

bool OnDentryFileReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<OnFetchPhotosVo>(this->records, parcel);
}

int32_t OnDentryFileReqBody::AddOnDentryFileRecord(const OnFetchPhotosVo &record)
{
    this->records.push_back(record);
    return 0;
}

std::vector<OnFetchPhotosVo> OnDentryFileReqBody::GetOnDentryFileRecord()
{
    return records;
}

std::string OnDentryFileReqBody::ToString() const
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

bool OnDentryFileRespBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->failedRecords, parcel);
}

bool OnDentryFileRespBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::Marshalling<std::string>(this->failedRecords, parcel);
}

std::string OnDentryFileRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"failedRecords\": [";
    for (size_t i = 0; i < failedRecords.size(); i++) {
        ss << failedRecords[i];
        if (i != failedRecords.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]"
       << "}";
    return ss.str();
}

bool OnDentryFileReqBody::SplitBy20K(std::vector<OnDentryFileReqBody> &reqBodyList) const
{
    CHECK_AND_RETURN_RET(!this->records.empty(), false);
    const size_t parcelGap = 4800;
    const size_t parcelCapacity = 204800 - parcelGap;
    size_t parcelSize = 0;
    size_t currIndex = 0;
    while (currIndex < this->records.size()) {
        MessageParcel data;
        size_t index = 0;
        OnDentryFileReqBody reqBody;
        std::vector<OnFetchPhotosVo> &childList = reqBody.records;
        for (index = currIndex; index < this->records.size(); index++) {
            this->records[index].Marshalling(data);
            parcelSize = data.GetDataSize();
            CHECK_AND_BREAK_INFO_LOG(parcelSize <= parcelCapacity,
                "exceed capacity, split it. parcelSize: %{public}zu, parcelCapacity: %{public}zu",
                parcelSize,
                parcelCapacity);
            childList.emplace_back(this->records[index]);
        }
        CHECK_AND_BREAK_ERR_LOG(!childList.empty(),
            "dead loop detected, "
            "currIndex: %{public}zu, index: %{public}zu",
            currIndex,
            index);
        currIndex = index;
        reqBodyList.emplace_back(reqBody);
    }
    MEDIA_INFO_LOG("SplitBy20K completed, totalSize: %{public}zu, splited size: %{public}zu",
        this->records.size(),
        reqBodyList.size());
    return true;
}
 
void OnDentryFileRespBody::MergeRespBody(const OnDentryFileRespBody &respBody)
{
    this->failedRecords.insert(this->failedRecords.end(), respBody.failedRecords.begin(), respBody.failedRecords.end());
    return;
}
}  // namespace OHOS::Media::CloudSync