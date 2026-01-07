/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "metadata.h"
#include "medialibrary_db_const.h"
#include "media_log.h"
#include "fetch_result.h"

namespace OHOS {
namespace Media {
using namespace std;
const MediaType FILE_MEDIA_TYPE_DEFAULT = MEDIA_TYPE_FILE;
const MediaType FILE_MEDIA_TYPE_MAX = MEDIA_TYPE_DEFAULT;

Metadata::Metadata()
    : id_(FILE_ID_DEFAULT),
    uri_(URI_DEFAULT),
    filePath_(FILE_PATH_DEFAULT),
    relativePath_(FILE_RELATIVE_PATH_DEFAULT),
    mimeType_(DEFAULT_FILE_MIME_TYPE),
    mediaType_(FILE_MEDIA_TYPE_DEFAULT),
    name_(FILE_NAME_DEFAULT),
    size_(FILE_SIZE_DEFAULT),
    dateModified_(FILE_DATE_MODIFIED_DEFAULT),
    dateAdded_(FILE_DATE_ADDED_DEFAULT),
    fileExt_(FILE_EXTENSION_DEFAULT),
    parentId_(FILE_ID_DEFAULT),
    title_(FILE_TITLE_DEFAULT),
    artist_(FILE_ARTIST_DEFAULT),
    album_(FILE_ALBUM_NAME_DEFAULT),
    height_(FILE_HEIGHT_DEFAULT),
    width_(FILE_WIDTH_DEFAULT),
    duration_(FILE_DURATION_DEFAULT),
    orientation_(FILE_ORIENTATION_DEFAULT),
    shootingMode_(FILE_SHOOTINGMODE_DEFAULT),
    lastVisitTime_(FILE_LAST_VISIT_TIME_DEFAULT),
    dynamicRangeType_(FILE_DYNAMIC_RANGE_TYPE_DEFAULT),
    hdrMode_(FILE_HDR_MODE_DEFAULT),
    aspectRatio_(FILE_ASPECT_RATIO_DEFAULT),
    dateTaken_(FILE_DATE_TAKEN_DEFAULT),
    longitude_(FILE_LONGITUDE_DEFAULT),
    latitude_(FILE_LATITUDE_DEFAULT),
    userComment_(FILE_USER_COMMENT_DEFAULT),
    allExif_(FILE_All_EXIF_DEFAULT),
    albumId_(FILE_ALBUM_ID_DEFAULT),
    albumName_(FILE_ALBUM_NAME_DEFAULT),
    recyclePath_(FILE_RECYCLE_PATH_DEFAULT),
    timePending_(FILE_TIME_PENDING_DEFAULT),
    isTemp_(FILE_IS_TEMP_DEFAULT),
    frontcamera_(FILE_FRONT_CAMERA_DEFAULT),
    detailTime_(FILE_DETAIL_TIME_DEFAULT), burstCoverLevel_(BURST_COVER_LEVEL_DEFAULT),
    stageVideoTaskStatus_(STAGE_VIDEO_TASK_STATUS),
    fileSourceType_(FILE_FILE_SOURCE_TYPE_DEFAULT),
    videoMode_(FILE_VIDEO_MODE_DEFAULT)
{
    Init();
}

void Metadata::InitV2()
{
    memberFuncMap_[PhotoColumn::MEDIA_FILE_PATH] = make_pair(ResultSetDataType::TYPE_STRING,
        &Metadata::SetMovingPhotoImagePath);
    memberFuncMap_[PhotoColumn::PHOTO_DETAIL_TIME] = make_pair(ResultSetDataType::TYPE_STRING,
        &Metadata::SetDetailTime);
    memberFuncMap_[PhotoColumn::PHOTO_BURST_COVER_LEVEL] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetBurstCoverLevel);
    memberFuncMap_[PhotoColumn::PHOTO_OWNER_ALBUM_ID] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetAlbumId);
    memberFuncMap_[PhotoColumn::STAGE_VIDEO_TASK_STATUS] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetStageVideoTaskStatus);
    memberFuncMap_[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetFileSourceType);
    memberFuncMap_[PhotoColumn::PHOTO_VIDEO_MODE] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetVideoMode);
}

