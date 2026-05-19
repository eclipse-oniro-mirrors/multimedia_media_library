/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaLibraryManagerNotifyUtils"

#include "medialibrary_manager_notify_utils.h"

#include <algorithm>
#include <cctype>
#include <map>

#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS {
namespace Media {
namespace {
const std::string PHOTO_CHANGE = "photoChange";
const std::string HIDDEN_PHOTO_CHANGE = "hiddenPhotoChange";
const std::string TRASHED_PHOTO_CHANGE = "trashedPhotoChange";
const std::string PHOTO_ALBUM_CHANGE = "photoAlbumChange";
const std::string HIDDEN_ALBUM_CHANGE = "hiddenAlbumChange";
const std::string TRASHED_ALBUM_CHANGE = "trashedAlbumChange";
const std::string SINGLE_PHOTO_CHANGE = "singlePhotoChange";
const std::string SINGLE_PHOTO_ALBUM_CHANGE = "singlePhotoAlbumChange";

const std::map<Notification::NotifyUriType, std::string> REGISTER_URI_MAP = {
    { Notification::NotifyUriType::PHOTO_URI, PHOTO_CHANGE },
    { Notification::NotifyUriType::HIDDEN_PHOTO_URI, HIDDEN_PHOTO_CHANGE },
    { Notification::NotifyUriType::TRASH_PHOTO_URI, TRASHED_PHOTO_CHANGE },
    { Notification::NotifyUriType::PHOTO_ALBUM_URI, PHOTO_ALBUM_CHANGE },
    { Notification::NotifyUriType::HIDDEN_ALBUM_URI, HIDDEN_ALBUM_CHANGE },
    { Notification::NotifyUriType::TRASH_ALBUM_URI, TRASHED_ALBUM_CHANGE },
};

const std::map<Notification::NotifyUriType, std::string> REGISTER_SINGLE_URI_MAP = {
    { Notification::NotifyUriType::SINGLE_PHOTO_URI, SINGLE_PHOTO_CHANGE },
    { Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI, SINGLE_PHOTO_ALBUM_CHANGE },
};

const std::map<Notification::AccurateNotifyType, NotifyChangeType> NOTIFY_CHANGE_TYPE_MAP = {
    { Notification::AccurateNotifyType::NOTIFY_ASSET_ADD, NotifyChangeType::NOTIFY_CHANGE_ADD },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_UPDATE, NotifyChangeType::NOTIFY_CHANGE_UPDATE },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE, NotifyChangeType::NOTIFY_CHANGE_REMOVE },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_ADD_ANALYSIS, NotifyChangeType::NOTIFY_CHANGE_ADD_ANALYSIS },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_REMOVE_ANALYSIS,
        NotifyChangeType::NOTIFY_CHANGE_REMOVE_ANALYSIS },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD, NotifyChangeType::NOTIFY_CHANGE_ADD },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_UPDATE, NotifyChangeType::NOTIFY_CHANGE_UPDATE },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE, NotifyChangeType::NOTIFY_CHANGE_REMOVE },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_ADD_ANALYSIS, NotifyChangeType::NOTIFY_CHANGE_ADD_ANALYSIS },
    { Notification::AccurateNotifyType::NOTIFY_ALBUM_REMOVE_ANALYSIS,
        NotifyChangeType::NOTIFY_CHANGE_REMOVE_ANALYSIS },
    { Notification::AccurateNotifyType::NOTIFY_ASSET_YUV_READY, NotifyChangeType::NOTIFY_CHANGE_YUV_READY },
};

bool IsNumeric(const std::string &value)
{
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}
} // namespace

int32_t MediaLibraryManagerNotifyUtils::GetNotifyTypeAndUri(Notification::NotifyUriType type,
    Notification::NotifyUriType &registerUriType, std::string &registerUri)
{
    auto iter = REGISTER_URI_MAP.find(type);
    CHECK_AND_RETURN_RET_LOG(iter != REGISTER_URI_MAP.end(), E_INVALID_ARGUMENTS,
        "notify uri type is invalid: %{public}d", static_cast<int32_t>(type));
    registerUriType = type;
    registerUri = iter->second;
    return E_OK;
}

