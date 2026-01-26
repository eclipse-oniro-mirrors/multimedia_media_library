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

#include "on_fetch_records_vo.h"

#include <sstream>

#include "media_itypes_utils.h"
#include "cloud_media_sync_const.h"
#include "media_log.h"
#include "message_parcel.h"

namespace OHOS::Media::CloudSync {
bool OnFetchRecordsReqBody::Unmarshalling(MessageParcel &parcel)
{
    return IPC::ITypeMediaUtil::UnmarshallingParcelable<OnFetchPhotosVo>(this->onFetchPhotos, parcel);
}

bool OnFetchRecordsReqBody::Marshalling(MessageParcel &parcel) const
{
    return IPC::ITypeMediaUtil::MarshallingParcelable<OnFetchPhotosVo>(this->onFetchPhotos, parcel);
}

int32_t OnFetchRecordsReqBody::AddOnFetchPhotoData(const OnFetchPhotosVo &data)
{
    this->onFetchPhotos.push_back(data);
    return 0;
}

std::vector<OnFetchPhotosVo> OnFetchRecordsReqBody::GetOnFetchPhotoData()
{
    return onFetchPhotos;
}

std::string OnFetchRecordsReqBody::ToString() const
{
    std::stringstream ss;
    return ss.str();
}

bool OnFetchRecordsRespBody::Unmarshalling(MessageParcel &parcel)
{
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Unmarshalling<std::string>(this->failedRecords, parcel), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::UnmarshallingParcelable<PhotosVo>(this->newDatas, parcel), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::UnmarshallingParcelable<PhotosVo>(this->fdirtyDatas, parcel), false);
    return IPC::ITypeMediaUtil::Unmarshalling<int32_t>(this->stats, parcel);
}

bool OnFetchRecordsRespBody::Marshalling(MessageParcel &parcel) const
{
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::Marshalling<std::string>(this->failedRecords, parcel), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::MarshallingParcelable<PhotosVo>(this->newDatas, parcel), false);
    CHECK_AND_RETURN_RET(IPC::ITypeMediaUtil::MarshallingParcelable<PhotosVo>(this->fdirtyDatas, parcel), false);
    return IPC::ITypeMediaUtil::Marshalling<int32_t>(this->stats, parcel);
}

std::string OnFetchRecordsRespBody::ToString() const
{
    std::stringstream ss;
    ss << "{"
       << "\"stats\":[" << stats[StatsIndex::NEW_RECORDS_COUNT] << "," << stats[StatsIndex::MERGE_RECORDS_COUNT] << ","
       << stats[StatsIndex::META_MODIFY_RECORDS_COUNT] << "," << stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT] << ","
       << stats[StatsIndex::DELETE_RECORDS_COUNT] << "],\"newDatas\":[";
    for (uint32_t i = 0; i < newDatas.size(); i++) {
        if (i != newDatas.size() - 1) {
            ss << newDatas[i].ToString() << ",";
            continue;
        }
        ss << newDatas[i].ToString();
    }
    ss << "],\"fdirtyDatas\": [";
    for (uint32_t i = 0; i < fdirtyDatas.size(); i++) {
        if (i != fdirtyDatas.size() - 1) {
            ss << fdirtyDatas[i].ToString() << ",";
            continue;
        }
        ss << fdirtyDatas[i].ToString();
    }
    ss << "],\"failedRecords\": [";
    for (uint32_t i = 0; i < failedRecords.size(); i++) {
        if (i != failedRecords.size() - 1) {
            ss << failedRecords[i] << ",";
            continue;
        }
        ss << failedRecords[i];
    }
    ss << "]}";
    return ss.str();
}

bool OnFetchRecordsReqBody::SplitBy20K(std::vector<OnFetchRecordsReqBody> &reqBodyList) const
{
    CHECK_AND_RETURN_RET(!this->onFetchPhotos.empty(), false);
    const size_t parcelGap = 4800;
    const size_t parcelCapacity = 204800 - parcelGap;
    size_t parcelSize = 0;
    size_t currIndex = 0;
    while (currIndex < this->onFetchPhotos.size()) {
        MessageParcel data;
        size_t index = 0;
        OnFetchRecordsReqBody reqBody;
        std::vector<OnFetchPhotosVo> &childList = reqBody.onFetchPhotos;
        for (index = currIndex; index < this->onFetchPhotos.size(); index++) {
            this->onFetchPhotos[index].Marshalling(data);
            parcelSize = data.GetDataSize();
            CHECK_AND_BREAK_INFO_LOG(parcelSize <= parcelCapacity,
                "exceed capacity, split it. parcelSize: %{public}zu, parcelCapacity: %{public}zu",
                parcelSize,
                parcelCapacity);
            childList.emplace_back(this->onFetchPhotos[index]);
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
        this->onFetchPhotos.size(),
        reqBodyList.size());
    return true;
}
 
void OnFetchRecordsRespBody::MergeRespBody(const OnFetchRecordsRespBody &respBody)
{
    this->failedRecords.insert(this->failedRecords.end(), respBody.failedRecords.begin(), respBody.failedRecords.end());
    this->newDatas.insert(this->newDatas.end(), respBody.newDatas.begin(), respBody.newDatas.end());
    this->fdirtyDatas.insert(this->fdirtyDatas.end(), respBody.fdirtyDatas.begin(), respBody.fdirtyDatas.end());
    for (size_t index = 0; index < this->stats.size() && index < respBody.stats.size(); index++) {
        this->stats[index] += respBody.stats[index];
    }
    return;
}
}  // namespace OHOS::Media::CloudSync