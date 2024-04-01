/*
 * Copyright (C) 2021-2023 Huawei Device Co., Ltd.
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

#include "datashare_business_error.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "directory_ex.h"
#include "media_column.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_db_const.h"
#include "medialibrary_helper_container.h"
#include "medialibrary_errno.h"
#include "medialibrary_type_const.h"
#include "sandbox_helper.h"
#include "uri.h"
#include "values_bucket.h"

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
    member_[MEDIA_DATA_DB_HEIGHT] = height;
}

int32_t FileAsset::GetDuration() const
{
    return GetInt32Member(MEDIA_DATA_DB_DURATION);
}

void FileAsset::SetDuration(int32_t duration)
{
    member_[MEDIA_DATA_DB_DURATION] = duration;
}

int32_t FileAsset::GetOrientation() const
{
    return GetInt32Member(MEDIA_DATA_DB_ORIENTATION);
}

void FileAsset::SetOrientation(int32_t orientation)
{
    member_[MEDIA_DATA_DB_ORIENTATION] = orientation;
}

int32_t FileAsset::GetAlbumId() const
{
    return GetInt32Member(MEDIA_DATA_DB_BUCKET_ID);
}

void FileAsset::SetAlbumId(int32_t albumId)
{
    member_[MEDIA_DATA_DB_BUCKET_ID] = albumId;
}

const string &FileAsset::GetAlbumName() const
{
    return GetStrMember(MEDIA_DATA_DB_BUCKET_NAME);
}

void FileAsset::SetAlbumName(const string &albumName)
{
    member_[MEDIA_DATA_DB_BUCKET_NAME] = albumName;
}

int32_t FileAsset::GetParent() const
{
    return GetInt32Member(MEDIA_DATA_DB_PARENT_ID);
}

void FileAsset::SetParent(int32_t parent)
{
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
    member_[MEDIA_DATA_DB_DATE_TAKEN] = dateTaken;
}

int64_t FileAsset::GetTimePending() const
{
    return GetInt64Member(MEDIA_DATA_DB_TIME_PENDING);
}

void FileAsset::SetTimePending(int64_t timePending)
{
    member_[MEDIA_DATA_DB_TIME_PENDING] = timePending;
}

bool FileAsset::IsFavorite() const
{
    return GetInt32Member(MEDIA_DATA_DB_IS_FAV);
}

void FileAsset::SetFavorite(bool isFavorite)
{
    member_[MEDIA_DATA_DB_IS_FAV] = isFavorite;
}

int64_t FileAsset::GetLatitude()
{
    return GetInt64Member(MEDIA_DATA_DB_LATITUDE);
}

void FileAsset::SetLatitude(int64_t latitude)
{
    member_[MEDIA_DATA_DB_LATITUDE] = latitude;
}

int64_t FileAsset::GetLongitude()
{
    return GetInt64Member(MEDIA_DATA_DB_LONGITUDE);
}

void FileAsset::SetLongitude(int64_t longitude)
{
    member_[MEDIA_DATA_DB_LONGITUDE] = longitude;
}

void FileAsset::SetPhotoIdAndQuality(const string &photoId, int photoQuality)
{
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
    member_[MEDIA_DATA_DB_DATE_TRASHED] = dateTrashed;
}

const string &FileAsset::GetSelfId() const
{
    return GetStrMember(MEDIA_DATA_DB_SELF_ID);
}

void FileAsset::SetSelfId(const string &selfId)
{
    member_[MEDIA_DATA_DB_SELF_ID] = selfId;
}

int32_t FileAsset::GetIsTrash() const
{
    if (resultNapiType_ == ResultNapiType::TYPE_USERFILE_MGR ||
        resultNapiType_ == ResultNapiType::TYPE_PHOTOACCESS_HELPER) {
        return static_cast<int32_t>(GetInt64Member(MediaColumn::MEDIA_DATE_TRASHED));
    }

    return GetInt32Member(MEDIA_DATA_DB_IS_TRASH);
}

void FileAsset::SetIsTrash(int32_t isTrash)
{
    member_[MEDIA_DATA_DB_IS_TRASH] = isTrash;
}

const string &FileAsset::GetRecyclePath() const
{
    return GetStrMember(MEDIA_DATA_DB_RECYCLE_PATH);
}

void FileAsset::SetRecyclePath(const string &recyclePath)
{
    member_[MEDIA_DATA_DB_RECYCLE_PATH] = recyclePath;
}

const string FileAsset::GetOwnerPackage() const
{
    return GetStrMember(MEDIA_DATA_DB_OWNER_PACKAGE);
}

void FileAsset::SetOwnerPackage(const string &ownerPackage)
{
    member_[MEDIA_DATA_DB_OWNER_PACKAGE] = ownerPackage;
}

const string FileAsset::GetOwnerAppId() const
{
    return GetStrMember(MEDIA_DATA_DB_OWNER_APPID);
}

void FileAsset::SetOwnerAppId(const string &ownerAppId)
{
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
    member_[PhotoColumn::PHOTO_SUBTYPE] = photoSubType;
}

const std::string &FileAsset::GetCameraShotKey() const
{
    return GetStrMember(PhotoColumn::CAMERA_SHOT_KEY);
}

void FileAsset::SetCameraShotKey(const std::string &cameraShotKey)
{
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
    member_[PhotoColumn::PHOTO_ALL_EXIF] = allExif;
}

const std::string &FileAsset::GetUserComment() const
{
    return GetStrMember(PhotoColumn::PHOTO_USER_COMMENT);
}

void FileAsset::SetUserComment(const string &userComment)
{
    member_[PhotoColumn::PHOTO_USER_COMMENT] = userComment;
}

const std::string &FileAsset::GetFilePath() const
{
    return GetStrMember(MediaColumn::MEDIA_FILE_PATH);
}

void FileAsset::SetFilePath(const std::string &filePath)
{
    member_[MediaColumn::MEDIA_FILE_PATH] = filePath;
}

int64_t FileAsset::GetPhotoEditTime() const
{
    return GetInt64Member(PhotoColumn::PHOTO_EDIT_TIME);
}

void FileAsset::SetPhotoEditTime(int64_t photoEditTime)
{
    member_[PhotoColumn::PHOTO_EDIT_TIME] = photoEditTime;
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
    if (openStatusMap_ == nullptr) {
        return;
    }
    openStatusMap_->erase(fd);
}

int32_t FileAsset::GetOpenStatus(int32_t fd)
{
    lock_guard<mutex> lock(openStatusMapMutex_);
    if (openStatusMap_ == nullptr) {
        return E_INVALID_VALUES;
    }
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
    return member_[name];
}

const string &FileAsset::GetStrMember(const string &name) const
{
    return (member_.count(name) > 0) ? get<string>(member_.at(name)) : DEFAULT_STR;
}

int32_t FileAsset::GetInt32Member(const string &name) const
{
    return (member_.count(name) > 0) ? get<int32_t>(member_.at(name)) : DEFAULT_INT32;
}

int64_t FileAsset::GetInt64Member(const string &name) const
{
    return (member_.count(name) > 0) ? get<int64_t>(member_.at(name)) : DEFAULT_INT64;
}

int32_t FileAsset::GetPhotoIndex() const
{
    return GetInt32Member(PHOTO_INDEX);
}

void FileAsset::SetResultTypeMap(const string &colName, ResultSetDataType type)
{
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
}  // namespace Media
}  // namespace OHOS
