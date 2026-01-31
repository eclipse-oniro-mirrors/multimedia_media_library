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
#define MLOG_TAG "Media_Client"

#include "mdk_record_photos_data.h"

#include <string>

#include "media_log.h"
#include "media_column.h"

namespace OHOS::Media::CloudSync {
MDKRecordPhotosData::MDKRecordPhotosData(const MDKRecord &record)
{
    this->UnMarshalling(record);
}
void MDKRecordPhotosData::UnMarshalling(const MDKRecord &record)
{
    this->record_ = record;
    this->record_.GetRecordData(this->fields_);
    if (this->fields_.find(this->KEY_PROPERTIES) != this->fields_.end()) {
        this->fields_[this->KEY_PROPERTIES].GetRecordMap(this->properties_);
    }
    if (this->fields_.find(this->KEY_ATTRIBUTES) != this->fields_.end()) {
        this->fields_[this->KEY_ATTRIBUTES].GetRecordMap(this->attributes_);
    }
}
void MDKRecordPhotosData::Marshalling()
{
    if (this->properties_.size() > 0) {
        this->fields_[this->KEY_PROPERTIES] = MDKRecordField(this->properties_);
    }
    if (this->attributes_.size() > 0) {
        this->fields_[this->KEY_ATTRIBUTES] = MDKRecordField(this->attributes_);
    }
    this->record_.SetRecordData(this->fields_);
    this->record_.SetRecordType(this->VALUE_RECORD_TYPE);
}

MDKRecord MDKRecordPhotosData::GetDKRecord()
{
    this->Marshalling();
    return this->record_;
}
void MDKRecordPhotosData::SetDKRecord(MDKRecord &record)
{
    this->record_ = record;
    record.GetRecordData(this->fields_);
}
std::optional<std::string> MDKRecordPhotosData::GetType() const
{
    return this->recordReader_.GetStringValue(this->fields_, this->KEY_TYPE);
}
// type, "directory" or "file"
void MDKRecordPhotosData::SetType(const std::string &type)
{
    this->fields_[this->KEY_TYPE] = MDKRecordField(type);
}

std::optional<int32_t> MDKRecordPhotosData::GetFileId() const
{
    return this->recordReader_.GetIntValue(this->attributes_, FILE_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetFileId(const int32_t fileId)
{
    this->attributes_[FILE_ID] = MDKRecordField(fileId);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetLocalId() const
{
    return this->recordReader_.GetIntValue(this->fields_, FILE_LOCAL_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetLocalId(const int32_t localId)
{
    this->fields_[FILE_LOCAL_ID] = MDKRecordField(localId);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetFileType() const
{
    return this->recordReader_.GetIntValue(this->fields_, KEY_FILE_TYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetFileType(const int32_t &fileType)
{
    this->fields_[KEY_FILE_TYPE] = MDKRecordField(fileType);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetFileName() const
{
    return this->recordReader_.GetStringValue(this->fields_, KEY_FILE_NAME);
}
void MDKRecordPhotosData::SetFileName(const std::string &fileName)
{
    this->fields_[KEY_FILE_NAME] = MDKRecordField(fileName);
}
std::optional<int64_t> MDKRecordPhotosData::GetCreatedTime() const
{
    return this->recordReader_.GetLongValue(this->fields_, KEY_CREATED_TIME);
}
void MDKRecordPhotosData::SetCreatedTime(const int64_t &createdTime)
{
    this->fields_[KEY_CREATED_TIME] = MDKRecordField(createdTime);
}
std::optional<std::string> MDKRecordPhotosData::GetHashId() const
{
    return this->recordReader_.GetStringValue(this->fields_, KEY_HASH_ID);
}
void MDKRecordPhotosData::SetHashId(const std::string &hashId)
{
    this->fields_[KEY_HASH_ID] = MDKRecordField(hashId);
}
std::optional<int64_t> MDKRecordPhotosData::GetSize() const
{
    return this->recordReader_.GetLongValue(this->fields_, KEY_SIZE);
}
void MDKRecordPhotosData::SetSize(const int64_t &size)
{
    this->fields_[KEY_SIZE] = MDKRecordField(size);
}
std::optional<std::string> MDKRecordPhotosData::GetSource() const
{
    return this->recordReader_.GetStringValue(this->fields_, KEY_SOURCE);
}
void MDKRecordPhotosData::SetSource(const std::string &source)
{
    this->fields_[KEY_SOURCE] = MDKRecordField(source);
}
std::optional<bool> MDKRecordPhotosData::GetRecycled() const
{
    return this->recordReader_.GetBoolValue(this->fields_, KEY_RECYCLED);
}
void MDKRecordPhotosData::SetRecycled(const bool &recycled)
{
    this->fields_[KEY_RECYCLED] = MDKRecordField(recycled);
}
std::optional<int64_t> MDKRecordPhotosData::GetRecycledTime() const
{
    return this->recordReader_.GetLongValue(this->fields_, KEY_RECYCLED_TIME);
}
void MDKRecordPhotosData::SetRecycledTime(const int64_t &recycledTime)
{
    this->fields_[KEY_RECYCLED_TIME] = MDKRecordField(recycledTime);
}
std::optional<bool> MDKRecordPhotosData::GetFavorite() const
{
    return this->recordReader_.GetBoolValue(this->fields_, KEY_FAVORITE);
}
void MDKRecordPhotosData::SetFavorite(const bool &favorite)
{
    this->fields_[KEY_FAVORITE] = MDKRecordField(favorite);
}
std::optional<std::string> MDKRecordPhotosData::GetDescription() const
{
    return this->recordReader_.GetStringValue(this->fields_, KEY_DESCRIPTION);
}
void MDKRecordPhotosData::SetDescription(const std::string &description)
{
    this->fields_[KEY_DESCRIPTION] = MDKRecordField(description);
}
std::optional<std::string> MDKRecordPhotosData::GetMimeType() const
{
    return this->recordReader_.GetStringValue(this->fields_, FILE_MIME_TYPE);
}
void MDKRecordPhotosData::SetMimeType(const std::string &mimeType)
{
    this->fields_[MediaColumn::MEDIA_MIME_TYPE] = MDKRecordField(mimeType);
}
std::optional<MDKAsset> MDKRecordPhotosData::GetFileContent() const
{
    return this->recordReader_.GetAssetValue(this->fields_, FILE_CONTENT);
}
void MDKRecordPhotosData::SetFileContent(const MDKAsset &asset)
{
    this->fields_[FILE_CONTENT] = MDKRecordField(asset);
}
std::optional<MDKAsset> MDKRecordPhotosData::GetFileRaw() const
{
    return this->recordReader_.GetAssetValue(this->fields_, FILE_RAW);
}
void MDKRecordPhotosData::SetFileRaw(const MDKAsset &asset)
{
    this->fields_[FILE_RAW] = MDKRecordField(asset);
}
std::optional<MDKAsset> MDKRecordPhotosData::GetFileEditData() const
{
    return this->recordReader_.GetAssetValue(this->fields_, FILE_EDIT_DATA);
}
void MDKRecordPhotosData::SetFileEditData(const MDKAsset &asset)
{
    this->fields_[FILE_EDIT_DATA] = MDKRecordField(asset);
}

std::optional<std::string> MDKRecordPhotosData::GetTitle() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::MEDIA_TITLE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetTitle(const std::string &title)
{
    this->attributes_[PhotoColumn::MEDIA_TITLE] = MDKRecordField(title);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetMediaType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::MEDIA_TYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetMediaType(const int32_t mediaType)
{
    this->attributes_[PhotoColumn::MEDIA_TYPE] = MDKRecordField(mediaType);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetDuration() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::MEDIA_DURATION);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDuration(const int32_t duration)
{
    this->attributes_[PhotoColumn::MEDIA_DURATION] = MDKRecordField(duration);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetPropertiesDuration() const
{
    return this->recordReader_.GetIntValue(this->properties_, PhotoColumn::MEDIA_DURATION);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetPropertiesDuration(const int32_t duration)
{
    this->properties_[PhotoColumn::MEDIA_DURATION] = MDKRecordField(duration);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetHidden() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::MEDIA_HIDDEN);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetHidden(const int32_t hidden)
{
    this->attributes_[PhotoColumn::MEDIA_HIDDEN] = MDKRecordField(hidden);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetHiddenTime() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::PHOTO_HIDDEN_TIME);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetHiddenTime(const int64_t hiddenTime)
{
    this->attributes_[PhotoColumn::PHOTO_HIDDEN_TIME] = MDKRecordField(hiddenTime);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetRelativePath() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::MEDIA_RELATIVE_PATH);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetRelativePath(const std::string &relativePath)
{
    this->attributes_[PhotoColumn::MEDIA_RELATIVE_PATH] = MDKRecordField(relativePath);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetVirtualPath() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::MEDIA_VIRTUAL_PATH);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetVirtualPath(const std::string &virtualPath)
{
    this->attributes_[PhotoColumn::MEDIA_VIRTUAL_PATH] = MDKRecordField(virtualPath);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetDateModified() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::MEDIA_DATE_MODIFIED);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDateModified(const int64_t dateModified)
{
    this->attributes_[PhotoColumn::MEDIA_DATE_MODIFIED] = MDKRecordField(dateModified);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetPhotoMetaDateModified() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::PHOTO_META_DATE_MODIFIED);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetPhotoMetaDateModified(const int64_t dateModified)
{
    this->attributes_[PhotoColumn::PHOTO_META_DATE_MODIFIED] = MDKRecordField(dateModified);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetSubType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_SUBTYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetSubType(const int32_t subType)
{
    this->attributes_[PhotoColumn::PHOTO_SUBTYPE] = MDKRecordField(subType);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetBurstCoverLevel() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_BURST_COVER_LEVEL);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetBurstCoverLevel(const int32_t burstCoverLevel)
{
    this->attributes_[PhotoColumn::PHOTO_BURST_COVER_LEVEL] = MDKRecordField(burstCoverLevel);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetBurstKey() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_BURST_KEY);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetBurstKey(const std::string &burstKey)
{
    this->attributes_[PhotoColumn::PHOTO_BURST_KEY] = MDKRecordField(burstKey);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetDateYear() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_DATE_YEAR);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDateYear(const std::string &dateYear)
{
    this->attributes_[PhotoColumn::PHOTO_DATE_YEAR] = MDKRecordField(dateYear);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetDateMonth() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_DATE_MONTH);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDateMonth(const std::string &dateMonth)
{
    this->attributes_[PhotoColumn::PHOTO_DATE_MONTH] = MDKRecordField(dateMonth);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetDateDay() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_DATE_DAY);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDateDay(const std::string &dateDay)
{
    this->attributes_[PhotoColumn::PHOTO_DATE_DAY] = MDKRecordField(dateDay);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetShootingMode() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_SHOOTING_MODE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetShootingMode(const std::string &shootingMode)
{
    this->attributes_[PhotoColumn::PHOTO_SHOOTING_MODE] = MDKRecordField(shootingMode);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetShootingModeTag() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_SHOOTING_MODE_TAG);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetShootingModeTag(const std::string &shootingModeTag)
{
    this->attributes_[PhotoColumn::PHOTO_SHOOTING_MODE_TAG] = MDKRecordField(shootingModeTag);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetDynamicRangeType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDynamicRangeType(const int32_t dynamicRangeType)
{
    this->attributes_[PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE] = MDKRecordField(dynamicRangeType);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetHdrMode() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_HDR_MODE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetHdrMode(const int32_t hdrMode)
{
    this->attributes_[PhotoColumn::PHOTO_HDR_MODE] = MDKRecordField(hdrMode);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetVideoMode() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_VIDEO_MODE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetVideoMode(const int32_t videoMode)
{
    this->attributes_[PhotoColumn::PHOTO_VIDEO_MODE] = MDKRecordField(videoMode);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetFrontCamera() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_FRONT_CAMERA);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetFrontCamera(const std::string &frontCamera)
{
    this->attributes_[PhotoColumn::PHOTO_FRONT_CAMERA] = MDKRecordField(frontCamera);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetEditTime() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::PHOTO_EDIT_TIME);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetEditTime(const int64_t editTime)
{
    this->attributes_[PhotoColumn::PHOTO_EDIT_TIME] = MDKRecordField(editTime);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetOriginalSubType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_ORIGINAL_SUBTYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetOriginalSubType(const int32_t originalSubType)
{
    this->attributes_[PhotoColumn::PHOTO_ORIGINAL_SUBTYPE] = MDKRecordField(originalSubType);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetCoverPosition() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::PHOTO_COVER_POSITION);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetCoverPosition(const int64_t coverPosition)
{
    this->attributes_[PhotoColumn::PHOTO_COVER_POSITION] = MDKRecordField(coverPosition);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetIsRectificationCover() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_IS_RECTIFICATION_COVER);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetIsRectificationCover(const int32_t isRectificationCover)
{
    this->attributes_[PhotoColumn::PHOTO_IS_RECTIFICATION_COVER] = MDKRecordField(isRectificationCover);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetExifRotate() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_EXIF_ROTATE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetExifRotate(const int32_t exifRotate)
{
    this->attributes_[PhotoColumn::PHOTO_EXIF_ROTATE] = MDKRecordField(exifRotate);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetMovingPhotoEffectMode() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetMovingPhotoEffectMode(const int32_t movingPhotoEffectMode)
{
    this->attributes_[PhotoColumn::MOVING_PHOTO_EFFECT_MODE] = MDKRecordField(movingPhotoEffectMode);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetSupportedWatermarkType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::SUPPORTED_WATERMARK_TYPE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetSupportedWatermarkType(const int32_t supportedWatermarkType)
{
    this->attributes_[PhotoColumn::SUPPORTED_WATERMARK_TYPE] = MDKRecordField(supportedWatermarkType);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetStrongAssociation() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_STRONG_ASSOCIATION);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetStrongAssociation(const int32_t strongAssociation)
{
    this->attributes_[PhotoColumn::PHOTO_STRONG_ASSOCIATION] = MDKRecordField(strongAssociation);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetCloudFileId() const
{
    return this->recordReader_.GetIntValue(this->attributes_, MediaColumn::MEDIA_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetCloudFileId(const int32_t fileId)
{
    this->attributes_[MediaColumn::MEDIA_ID] = MDKRecordField(fileId);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetCloudId() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_CLOUD_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetCloudId(const std::string &cloudId)
{
    this->attributes_[PhotoColumn::PHOTO_CLOUD_ID] = MDKRecordField(cloudId);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetOriginalAssetCloudId() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetOriginalAssetCloudId(const std::string &originalAssetCloudId)
{
    this->attributes_[PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID] = MDKRecordField(originalAssetCloudId);
    return *this;
}
std::optional<std::string> MDKRecordPhotosData::GetFilePath() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::MEDIA_FILE_PATH);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetFilePath(const std::string &filePath)
{
    this->attributes_[PhotoColumn::MEDIA_FILE_PATH] = MDKRecordField(filePath);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetDateAdded() const
{
    return this->recordReader_.GetLongValue(this->attributes_, PhotoColumn::MEDIA_DATE_ADDED);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetDateAdded(const int64_t dateAdded)
{
    this->attributes_[PhotoColumn::MEDIA_DATE_ADDED] = MDKRecordField(dateAdded);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetOwnerAlbumId() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_OWNER_ALBUM_ID);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetOwnerAlbumId(const int32_t ownerAlbumId)
{
    this->attributes_[PhotoColumn::PHOTO_OWNER_ALBUM_ID] = MDKRecordField(ownerAlbumId);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetFixVersion() const
{
    return this->recordReader_.GetLongValue(this->attributes_, FILE_FIX_VERSION);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetFixVersion(const int64_t fixVersion)
{
    this->attributes_[FILE_FIX_VERSION] = MDKRecordField(fixVersion);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetLcdSize() const
{
    return this->recordReader_.GetLongValue(this->attributes_, MDKRecordPhotosData::KEY_LCD_SIZE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetLcdSize(const int64_t lcdSize)
{
    this->attributes_[MDKRecordPhotosData::KEY_LCD_SIZE] = MDKRecordField(lcdSize);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetThmSize() const
{
    return this->recordReader_.GetLongValue(this->attributes_, MDKRecordPhotosData::KEY_THUMB_SIZE);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetThmSize(const int64_t thmSize)
{
    this->attributes_[MDKRecordPhotosData::KEY_THUMB_SIZE] = MDKRecordField(thmSize);
    return *this;
}
std::optional<int64_t> MDKRecordPhotosData::GetEditTimeMs() const
{
    return this->recordReader_.GetLongValue(this->attributes_, MDKRecordPhotosData::KEY_EDIT_TIME_MS);
}
void MDKRecordPhotosData::SetEditTimeMs(int64_t editedTimeMs)
{
    this->attributes_[MDKRecordPhotosData::KEY_EDIT_TIME_MS] = MDKRecordField(editedTimeMs);
}
std::optional<std::string> MDKRecordPhotosData::GetEditDataCamera() const
{
    return this->recordReader_.GetStringValue(this->attributes_, MDKRecordPhotosData::FILE_EDIT_DATA_CAMERA);
}
MDKRecordPhotosData &MDKRecordPhotosData::SetEditDataCamera(const std::string &editDataCamera)
{
    this->attributes_[MDKRecordPhotosData::FILE_EDIT_DATA_CAMERA] = MDKRecordField(editDataCamera);
    return *this;
}
std::optional<int32_t> MDKRecordPhotosData::GetFileSourceType() const
{
    return this->recordReader_.GetIntValue(this->attributes_, PhotoColumn::PHOTO_FILE_SOURCE_TYPE);
}
void MDKRecordPhotosData::SetFileSourceType(int32_t fileSourceType)
{
    this->attributes_[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = MDKRecordField(fileSourceType);
}
std::optional<std::string> MDKRecordPhotosData::GetStoragePath() const
{
    return this->recordReader_.GetStringValue(this->attributes_, PhotoColumn::PHOTO_STORAGE_PATH);
}
void MDKRecordPhotosData::SetStoragePath(const std::string &storagePath)
{
    this->attributes_[PhotoColumn::PHOTO_STORAGE_PATH] = MDKRecordField(storagePath);
}

std::optional<std::string> MDKRecordPhotosData::GetSourcePath() const
{
    return this->recordReader_.GetStringValue(this->properties_, KEY_SOURCE_PATH);
}
void MDKRecordPhotosData::SetSourcePath(const std::string &sourcePath)
{
    this->properties_[KEY_SOURCE_PATH] = MDKRecordField(sourcePath);
}
std::optional<std::string> MDKRecordPhotosData::GetSourceFileName() const
{
    return this->recordReader_.GetStringValue(this->properties_, KEY_SOURCE_FILE_NAME);
}
void MDKRecordPhotosData::SetSourceFileName(const std::string &sourceFileName)
{
    this->properties_[KEY_SOURCE_FILE_NAME] = MDKRecordField(sourceFileName);
}
std::optional<std::string> MDKRecordPhotosData::GetFirstUpdateTime() const
{
    return this->recordReader_.GetStringValue(this->properties_, KEY_FIRST_UPDATE_TIME);
}
void MDKRecordPhotosData::SetFirstUpdateTime(const std::string firstUpdateTime)
{
    this->properties_[KEY_FIRST_UPDATE_TIME] = MDKRecordField(firstUpdateTime);
}
std::optional<std::string> MDKRecordPhotosData::GetFileCreateTime() const
{
    return this->recordReader_.GetStringValue(this->properties_, KEY_FILE_CREATE_TIME);
}
void MDKRecordPhotosData::SetFileCreateTime(const std::string &fileCreateTime)
{
    this->properties_[KEY_FILE_CREATE_TIME] = MDKRecordField(fileCreateTime);
}
std::optional<std::string> MDKRecordPhotosData::GetDetailTime() const
{
    return this->recordReader_.GetStringValue(this->properties_, KEY_DETAIL_TIME);
}
void MDKRecordPhotosData::SetDetailTime(const std::string &detailTime)
{
    this->properties_[KEY_DETAIL_TIME] = MDKRecordField(detailTime);
}
std::optional<int32_t> MDKRecordPhotosData::GetHeight() const
{
    return this->recordReader_.GetIntValue(this->properties_, KEY_HEIGHT);
}
void MDKRecordPhotosData::SetHeight(const int32_t &height)
{
    this->properties_[KEY_HEIGHT] = MDKRecordField(height);
}
std::optional<int32_t> MDKRecordPhotosData::GetWidth() const
{
    return this->recordReader_.GetIntValue(this->properties_, KEY_WIDTH);
}
void MDKRecordPhotosData::SetWidth(const int32_t &width)
{
    this->properties_[KEY_WIDTH] = MDKRecordField(width);
}

std::optional<std::string> MDKRecordPhotosData::GetPosition() const
{
    return this->recordReader_.GetStringValue(this->properties_, MDKRecordPhotosData::KEY_POSITION);
}

void MDKRecordPhotosData::SetPosition(const std::string &position)
{
    this->properties_[MDKRecordPhotosData::KEY_POSITION] = MDKRecordField(position);
}

std::optional<int32_t> MDKRecordPhotosData::GetRotate() const
{
    return this->recordReader_.GetIntValue(this->properties_, MDKRecordPhotosData::KEY_ROTATE);
}
void MDKRecordPhotosData::SetRotate(const int32_t &rotate)
{
    this->properties_[MDKRecordPhotosData::KEY_ROTATE] = MDKRecordField(rotate);
}

bool MDKRecordPhotosData::hasAttributes()
{
    return !this->attributes_.empty();
}

bool MDKRecordPhotosData::hasProperties()
{
    return !this->properties_.empty();
}

std::optional<std::string> MDKRecordPhotosData::GetAttributeFieldValue(const std::string &fieldName) const
{
    return this->recordReader_.GetStringValue(this->attributes_, fieldName);
}
}  // namespace OHOS::Media::CloudSync