void Metadata::Init()
{
    memberFuncMap_[MEDIA_DATA_DB_ID] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetFileId);
    memberFuncMap_[MEDIA_DATA_DB_URI] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetUri);
    memberFuncMap_[MEDIA_DATA_DB_FILE_PATH] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetFilePath);
    memberFuncMap_[MEDIA_DATA_DB_RELATIVE_PATH] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetRelativePath);
    memberFuncMap_[MEDIA_DATA_DB_MEDIA_TYPE] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetFileMediaType);
    memberFuncMap_[MEDIA_DATA_DB_MIME_TYPE] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetFileMimeType);
    memberFuncMap_[MEDIA_DATA_DB_NAME] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetFileName);
    memberFuncMap_[MEDIA_DATA_DB_SIZE] = make_pair(ResultSetDataType::TYPE_INT64, &Metadata::SetFileSize);
    memberFuncMap_[MEDIA_DATA_DB_DATE_ADDED] = make_pair(ResultSetDataType::TYPE_INT64, &Metadata::SetFileDateAdded);
    memberFuncMap_[MEDIA_DATA_DB_TITLE] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetFileTitle);
    memberFuncMap_[MEDIA_DATA_DB_ARTIST] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetFileArtist);
    memberFuncMap_[MEDIA_DATA_DB_AUDIO_ALBUM] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetAlbum);
    memberFuncMap_[MEDIA_DATA_DB_HEIGHT] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetFileHeight);
    memberFuncMap_[MEDIA_DATA_DB_WIDTH] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetFileWidth);
    memberFuncMap_[MEDIA_DATA_DB_ORIENTATION] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetOrientation);
    memberFuncMap_[MEDIA_DATA_DB_DURATION] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetFileDuration);
    memberFuncMap_[MEDIA_DATA_DB_LONGITUDE] = make_pair(ResultSetDataType::TYPE_DOUBLE, &Metadata::SetLongitude);
    memberFuncMap_[MEDIA_DATA_DB_LATITUDE] = make_pair(ResultSetDataType::TYPE_DOUBLE, &Metadata::SetLatitude);
    memberFuncMap_[MEDIA_DATA_DB_BUCKET_NAME] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetAlbumName);
    memberFuncMap_[MEDIA_DATA_DB_PARENT_ID] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetParentId);
    memberFuncMap_[MEDIA_DATA_DB_RECYCLE_PATH] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetRecyclePath);
    memberFuncMap_[MEDIA_DATA_DB_DATE_TAKEN] = make_pair(ResultSetDataType::TYPE_INT64, &Metadata::SetDateTaken);
    memberFuncMap_[MEDIA_DATA_DB_DATE_MODIFIED] = make_pair(ResultSetDataType::TYPE_INT64,
        &Metadata::SetFileDateModified);
    memberFuncMap_[MEDIA_DATA_DB_TIME_PENDING] = make_pair(ResultSetDataType::TYPE_INT64, &Metadata::SetTimePending);
    memberFuncMap_[PhotoColumn::PHOTO_USER_COMMENT] = make_pair(ResultSetDataType::TYPE_STRING,
        &Metadata::SetUserComment);
    memberFuncMap_[PhotoColumn::PHOTO_ALL_EXIF] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetAllExif);
    memberFuncMap_[PhotoColumn::PHOTO_DATE_YEAR] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetDateYear);
    memberFuncMap_[PhotoColumn::PHOTO_DATE_MONTH] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetDateMonth);
    memberFuncMap_[PhotoColumn::PHOTO_DATE_DAY] = make_pair(ResultSetDataType::TYPE_STRING, &Metadata::SetDateDay);
    memberFuncMap_[PhotoColumn::MEDIA_OWNER_PACKAGE] = make_pair(ResultSetDataType::TYPE_STRING,
        &Metadata::SetOwnerPackage);
    memberFuncMap_[PhotoColumn::PHOTO_SUBTYPE] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetPhotoSubType);
    memberFuncMap_[PhotoColumn::MOVING_PHOTO_EFFECT_MODE] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetMovingPhotoEffectMode);
    memberFuncMap_[PhotoColumn::PHOTO_DYNAMIC_RANGE_TYPE] = make_pair(ResultSetDataType::TYPE_INT32,
        &Metadata::SetDynamicRangeType);
    memberFuncMap_[PhotoColumn::PHOTO_IS_TEMP] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetIsTemp);
    memberFuncMap_[PhotoColumn::PHOTO_QUALITY] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetPhotoQuality);
    memberFuncMap_[PhotoColumn::PHOTO_DIRTY] = make_pair(ResultSetDataType::TYPE_INT32, &Metadata::SetDirty);
    memberFuncMap_[PhotoColumn::PHOTO_FRONT_CAMERA] = make_pair(ResultSetDataType::TYPE_STRING,
        &Metadata::SetFrontCamera);
    InitV2();
}

