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

#ifndef OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PULL_DATA_H
#define OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PULL_DATA_H

#include <string>
#include <vector>
#include <sstream>

#include "cloud_media_define.h"
#include "photo_album_po.h"
#include "photos_po.h"

namespace OHOS::Media::CloudSync {
using namespace OHOS::Media::ORM;
class EXPORT CloudMediaPullDataDto {
public:
    std::optional<PhotoAlbumPo> albumInfoOp;
    std::optional<PhotosPo> localPhotosPoOp;
public:
    // key
    std::string cloudId;
    // basic
    bool basicIsDelete{false};
    int64_t basicSize{-1};
    std::string basicDisplayName;
    std::string basicMimeType;
    std::string basicDeviceName;
    int64_t modifiedTime{-1};
    int64_t basicEditedTime{-1};
    int64_t basicCreatedTime{-1};
    int64_t dateTaken{-1};
    int32_t basicIsFavorite{-1};
    int32_t basicIsRecycle{-1};
    int64_t basicRecycledTime{-1};
    std::string basicDescription;
    int32_t basicFileType{-1};
    std::string basicFileName;
    int64_t basicCloudVersion{-1};
    int32_t duration{-1};                         /* duration */
    // "attributes"
    bool hasAttributes{false};
    std::string attributesTitle;                  /* title */
    int32_t attributesMediaType{-1};              /* media_type */
    int32_t attributesHidden{-1};                 /* hidden */
    int64_t attributesHiddenTime{-1};             /* hidden_time */
    std::string attributesRelativePath;           /* relative_path */
    std::string attributesVirtualPath;            /* virtual_path */
    std::string attributesPath;                   /* data */
    int64_t attributesMetaDateModified{-1};       /* meta_date_modified */
    int32_t attributesSubtype{-1};                /* subtype */
    int32_t attributesBurstCoverLevel{-1};        /* burst_cover_level */
    std::string attributesBurstKey;               /* burst_key */
    std::string attributesDateYear;               /* date_year */
    std::string attributesDateMonth;              /* date_month */
    std::string attributesDateDay;                /* date_day */
    std::string attributesShootingMode;           /* shooting_mode */
    std::string attributesShootingModeTag;        /* shooting_mode_tag */
    int32_t attributesDynamicRangeType{-1};       /* dynamic_range_type */
    int32_t attributesHdrMode{-1};                /* hdr_mode */
    int32_t attributesVideoMode{-1};              /* video_mode */
    std::string attributesFrontCamera;            /* front_camera */
    int64_t attributesEditTime{-1};               /* edit_time */
    int32_t attributesOriginalSubtype{-1};        /* original_subtype */
    int64_t attributesCoverPosition{-1};          /* cover_position */
    int32_t attributesIsRectificationCover{-1};   /* is_rectification_cover */
    int32_t exifRotate{-1};             /* exif_rotate*/
    int32_t attributesMovingPhotoEffectMode{-1};  /* moving_photo_effect_mode */
    int32_t attributesSupportedWatermarkType{-1}; /* supported_watermark_type */
    int32_t attributesStrongAssociation{-1};      /* strong_association */
    int32_t attributesFileId{-1};                 /* file_id */
    std::string attributesCloudId;                /* cloud_id */
    std::string attributesOriginCloudId;          /* origin cloud_id */
    int32_t deferredEffectsStatus{-2};            /* deferred_effect_status */
    int64_t attributesEditedTimeMs{-1}; /* editedTime_ms */
    int32_t attributesFixVersion{-1};
    std::string attributesEditDataCamera;
    std::vector<std::string> attributesSrcAlbumIds;
    int32_t attributesFileSourceType{0};          /* file_source_type */
    std::string attributesStoragePath;            /* storage_path */
    
    // Safe Album: critical type for children's watch
    int32_t attributesRiskStatus{0};              /* photo_risk_status */
    int32_t attributesIsCritical{0};              /* is_critical */

    // "properties"
    bool hasProperties{false};
    std::string propertiesSourceFileName;
    std::string propertiesSourcePath;
    int32_t propertiesRotate{-1};
    double latitude;
    double longitude;
    int32_t propertiesHeight{-1};
    int32_t propertiesWidth{-1};
    std::string propertiesFirstUpdateTime;
    std::string propertiesDetailTime;

    // local data
    int32_t localFileId{-1};
    std::string localPath;
    int64_t localSize{-1};
    int64_t lcdSize{-1};
    int64_t thmSize{-1};
    int32_t localMediaType{-1};
    std::string localDateAdded;
    std::string localDateModified;
    int32_t localDirty{-1};
    int32_t localPosition{-1};
    int32_t localOwnerAlbumId;
    int32_t localOrientation{-1};
    int32_t localThumbState{-1};
    std::string localOriginalAssetCloudId;
    int32_t localExifRotate{-1};
    std::string localDisplayName;
    std::map<std::string, std::string> stringfields;

public:  // basic function
    std::string ToString() const;
    bool IsHiddenAsset() const;
    bool IsVideoAsset();
    bool FindAlbumCloudId(std::string &albumCloudId);
    bool FindAlbumUploadStatus() const;
    bool GetIsRecycleUpdated() const;
    void SetIsRecycleUpdated(bool isRecycleUpdated);
private:
    void GetBasicInfo(std::stringstream &ss) const;
    void GetAttributesInfo(std::stringstream &ss) const;
    void GetPropertiesInfo(std::stringstream &ss) const;
    void GetCloudInfo(std::stringstream &ss) const;
    void GetAlbumIds(std::stringstream &ss) const;
    bool isRecycleUpdated{false};
    void GetAttributesHashMap(std::stringstream &ss) const;
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PULL_DATA_H
