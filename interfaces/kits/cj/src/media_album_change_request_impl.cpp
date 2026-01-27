/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "media_album_change_request_impl.h"

#include "result_set_utils.h"
#include "userfile_client.h"
#include "album_operation_uri.h"
namespace OHOS {
namespace Media {
MediaAlbumChangeRequestImpl::MediaAlbumChangeRequestImpl(shared_ptr<PhotoAlbum> photoAlbumPtr)
{
    photoAlbum_ = photoAlbumPtr;
}

shared_ptr<PhotoAlbum> MediaAlbumChangeRequestImpl::GetPhotoAlbumInstance() const
{
    return photoAlbum_;
}

vector<string> MediaAlbumChangeRequestImpl::GetAddAssetArray() const
{
    return assetsToAdd_;
}

vector<string> MediaAlbumChangeRequestImpl::GetRemoveAssetArray() const
{
    return assetsToRemove_;
}

std::vector<AlbumChangeOperation> MediaAlbumChangeRequestImpl::GetAlbumChangeOperations() const
{
    return albumChangeOperations_;
}

void MediaAlbumChangeRequestImpl::ClearAddAssetArray()
{
    assetsToAdd_.clear();
}

void MediaAlbumChangeRequestImpl::ClearRemoveAssetArray()
{
    assetsToRemove_.clear();
}

int64_t MediaAlbumChangeRequestImpl::CJGetAlbum(int32_t* errCode)
{
    if (photoAlbum_ == nullptr) {
        *errCode = JS_INNER_FAIL;
        return 0;
    }
    if (photoAlbum_->GetAlbumId() > 0) {
        auto photoAlbumImpl = FFIData::Create<PhotoAlbumImpl>(photoAlbum_);
        if (photoAlbumImpl == nullptr) {
            *errCode = JS_INNER_FAIL;
            return 0;
        }
        return photoAlbumImpl->GetID();
    }
    return 0;
}

int32_t MediaAlbumChangeRequestImpl::CJSetAlbumName(std::string albumName)
{
    if (MediaFileUtils::CheckAlbumName(albumName) != E_OK) {
        LOGE("Invalid album name");
        return OHOS_INVALID_PARAM_CODE;
    }
    if (photoAlbum_ == nullptr) {
        LOGE("photoAlbum is null")
        return OHOS_INVALID_PARAM_CODE;
    }
    bool isUserPhotoAlbum = PhotoAlbum::IsUserPhotoAlbum(photoAlbum_->GetPhotoAlbumType(),
        photoAlbum_->GetPhotoAlbumSubType());
    bool isSmartPortraitPhotoAlbum = PhotoAlbum::IsSmartPortraitPhotoAlbum(photoAlbum_->GetPhotoAlbumType(),
        photoAlbum_->GetPhotoAlbumSubType());
    bool isSmartGroupPhotoAlbum = PhotoAlbum::IsSmartGroupPhotoAlbum(photoAlbum_->GetPhotoAlbumType(),
        photoAlbum_->GetPhotoAlbumSubType());
    bool isSmartPetAlbum = PhotoAlbum::IsPetAlbum(photoAlbum_->GetPhotoAlbumType(),
        photoAlbum_->GetPhotoAlbumSubType());
    if (!(isUserPhotoAlbum || isSmartPortraitPhotoAlbum || isSmartGroupPhotoAlbum || isSmartPetAlbum)) {
        LOGE("Only user album, pet, smart portrait album and group photo can set album name");
        return OHOS_INVALID_PARAM_CODE;
    }
    photoAlbum_->SetAlbumName(albumName);
    albumChangeOperations_.push_back(AlbumChangeOperation::SET_ALBUM_NAME);
    return 0;
}

static bool CheckDuplicatedAssetArray(const vector<string>& arrayToCheck, const vector<string>& currentArray)
{
    if (currentArray.empty()) {
        return true;
    }

    for (const auto& element : arrayToCheck) {
        if (std::find(currentArray.begin(), currentArray.end(), element) != currentArray.end()) {
            return false;
        }
    }
    return true;
}

int32_t MediaAlbumChangeRequestImpl::CJAddAssets(std::vector<std::string> assetUriArray)
{
    if (photoAlbum_ == nullptr) {
        LOGE("photoAlbum is null")
        return OHOS_INVALID_PARAM_CODE;
    }
    if (!(PhotoAlbum::IsUserPhotoAlbum(photoAlbum_->GetPhotoAlbumType(), photoAlbum_->GetPhotoAlbumSubType()))) {
        LOGE("Only user album can add assets");
        return OHOS_INVALID_PARAM_CODE;
    }
    if (!CheckDuplicatedAssetArray(assetUriArray, assetsToAdd_)) {
        LOGE("The previous addAssets operation has contained the same asset");
        return JS_E_OPERATION_NOT_SUPPORT;
    }
    assetsToAdd_.insert(assetsToAdd_.end(), assetUriArray.begin(), assetUriArray.end());
    albumChangeOperations_.push_back(AlbumChangeOperation::ADD_ASSETS);
    return 0;
}

int32_t MediaAlbumChangeRequestImpl::CJRemoveAssets(std::vector<std::string> assetUriArray)
{
    if (photoAlbum_ == nullptr) {
        LOGE("photoAlbum is null")
        return OHOS_INVALID_PARAM_CODE;
    }
    if (!(PhotoAlbum::IsUserPhotoAlbum(photoAlbum_->GetPhotoAlbumType(), photoAlbum_->GetPhotoAlbumSubType()))) {
        LOGE("Only user album can add assets");
        return OHOS_INVALID_PARAM_CODE;
    }
    if (!CheckDuplicatedAssetArray(assetUriArray, assetsToAdd_)) {
        LOGE("The previous addAssets operation has contained the same asset");
        return JS_E_OPERATION_NOT_SUPPORT;
    }
    assetsToRemove_.insert(assetsToRemove_.end(), assetUriArray.begin(), assetUriArray.end());
    albumChangeOperations_.push_back(AlbumChangeOperation::REMOVE_ASSETS);
    return 0;
}

bool MediaAlbumChangeRequestImpl::CheckChangeOperations()
{
    if (albumChangeOperations_.empty()) {
        LOGE("None request to apply");
        return false;
    }

    auto photoAlbum = GetPhotoAlbumInstance();
    if (photoAlbum == nullptr) {
        LOGE("photoAlbum is null");
        return false;
    }

    if (albumChangeOperations_.front() != AlbumChangeOperation::CREATE_ALBUM && photoAlbum->GetAlbumId() <= 0) {
        LOGE("Invalid album change request");
        return false;
    }

    return true;
}

static bool FetchNewCount(shared_ptr<PhotoAlbum>& album)
{
    if (album == nullptr) {
        LOGE("Album is null");
        return false;
    }

    Uri queryUri(CONST_PAH_QUERY_PHOTO_ALBUM);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, album->GetAlbumId());
    vector<string> fetchColumns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COUNT,
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT, PhotoAlbumColumns::ALBUM_VIDEO_COUNT };
    int errCode = 0;
    auto resultSet = UserFileClient::Query(queryUri, predicates, fetchColumns, errCode);
    if (resultSet == nullptr) {
        NAPI_ERR_LOG("resultSet == nullptr, errCode is %{public}d", errCode);
        return false;
    }
    if (resultSet->GoToFirstRow() != 0) {
        NAPI_ERR_LOG("go to first row failed when fetch new count");
        return false;
    }

