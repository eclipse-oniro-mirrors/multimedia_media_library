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

#ifndef OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_PHOTOS_VO_H
#define OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_PHOTOS_VO_H

#include <string>
#include <vector>
#include <sstream>

#include "i_media_parcelable.h"
#include "photos_vo.h"
#include "media_itypes_utils.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT OnFetchPhotosVo : public IPC::IMediaParcelable {
public:
    std::string cloudId;
    std::string fileName;
    std::string fileSourcePath;
    std::string mimeType;
    std::string firstVisitTime;  // MDKRecord first_update_time
    std::string detailTime;
    std::string frontCamera;
    std::string editDataCamera;
    std::string title;
    std::string relativePath;
    std::string virtualPath;
    std::string dateYear;
    std::string dateMonth;
    std::string dateDay;
    std::string shootingMode;
    std::string shootingModeTag;
    std::string burstKey;
    std::string localPath;
    double latitude;
    double longitude;
    std::string description;
    std::string source;  // decice_name
    int32_t fileId;
    int32_t mediaType;
    int32_t fileType;
    int32_t rotation;
    int32_t photoHeight;
    int32_t photoWidth;
    int32_t duration;
    int32_t hidden;
    int32_t burstCoverLevel;
    int32_t subtype;
    int32_t originalSubtype;
    int32_t dynamicRangeType;
    int32_t hdrMode;
    int32_t videoMode;
    int32_t movingPhotoEffectMode;
    int32_t supportedWatermarkType;
    int32_t strongAssociation;
    int64_t fixVersion;
    int64_t version;
    int64_t size;
    int64_t lcdSize;
    int64_t thmSize;
    int64_t createTime;
    int64_t metaDateModified;
    int64_t dualEditTime;
    int64_t editTime;
    int64_t editedTimeMs;
    int64_t recycledTime;
    int64_t hiddenTime;
    int64_t coverPosition;
    int32_t isRectificationCover;
    int32_t exifRotate;
    bool isDelete;
    bool hasAttributes;
    bool hasproperties;
    bool isFavorite;
    bool isRecycle;
    std::vector<std::string> sourceAlbumIds;
    // lake
    int32_t fileSourceType;
    std::string storagePath;
    std::map<std::string, std::string> stringfields;
    // Safe Album: critical type for children's watch
    int32_t criticalType{0};
    int32_t isCritical{0};

public:  // functions of Parcelable.
    virtual ~OnFetchPhotosVo() = default;
    bool Unmarshalling(MessageParcel &parcel) override;
    bool Marshalling(MessageParcel &parcel) const override;

public:  // basic functions
    std::string ToString() const;

private:
    bool MarshallingBasicInfo(Parcel &parcel) const;
    bool MarshallingAttributesInfo(Parcel &parcel) const;
    bool ReadBasicInfo(Parcel &parcel);
    bool ReadAttributesInfo(Parcel &parcel);
    void GetBasicInfo(std::stringstream &ss) const;
    void GetAttributesInfo(std::stringstream &ss) const;
    void GetAttributesHashMap(std::stringstream &ss) const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_ON_FETCH_PHOTOS_VO_H