void Metadata::SetFileId(const VariantData &id)
{
    id_ = std::get<int32_t>(id);
}

int32_t Metadata::GetFileId() const
{
    return id_;
}

void Metadata::SetUri(const VariantData &uri)
{
    uri_ = std::get<string>(uri);
}

const std::string &Metadata::GetUri() const
{
    return uri_;
}

void Metadata::SetFilePath(const VariantData &filePath)
{
    filePath_ = std::get<string>(filePath);
}

const std::string &Metadata::GetFilePath() const
{
    return filePath_;
}

void Metadata::SetRelativePath(const VariantData &relativePath)
{
    relativePath_ = std::get<string>(relativePath);
}

const std::string &Metadata::GetRelativePath() const
{
    return relativePath_;
}

void Metadata::SetFileMimeType(const VariantData &mimeType)
{
    mimeType_ = std::get<string>(mimeType);
}

const std::string &Metadata::GetFileMimeType() const
{
    return mimeType_;
}

void Metadata::SetFileMediaType(const VariantData &mediaType)
{
    if (std::get<int32_t>(mediaType) < FILE_MEDIA_TYPE_DEFAULT || std::get<int32_t>(mediaType) > FILE_MEDIA_TYPE_MAX) {
        mediaType_ = FILE_MEDIA_TYPE_DEFAULT;
        MEDIA_ERR_LOG("Invalid media type %{public}d, set DEFAULT instead", std::get<int32_t>(mediaType));
        return;
    }
    mediaType_ = static_cast<MediaType>(std::get<int32_t>(mediaType));
}

MediaType Metadata::GetFileMediaType() const
{
    return mediaType_;
}

void Metadata::SetFileName(const VariantData &name)
{
    name_ = std::get<string>(name);
}

const std::string &Metadata::GetFileName() const
{
    return name_;
}

void Metadata::SetFileSize(const VariantData &size)
{
    size_ = std::get<int64_t>(size);
}

int64_t Metadata::GetFileSize() const
{
    return size_;
}

void Metadata::SetFileDateAdded(const VariantData &dateAdded)
{
    dateAdded_ = std::get<int64_t>(dateAdded);
}

int64_t Metadata::GetFileDateAdded() const
{
    return dateAdded_;
}

void Metadata::SetFileDateModified(const VariantData &dateModified)
{
    dateModified_ = std::get<int64_t>(dateModified);
}

int64_t Metadata::GetFileDateModified() const
{
    return dateModified_;
}

void Metadata::SetFileExtension(const VariantData &fileExt)
{
    fileExt_ = std::get<string>(fileExt);
}

const std::string &Metadata::GetFileExtension() const
{
    return fileExt_;
}

void Metadata::SetFileTitle(const VariantData &title)
{
    title_ = std::get<string>(title);
}

const std::string &Metadata::GetFileTitle() const
{
    return title_;
}

void Metadata::SetFileArtist(const VariantData &artist)
{
    artist_ = std::get<string>(artist);
}

const std::string &Metadata::GetFileArtist() const
{
    return artist_;
}

void Metadata::SetAlbum(const VariantData &album)
{
    album_ = std::get<string>(album);
}

const std::string &Metadata::GetAlbum() const
{
    return album_;
}

void Metadata::SetFileHeight(const VariantData &height)
{
    height_ = std::get<int32_t>(height);
}

int32_t Metadata::GetFileHeight() const
{
    return height_;
}

void Metadata::SetFileWidth(const VariantData &width)
{
    width_ = std::get<int32_t>(width);
}

int32_t Metadata::GetFileWidth() const
{
    return width_;
}

void Metadata::SetFileAspectRatio(const VariantData &aspectRatio)
{
    aspectRatio_ = std::get<double>(aspectRatio);
}