    bool hiddenOnly = album->GetHiddenOnly();
    int imageCount = hiddenOnly ? -1 : get<int32_t>(ResultSetUtils::GetValFromColumn(
        PhotoAlbumColumns::ALBUM_IMAGE_COUNT, resultSet, TYPE_INT32));
    int videoCount = hiddenOnly ? -1 : get<int32_t>(ResultSetUtils::GetValFromColumn(
        PhotoAlbumColumns::ALBUM_VIDEO_COUNT, resultSet, TYPE_INT32));
    album->SetCount(
        get<int32_t>(ResultSetUtils::GetValFromColumn(PhotoAlbumColumns::ALBUM_COUNT, resultSet, TYPE_INT32)));
    album->SetImageCount(imageCount);
    album->SetVideoCount(videoCount);
    return true;
}

static bool AddAssetsExecute(MediaAlbumChangeRequestImpl* changeRequest)
{
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    int32_t albumId = photoAlbum->GetAlbumId();
    vector<DataShare::DataShareValuesBucket> valuesBuckets;
    for (const auto& asset : changeRequest->GetAddAssetArray()) {
        DataShare::DataShareValuesBucket pair;
        pair.Put(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
        pair.Put(PhotoColumn::MEDIA_ID, asset);
        valuesBuckets.push_back(pair);
    }

    Uri addAssetsUri(CONST_PAH_PHOTO_ALBUM_ADD_ASSET);
    int ret = UserFileClient::BatchInsert(addAssetsUri, valuesBuckets);
    changeRequest->ClearAddAssetArray();
    if (ret < 0) {
        LOGE("Failed to add assets into album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    LOGE("Add %{public}d asset(s) into album %{public}d", ret, albumId);
    FetchNewCount(photoAlbum);
    return true;
}

static bool RemoveAssetsExecute(MediaAlbumChangeRequestImpl* changeRequest)
{
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    int32_t albumId = photoAlbum->GetAlbumId();
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    predicates.And()->In(PhotoColumn::MEDIA_ID, changeRequest->GetRemoveAssetArray());

    Uri removeAssetsUri(CONST_PAH_PHOTO_ALBUM_REMOVE_ASSET);
    int ret = UserFileClient::Delete(removeAssetsUri, predicates);
    changeRequest->ClearRemoveAssetArray();
    if (ret < 0) {
        LOGE("Failed to remove assets from album %{public}d, err: %{public}d", albumId, ret);
        return false;
    }

    LOGE("Remove %{public}d asset(s) from album %{public}d", ret, albumId);
    FetchNewCount(photoAlbum);
    return true;
}

static void GetAlbumUpdateCoverUri(shared_ptr<PhotoAlbum>& photoAlbum, string& uri)
{
    if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::PORTRAIT) {
        uri = CONST_PAH_PORTRAIT_ANAALBUM_COVER_URI;
    } else if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::GROUP_PHOTO) {
        uri = CONST_PAH_GROUP_ANAALBUM_COVER_URI;
    } else if (PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        uri = CONST_PAH_UPDATE_USER_ALBUM_COVER_URI;
    } else if (PhotoAlbum::IsUserPhotoAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        uri = CONST_PAH_UPDATE_USER_ALBUM_COVER_URI;
    } else if (PhotoAlbum::IsSourceAlbum(photoAlbum->GetPhotoAlbumType(), photoAlbum->GetPhotoAlbumSubType())) {
        uri = CONST_PAH_UPDATE_SOURCE_ALBUM_COVER_URI;
    } else {
        uri = CONST_PAH_UPDATE_SYSTEM_ALBUM_COVER_URI;
    }
}

static bool GetAlbumUpdateValue(shared_ptr<PhotoAlbum>& photoAlbum, const AlbumChangeOperation changeOperation,
    string& uri, DataShare::DataShareValuesBucket& valuesBucket, string& property)
{
    if (photoAlbum == nullptr) {
        LOGE("photoAlbum is null");
        return false;
    }

    switch (changeOperation) {
        case AlbumChangeOperation::SET_ALBUM_NAME:
            if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::PORTRAIT) {
                uri = CONST_PAH_PORTRAIT_ANAALBUM_ALBUM_NAME;
            } else if (photoAlbum->GetPhotoAlbumSubType() == PhotoAlbumSubType::GROUP_PHOTO) {
                uri = CONST_PAH_GROUP_ANAALBUM_ALBUM_NAME;
            } else {
                uri = CONST_PAH_SET_PHOTO_ALBUM_NAME;
            }
            property = PhotoAlbumColumns::ALBUM_NAME;
            valuesBucket.Put(property, photoAlbum->GetAlbumName());
            break;
        case AlbumChangeOperation::SET_COVER_URI:
            GetAlbumUpdateCoverUri(photoAlbum, uri);
            property = PhotoAlbumColumns::ALBUM_COVER_URI;
            valuesBucket.Put(property, photoAlbum->GetCoverUri());
            break;
        case AlbumChangeOperation::RESET_COVER_URI:
            uri = CONST_PAH_RESET_ALBUM_COVER_URI;
            break;
        case AlbumChangeOperation::SET_DISPLAY_LEVEL:
            uri = CONST_PAH_PORTRAIT_DISPLAY_LEVLE;
            property = USER_DISPLAY_LEVEL;
            valuesBucket.Put(property, photoAlbum->GetDisplayLevel());
            break;
        case AlbumChangeOperation::SET_IS_ME:
            uri = CONST_PAH_PORTRAIT_IS_ME;
            property = IS_ME;
            valuesBucket.Put(property, 1);
            break;
        case AlbumChangeOperation::DISMISS:
            uri = CONST_PAH_GROUP_ANAALBUM_DISMISS;
            property = IS_REMOVED;
            valuesBucket.Put(property, 1);
            break;
        default:
            return false;
    }
    return true;
}