int32_t MediaLibraryManagerNotifyUtils::GetSingleNotifyTypeAndUri(Notification::NotifyUriType type,
    Notification::NotifyUriType &registerUriType, std::string &registerUri)
{
    auto iter = REGISTER_SINGLE_URI_MAP.find(type);
    CHECK_AND_RETURN_RET_LOG(iter != REGISTER_SINGLE_URI_MAP.end(), E_INVALID_ARGUMENTS,
        "single notify uri type is invalid: %{public}d", static_cast<int32_t>(type));
    registerUriType = type;
    registerUri = iter->second;
    return E_OK;
}

int32_t MediaLibraryManagerNotifyUtils::ExtractSingleId(const std::string &uri, std::string &singleId)
{
    CHECK_AND_RETURN_RET_LOG(!uri.empty(), E_INVALID_ARGUMENTS, "uri is empty");
    singleId = MediaFileUtils::GetIdFromUri(uri);
    CHECK_AND_RETURN_RET_LOG(IsNumeric(singleId), E_INVALID_ARGUMENTS,
        "single id is invalid: %{public}s", singleId.c_str());
    return E_OK;
}

int32_t MediaLibraryManagerNotifyUtils::ConvertProviderError(int32_t errCode)
{
    if (errCode == E_OK || errCode == E_SUCCESS) {
        return E_OK;
    }
    if (errCode == E_PERMISSION_DENIED || errCode == E_MAX_ON_SINGLE_NUM || errCode == E_INVALID_ARGUMENTS) {
        return errCode;
    }
    if (errCode == E_URI_IS_INVALID || errCode == E_URI_NOT_EXIST || errCode == E_DATAOBSERVER_IS_NULL ||
        errCode == E_DATAOBSERVER_IS_REPEATED) {
        return E_INVALID_ARGUMENTS;
    }
    if (errCode == E_CHECK_SYSTEMAPP_FAIL || errCode == -E_CHECK_SYSTEMAPP_FAIL) {
        return -E_CHECK_SYSTEMAPP_FAIL;
    }
    if (errCode < E_OK) {
        return errCode;
    }
    return E_FAIL;
}

NotifyChangeType MediaLibraryManagerNotifyUtils::ConvertNotifyChangeType(Notification::AccurateNotifyType notifyType)
{
    auto iter = NOTIFY_CHANGE_TYPE_MAP.find(notifyType);
    CHECK_AND_RETURN_RET_LOG(iter != NOTIFY_CHANGE_TYPE_MAP.end(), NotifyChangeType::NOTIFY_CHANGE_INVALID,
        "notify type is invalid: %{public}d", static_cast<int32_t>(notifyType));
    return iter->second;
}

bool MediaLibraryManagerNotifyUtils::IsAssetNotifyType(Notification::NotifyUriType uriType)
{
    return uriType == Notification::NotifyUriType::PHOTO_URI ||
        uriType == Notification::NotifyUriType::HIDDEN_PHOTO_URI ||
        uriType == Notification::NotifyUriType::TRASH_PHOTO_URI ||
        uriType == Notification::NotifyUriType::SINGLE_PHOTO_URI;
}

bool MediaLibraryManagerNotifyUtils::IsAlbumNotifyType(Notification::NotifyUriType uriType)
{
    return uriType == Notification::NotifyUriType::PHOTO_ALBUM_URI ||
        uriType == Notification::NotifyUriType::HIDDEN_ALBUM_URI ||
        uriType == Notification::NotifyUriType::TRASH_ALBUM_URI ||
        uriType == Notification::NotifyUriType::SINGLE_PHOTO_ALBUM_URI;
}

