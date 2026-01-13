/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_ALBUM_BASE_UPLOAD_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_ALBUM_BASE_UPLOAD_VO_H

#include <string>

#include "i_media_parcelable.h"
#include "photo_album_po.h"
#include "media_itypes_utils.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMdkRecordPhotoAlbumVo : public IPC::IMediaParcelable {
public:
    int32_t albumId;
    int32_t albumType;
    std::string albumName;
    std::string lpath;
    std::string cloudId;
    int32_t albumSubtype;
    int64_t dateAdded;
    int64_t dateModified;
    std::string bundleName;
    std::string localLanguage;
    int32_t coverUriSource;
    std::string coverCloudId;
    /* album_plugin columns */
    std::string albumPluginCloudId;
    std::string albumNameEn;
    std::string dualAlbumName;
    int32_t priority;
    bool isInWhiteList;

public:
    virtual ~CloudMdkRecordPhotoAlbumVo() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT CloudMdkRecordPhotoAlbumReqBody : public IPC::IMediaParcelable {
public:
    int32_t size;
    bool isCloudSpaceFull;
    
public:  // functions of Parcelable.
    virtual ~CloudMdkRecordPhotoAlbumReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

// 资产处理，相册后续填充
class EXPORT CloudMdkRecordPhotoAlbumRespBody : public IPC::IMediaParcelable {
private:
    std::vector<CloudMdkRecordPhotoAlbumVo> baseAlbumUploadVo;

public:
    CloudMdkRecordPhotoAlbumRespBody() = default;
    CloudMdkRecordPhotoAlbumRespBody(std::vector<CloudMdkRecordPhotoAlbumVo> record) : baseAlbumUploadVo(record)
    {}
    std::vector<CloudMdkRecordPhotoAlbumVo> GetPhotoAlbumRecords();
    bool GetRecords(std::vector<CloudMdkRecordPhotoAlbumVo> &val, MessageParcel &parcel);

public:  // functions of Parcelable.
    virtual ~CloudMdkRecordPhotoAlbumRespBody() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

public:  // basic functions
    std::string ToString() const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_ALBUM_BASE_UPLOAD_VO_H