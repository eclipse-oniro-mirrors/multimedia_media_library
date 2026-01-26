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

#ifndef OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "photos_vo.h"
#include "on_fetch_photos_vo.h"
#include "cloud_file_data_vo.h"
#include "media_itypes_utils.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT OnFetchRecordsReqBody : public IPC::IMediaParcelable {
public:
    std::vector<OnFetchPhotosVo> onFetchPhotos;

public:  // functions of Parcelable.
    virtual ~OnFetchRecordsReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:
    int32_t AddOnFetchPhotoData(const OnFetchPhotosVo &data);
    std::vector<OnFetchPhotosVo> GetOnFetchPhotoData();
    bool SplitBy20K(std::vector<OnFetchRecordsReqBody> &reqBodyList) const;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT OnFetchRecordsRespBody : public IPC::IMediaParcelable {
public:
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};
    std::vector<PhotosVo> newDatas;
    std::vector<PhotosVo> fdirtyDatas;

public:  // functions of Parcelable.
    virtual ~OnFetchRecordsRespBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
    void MergeRespBody(const OnFetchRecordsRespBody &respBody);
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_VO_H