std::shared_ptr<PhotoAssetChangeInfo> MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfo(
    const AccurateRefresh::PhotoAssetChangeInfo &changeInfo, Notification::NotifyUriType uriType)
{
    CHECK_AND_RETURN_RET(changeInfo.fileId_ != AccurateRefresh::INVALID_INT32_VALUE, nullptr);
    auto result = std::make_shared<PhotoAssetChangeInfo>();
    CHECK_AND_RETURN_RET(result != nullptr, nullptr);
    result->fileId_ = changeInfo.fileId_;
    result->uri_ = changeInfo.uri_;
    result->mediaType_ = changeInfo.mediaType_;
    result->ownerAlbumUri_ = changeInfo.ownerAlbumUri_;
    result->dateDay_ = changeInfo.dateDay_;
    result->isFavorite_ = changeInfo.isFavorite_;
    result->isHidden_ = changeInfo.isHidden_;
    result->strongAssociation_ = changeInfo.strongAssociation_;
    result->thumbnailVisible_ = changeInfo.thumbnailVisible_;
    result->dateTrashedMs_ = changeInfo.dateTrashedMs_;
    result->dateAddedMs_ = changeInfo.dateAddedMs_;
    result->dateTakenMs_ = changeInfo.dateTakenMs_;
    result->dateModifiedMs_ = changeInfo.dateModifiedMs_;
    result->position_ = changeInfo.position_;
    result->displayName_ = changeInfo.displayName_;
    result->size_ = changeInfo.size_;
    result->livephoto4dStatus_ = changeInfo.livephoto4dStatus_;
    if (uriType == Notification::NotifyUriType::HIDDEN_PHOTO_URI) {
        result->hiddenTime_ = changeInfo.hiddenTime_;
    }
    for (const auto &albumInfoPtr : changeInfo.albumChangeInfos_) {
        if (albumInfoPtr == nullptr) {
            result->albumChangeInfos_.push_back(nullptr);
            continue;
        }
        auto albumInfo = BuildAlbumChangeInfo(*albumInfoPtr);
        result->albumChangeInfos_.push_back(albumInfo);
    }
    return result;
}

std::shared_ptr<AlbumChangeInfo> MediaLibraryManagerNotifyUtils::BuildAlbumChangeInfo(
    const AccurateRefresh::AlbumChangeInfo &changeInfo)
{
    CHECK_AND_RETURN_RET(changeInfo.albumId_ != AccurateRefresh::INVALID_INT32_VALUE, nullptr);
    auto result = std::make_shared<AlbumChangeInfo>();
    CHECK_AND_RETURN_RET(result != nullptr, nullptr);
    result->albumId_ = changeInfo.albumId_;
    result->albumType_ = changeInfo.albumType_;
    result->albumSubType_ = changeInfo.albumSubType_;
    result->albumName_ = changeInfo.albumName_;
    result->albumUri_ = changeInfo.albumUri_;
    result->imageCount_ = changeInfo.imageCount_;
    result->videoCount_ = changeInfo.videoCount_;
    result->count_ = changeInfo.count_;
    result->coverUri_ = changeInfo.coverUri_;
    result->hiddenCount_ = changeInfo.hiddenCount_;
    result->hiddenCoverUri_ = changeInfo.hiddenCoverUri_;
    result->isCoverChange_ = changeInfo.isCoverChange_;
    result->isHiddenCoverChange_ = changeInfo.isHiddenCoverChange_;
    result->orderSection_ = changeInfo.orderSection_;
    result->albumsOrder_ = changeInfo.albumsOrder_;
    result->lpath_ = changeInfo.lpath_;
    if (!(changeInfo.albumSubType_ >= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_START) &&
        changeInfo.albumSubType_ <= static_cast<int32_t>(PhotoAlbumSubType::ANALYSIS_END))) {
        result->hidden_ = changeInfo.hidden_;
    }
    auto coverInfo = BuildPhotoAssetChangeInfo(changeInfo.coverInfo_);
    if (coverInfo != nullptr) {
        result->coverInfo_ = *coverInfo;
    }
    auto hiddenCoverInfo = BuildPhotoAssetChangeInfo(changeInfo.hiddenCoverInfo_);
    if (hiddenCoverInfo != nullptr) {
        result->hiddenCoverInfo_ = *hiddenCoverInfo;
    }
    return result;
}

PhotoAssetChangeData MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeData(
    const AccurateRefresh::PhotoAssetChangeData &changeData, Notification::NotifyUriType uriType)
{
    PhotoAssetChangeData result;
    result.assetBeforeChange = BuildPhotoAssetChangeInfo(changeData.infoBeforeChange_, uriType);
    result.assetAfterChange = BuildPhotoAssetChangeInfo(changeData.infoAfterChange_, uriType);
    result.isContentChanged = changeData.isContentChanged_;
    result.isDeleted = changeData.isDelete_;
    result.thumbnailChangeStatus = changeData.thumbnailChangeStatus_;
    result.version = changeData.version_;
    return result;
}