static bool SetAlbumPropertyExecute(
    MediaAlbumChangeRequestImpl* changeRequest, const AlbumChangeOperation changeOperation)
{
    if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME &&
        changeRequest->GetAlbumChangeOperations().front() == AlbumChangeOperation::CREATE_ALBUM) {
        return true;
    }

    DataShare::DataSharePredicates predicates;
    DataShare::DataShareValuesBucket valuesBucket;
    auto photoAlbum = changeRequest->GetPhotoAlbumInstance();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, std::to_string(photoAlbum->GetAlbumId()));
    string uri;
    string property;
    if (!GetAlbumUpdateValue(photoAlbum, changeOperation, uri, valuesBucket, property)) {
        LOGE("Failed to parse album change operation: %{public}d", changeOperation);
        return false;
    }
    valuesBucket.Put(PhotoAlbumColumns::ALBUM_SUBTYPE, photoAlbum->GetPhotoAlbumSubType());
    MediaLibraryNapiUtils::UriAppendKeyValue(uri, API_VERSION, to_string(MEDIA_API_VERSION_V10));
    Uri updateAlbumUri(uri);
    int32_t changedRows = UserFileClient::Update(updateAlbumUri, predicates, valuesBucket);
    if (changedRows < 0) {
        LOGE("Failed to set %{public}s, err: %{public}d", property.c_str(), changedRows);
        return false;
    }
    return true;
}