double Metadata::GetFileAspectRatio() const
{
    return aspectRatio_;
}

void Metadata::SetFileDuration(const VariantData &duration)
{
    duration_ = std::get<int32_t>(duration);
}

int32_t Metadata::GetFileDuration() const
{
    return duration_;
}

void Metadata::SetOrientation(const VariantData &orientation)
{
    orientation_ = std::get<int32_t>(orientation);
}

int32_t Metadata::GetOrientation() const
{
    return orientation_;
}

void Metadata::SetExifRotate(const VariantData &exifRotate)
{
    exifRotate_ = std::get<int32_t>(exifRotate);
}

int32_t Metadata::GetExifRotate() const
{
    return exifRotate_;
}

void Metadata::SetAlbumId(const VariantData &albumId)
{
    albumId_ = std::get<int32_t>(albumId);
}

int32_t Metadata::GetAlbumId() const
{
    return albumId_;
}

void Metadata::SetAlbumName(const VariantData &albumName)
{
    albumName_ = std::get<string>(albumName);
}

const std::string &Metadata::GetAlbumName() const
{
    return albumName_;
}

void Metadata::SetParentId(const VariantData &parentId)
{
    parentId_ = std::get<int32_t>(parentId);
}

int32_t Metadata::GetParentId() const
{
    return parentId_;
}

void Metadata::SetRecyclePath(const VariantData &recyclePath)
{
    recyclePath_ = std::get<string>(recyclePath);
}

const std::string &Metadata::GetRecyclePath() const
{
    return recyclePath_;
}

void Metadata::SetDateTaken(const VariantData &dateTaken)
{
    dateTaken_ = std::get<int64_t>(dateTaken);
}

int64_t Metadata::GetDateTaken() const
{
    return dateTaken_;
}

void Metadata::SetLongitude(const VariantData &longitude)
{
    longitude_ = std::get<double>(longitude);
}

double Metadata::GetLongitude() const
{
    return longitude_;
}

void Metadata::SetLatitude(const VariantData &latitude)
{
    latitude_ = std::get<double>(latitude);
}

double Metadata::GetLatitude() const
{
    return latitude_;
}

void Metadata::SetTimePending(const VariantData &timePending)
{
    timePending_ = std::get<int64_t>(timePending);
}

int64_t Metadata::GetTimePending() const
{
    return timePending_;
}

void Metadata::SetUserComment(const VariantData &userComment)
{
    userComment_ = get<string>(userComment);
}

const string &Metadata::GetUserComment() const
{
    return userComment_;
}

void Metadata::SetAllExif(const VariantData &allExif)
{
    allExif_ = get<string>(allExif);
}

const string &Metadata::GetAllExif() const
{
    return allExif_;
}

void Metadata::SetDateYear(const VariantData &dateYear)
{
    dateYear_ = get<string>(dateYear);
}

const string &Metadata::GetDateYear() const
{
    return dateYear_;
}

void Metadata::SetDateMonth(const VariantData &dateMonth)
{
    dateMonth_ = get<string>(dateMonth);
}

const string &Metadata::GetDateMonth() const
{
    return dateMonth_;
}

void Metadata::SetDateDay(const VariantData &dateDay)
{
    dateDay_ = get<string>(dateDay);
}

const string &Metadata::GetDateDay() const
{
    return dateDay_;
}

void Metadata::SetShootingMode(const VariantData &shootingMode)
{
    shootingMode_ = get<string>(shootingMode);
}

void Metadata::SetShootingModeTag(const VariantData &shootingModeTag)
{
    shootingModeTag_ = get<string>(shootingModeTag);
}

const string &Metadata::GetShootingMode() const
{
    return shootingMode_;
}

const string &Metadata::GetShootingModeTag() const
{
    return shootingModeTag_;
}

void Metadata::SetPhotoSubType(const VariantData &photoSubType)
{
    photoSubType_ = std::get<int32_t>(photoSubType);
}

int32_t Metadata::GetPhotoSubType() const
{
    return photoSubType_;
}

void Metadata::SetMovingPhotoEffectMode(const VariantData &movingPhotoEffectMode)
{
    movingPhotoEffectMode_ = std::get<int32_t>(movingPhotoEffectMode);
}

