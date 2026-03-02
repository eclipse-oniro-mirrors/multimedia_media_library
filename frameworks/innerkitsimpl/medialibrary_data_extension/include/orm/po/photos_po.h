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

#ifndef OHOS_MEDIA_ORM_PHOTOS_PO_H
#define OHOS_MEDIA_ORM_PHOTOS_PO_H

#include <string>
#include <vector>
#include <sstream>
#include "cloud_media_define.h"

namespace OHOS::Media::ORM {
class EXPORT PhotosPo {
public:
    std::optional<std::string> data;                  // PhotoColumn::MEDIA_FILE_PATH,
    std::optional<std::string> title;                 // PhotoColumn::MEDIA_TITLE;
    std::optional<int64_t> size;                      // PhotoColumn::MEDIA_SIZE;
    std::optional<std::string> displayName;           // PhotoColumn::MEDIA_NAME;
    std::optional<int32_t> mediaType;                 // PhotoColumn::MEDIA_TYPE;
    std::optional<std::string> mimeType;              // PhotoColumn::MEDIA_MIME_TYPE;
    std::optional<std::string> deviceName;            // PhotoColumn::MEDIA_DEVICE_NAME;
    std::optional<int64_t> dateAdded;                 // PhotoColumn::MEDIA_DATE_ADDED;
    std::optional<int64_t> dateModified;              // PhotoColumn::MEDIA_DATE_MODIFIED;
    std::optional<int64_t> dateTaken;                 // PhotoColumn::MEDIA_DATE_TAKEN;
    std::optional<int32_t> duration;                  // PhotoColumn::MEDIA_DURATION;
    std::optional<int32_t> isFavorite;                // PhotoColumn::MEDIA_IS_FAV;
    std::optional<int64_t> dateTrashed;               // PhotoColumn::MEDIA_DATE_TRASHED;
    std::optional<int32_t> hidden;                    // PhotoColumn::MEDIA_HIDDEN;
    std::optional<int64_t> hiddenTime;                // PhotoColumn::PHOTO_hidden_TIME;
    std::optional<std::string> relativePath;          // PhotoColumn::MEDIA_RELATIVE_PATH;
    std::optional<std::string> virtualPath;           // PhotoColumn::MEDIA_VIRTUAL_PATH;
    std::optional<int64_t> metaDateModified;          // PhotoColumn::PHOTO_META_DATE_MODIFIED;
    std::optional<int32_t> orientation;               // PhotoColumn::PHOTO_ORIENTATION;
    std::optional<double> latitude;                   // PhotoColumn::PHOTO_LATITUDE;
    std::optional<double> longitude;                  // PhotoColumn::PHOTO_LONGITUDE;
    std::optional<int32_t> height;                    // PhotoColumn::PHOTO_HEIGHT;
    std::optional<int32_t> width;                     // PhotoColumn::PHOTO_WIDTH;
    std::optional<int32_t> subtype;                   // PhotoColumn::PHOTO_SUBTYPE;
    std::optional<int32_t> burstCoverLevel;           // PhotoColumn::PHOTO_BURST_COVER_LEVEL;
    std::optional<std::string> burstKey;              // PhotoColumn::PHOTO_BURST_KEY;
    std::optional<std::string> dateYear;              // PhotoColumn::PHOTO_DATE_YEAR;
    std::optional<std::string> dateMonth;             // PhotoColumn::PHOTO_DATE_MONTH;
    std::optional<std::string> dateDay;               // PhotoColumn::PHOTO_DATE_DAY;
    std::optional<std::string> userComment;           // PhotoColumn::PHOTO_USER_COMMENT;
    std::optional<int32_t> thumbStatus;               // PhotoColumn::PHOTO_THUMB_STATUS;
    std::optional<int32_t> syncStatus;                // PhotoColumn::PHOTO_SYNC_STATUS;
    std::optional<std::string> shootingMode;          // PhotoColumn::PHOTO_SHOOTING_MODE;
    std::optional<std::string> shootingModeTag;       // PhotoColumn::PHOTO_SHOOTING_MODE_TAG;
    std::optional<int32_t> dynamicRangeType;          // PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE;
    std::optional<int32_t> hdrMode;                   // PhotoColumn::PHOTO_HDR_MODE;
    std::optional<int32_t> videoMode;                 // PhotoColumn::PHOTO_VIDEO_MODE;
    std::optional<std::string> frontCamera;           // PhotoColumn::PHOTO_FRONT_CAMERA;
    std::optional<std::string> detailTime;            // PhotoColumn::PHOTO_DETAIL_TIME;
    std::optional<int64_t> editTime;                  // PhotoColumn::PHOTO_EDIT_TIME;
    std::optional<int32_t> originalSubtype;           // PhotoColumn::PHOTO_ORIGINAL_SUBTYPE;
    std::optional<int64_t> coverPosition;             // PhotoColumn::PHOTO_COVER_POSITION;
    std::optional<int32_t> isRectificationCover;      // PhotoColumn::PHOTO_IS_RECTIFICATION_COVER;
    std::optional<int32_t> exifRotate;                // PhotoColumn::PHOTO_EXIF_ROTATE;
    std::optional<int32_t> movingPhotoEffectMode;     // PhotoColumn::MOVING_PHOTO_EFFECT_MODE;
    std::optional<int32_t> ownerAlbumId;              // PhotoColumn::PHOTO_OWNER_ALBUM_ID;
    std::optional<std::string> originalAssetCloudId;  // PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID;
    std::optional<std::string> sourcePath;            // PhotoColumn::PHOTO_SOURCE_PATH;
    std::optional<int32_t> supportedWatermarkType;    // PhotoColumn::SUPPORTED_WATERMARK_TYPE;
    std::optional<int32_t> isStylePhoto;              // PhotoColumn::IS_STYLE_PHOTO;
    std::optional<int32_t> strongAssociation;         // PhotoColumn::PHOTO_STRONG_ASSOCIATION;
    std::optional<int32_t> dirty;                     // PhotoColumn::PHOTO_DIRTY
    std::optional<int32_t> position;                  // PhotoColumn::PHOTO_POSITION
    std::optional<int64_t> cloudVersion;              // PhotoColumn::PHOTO_CLOUD_VERSION
    std::optional<int64_t> baseVersion;               // PhotoColumn::PHOTO_CLOUD_VERSION
    std::optional<std::string> recordType;            // PhotoColumn::PHOTO_CLOUD_ID
    std::optional<std::string> recordId;              // PhotoColumn::PHOTO_CLOUD_ID
    std::optional<bool> isNew;
    std::optional<int64_t> lcdVisitTime;              // PhotoColumn::PHOTO_LCD_VISIT_TIME
    std::optional<int64_t> thumbnailReady;            // PhotoColumn::PHOTO_THUMBNAIL_READY
    std::optional<int64_t> timePending;               // PhotoColumn::MEDIA_TIME_PENDING
    std::optional<int32_t> fileSourceType;            // PhotoColumn::PHOTO_FILE_SOURCE_TYPE
    std::optional<std::string> storagePath;           // PhotoColumn::PHOTO_STORAGE_PATH
    std::optional<std::string> lcdSize;               // PhotoColumn::PHOTO_LCD_SIZE
    std::optional<std::string> thumbSize;             // PhotoColumn::PHOTO_THUMB_SIZE
    std::optional<std::string> packageName;           // MediaColumn::MEDIA_PACKAGE_NAME
    std::optional<int32_t> southDeviceType;           // PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE

