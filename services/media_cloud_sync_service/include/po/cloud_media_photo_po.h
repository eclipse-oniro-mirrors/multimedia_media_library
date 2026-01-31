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

#ifndef OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PHOTO_PO_H
#define OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PHOTO_PO_H

#include <string>
#include <vector>
#include <sstream>

namespace OHOS::Media::CloudSync {
class CloudMediaPhotoPo {
public:
    std::string data;                  // PhotoColumn::MEDIA_FILE_PATH,
    std::string title;                 // PhotoColumn::MEDIA_TITLE;
    int64_t size;                      // PhotoColumn::MEDIA_SIZE;
    std::string displayName;           // PhotoColumn::MEDIA_NAME;
    int32_t mediaType;                 // PhotoColumn::MEDIA_TYPE;
    std::string mimeType;              // PhotoColumn::MEDIA_MIME_TYPE;
    std::string deviceName;            // PhotoColumn::MEDIA_DEVICE_NAME;
    int64_t dateAdded;                 // PhotoColumn::MEDIA_DATE_ADDED;
    int64_t dateModified;              // PhotoColumn::MEDIA_DATE_MODIFIED;
    int64_t dateTaken;                 // PhotoColumn::MEDIA_DATE_TAKEN;
    int32_t duration;                  // PhotoColumn::MEDIA_DURATION;
    int32_t isFavorite;                // PhotoColumn::MEDIA_IS_FAV;
    int64_t dateTrashed;               // PhotoColumn::MEDIA_DATE_TRASHED;
    int32_t hidden;                    // PhotoColumn::MEDIA_HIDDEN;
    int64_t hiddenTime;                // PhotoColumn::PHOTO_hidden_TIME;
    std::string relativePath;          // PhotoColumn::MEDIA_RELATIVE_PATH;
    std::string virtualPath;           // PhotoColumn::MEDIA_VIRTUAL_PATH;
    int64_t metaDateModified;          // PhotoColumn::PHOTO_META_DATE_MODIFIED;
    int32_t orientation;               // PhotoColumn::PHOTO_ORIENTATION;
    double latitude;                   // PhotoColumn::PHOTO_LATITUDE;
    double longitude;                  // PhotoColumn::PHOTO_LONGITUDE;
    int32_t height;                    // PhotoColumn::PHOTO_HEIGHT;
    int32_t width;                     // PhotoColumn::PHOTO_WIDTH;
    int32_t subtype;                   // PhotoColumn::PHOTO_SUBTYPE;
    int32_t burstCoverLevel;           // PhotoColumn::PHOTO_BURST_COVER_LEVEL;
    std::string burstKey;              // PhotoColumn::PHOTO_BURST_KEY;
    std::string dateYear;              // PhotoColumn::PHOTO_DATE_YEAR;
    std::string dateMonth;             // PhotoColumn::PHOTO_DATE_MONTH;
    std::string dateDay;               // PhotoColumn::PHOTO_DATE_DAY;
    std::string userComment;           // PhotoColumn::PHOTO_USER_COMMENT;
    int32_t thumbStatus;               // PhotoColumn::PHOTO_THUMB_STATUS;
    int32_t syncStatus;                // PhotoColumn::PHOTO_SYNC_STATUS;
    std::string shootingMode;          // PhotoColumn::PHOTO_SHOOTING_MODE;
    std::string shootingModeTag;       // PhotoColumn::PHOTO_SHOOTING_MODE_TAG;
    int32_t dynamicRangeType;          // PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE;
    int32_t hdrMode;                   // PhotoColumn::PHOTO_HDR_MODE;
    int32_t videoMode;                 // PhotoColumn::PHOTO_VIDEO_MODE;
    std::string frontCamera;           // PhotoColumn::PHOTO_FRONT_CAMERA;
    std::string detailTime;            // PhotoColumn::PHOTO_DETAIL_TIME;
    int64_t editTime;                  // PhotoColumn::PHOTO_EDIT_TIME;
    int32_t originalSubtype;           // PhotoColumn::PHOTO_ORIGINAL_SUBTYPE;
    int64_t coverPosition;             // PhotoColumn::PHOTO_COVER_POSITION;
    int32_t isRectificationCover;      // PhotoColumn::PHOTO_IS_RECTIFICATION_COVER;
    int32_t exifRotate;                // PhotoColumn::PHOTO_EXIF_ROTATE;
    int32_t movingPhotoEffectMode;     // PhotoColumn::MOVING_PHOTO_EFFECT_MODE;
    int32_t ownerAlbumId;              // PhotoColumn::PHOTO_OWNER_ALBUM_ID;
    std::string originalAssetCloudId;  // PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID;
    std::string sourcePath;            // PhotoColumn::PHOTO_SOURCE_PATH;
    int32_t supportedWatermarkType;    // PhotoColumn::SUPPORTED_WATERMARK_TYPE;
    int32_t strongAssociation;         // PhotoColumn::PHOTO_STRONG_ASSOCIATION;
    int32_t dirty;                     // PhotoColumn::PHOTO_DIRTY
    int32_t position;                  // PhotoColumn::PHOTO_POSITION
    int64_t cloudVersion;              // PhotoColumn::PHOTO_CLOUD_VERSION
    int64_t baseVersion;               // PhotoColumn::PHOTO_CLOUD_VERSION
    std::string recordType;            // PhotoColumn::PHOTO_CLOUD_ID
    std::string recordId;              // PhotoColumn::PHOTO_CLOUD_ID
    std::string fileName;
    bool isNew;

    /* keep cloud_id at the last; so RecordToValueBucket can skip it*/
    int32_t fileId;       //  MediaColumn::MEDIA_ID;
    std::string cloudId;  // PhotoColumn::PHOTO_CLOUD_ID

    /* Photo Album */
    std::string albumCloudId;  // SetSourceAlbum()函数相关
    std::string albumLPath;

    /* Photo Map */
    std::vector<std::string> removeAlbumCloudId;

public:  // basic function
    std::string ToString() const
    {
        std::stringstream ss;
        ss << "{";
        ss << "\"data\": \"" << data << "\", "
           << "\"title\": \"" << title << "\", "
           << "\"size\": " << size << ", "
           << "\"displayName\": \"" << displayName << "\", "
           << "\"mediaType\": " << mediaType << ", "
           << "\"duration\": " << duration << ", "
           << "\"isFavorite\": " << isFavorite << ", "
           << "\"dateTrashed\": " << dateTrashed << ", "
           << "\"hidden\": " << hidden << ", "
           << "\"hiddenTime\": " << hiddenTime << ", "
           << "\"orientation\": " << orientation << ", "
           << "\"subtype\": " << subtype << ", "
           << "\"burstCoverLevel\": " << burstCoverLevel << ", "
           << "\"burstKey\": \"" << burstKey << "\", "
           << "\"thumbStatus\": " << thumbStatus << ", "
           << "\"ownerAlbumId\": " << ownerAlbumId << ", "
           << "\"sourcePath\": \"" << sourcePath << "\", "
           << "\"fileId\": " << fileId << ", "
           << "\"cloudId\": \"" << cloudId << "\", "
           << "\"albumCloudId\": \"" << albumCloudId << "\", "
           << "\"albumLPath\": \"" << albumLPath << "\", "
           << "\"dirty\": \"" << dirty << "\", "
           << "\"position\": \"" << position << "\", ";
        ss << "}";
        return ss.str();
    }
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUDSYNC_CLOUD_MEDIA_PHOTO_PO_H