int32_t Metadata::GetMovingPhotoEffectMode() const
{
    return movingPhotoEffectMode_;
}

void Metadata::SetTableName(const string &tableName)
{
    tableName_ = tableName;
}

string Metadata::GetTableName()
{
    return tableName_;
}

void Metadata::SetForAdd(bool forAdd)
{
    forAdd_ = forAdd;
}

bool Metadata::GetForAdd() const
{
    return forAdd_;
}

void Metadata::SetLastVisitTime(const VariantData &lastVisitTime)
{
    lastVisitTime_ = std::get<int64_t>(lastVisitTime);
}

int64_t Metadata::GetLastVisitTime() const
{
    return lastVisitTime_;
}

void Metadata::SetOwnerPackage(const VariantData &ownerPackage)
{
    ownerPackage_ = get<string>(ownerPackage);
}

const std::string Metadata::GetOwnerPackage() const
{
    return ownerPackage_;
}

void Metadata::SetDynamicRangeType(const VariantData &type)
{
    dynamicRangeType_ = std::get<int32_t>(type);
}

int32_t Metadata::GetDynamicRangeType() const
{
    return dynamicRangeType_;
}

void Metadata::SetHdrMode(const VariantData &type)
{
    hdrMode_ = std::get<int32_t>(type);
}

int32_t Metadata::GetHdrMode() const
{
    return hdrMode_;
}

void Metadata::SetMovingPhotoImagePath(const VariantData &imagePath)
{
    movingPhotoImagePath_ = std::get<string>(imagePath);
}

std::string Metadata::GetMovingPhotoImagePath() const
{
    return movingPhotoImagePath_;
}

void Metadata::SetCoverPosition(const VariantData &coverPosition)
{
    coverPosition_ = std::get<int64_t>(coverPosition);
}

int64_t Metadata::GetCoverPosition() const
{
    return coverPosition_;
}

void Metadata::SetFrameIndex(const VariantData &frameIndex)
{
    frameIndex_ = std::get<int32_t>(frameIndex);
}

int32_t Metadata::GetFrameIndex() const
{
    return frameIndex_;
}

void Metadata::SetIsTemp(const VariantData &isTemp)
{
    isTemp_ = std::get<int32_t>(isTemp);
}

int32_t Metadata::GetIsTemp() const
{
    return isTemp_;
}

void Metadata::SetPhotoQuality(const VariantData &photoQuality)
{
    photoQuality_ = std::get<int32_t>(photoQuality);
}

int32_t Metadata::GetPhotoQuality() const
{
    return photoQuality_;
}

void Metadata::SetDirty(const VariantData &dirty)
{
    dirty_ = std::get<int32_t>(dirty);
}

int32_t Metadata::GetDirty() const
{
    return dirty_;
}

void Metadata::SetFrontCamera(const VariantData &frontcamera)
{
    frontcamera_ = std::get<string>(frontcamera);
}

std::string Metadata::GetFrontCamera() const
{
    return frontcamera_;
}

void Metadata::SetDetailTime(const VariantData &detailTime)
{
    detailTime_ = std::get<string>(detailTime);
}

std::string Metadata::GetDetailTime() const
{
    return detailTime_;
}

void Metadata::SetBurstCoverLevel(const VariantData &burstCoverLevel)
{
    burstCoverLevel_ = std::get<int32_t>(burstCoverLevel);
}

int32_t Metadata::GetBurstCoverLevel() const
{
    return burstCoverLevel_;
}

void Metadata::SetStageVideoTaskStatus(const VariantData &stageVideoTaskStatus)
{
    stageVideoTaskStatus_ = std::get<int32_t>(stageVideoTaskStatus);
}

int32_t Metadata::GetStageVideoTaskStatus() const
{
    return stageVideoTaskStatus_;
}

void Metadata::SetFileSourceType(const VariantData &fileSourceType)
{
    fileSourceType_ = std::get<int32_t>(fileSourceType);
}

int32_t Metadata::GetFileSourceType() const
{
    return fileSourceType_;
}

void Metadata::SetVideoMode(const VariantData &videoMode)
{
    videoMode_ = std::get<int32_t>(videoMode);
}

int32_t Metadata::GetVideoMode() const
{
    return videoMode_;
}
} // namespace Media
} // namespace OHOS
