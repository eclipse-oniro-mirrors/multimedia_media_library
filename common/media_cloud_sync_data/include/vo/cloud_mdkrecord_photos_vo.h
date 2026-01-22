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

#ifndef OHOS_MEDIA_CLOUD_SYNC_CLOUD_MDKRECORD_PHOTOS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_CLOUD_MDKRECORD_PHOTOS_VO_H

#include <string>

#include "i_media_parcelable.h"
#include "photos_po.h"
#include "media_itypes_utils.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMdkRecordPhotosVo : public IPC::IMediaParcelable {
public:
    std::string title;
    int32_t mediaType;
    int32_t duration;
    int32_t hidden;
    int64_t hiddenTime;
    std::string relativePath;
    std::string virtualPath;
    int64_t metaDateModified;
    int32_t subtype;
    int32_t burstCoverLevel;
    std::string burstKey;
    std::string dateYear;
    std::string dateMonth;
    std::string dateDay;
    std::string shootingMode;
    std::string shootingModeTag;
    int32_t dynamicRangeType;
    int32_t hdrMode;
    int32_t videoMode;
    std::string frontCamera;
    int64_t editTime;
    int32_t originalSubtype;
    int64_t coverPosition;
    int32_t isRectificationCover;
    int32_t exifRotate;
    int32_t movingPhotoEffectMode;
    int32_t supportedWatermarkType;
    int32_t strongAssociation;
    int32_t fileId;
    std::string cloudId;
    std::string originalAssetCloudId;
    std::string data;
    int64_t dateAdded;
    int64_t dateModified;
    int32_t ownerAlbumId;  // it is equal to PAC::ALBUM_ID
    double latitude;
    double longitude;
    std::string sourcePath;
    std::string displayName;
    int64_t dateTaken;
    std::string detailTime;
    int32_t height;
    int32_t width;
    std::string deviceName;
    int64_t dateTrashed;
    int32_t isFavorite;
    std::string userComment;
    int32_t dirty;
    int32_t orientation;
    int64_t size;
    int64_t baseVersion;
    std::string mimeType;
    std::string recordType;
    std::string recordId;
    bool isNew;
    int32_t fileSourceType;
    std::string storagePath;

    // Photo Album
    std::string albumCloudId;
    std::string albumLPath;
    int32_t coverUriSource;

    // Photo Map
    std::vector<std::string> removeAlbumCloudId;

    // Hash Map for attributes
    std::map<std::string, std::string> stringfields;

public:
    virtual ~CloudMdkRecordPhotosVo() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

private:
    // functions for ToString
    void GetAlbumInfo(std::stringstream &ss) const;
    void GetBasicInfo(std::stringstream &ss) const;
    void GetPropertiesInfo(std::stringstream &ss) const;
    void GetCloudInfo(std::stringstream &ss) const;
    void GetAttributesInfo(std::stringstream &ss) const;
    void GetAttributesHashMap(std::stringstream &ss) const;
    void GetRemoveAlbumInfo(std::stringstream &ss) const;
    // functions for Marshalling
    bool MarshallingBasicInfo(Parcel &parcel) const;
    bool MarshallingAttributesInfo(MessageParcel &parcel) const;
    bool ReadBasicInfo(Parcel &parcel);
    bool ReadAttributesInfo(MessageParcel &parcel);

public:  // basic functions
    std::string ToString();
};

class EXPORT CloudMdkRecordPhotosReqBody : public IPC::IMediaParcelable {
public:
    int32_t size;
    int32_t dirtyType;

public:  // functions of Parcelable.
    virtual ~CloudMdkRecordPhotosReqBody() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;
};

class EXPORT CloudMdkRecordPhotosRespBody : public IPC::IMediaParcelable {
private:
    std::vector<CloudMdkRecordPhotosVo> cloudPhotosUploadRecord;

public:
    CloudMdkRecordPhotosRespBody() = default;
    CloudMdkRecordPhotosRespBody(std::vector<CloudMdkRecordPhotosVo> record) : cloudPhotosUploadRecord(record)
    {}
    std::vector<CloudMdkRecordPhotosVo> GetPhotosRecords();
    bool GetRecords(std::vector<CloudMdkRecordPhotosVo> &val, MessageParcel &parcel);

public:  // functions of Parcelable.
    virtual ~CloudMdkRecordPhotosRespBody() = default;
    bool Marshalling(MessageParcel &parcel) const override;
    bool Unmarshalling(MessageParcel &parcel) override;

public:  // basic functions
    std::string ToString() const;

public:  // member functions
    size_t GetDataSize() const;
    bool TruncateDataBy200K();
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_CLOUD_MDKRECORD_PHOTOS_VO_H
