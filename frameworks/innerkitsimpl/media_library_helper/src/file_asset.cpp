/*
 * Copyright (C) 2021-2025 Huawei Device Co., Ltd.
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
#define MLOG_TAG "FileAsset"

#include "file_asset.h"

#include <nlohmann/json.hpp>

#include "media_exif.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "sandbox_helper.h"

using namespace std;

namespace OHOS {
namespace Media {
static constexpr int MAP_INT_MAX = 50;
using json = nlohmann::json;
FileAsset::FileAsset()
    : albumUri_(DEFAULT_MEDIA_ALBUM_URI), resultNapiType_(ResultNapiType::TYPE_NAPI_MAX)
{
    member_.reserve(MAP_INT_MAX);
}

int32_t FileAsset::GetId() const
{
    return GetInt32Member(MEDIA_DATA_DB_ID);
}

void FileAsset::SetId(int32_t id)
{
    member_[MEDIA_DATA_DB_ID] = id;
}

int32_t FileAsset::GetCount() const
{
    return GetInt32Member(MEDIA_DATA_DB_COUNT);
}

void FileAsset::SetCount(int32_t count)
{
    member_[MEDIA_DATA_DB_COUNT] = count;
}

const string &FileAsset::GetUri() const
{
    return GetStrMember(MEDIA_DATA_DB_URI);
}

void FileAsset::SetUri(const string &uri)
{
    member_[MEDIA_DATA_DB_URI] = uri;
}

const string &FileAsset::GetPath() const
{
    return GetStrMember(MEDIA_DATA_DB_FILE_PATH);
}

void FileAsset::SetPath(const string &path)
{
    member_[MEDIA_DATA_DB_FILE_PATH] = path;
}

const string &FileAsset::GetRelativePath() const
{
    return GetStrMember(MEDIA_DATA_DB_RELATIVE_PATH);
}

void FileAsset::SetRelativePath(const string &relativePath)
{
    member_[MEDIA_DATA_DB_RELATIVE_PATH] = relativePath;
}

const string &FileAsset::GetMimeType() const
{
    return GetStrMember(MEDIA_DATA_DB_MIME_TYPE);
}

void FileAsset::SetMimeType(const string &mimeType)
{
    member_[MEDIA_DATA_DB_MIME_TYPE] = mimeType;
}

MediaType FileAsset::GetMediaType() const
{
    return static_cast<Media::MediaType>(GetInt32Member(MEDIA_DATA_DB_MEDIA_TYPE));
}

void FileAsset::SetMediaType(MediaType mediaType)
{
    member_[MEDIA_DATA_DB_MEDIA_TYPE] = mediaType;
}

const string &FileAsset::GetDisplayName() const
{
    return GetStrMember(MEDIA_DATA_DB_NAME);
}

void FileAsset::SetDisplayName(const string &displayName)
{
    member_[MEDIA_DATA_DB_NAME] = displayName;
}

int64_t FileAsset::GetSize() const
{
    return GetInt64Member(MEDIA_DATA_DB_SIZE);
}

void FileAsset::SetSize(int64_t size)
{
    member_[MEDIA_DATA_DB_SIZE] = size;
}

const string &FileAsset::GetCloudId() const
{
    return GetStrMember(PhotoColumn::PHOTO_CLOUD_ID);
}

void FileAsset::SetCloudId(const string &cloudId)
{
    member_[PhotoColumn::PHOTO_CLOUD_ID] = cloudId;
}

int64_t FileAsset::GetDateAdded() const
{
    return GetInt64Member(MEDIA_DATA_DB_DATE_ADDED);
}

void FileAsset::SetDateAdded(int64_t dateAdded)
{
    member_[MEDIA_DATA_DB_DATE_ADDED] = dateAdded;
}

int64_t FileAsset::GetDateModified() const
{
    return GetInt64Member(MEDIA_DATA_DB_DATE_MODIFIED);
}

void FileAsset::SetDateModified(int64_t dateModified)
{
    member_[MEDIA_DATA_DB_DATE_MODIFIED] = dateModified;
}

const string &FileAsset::GetTitle() const
{
    return GetStrMember(MEDIA_DATA_DB_TITLE);
}

void FileAsset::SetTitle(const string &title)
{
    member_[MEDIA_DATA_DB_TITLE] = title;
}

const string &FileAsset::GetArtist() const
{
    return GetStrMember(MEDIA_DATA_DB_ARTIST);
}

void FileAsset::SetArtist(const string &artist)
{
    member_[MEDIA_DATA_DB_ARTIST] = artist;
}

const string &FileAsset::GetAlbum() const
{
    return GetStrMember(MEDIA_DATA_DB_ALBUM);
}

void FileAsset::SetAlbum(const string &album)
{
    member_[MEDIA_DATA_DB_ALBUM] = album;
}

int32_t FileAsset::GetPosition() const
{
    return GetInt32Member(MEDIA_DATA_DB_POSITION);
}

void FileAsset::SetPosition(int32_t position)
{
    member_[MEDIA_DATA_DB_POSITION] = position;
}

int32_t FileAsset::GetWidth() const
{
    return GetInt32Member(MEDIA_DATA_DB_WIDTH);
}

void FileAsset::SetWidth(int32_t width)
{
    member_[MEDIA_DATA_DB_WIDTH] = width;
}

int32_t FileAsset::GetHeight() const
{
    return GetInt32Member(MEDIA_DATA_DB_HEIGHT);
}

void FileAsset::SetHeight(int32_t height)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_HEIGHT] = height;
}

double FileAsset::GetAspectRatio()
{
    return GetDoubleMember(MEDIA_DATA_DB_ASPECT_RATIO);
}

void FileAsset::SetAspectRatio(double aspectRatio)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_ASPECT_RATIO] = aspectRatio;
}

int32_t FileAsset::GetDuration() const
{
    return GetInt32Member(MEDIA_DATA_DB_DURATION);
}

void FileAsset::SetDuration(int32_t duration)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_DURATION] = duration;
}

int32_t FileAsset::GetOrientation() const
{
    return GetInt32Member(MEDIA_DATA_DB_ORIENTATION);
}

void FileAsset::SetOrientation(int32_t orientation)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_ORIENTATION] = orientation;
}

int32_t FileAsset::GetAlbumId() const
{
    return GetInt32Member(MEDIA_DATA_DB_BUCKET_ID);
}

void FileAsset::SetAlbumId(int32_t albumId)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_BUCKET_ID] = albumId;
}

int32_t FileAsset::GetOwnerAlbumId() const
{
    return GetInt32Member(PhotoColumn::PHOTO_OWNER_ALBUM_ID);
}

void FileAsset::SetOwnerAlbumId(int32_t ownerAlbumId)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_OWNER_ALBUM_ID] = ownerAlbumId;
}

const string &FileAsset::GetAlbumName() const
{
    return GetStrMember(MEDIA_DATA_DB_BUCKET_NAME);
}

void FileAsset::SetAlbumName(const string &albumName)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_BUCKET_NAME] = albumName;
}

int32_t FileAsset::GetParent() const
{
    return GetInt32Member(MEDIA_DATA_DB_PARENT_ID);
}

void FileAsset::SetParent(int32_t parent)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_PARENT_ID] = parent;
}

const string &FileAsset::GetAlbumUri() const
{
    return albumUri_;
}

void FileAsset::SetAlbumUri(const string &albumUri)
{
    albumUri_ = albumUri;
}

int64_t FileAsset::GetDateTaken() const
{
    return GetInt64Member(MEDIA_DATA_DB_DATE_TAKEN);
}

void FileAsset::SetDateTaken(int64_t dateTaken)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_DATE_TAKEN] = dateTaken;
}

int64_t FileAsset::GetTimePending() const
{
    return GetInt64Member(MEDIA_DATA_DB_TIME_PENDING);
}

void FileAsset::SetTimePending(int64_t timePending)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_TIME_PENDING] = timePending;
}

int32_t FileAsset::GetVisitCount() const
{
    return GetInt32Member(PhotoColumn::PHOTO_VISIT_COUNT);
}

void FileAsset::SetVisitCount(int32_t visitCount)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_VISIT_COUNT] = visitCount;
}

int32_t FileAsset::GetLcdVisitCount() const
{
    return GetInt32Member(PhotoColumn::PHOTO_LCD_VISIT_COUNT);
}

void FileAsset::SetLcdVisitCount(int32_t lcdVisitCount)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_LCD_VISIT_COUNT] = lcdVisitCount;
}

bool FileAsset::IsFavorite() const
{
    return GetInt32Member(MEDIA_DATA_DB_IS_FAV);
}

void FileAsset::SetFavorite(bool isFavorite)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_IS_FAV] = isFavorite;
}

bool FileAsset::IsRecentShow() const
{
    return GetInt32Member(PhotoColumn::PHOTO_IS_RECENT_SHOW);
}

void FileAsset::SetRecentShow(bool isRecentShow)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_IS_RECENT_SHOW] = isRecentShow;
}

double FileAsset::GetLatitude()
{
    return GetDoubleMember(MEDIA_DATA_DB_LATITUDE);
}

void FileAsset::SetLatitude(double latitude)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_LATITUDE] = latitude;
}

double FileAsset::GetLongitude()
{
    return GetDoubleMember(MEDIA_DATA_DB_LONGITUDE);
}

void FileAsset::SetLongitude(double longitude)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_LONGITUDE] = longitude;
}

void FileAsset::SetPhotoId(const string &photoId)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_PHOTO_ID] = photoId;
}

string FileAsset::GetPhotoId() const
{
    return GetStrMember(MEDIA_DATA_DB_PHOTO_ID);
}

void FileAsset::SetPhotoIdAndQuality(const string &photoId, int photoQuality)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_PHOTO_ID] = photoId;
    member_[MEDIA_DATA_DB_PHOTO_QUALITY] = photoQuality;
}

pair<string, int> FileAsset::GetPhotoIdAndQuality() const
{
    return make_pair(GetStrMember(MEDIA_DATA_DB_PHOTO_ID), GetInt32Member(MEDIA_DATA_DB_PHOTO_QUALITY));
}

int64_t FileAsset::GetDateTrashed() const
{
    return GetInt64Member(MEDIA_DATA_DB_DATE_TRASHED);
}

void FileAsset::SetDateTrashed(int64_t dateTrashed)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_DATE_TRASHED] = dateTrashed;
}

const string &FileAsset::GetSelfId() const
{
    return GetStrMember(MEDIA_DATA_DB_SELF_ID);
}

void FileAsset::SetSelfId(const string &selfId)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_SELF_ID] = selfId;
}

int32_t FileAsset::GetIsTrash() const
{
    bool cond = (resultNapiType_ == ResultNapiType::TYPE_USERFILE_MGR ||
        resultNapiType_ == ResultNapiType::TYPE_PHOTOACCESS_HELPER);
    CHECK_AND_RETURN_RET(!cond, static_cast<int32_t>(GetInt64Member(MediaColumn::MEDIA_DATE_TRASHED)));
    return GetInt32Member(MEDIA_DATA_DB_IS_TRASH);
}

void FileAsset::SetIsTrash(int32_t isTrash)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_IS_TRASH] = isTrash;
}

const string &FileAsset::GetRecyclePath() const
{
    return GetStrMember(MEDIA_DATA_DB_RECYCLE_PATH);
}

void FileAsset::SetRecyclePath(const string &recyclePath)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_RECYCLE_PATH] = recyclePath;
}

const string FileAsset::GetOwnerPackage() const
{
    return GetStrMember(MEDIA_DATA_DB_OWNER_PACKAGE);
}

void FileAsset::SetOwnerPackage(const string &ownerPackage)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_OWNER_PACKAGE] = ownerPackage;
}

const string FileAsset::GetOwnerAppId() const
{
    return GetStrMember(MEDIA_DATA_DB_OWNER_APPID);
}

void FileAsset::SetOwnerAppId(const string &ownerAppId)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MEDIA_DATA_DB_OWNER_APPID] = ownerAppId;
}

ResultNapiType FileAsset::GetResultNapiType() const
{
    return resultNapiType_;
}

const string FileAsset::GetPackageName() const
{
    return GetStrMember(MediaColumn::MEDIA_PACKAGE_NAME);
}

void FileAsset::SetPackageName(const string &packageName)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MediaColumn::MEDIA_PACKAGE_NAME] = packageName;
}

void FileAsset::SetResultNapiType(const ResultNapiType type)
{
    resultNapiType_ = type;
}

int32_t FileAsset::GetPhotoSubType() const
{
    return GetInt32Member(PhotoColumn::PHOTO_SUBTYPE);
}

void FileAsset::SetPhotoSubType(int32_t photoSubType)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_SUBTYPE] = photoSubType;
}

int32_t FileAsset::GetOriginalSubType() const
{
    return GetInt32Member(PhotoColumn::PHOTO_ORIGINAL_SUBTYPE);
}

const std::string &FileAsset::GetCameraShotKey() const
{
    return GetStrMember(PhotoColumn::CAMERA_SHOT_KEY);
}

void FileAsset::SetCameraShotKey(const std::string &cameraShotKey)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::CAMERA_SHOT_KEY] = cameraShotKey;
}

bool FileAsset::IsHidden() const
{
    return GetInt32Member(MediaColumn::MEDIA_HIDDEN);
}

void FileAsset::SetHidden(bool isHidden)
{
    member_[MediaColumn::MEDIA_HIDDEN] = isHidden;
}

const std::string &FileAsset::GetAllExif() const
{
    return GetStrMember(PhotoColumn::PHOTO_ALL_EXIF);
}

void FileAsset::SetAllExif(const string &allExif)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_ALL_EXIF] = allExif;
}

const std::string &FileAsset::GetFrontCamera() const
{
    return GetStrMember(PhotoColumn::PHOTO_FRONT_CAMERA);
}

void FileAsset::SetFrontCamera(const string &frontCamera)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_FRONT_CAMERA] = frontCamera;
}

const std::string &FileAsset::GetUserComment() const
{
    return GetStrMember(PhotoColumn::PHOTO_USER_COMMENT);
}

void FileAsset::SetUserComment(const string &userComment)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_USER_COMMENT] = userComment;
}

const std::string &FileAsset::GetFilePath() const
{
    return GetStrMember(MediaColumn::MEDIA_FILE_PATH);
}

void FileAsset::SetFilePath(const std::string &filePath)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[MediaColumn::MEDIA_FILE_PATH] = filePath;
}

int64_t FileAsset::GetPhotoEditTime() const
{
    return GetInt64Member(PhotoColumn::PHOTO_EDIT_TIME);
}

void FileAsset::SetPhotoEditTime(int64_t photoEditTime)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_EDIT_TIME] = photoEditTime;
}

int64_t FileAsset::GetTransCodeTime() const
{
    return GetInt64Member(PhotoColumn::PHOTO_TRANSCODE_TIME);
}

int32_t FileAsset::GetMovingPhotoEffectMode() const
{
    return GetInt32Member(PhotoColumn::MOVING_PHOTO_EFFECT_MODE);
}

void FileAsset::SetMovingPhotoEffectMode(int32_t effectMode)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::MOVING_PHOTO_EFFECT_MODE] = effectMode;
}

int64_t FileAsset::GetCoverPosition() const
{
    return GetInt64Member(PhotoColumn::PHOTO_COVER_POSITION);
}

void FileAsset::SetCoverPosition(int64_t coverPosition)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_COVER_POSITION] = coverPosition;
}

const std::string &FileAsset::GetBurstKey() const
{
    return GetStrMember(PhotoColumn::PHOTO_BURST_KEY);
}

void FileAsset::SetBurstKey(const std::string &burstKey)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_BURST_KEY] = burstKey;
}

int32_t FileAsset::GetBurstCoverLevel() const
{
    return GetInt32Member(PhotoColumn::PHOTO_BURST_COVER_LEVEL);
}

void FileAsset::SetBurstCoverLevel(int32_t burstCoverLevel)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_BURST_COVER_LEVEL] = burstCoverLevel;
}

int32_t FileAsset::GetCEAvailable() const
{
    return GetInt32Member(PhotoColumn::PHOTO_CE_AVAILABLE);
}

void FileAsset::SetCEAvailable(int32_t ceAvailable)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_CE_AVAILABLE] = ceAvailable;
}

const std::string &FileAsset::GetDetailTime() const
{
    return GetStrMember(PhotoColumn::PHOTO_DETAIL_TIME);
}

void FileAsset::SetDetailTime(const string &detailTime)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_DETAIL_TIME] = detailTime;
}

int32_t FileAsset::GetSupportedWatermarkType() const
{
    return GetInt32Member(PhotoColumn::SUPPORTED_WATERMARK_TYPE);
}

void FileAsset::SetSupportedWatermarkType(int32_t watermarkType)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::SUPPORTED_WATERMARK_TYPE] = watermarkType;
}

int32_t FileAsset::GetHasAppLink() const
{
    return GetInt32Member(PhotoColumn::PHOTO_HAS_APPLINK);
}
 
void FileAsset::SetHasAppLink(int32_t hasAppLink)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_HAS_APPLINK] = hasAppLink;
}
 
const std::string &FileAsset::GetAppLink() const
{
    return GetStrMember(PhotoColumn::PHOTO_APPLINK);
}
 
void FileAsset::SetAppLink(const string appLink)
{
    member_[PhotoColumn::PHOTO_APPLINK] = appLink;
}

int32_t FileAsset::GetCompositeDisplayStatus() const
{
    return GetInt32Member(PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS);
}

void FileAsset::SetCompositeDisplayStatus(int32_t compositeDisplayStatus)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_COMPOSITE_DISPLAY_STATUS] = compositeDisplayStatus;
}

int32_t FileAsset::GetIsAuto() const
{
    return GetInt32Member(PhotoColumn::PHOTO_IS_AUTO);
}

void FileAsset::SetIsAuto(int32_t isAuto)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_IS_AUTO] = isAuto;
}

int32_t FileAsset::GetFileResourceType() const
{
    return GetInt32Member(PhotoColumn::PHOTO_FILE_SOURCE_TYPE);
}

void FileAsset::SetFileResourceType(int32_t fileResourceType)
{
    member_[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = fileResourceType;
}

int32_t FileAsset::GetFileSourceType() const
{
    return GetInt32Member(PhotoColumn::PHOTO_FILE_SOURCE_TYPE);
}
 
void FileAsset::SetFileSourceType(int32_t fileSourceType)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_FILE_SOURCE_TYPE] = fileSourceType;
}
 
std::string FileAsset::GetStoragePath() const
{
    return GetStrMember(PhotoColumn::PHOTO_STORAGE_PATH);
}
 
void FileAsset::SetStoragePath(const std::string &storagePath)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_STORAGE_PATH] = storagePath;
}

void FileAsset::SetOpenStatus(int32_t fd, int32_t openStatus)
{
    lock_guard<mutex> lock(openStatusMapMutex_);
    if (openStatusMap_ == nullptr) {
        openStatusMap_ = make_shared<unordered_map<int32_t, int32_t>>();
    }
    openStatusMap_->insert({fd, openStatus});
}

void FileAsset::RemoveOpenStatus(int32_t fd)
{
    lock_guard<mutex> lock(openStatusMapMutex_);
    CHECK_AND_RETURN(openStatusMap_ != nullptr);
    openStatusMap_->erase(fd);
}

int32_t FileAsset::GetOpenStatus(int32_t fd)
{
    lock_guard<mutex> lock(openStatusMapMutex_);
    CHECK_AND_RETURN_RET(openStatusMap_ != nullptr, E_INVALID_VALUES);

    if (openStatusMap_->find(fd) != openStatusMap_->end()) {
        return openStatusMap_->at(fd);
    } else {
        MEDIA_ERR_LOG("can not find this fd: [%{public}d]", fd);
        return E_INVALID_VALUES;
    }
}

unordered_map<string, variant<int32_t, int64_t, string, double>> &FileAsset::GetMemberMap()
{
    return member_;
}

variant<int32_t, int64_t, string, double> &FileAsset::GetMemberValue(const string &name)
{
    std::shared_lock<std::shared_mutex> sharedlock(memberMapMutex_);
    return member_[name];
}

const string &FileAsset::GetStrMember(const string &name) const
{
    std::shared_lock<std::shared_mutex> sharedlock(memberMapMutex_);
    return (member_.count(name) > 0) ? get<string>(member_.at(name)) : DEFAULT_STR;
}

int32_t FileAsset::GetInt32Member(const string &name) const
{
    std::shared_lock<std::shared_mutex> sharedlock(memberMapMutex_);
    return (member_.count(name) > 0) ? get<int32_t>(member_.at(name)) : DEFAULT_INT32;
}

int64_t FileAsset::GetInt64Member(const string &name) const
{
    std::shared_lock<std::shared_mutex> sharedlock(memberMapMutex_);
    return (member_.count(name) > 0) ? get<int64_t>(member_.at(name)) : DEFAULT_INT64;
}

double FileAsset::GetDoubleMember(const string &name) const
{
    std::shared_lock<std::shared_mutex> sharedlock(memberMapMutex_);
    return (member_.count(name) > 0) ? get<double>(member_.at(name)) : DEFAULT_DOUBLE;
}

int32_t FileAsset::GetPhotoIndex() const
{
    return GetInt32Member(PHOTO_INDEX);
}

int32_t FileAsset::GetUserId()
{
    return userId_;
}

void FileAsset::SetUserId(int32_t userId)
{
    userId_ = userId;
}

int32_t FileAsset::GetExistCompatibleDuplicate() const
{
    return GetInt32Member(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE);
}

void FileAsset::SetExistCompatibleDuplicate(int32_t existCompatibleDuplicate)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE] = existCompatibleDuplicate;
}

void FileAsset::SetChangeTime(const int64_t changeTime)
{
    member_[PhotoColumn::PHOTO_CHANGE_TIME] = changeTime;
}

int64_t FileAsset::GetChangeTime() const
{
    return GetInt64Member(PhotoColumn::PHOTO_CHANGE_TIME);
}

void FileAsset::SetResultTypeMap(const string &colName, ResultSetDataType type)
{
    lock_guard<mutex> lock(resultTypeMapMutex_);
    if (resultTypeMap_.count(colName) != 0) {
        return;
    }
    resultTypeMap_.insert(make_pair(colName, type));
}

string FileAsset::GetAssetJson()
{
    json jsonObject;
    for (auto &[colName, _]  : member_) {
        if (resultTypeMap_.count(colName) == 0) {
            continue;
        }
        switch (resultTypeMap_.at(colName)) {
            case TYPE_STRING:
                jsonObject[colName] = GetStrMember(colName);
                break;
            case TYPE_INT32:
                jsonObject[colName] = GetInt32Member(colName);
                break;
            case TYPE_INT64:
                jsonObject[colName] = GetInt64Member(colName);
                break;
            default:
                break;
        }
    }
    jsonObject[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION] =
        AppFileService::SandboxHelper::Decode(jsonObject[PHOTO_DATA_IMAGE_IMAGE_DESCRIPTION]);
    return jsonObject.dump();
}

void FileAsset::SetStageVideoTaskStatus(int32_t stageVideoTaskStatus)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::STAGE_VIDEO_TASK_STATUS] = stageVideoTaskStatus;
}

int32_t FileAsset::GetStageVideoTaskStatus() const
{
    return GetInt32Member(PhotoColumn::STAGE_VIDEO_TASK_STATUS);
}

void FileAsset::SetPhotoIsTemp(int32_t isTemp)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_IS_TEMP] = isTemp;
}

int32_t FileAsset::GetPhotoIsTemp() const
{
    return GetInt32Member(PhotoColumn::PHOTO_IS_TEMP);
}

void FileAsset::SetExifRotate(int32_t exifRotate)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_EXIF_ROTATE] = exifRotate;
}

int32_t FileAsset::GetExifRotate() const
{
    return GetInt32Member(PhotoColumn::PHOTO_EXIF_ROTATE);
}

void FileAsset::SetCritical(int32_t isCritical)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_IS_CRITICAL] = isCritical;
}

int32_t FileAsset::GetCritical() const
{
    return GetInt32Member(PhotoColumn::PHOTO_IS_CRITICAL);
}

void FileAsset::SetRiskStatus(int32_t photoRiskStatus)
{
    std::unique_lock<std::shared_mutex> sharedLock(memberMapMutex_);
    member_[PhotoColumn::PHOTO_RISK_STATUS] = photoRiskStatus;
}

int32_t FileAsset::GetRiskStatus() const
{
    return GetInt32Member(PhotoColumn::PHOTO_RISK_STATUS);
}
}  // namespace Media
}  // namespace OHOS