AlbumChangeData MediaLibraryManagerNotifyUtils::BuildAlbumChangeData(
    const AccurateRefresh::AlbumChangeData &changeData)
{
    AlbumChangeData result;
    result.albumBeforeChange = BuildAlbumChangeInfo(changeData.infoBeforeChange_);
    result.albumAfterChange = BuildAlbumChangeInfo(changeData.infoAfterChange_);
    result.version = changeData.version_;
    return result;
}

PhotoAssetChangeInfos MediaLibraryManagerNotifyUtils::BuildPhotoAssetChangeInfos(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo, Notification::NotifyUriType uriType)
{
    PhotoAssetChangeInfos result;
    CHECK_AND_RETURN_RET_LOG(changeInfo != nullptr, result, "changeInfo is nullptr");
    result.type = ConvertNotifyChangeType(changeInfo->notifyType);
    result.isForRecheck = changeInfo->isForRecheck;
    for (const auto &item : changeInfo->changeInfos) {
        const auto *assetData = std::get_if<AccurateRefresh::PhotoAssetChangeData>(&item);
        if (assetData == nullptr) {
            MEDIA_WARN_LOG("changeInfo item is not photo asset data");
            continue;
        }
        result.assetChangeDatas.push_back(BuildPhotoAssetChangeData(*assetData, uriType));
    }
    return result;
}

AlbumChangeInfos MediaLibraryManagerNotifyUtils::BuildAlbumChangeInfos(
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    AlbumChangeInfos result;
    CHECK_AND_RETURN_RET_LOG(changeInfo != nullptr, result, "changeInfo is nullptr");
    result.type = ConvertNotifyChangeType(changeInfo->notifyType);
    result.isForRecheck = changeInfo->isForRecheck;
    for (const auto &item : changeInfo->changeInfos) {
        const auto *albumData = std::get_if<AccurateRefresh::AlbumChangeData>(&item);
        if (albumData == nullptr) {
            MEDIA_WARN_LOG("changeInfo item is not album data");
            continue;
        }
        result.albumChangeDatas.push_back(BuildAlbumChangeData(*albumData));
    }
    return result;
}

PhotoAssetChangeInfos MediaLibraryManagerNotifyUtils::BuildSinglePhotoAssetChangeInfos(
    const AccurateRefresh::PhotoAssetChangeData &changeData,
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    PhotoAssetChangeInfos result;
    CHECK_AND_RETURN_RET_LOG(changeInfo != nullptr, result, "changeInfo is nullptr");
    result.type = ConvertNotifyChangeType(changeInfo->notifyType);
    result.isForRecheck = changeInfo->isForRecheck;
    result.assetChangeDatas.push_back(BuildPhotoAssetChangeData(changeData));
    return result;
}

AlbumChangeInfos MediaLibraryManagerNotifyUtils::BuildSingleAlbumChangeInfos(
    const AccurateRefresh::AlbumChangeData &changeData,
    const std::shared_ptr<Notification::MediaChangeInfo> &changeInfo)
{
    AlbumChangeInfos result;
    CHECK_AND_RETURN_RET_LOG(changeInfo != nullptr, result, "changeInfo is nullptr");
    result.type = ConvertNotifyChangeType(changeInfo->notifyType);
    result.isForRecheck = changeInfo->isForRecheck;
    result.albumChangeDatas.push_back(BuildAlbumChangeData(changeData));
    return result;
}

PhotoAssetChangeInfos MediaLibraryManagerNotifyUtils::BuildPhotoAssetRecheckChangeInfos()
{
    PhotoAssetChangeInfos result;
    result.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    result.isForRecheck = true;
    return result;
}

AlbumChangeInfos MediaLibraryManagerNotifyUtils::BuildAlbumRecheckChangeInfos()
{
    AlbumChangeInfos result;
    result.type = NotifyChangeType::NOTIFY_CHANGE_UPDATE;
    result.isForRecheck = true;
    return result;
}
} // namespace Media
} // namespace OHOS