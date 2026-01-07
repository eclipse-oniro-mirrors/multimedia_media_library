/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#define MLOG_TAG "PhotoAlbum"

#include "photo_album.h"

#include "medialibrary_type_const.h"

using namespace std;

namespace OHOS {
namespace Media {
PhotoAlbum::PhotoAlbum()
{
    albumId_ = DEFAULT_ALBUM_ID;
    type_ = USER;
    subType_ = USER_GENERIC;
    count_ = DEFAULT_COUNT;
}

PhotoAlbum::~PhotoAlbum() = default;

void PhotoAlbum::SetAlbumId(const int32_t albumId)
{
    albumId_ = albumId;
}

int32_t PhotoAlbum::GetAlbumId() const
{
    return albumId_;
}

void PhotoAlbum::SetPhotoAlbumType(const PhotoAlbumType type)
{
    type_ = type;
}

PhotoAlbumType PhotoAlbum::GetPhotoAlbumType() const
{
    return type_;
}

void PhotoAlbum::SetPhotoAlbumSubType(const PhotoAlbumSubType subType)
{
    subType_ = subType;
}

PhotoAlbumSubType PhotoAlbum::GetPhotoAlbumSubType() const
{
    return subType_;
}

void PhotoAlbum::SetAlbumUri(const string &uri)
{
    uri_ = uri;
}

const string& PhotoAlbum::GetAlbumUri() const
{
    return uri_;
}

void PhotoAlbum::SetAlbumName(const string &albumName)
{
    albumName_ = albumName;
}

const string& PhotoAlbum::GetAlbumName() const
{
    return albumName_;
}

void PhotoAlbum::SetCoverUri(const string &coverUri)
{
    coverUri_ = coverUri;
}

const string& PhotoAlbum::GetCoverUri() const
{
    return coverUri_;
}

void PhotoAlbum::SetDateModified(const int64_t dateModified)
{
    dateModified_ = dateModified;
}

int64_t PhotoAlbum::GetDateModified() const
{
    return dateModified_;
}

void PhotoAlbum::SetCount(const int32_t count)
{
    count_ = count;
}

int32_t PhotoAlbum::GetCount() const
{
    return count_;
}

void PhotoAlbum::SetImageCount(const int32_t count)
{
    imageCount_ = count;
}

int32_t PhotoAlbum::GetImageCount() const
{
    return imageCount_;
}

void PhotoAlbum::SetVideoCount(const int32_t count)
{
    videoCount_ = count;
}

int32_t PhotoAlbum::GetVideoCount() const
{
    return videoCount_;
}

void PhotoAlbum::SetLatitude(const double latitude)
{
    latitude_ = latitude;
}

double PhotoAlbum::GetLatitude() const
{
    return latitude_;
}

void PhotoAlbum::SetLongitude(const double longitude)
{
    longitude_ = longitude;
}

double PhotoAlbum::GetLongitude() const
{
    return longitude_;
}

void PhotoAlbum::SetRelativePath(const string &relativePath)
{
    relativePath_ = relativePath;
}

const string& PhotoAlbum::GetRelativePath() const
{
    return relativePath_;
}

void PhotoAlbum::SetResultNapiType(const ResultNapiType resultNapiType)
{
    resultNapiType_ = resultNapiType;
}

ResultNapiType PhotoAlbum::GetResultNapiType() const
{
    return resultNapiType_;
}

void PhotoAlbum::SetHiddenOnly(const bool hiddenOnly)
{
    hiddenOnly_ = hiddenOnly;
}

bool PhotoAlbum::GetHiddenOnly() const
{
    return hiddenOnly_;
}

void PhotoAlbum::SetLocationOnly(const bool locationOnly)
{
    locationOnly_ = locationOnly;
}

bool PhotoAlbum::GetLocationOnly() const
{
    return locationOnly_;
}

void PhotoAlbum::SetDisplayLevel(const int32_t displayLevel)
{
    displayLevel_ = displayLevel;
}

int32_t PhotoAlbum::GetDisplayLevel() const
{
    return displayLevel_;
}

void PhotoAlbum::SetPriority(const int32_t priority)
{
    priority_ = priority;
}

int32_t PhotoAlbum::GetPriority() const
{
    return priority_;
}

void PhotoAlbum::SetLPath(const std::string &path)
{
    lPath_ = path;
}

const std::string& PhotoAlbum::GetLPath() const
{
    return lPath_;
}

void PhotoAlbum::SetBundleName(const std::string &name)
{
    bundleName_ = name;
}

const std::string& PhotoAlbum::GetBundleName() const
{
    return bundleName_;
}

void PhotoAlbum::SetDateAdded(const int64_t date)
{
    dateAdded_ = date;
}

int64_t PhotoAlbum::GetDateAdded() const
{
    return dateAdded_;
}

void PhotoAlbum::SetContainsHidden(const int32_t hidden)
{
    containsHidden_ = hidden;
}

int32_t PhotoAlbum::GetContainsHidden() const
{
    return containsHidden_;
}

void PhotoAlbum::SetOrder(const int32_t order)
{
    order_ = order;
}

int32_t PhotoAlbum::GetOrder() const
{
    return order_;
}

void PhotoAlbum::SetLocalLanguage(const std::string &language)
{
    localLanguage_ = language;
}

const std::string& PhotoAlbum::GetLocalLanguage() const
{
    return localLanguage_;
}

void PhotoAlbum::SetIsLocal(const int32_t isLocal)
{
    isLocal_ = isLocal;
}

int32_t PhotoAlbum::GetIsLocal() const
{
    return isLocal_;
}

bool PhotoAlbum::IsUserPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::USER) && (albumSubType == PhotoAlbumSubType::USER_GENERIC);
}