    /* keep cloud_id at the last; so RecordToValueBucket can skip it*/
    std::optional<int32_t> fileId;       //  MediaColumn::MEDIA_ID;
    std::optional<std::string> cloudId;  // PhotoColumn::PHOTO_CLOUD_ID

    /* Photo Album */
    std::optional<std::string> albumCloudId;  // SetSourceAlbum()函数相关
    std::optional<std::string> albumLPath;

    /* Photo Map */
    std::vector<std::string> removeAlbumCloudId;

    // All the properties in the photo table are optional.
    // If they are not set, they will not be updated in the database.
    std::unordered_map<std::string, std::string> attributes;

private:
    void GetAlbumInfo(std::stringstream &ss) const;
    void GetBasicInfo(std::stringstream &ss) const;
    void GetPropertiesInfo(std::stringstream &ss) const;
    void GetAttributesInfo(std::stringstream &ss) const;
    void GetCloudInfo(std::stringstream &ss) const;
    void GetRemoveAlbumCloudInfo(std::stringstream &ss) const;

public:  // basic function
    std::string ToString() const;
    bool IsCloudAsset() const;
    int32_t TryGetMdirty() const;
    bool ShouldHandleAsMediaFile() const;
    bool ShouldHandleAsLakeFile() const;
    std::string BuildFileUri() const;
};
}  // namespace OHOS::Media::ORM
#endif  // OHOS_MEDIA_ORM_PHOTOS_PO_H
