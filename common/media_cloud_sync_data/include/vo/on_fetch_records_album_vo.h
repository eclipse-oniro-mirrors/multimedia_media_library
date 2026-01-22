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

#ifndef OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_ALBUM_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_RECORDS_ALBUM_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT OnFetchRecordsAlbumReqBody : public IPC::IMediaParcelable {
public:
    class AlbumReqData : public IPC::IMediaParcelable {
    public:
        std::string cloudId;
        std::string localPath;
        std::string albumName;
        std::string albumBundleName;
        std::string localLanguage;
        int32_t albumId;
        int32_t priority;
        int32_t albumType;
        int32_t albumSubType;
        int64_t albumDateCreated;
        int64_t albumDateAdded;
        int64_t albumDateModified;
        bool isDelete;
        int32_t coverUriSource;
        std::string coverCloudId;

    public:  // functions of Parcelable.
        virtual ~AlbumReqData() = default;
        bool Unmarshalling(MessageParcel &parcel) override;
        bool Marshalling(MessageParcel &parcel) const override;

    public:  // basic functions
        std::string ToString() const;
    };

public:
    std::vector<AlbumReqData> albums;

public:  // functions of Parcelable.
    virtual ~OnFetchRecordsAlbumReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT OnFetchRecordsAlbumRespBody : public IPC::IMediaParcelable {
public:
    std::vector<std::string> failedRecords;
    std::vector<int32_t> stats{0, 0, 0, 0, 0};

public:  // functions of Parcelable.
    virtual ~OnFetchRecordsAlbumRespBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_Fetch_RECORDS_ALBUM_VO_H