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

#ifndef OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_PHOTOS_DATA_H
#define OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_PHOTOS_DATA_H

#include <map>
#include <vector>

#include "mdk_record.h"
#include "mdk_reference.h"
#include "mdk_database.h"
#include "mdk_record_reader.h"
#include "cloud_media_define.h"

namespace OHOS::Media::CloudSync {
class EXPORT MDKRecordPhotosData {
private:  // data member
    MDKRecord record_;
    std::map<std::string, MDKRecordField> fields_;
    std::map<std::string, MDKRecordField> properties_;
    std::map<std::string, MDKRecordField> attributes_;

private:  // composited class
    MDKRecordReader recordReader_;

public:  // constructor & destructor
    MDKRecordPhotosData() = default;
    MDKRecordPhotosData(const MDKRecord &record);
    virtual ~MDKRecordPhotosData() = default;

private:  // Marshalling & UnMarshalling
    MDKRecordPhotosData &UnMarshalling(const MDKRecord &record);
    MDKRecordPhotosData &Marshalling();

public:  // getter & setter
    MDKRecord GetDKRecord();
    MDKRecordPhotosData &SetDKRecord(MDKRecord &record);
    std::optional<std::string> GetType() const;
    // type, "directory" or "file"
    MDKRecordPhotosData &SetType(const std::string &type);

public:  // record data getter & setter - gallery-specific or shared fileds
    std::optional<int32_t> GetFileId() const;
    MDKRecordPhotosData &SetFileId(const int32_t fileId);
    std::optional<int32_t> GetLocalId() const;
    MDKRecordPhotosData &SetLocalId(const int32_t localId);
    std::optional<int32_t> GetFileType() const;
    MDKRecordPhotosData &SetFileType(const int32_t &fileType);
    std::optional<std::string> GetFileName() const;
    MDKRecordPhotosData &SetFileName(const std::string &fileName);
    std::optional<int64_t> GetCreatedTime() const;
    MDKRecordPhotosData &SetCreatedTime(const int64_t &createdTime);
    std::optional<std::string> GetHashId() const;
    MDKRecordPhotosData &SetHashId(const std::string &hashId);
    std::optional<int64_t> GetSize() const;
    MDKRecordPhotosData &SetSize(const int64_t &size);
    std::optional<std::string> GetSource() const;
    MDKRecordPhotosData &SetSource(const std::string &source);
    std::optional<bool> GetRecycled() const;
    MDKRecordPhotosData &SetRecycled(const bool recycled);
    std::optional<int64_t> GetRecycledTime() const;
    MDKRecordPhotosData &SetRecycledTime(const int64_t &recycledTime);
    std::optional<bool> GetFavorite() const;
    MDKRecordPhotosData &SetFavorite(const bool favorite);
    std::optional<std::string> GetDescription() const;
    MDKRecordPhotosData &SetDescription(const std::string &description);
    std::optional<std::string> GetMimeType() const;
    MDKRecordPhotosData &SetMimeType(const std::string &mimeType);
    std::optional<MDKAsset> GetFileContent() const;
    MDKRecordPhotosData &SetFileContent(const MDKAsset &asset);
    std::optional<MDKAsset> GetFileRaw() const;
    MDKRecordPhotosData &SetFileRaw(const MDKAsset &asset);
    std::optional<MDKAsset> GetFileEditData() const;
    MDKRecordPhotosData &SetFileEditData(const MDKAsset &asset);

public:  // attributes getter & setter
    std::optional<std::string> GetTitle() const;
    MDKRecordPhotosData &SetTitle(const std::string &title);
    std::optional<int32_t> GetMediaType() const;
    MDKRecordPhotosData &SetMediaType(const int32_t mediaType);
    std::optional<int32_t> GetDuration() const;
    MDKRecordPhotosData &SetDuration(const int32_t duration);
    std::optional<int32_t> GetPropertiesDuration() const;
    MDKRecordPhotosData &SetPropertiesDuration(const int32_t duration);
    std::optional<int32_t> GetHidden() const;
    MDKRecordPhotosData &SetHidden(const int32_t hidden);
    std::optional<int64_t> GetHiddenTime() const;
    MDKRecordPhotosData &SetHiddenTime(const int64_t hiddenTime);
    std::optional<std::string> GetRelativePath() const;
    MDKRecordPhotosData &SetRelativePath(const std::string &relativePath);
    std::optional<std::string> GetVirtualPath() const;
    MDKRecordPhotosData &SetVirtualPath(const std::string &virtualPath);
    std::optional<int64_t> GetDateModified() const;
    MDKRecordPhotosData &SetDateModified(const int64_t dateModified);
    std::optional<int64_t> GetPhotoMetaDateModified() const;
    MDKRecordPhotosData &SetPhotoMetaDateModified(const int64_t dateModified);
    std::optional<int32_t> GetSubType() const;
    MDKRecordPhotosData &SetSubType(const int32_t subType);
    std::optional<int32_t> GetBurstCoverLevel() const;
    MDKRecordPhotosData &SetBurstCoverLevel(const int32_t burstCoverLevel);
    std::optional<std::string> GetBurstKey() const;
    MDKRecordPhotosData &SetBurstKey(const std::string &burstKey);
    std::optional<std::string> GetDateYear() const;
    MDKRecordPhotosData &SetDateYear(const std::string &dateYear);
    std::optional<std::string> GetDateMonth() const;
    MDKRecordPhotosData &SetDateMonth(const std::string &dateMonth);
    std::optional<std::string> GetDateDay() const;
    MDKRecordPhotosData &SetDateDay(const std::string &dateDay);
    std::optional<std::string> GetShootingMode() const;
    MDKRecordPhotosData &SetShootingMode(const std::string &shootingMode);
    std::optional<std::string> GetShootingModeTag() const;
    MDKRecordPhotosData &SetShootingModeTag(const std::string &shootingModeTag);
    std::optional<int32_t> GetDynamicRangeType() const;
    MDKRecordPhotosData &SetDynamicRangeType(const int32_t dynamicRangeType);
    std::optional<int32_t> GetHdrMode() const;
    MDKRecordPhotosData &SetHdrMode(const int32_t hdrMode);
    std::optional<int32_t> GetVideoMode() const;
    MDKRecordPhotosData &SetVideoMode(const int32_t videoMode);
    std::optional<std::string> GetFrontCamera() const;
    MDKRecordPhotosData &SetFrontCamera(const std::string &frontCamera);
    std::optional<int64_t> GetEditTime() const;
    MDKRecordPhotosData &SetEditTime(const int64_t editTime);
    std::optional<int32_t> GetOriginalSubType() const;
    MDKRecordPhotosData &SetOriginalSubType(const int32_t originalSubType);
    std::optional<int64_t> GetCoverPosition() const;
    MDKRecordPhotosData &SetCoverPosition(const int64_t coverPosition);
    std::optional<int32_t> GetIsRectificationCover() const;
    MDKRecordPhotosData &SetIsRectificationCover(const int32_t isRectificationCover);
    std::optional<int32_t> GetExifRotate() const;
    MDKRecordPhotosData &SetExifRotate(const int32_t exifRotate);
    std::optional<int32_t> GetMovingPhotoEffectMode() const;
    MDKRecordPhotosData &SetMovingPhotoEffectMode(const int32_t movingPhotoEffectMode);
    std::optional<int32_t> GetSupportedWatermarkType() const;
    MDKRecordPhotosData &SetSupportedWatermarkType(const int32_t supportedWatermarkType);
    std::optional<int32_t> GetStrongAssociation() const;
    MDKRecordPhotosData &SetStrongAssociation(const int32_t strongAssociation);
    std::optional<int32_t> GetCloudFileId() const;
    MDKRecordPhotosData &SetCloudFileId(const int32_t fileId);
    std::optional<std::string> GetCloudId() const;
    MDKRecordPhotosData &SetCloudId(const std::string &cloudId);
    std::optional<std::string> GetOriginalAssetCloudId() const;
    MDKRecordPhotosData &SetOriginalAssetCloudId(const std::string &originalAssetCloudId);
    std::optional<std::string> GetFilePath() const;
    MDKRecordPhotosData &SetFilePath(const std::string &filePath);
    std::optional<int64_t> GetDateAdded() const;
    MDKRecordPhotosData &SetDateAdded(const int64_t dateAdded);
    std::optional<int32_t> GetOwnerAlbumId() const;
    MDKRecordPhotosData &SetOwnerAlbumId(const int32_t ownerAlbumId);
    std::optional<int64_t> GetFixVersion() const;
    MDKRecordPhotosData &SetFixVersion(const int64_t fixVersion);
    std::optional<int64_t> GetLcdSize() const;
    MDKRecordPhotosData &SetLcdSize(const int64_t lcdSize);
    std::optional<int64_t> GetThmSize() const;
    MDKRecordPhotosData &SetThmSize(const int64_t thmSize);
    std::optional<int64_t> GetEditedTimeMs() const;
    MDKRecordPhotosData &SetEditedTimeMs(int64_t editedTimeMs);
    std::optional<std::string> GetEditDataCamera() const;
    MDKRecordPhotosData &SetEditDataCamera(const std::string &editDataCamera);
    std::optional<int32_t> GetFileSourceType() const;
    MDKRecordPhotosData &SetFileSourceType(int32_t fileSourceType);
    std::optional<std::string> GetStoragePath() const;
    MDKRecordPhotosData &SetStoragePath(const std::string &storagePath);
    std::optional<std::string> GetAttributeFieldValue(const std::string &fieldName) const;

public:  // properties getter & setter - gallery expand fields
    std::optional<std::string> GetSourcePath() const;
    MDKRecordPhotosData &SetSourcePath(const std::string &sourcePath);
    std::optional<std::string> GetSourceFileName() const;
    MDKRecordPhotosData &SetSourceFileName(const std::string &sourceFileName);
    std::optional<std::string> GetFirstUpdateTime() const;
    MDKRecordPhotosData &SetFirstUpdateTime(const std::string firstUpdateTime);
    std::optional<std::string> GetFileCreateTime() const;
    MDKRecordPhotosData &SetFileCreateTime(const std::string &fileCreateTime);
    std::optional<std::string> GetDetailTime() const;
    MDKRecordPhotosData &SetDetailTime(const std::string &detailTime);
    std::optional<int32_t> GetHeight() const;
    MDKRecordPhotosData &SetHeight(const int32_t &height);
    std::optional<int32_t> GetWidth() const;
    MDKRecordPhotosData &SetWidth(const int32_t &width);
    std::optional<std::string> GetPosition() const;
    MDKRecordPhotosData &SetPosition(const std::string &position);
    std::optional<int32_t> GetRotate() const;
    MDKRecordPhotosData &SetRotate(const int32_t &rotate);
    bool hasAttributes();
    bool hasProperties();

private:
    const std::string VALUE_RECORD_TYPE = "album";
    /* basic */
    const std::string KEY_PROPERTIES = "properties";
    const std::string KEY_TYPE = "type";
    const std::string KEY_ATTRIBUTES = "attributes";
    const std::string FILE_LOCAL_ID = "local_id";
    const std::string FILE_ID = "file_id";
    const std::string FILE_CONTENT = "content";
    const std::string FILE_RAW = "raw";
    const std::string FILE_THUMBNAIL = "thumbnail";
    const std::string FILE_LCD = "lcdThumbnail";
    const std::string FILE_EDIT_DATA = "editData";
    const std::string FILE_MIME_TYPE = "mimeType";
    const std::string FILE_EDIT_DATA_CAMERA = "editDataCamera";
    const std::string KEY_EDITED_TIME_MS = "editedTime_ms";
    const std::string KEY_LCD_SIZE = "lcd_size";
    const std::string KEY_THUMB_SIZE = "thumb_size";
    const std::string KEY_ROTATE = "rotate";
    const std::string KEY_POSITION = "position";
    const std::string KEY_FILE_TYPE = "fileType";
    const std::string KEY_FILE_NAME = "fileName";
    const std::string KEY_CREATED_TIME = "createdTime";
    const std::string KEY_HASH_ID = "hashId";
    const std::string KEY_SIZE = "size";
    const std::string KEY_SOURCE = "source";
    const std::string KEY_RECYCLED = "recycled";
    const std::string KEY_RECYCLED_TIME = "recycledTime";
    const std::string KEY_FAVORITE = "favorite";
    const std::string KEY_DESCRIPTION = "description";
    const std::string KEY_SOURCE_PATH = "sourcePath";
    const std::string KEY_SOURCE_FILE_NAME = "sourceFileName";
    const std::string KEY_FIRST_UPDATE_TIME = "first_update_time";
    const std::string KEY_FILE_CREATE_TIME = "fileCreateTime";
    const std::string KEY_DETAIL_TIME = "detail_time";
    const std::string KEY_HEIGHT = "height";
    const std::string KEY_WIDTH = "width";

    /* attributes */
    const std::string FILE_FIX_VERSION = "fix_version";
};
}  // namespace OHOS::Media::CloudSync
#endif  // OHOS_MEDIA_CLOUD_SYNC_MDK_RECORD_PHOTOS_DATA_H