bool PhotoAlbum::IsSmartPortraitPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType == PhotoAlbumSubType::PORTRAIT);
}

bool PhotoAlbum::IsSmartGroupPhotoAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType == PhotoAlbumSubType::GROUP_PHOTO);
}

bool PhotoAlbum::IsSmartClassifyAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType == PhotoAlbumSubType::CLASSIFY);
}

bool PhotoAlbum::IsTrashAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SYSTEM) && (albumSubType == PhotoAlbumSubType::TRASH);
}

bool PhotoAlbum::IsSourceAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SOURCE) && (albumSubType == PhotoAlbumSubType::SOURCE_GENERIC);
}

bool PhotoAlbum::IsPetAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType == PhotoAlbumSubType::PET);
}

bool PhotoAlbum::IsSystemAlbum(const PhotoAlbumType albumType)
{
    return albumType == PhotoAlbumType::SYSTEM;
}

bool PhotoAlbum::IsHiddenAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SYSTEM) && (albumSubType == PhotoAlbumSubType::HIDDEN);
}

bool PhotoAlbum::CheckPhotoAlbumType(const PhotoAlbumType albumType)
{
    return (albumType == PhotoAlbumType::USER) || (albumType == PhotoAlbumType::SYSTEM) ||
        (albumType == PhotoAlbumType::SMART) || (albumType == PhotoAlbumType::SOURCE);
}

bool PhotoAlbum::CheckPhotoAlbumSubType(const PhotoAlbumSubType albumSubType)
{
    return (albumSubType == PhotoAlbumSubType::USER_GENERIC) || (albumSubType == PhotoAlbumSubType::ANY) ||
        ((albumSubType >= PhotoAlbumSubType::SYSTEM_START) && (albumSubType <= PhotoAlbumSubType::SYSTEM_END)) ||
        ((albumSubType >= PhotoAlbumSubType::ANALYSIS_START) && (albumSubType <= PhotoAlbumSubType::ANALYSIS_END)) ||
        (albumSubType == PhotoAlbumSubType::SOURCE_GENERIC);
}

bool PhotoAlbum::IsHighlightAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType == PhotoAlbumSubType::HIGHLIGHT ||
        albumSubType == PhotoAlbumSubType::HIGHLIGHT_SUGGESTIONS);
}

bool PhotoAlbum::IsAnalysisAlbum(const PhotoAlbumType albumType, const PhotoAlbumSubType albumSubType)
{
    return (albumType == PhotoAlbumType::SMART) && (albumSubType >= PhotoAlbumSubType::ANALYSIS_START &&
            albumSubType <= PhotoAlbumSubType::ANALYSIS_END);
}

bool PhotoAlbum::CheckOrderStyleType(const OrderStyleType orderStyle)
{
    return (orderStyle == OrderStyleType::MIX) || (orderStyle == OrderStyleType::SPLIT);
}

void PhotoAlbum::SetUserId(int32_t userId)
{
    targetUserId_ = userId;
}
 
int32_t PhotoAlbum::GetUserId()
{
    return targetUserId_;
}

void PhotoAlbum::SetCoverUriSource(int32_t coverUriSource)
{
    coverUriSource_ = coverUriSource;
}
 
int32_t PhotoAlbum::GetCoverUriSource()
{
    return coverUriSource_ > 0 ? static_cast<int32_t>(CoverUriSource::MANUAL_CLOUD_COVER) :
        static_cast<int32_t>(CoverUriSource::DEFAULT_COVER);
}

void PhotoAlbum::SetChangeTime(const int64_t changeTime)
{
    changeTime_ = changeTime;
}

int64_t PhotoAlbum::GetChangeTime() const
{
    return changeTime_;
}

void PhotoAlbum::SetUploadStatus(int32_t uploadStatus)
{
    uploadStatus_ = uploadStatus;
}

int32_t PhotoAlbum::GetUploadStatus() const
{
    return uploadStatus_;
}

void PhotoAlbum::SetHidden(const int32_t hidden)
{
    hidden_ = hidden;
}

int32_t PhotoAlbum::GetHidden() const
{
    return hidden_;
}

bool PhotoAlbum::IsUserPhotoAlbumByType(const PhotoAlbumType albumType)
{
    return albumType == PhotoAlbumType::USER;
}
}  // namespace Media
}  // namespace OHOS