static const unordered_map<AlbumChangeOperation, bool (*)(MediaAlbumChangeRequestImpl*)> EXECUTE_MAP = {
    { AlbumChangeOperation::ADD_ASSETS, AddAssetsExecute },
    { AlbumChangeOperation::REMOVE_ASSETS, RemoveAssetsExecute },
};

int32_t MediaAlbumChangeRequestImpl::ApplyChanges()
{
    if (!CheckChangeOperations()) {
        LOGE("Failed to check album change request operations");
        return OHOS_INVALID_PARAM_CODE;
    }
    unordered_set<AlbumChangeOperation> appliedOperations;
    for (const auto& changeOperation : albumChangeOperations_) {
        // Keep the final result(s) of each operation, and commit only once.
        if (appliedOperations.find(changeOperation) != appliedOperations.end()) {
            continue;
        }

        bool valid = false;
        auto iter = EXECUTE_MAP.find(changeOperation);
        if (iter != EXECUTE_MAP.end()) {
            valid = iter->second(this);
        } else if (changeOperation == AlbumChangeOperation::SET_ALBUM_NAME ||
                   changeOperation == AlbumChangeOperation::SET_COVER_URI ||
                   changeOperation == AlbumChangeOperation::SET_IS_ME ||
                   changeOperation == AlbumChangeOperation::SET_DISPLAY_LEVEL ||
                   changeOperation == AlbumChangeOperation::DISMISS ||
                   changeOperation == AlbumChangeOperation::RESET_COVER_URI) {
            valid = SetAlbumPropertyExecute(this, changeOperation);
        } else {
            LOGE("Invalid album change operation: %{public}d", changeOperation);
            albumChangeOperations_.clear();
            return OHOS_INVALID_PARAM_CODE;
        }

        if (!valid) {
            LOGE("Failed to apply album change request, operation: %{public}d", changeOperation);
            albumChangeOperations_.clear();
            return 0;
        }
        appliedOperations.insert(changeOperation);
    }
    albumChangeOperations_.clear();
    return 0;
}
} // namespace Media
} // namespace OHOS