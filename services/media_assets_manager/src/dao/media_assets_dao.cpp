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

#define MLOG_TAG "Media_Dao"

#include "media_assets_dao.h"

#include "media_log.h"
#include "medialibrary_unistore_manager.h"
#include "media_column.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "media_file_utils.h"
#include "medialibrary_notify.h"
#include "medialibrary_errno.h"

namespace OHOS::Media::Common {
using namespace OHOS::Media::ORM;
int32_t MediaAssetsDao::QueryAssets(const std::vector<std::string> &fileIds, std::vector<PhotosPo> &queryResult)
{
    CHECK_AND_RETURN_RET_LOG(fileIds.size() > 0, E_INVALID_ARGUMENTS, "Empty query condition.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::MEDIA_ID, fileIds);
    /* query */
    auto resultSet = rdbStore->Query(predicates, {});
    /* results to records */
    return ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(queryResult);
}

int32_t MediaAssetsDao::CreateNewAsset(std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh,
    int64_t &newAssetId, NativeRdb::ValuesBucket &values)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "CreateNewAsset Failed to get photoRefresh.");
    int32_t ret = photoRefresh->Insert(newAssetId, PhotoColumn::PHOTOS_TABLE, values);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ret, "Failed to insert new asset, ret = %{public}d.", ret);
    MEDIA_INFO_LOG("Insert meta data success, rowId = %{public}" PRId64 ", ret = %{public}d", newAssetId, ret);
    return ret;
}

int32_t MediaAssetsDao::ClearCloudInfo(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, const int32_t fileId)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_INVAL_ARG, "ClearCloudInfo invalid fileId.");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "ClearCloudInfo Failed to get photoRefresh.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    NativeRdb::ValuesBucket values;
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL));
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("ClearCloudInfo Update Ret: %{public}d, ChangedRows: %{public}d", ret, changedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to ClearCloudInfo.");
    CHECK_AND_RETURN_RET_WARN_LOG(changedRows > 0, ret, "ClearCloudInfo Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_UPDATE);
    return ret;
}

int32_t MediaAssetsDao::ResetPositionToCloudOnly(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, int32_t fileId)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_INVAL_ARG, "ResetPositionToCloudOnly invalid fileId.");
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "ResetPositionToCloudOnly Failed to get photoRefresh.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("ResetPositionToCloudOnly Update Ret: %{public}d, ChangedRows: %{public}d", ret, changedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to ResetPositionToCloudOnly.");
    CHECK_AND_RETURN_RET_WARN_LOG(
        changedRows > 0, ret, "ResetPositionToCloudOnly Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDao::QueryAlbumByAlbumId(const int32_t albumId, std::optional<PhotoAlbumPo> &albumInfo)
{
    CHECK_AND_RETURN_RET_LOG(albumId > 0, E_INVAL_ARG, "Invalid albumId, albumId: %{public}d", albumId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_ID, albumId);
    /* query */
    auto resultSet = rdbStore->Query(predicates, {});
    /* results to records */
    std::vector<PhotoAlbumPo> albumInfos;
    int32_t ret = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(resultSet).ReadRecords(albumInfos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && albumInfos.size() > 0,
        E_QUERY_CONTENT_IS_EMPTY,
        "Failed to query OR content is empty. ret: %{public}d, size: %{public}zu",
        ret,
        albumInfos.size());
    albumInfo = albumInfos[0];
    return E_OK;
}

int32_t MediaAssetsDao::QueryAlbumBylPath(const std::string &lPath, std::optional<PhotoAlbumPo> &albumInfo)
{
    CHECK_AND_RETURN_RET_LOG(!lPath.empty(), E_INVAL_ARG, "Invalid lPath, lPath: %{public}s", lPath.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    std::vector<NativeRdb::ValueObject> bindArgs = {lPath};
    auto resultSet = rdbStore->QuerySql(this->SQL_PHOTO_ALBUM_QUERY_BY_LPATH, bindArgs);
    /* results to records */
    std::vector<PhotoAlbumPo> albumInfos;
    int32_t ret = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(resultSet).ReadRecords(albumInfos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && albumInfos.size() > 0,
        E_QUERY_CONTENT_IS_EMPTY,
        "Failed to query OR content is empty. ret: %{public}d, size: %{public}zu",
        ret,
        albumInfos.size());
    albumInfo = albumInfos[0];
    return E_OK;
}

std::string MediaAssetsDao::GetLpathFromSourcePath(const std::string &sourcePath)
{
    size_t pos = sourcePath.find(SOURCE_PATH_PERFIX);
    if (pos == std::string::npos) {
        MEDIA_ERR_LOG("invalid path %{private}s", MediaFileUtils::DesensitizePath(sourcePath).c_str());
        return "";
    }

    size_t lpathStart = pos + SOURCE_PATH_PERFIX.length();
    size_t lpathEnd = sourcePath.rfind('/');
    if (lpathEnd == std::string::npos || lpathEnd <= lpathStart) {
        MEDIA_ERR_LOG("invalid path %{private}s", MediaFileUtils::DesensitizePath(sourcePath).c_str());
        return "";
    }
    return sourcePath.substr(lpathStart, lpathEnd - lpathStart);
}

int32_t MediaAssetsDao::QueryAlbum(
    const int32_t albumId, const std::string &sourcePath, std::optional<PhotoAlbumPo> &albumInfo)
{
    int32_t ret = this->QueryAlbumByAlbumId(albumId, albumInfo);
    bool isValid = ret == E_OK && albumInfo.has_value();
    CHECK_AND_RETURN_RET(!isValid, ret);
    std::string lPath = this->GetLpathFromSourcePath(sourcePath);
    return this->QueryAlbumBylPath(lPath, albumInfo);
}

int32_t MediaAssetsDao::FindSamePhoto(const PhotosPo &photoInfo, std::optional<PhotosPo> &samePhotoInfoOp)
{
    // Find the corresponding PhotoAlbum of the asset.
    // query album info from database by owner_album_id or source_path
    std::optional<PhotoAlbumPo> photoAlbumPoOp;
    int32_t ret =
        this->QueryAlbum(photoInfo.ownerAlbumId.value_or(0), photoInfo.sourcePath.value_or(""), photoAlbumPoOp);
    bool isValid = ret == E_OK && photoAlbumPoOp.has_value();
    if (isValid) {
        // Find the same asset in the PhotoAlbum, which has the same display_name, size, orientation(picture only).
        ret = this->FindSamePhotoInTargetAlbum(photoInfo, photoAlbumPoOp.value(), samePhotoInfoOp);
    }
    bool hasFoundSameAsset = ret == E_OK && samePhotoInfoOp.has_value();
    CHECK_AND_RETURN_RET(!hasFoundSameAsset, ret);
    // Find the same asset without PhotoAlbum, which has the same display_name, size, orientation(picture only).
    ret = this->FindSamePhotoInHiddenAlbum(photoInfo, samePhotoInfoOp);
    return ret;
}

int32_t MediaAssetsDao::FindSamePhotoInHiddenAlbum(const PhotosPo &photoInfo, std::optional<PhotosPo> &samePhotoInfoOp)
{
    bool isValid = photoInfo.sourcePath.has_value();
    isValid = isValid && photoInfo.displayName.has_value();
    isValid = isValid && photoInfo.size.has_value();
    isValid = isValid && photoInfo.orientation.has_value();
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_INVAL_ARG, "No need to find same photo, not hidden asset.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    std::vector<NativeRdb::ValueObject> bindArgs = {
        photoInfo.sourcePath.value_or(""),
        photoInfo.displayName.value_or(""),
        photoInfo.size.value_or(0),
        photoInfo.orientation.value_or(0),
    };
    auto resultSet = rdbStore->QuerySql(this->SQL_PHOTOS_QUERY_FOR_SAME_IN_HIDDEN_ALBUM, bindArgs);
    /* results to records */
    std::vector<PhotosPo> photosPos;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && photosPos.size() > 0,
        E_QUERY_CONTENT_IS_EMPTY,
        "Failed to query OR content is empty. ret: %{public}d, size: %{public}zu",
        ret,
        photosPos.size());
    samePhotoInfoOp = photosPos[0];
    MEDIA_INFO_LOG("FindSamePhotoInHiddenAlbum, found same photo, id: %{public}d", photosPos[0].fileId.value_or(0));
    return E_OK;
}

int32_t MediaAssetsDao::FindSamePhotoInTargetAlbum(
    const PhotosPo &photoInfo, const PhotoAlbumPo &albumInfo, std::optional<PhotosPo> &samePhotoInfoOp)
{
    bool isValid = albumInfo.albumId.has_value();
    isValid = isValid && photoInfo.displayName.has_value();
    isValid = isValid && photoInfo.size.has_value();
    isValid = isValid && photoInfo.orientation.has_value();
    CHECK_AND_RETURN_RET_LOG(isValid, E_INVAL_ARG, "Invalid args.");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    std::vector<NativeRdb::ValueObject> bindArgs = {
        albumInfo.albumId.value_or(0),
        photoInfo.displayName.value_or(""),
        photoInfo.size.value_or(0),
        photoInfo.orientation.value_or(0),
    };
    auto resultSet = rdbStore->QuerySql(this->SQL_PHOTOS_QUERY_FOR_SAME, bindArgs);
    /* results to records */
    std::vector<PhotosPo> photosPos;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photosPos);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && photosPos.size() > 0,
        E_QUERY_CONTENT_IS_EMPTY,
        "Failed to query OR content is empty. ret: %{public}d, size: %{public}zu",
        ret,
        photosPos.size());
    samePhotoInfoOp = photosPos[0];
    MEDIA_INFO_LOG("FindSamePhotoInTargetAlbum, found same photo, id: %{public}d", photosPos[0].fileId.value_or(0));
    return E_OK;
}

int32_t MediaAssetsDao::MergeCloudInfoIntoTargetPhoto(const PhotosPo &sourcePhotoInfo, const PhotosPo &targetPhotoInfo,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = sourcePhotoInfo.fileId.value_or(0) != targetPhotoInfo.fileId.value_or(0);
    CHECK_AND_RETURN_RET_LOG(isValid, E_OK, "MergeCloudInfoIntoTargetPhoto, same fileId, no need to merge.");
    isValid = !sourcePhotoInfo.cloudId.value_or("").empty() && targetPhotoInfo.cloudId.value_or("").empty();
    CHECK_AND_RETURN_RET_LOG(
        isValid, E_INVALID_ARGUMENTS, "MergeCloudInfoIntoTargetPhoto, invalid cloud info, no need to merge.");
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "MergeCloudInfoIntoTargetPhoto Failed to get photoRefresh.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, targetPhotoInfo.fileId.value_or(0));
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, sourcePhotoInfo.cloudId.value_or(""));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, sourcePhotoInfo.cloudVersion.value_or(0));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, sourcePhotoInfo.TryGetMdirty());
    // bitwise OR, merge position flag. 1 | 2 = 3, means both local and cloud.
    uint32_t position = static_cast<uint32_t>(sourcePhotoInfo.position.value_or(0)) |
                        static_cast<uint32_t>(targetPhotoInfo.position.value_or(0));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(position));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("MergeCloudInfoIntoTargetPhoto Completed, "
                   "ret: %{public}d, ChangedRows: %{public}d, source-cloudId: %{public}s",
        ret,
        changedRows,
        sourcePhotoInfo.cloudId.value_or("").c_str());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to MergeCloudInfoIntoTargetPhoto.");
    CHECK_AND_RETURN_RET_WARN_LOG(
        changedRows > 0, ret, "MergeCloudInfoIntoTargetPhoto Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(
        PhotoColumn::PHOTO_URI_PREFIX + to_string(targetPhotoInfo.fileId.value_or(0)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDao::DeletePhotoInfo(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, const int32_t fileId)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, E_INVAL_ARG, "DeletePhotoInfo invalid fileId.");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "photoRefresh Failed to get photoRefresh.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    int32_t deletedRows = -1;
    auto ret = photoRefresh->Delete(deletedRows, predicates);
    bool cond = (ret != NativeRdb::E_OK || deletedRows < 0);
    CHECK_AND_PRINT_LOG(!cond, "Delete db failed, errCode = %{public}d", ret);
    MEDIA_INFO_LOG("DeletePhotoInfo Delete retVal: %{public}d, deletedRows: %{public}d", ret, deletedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_REMOVE);
    return E_OK;
}

int32_t MediaAssetsDao::MoveOutTrash(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "MoveOutTrash Failed to get photoRefresh.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, photoInfo.fileId.value_or(0));
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    NativeRdb::ValuesBucket values;
    values.PutInt(MediaColumn::MEDIA_DATE_TRASHED, 0);
    values.PutLong(PhotoColumn::PHOTO_META_DATE_MODIFIED, MediaFileUtils::UTCTimeMilliSeconds());
    values.PutLong(PhotoColumn::PHOTO_LAST_VISIT_TIME, MediaFileUtils::UTCTimeMilliSeconds());
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("MoveOutTrash Update Ret: %{public}d, ChangedRows: %{public}d", ret, changedRows);
    CHECK_AND_RETURN_RET_WARN_LOG(changedRows > 0, ret, "MoveOutTrash Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(photoInfo.fileId.value_or(0)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDao::LogicalDeleteCloudTrashedPhoto(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "MoveOutTrash Failed to get photoRefresh.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, photoInfo.fileId.value_or(0));
    predicates.GreaterThan(MediaColumn::MEDIA_DATE_TRASHED, to_string(0));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->LogicalDeleteReplaceByUpdate(predicates, changedRows);
    MEDIA_INFO_LOG("MoveOutTrash Update Ret: %{public}d, ChangedRows: %{public}d", ret, changedRows);
    CHECK_AND_RETURN_RET_WARN_LOG(changedRows > 0, ret, "MoveOutTrash Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(photoInfo.fileId.value_or(0)), NotifyType::NOTIFY_REMOVE);
    return E_OK;
}

bool MediaAssetsDao::IsSameAssetIgnoreAlbum(const PhotosPo &photoInfo, const PhotosPo &targetPhotoInfo)
{
    bool isSame = photoInfo.displayName.value_or("") == targetPhotoInfo.displayName.value_or("");
    isSame = isSame && photoInfo.mediaType.value_or(0) == targetPhotoInfo.mediaType.value_or(0);
    isSame = isSame && photoInfo.size.value_or(0) == targetPhotoInfo.size.value_or(0);
    bool isSameOrientation = photoInfo.mediaType.value_or(0) == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)
                                 ? photoInfo.orientation.value_or(0) == targetPhotoInfo.orientation.value_or(0)
                                 : true;
    return isSame && isSameOrientation;
}

int32_t MediaAssetsDao::UpdatePositionToBoth(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t position = photoInfo.position.value_or(1);
    bool isValid = position != static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "UpdatePositionToBoth, position is correct, no need to update.");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "UpdatePosition Failed to get photoRefresh.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, photoInfo.fileId.value_or(0));
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("UpdatePositionToBoth Completed, "
                   "ret: %{public}d, ChangedRows: %{public}d, cloudId: %{public}s",
        ret,
        changedRows,
        photoInfo.cloudId.value_or("").c_str());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdatePositionToBoth.");
    CHECK_AND_RETURN_RET_WARN_LOG(
        changedRows > 0, ret, "UpdatePositionToBoth Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(photoInfo.fileId.value_or(0)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDao::UpdatePositionToBothAndFileSourceTypeToLake(
    const PhotosPo &photoInfo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t position = photoInfo.position.value_or(1);
    bool isValid = position != static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD);
    CHECK_AND_RETURN_RET_INFO_LOG(
        isValid, E_OK, "UpdatePositionToBothAndFileSourceTypeToLake, position is correct, no need to update.");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr,
        E_RDB_STORE_NULL,
        "UpdatePositionToBothAndFileSourceTypeToLake Failed to get photoRefresh.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, photoInfo.fileId.value_or(0));
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE));
    int32_t changedRows = -1;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("UpdatePositionToBothAndFileSourceTypeToLake Completed, "
                   "ret: %{public}d, ChangedRows: %{public}d, cloudId: %{public}s",
        ret,
        changedRows,
        photoInfo.cloudId.value_or("").c_str());
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdatePositionToBothAndFileSourceTypeToLake.");
    CHECK_AND_RETURN_RET_WARN_LOG(
        changedRows > 0, ret, "UpdatePositionToBothAndFileSourceTypeToLake Check updateRows: %{public}d.", changedRows);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, ret, "watch is nullptr");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(photoInfo.fileId.value_or(0)), NotifyType::NOTIFY_UPDATE);
    return E_OK;
}

int32_t MediaAssetsDao::FindAssetsByBurstKey(const std::string &burstKey, std::vector<PhotosPo> &photoInfoList)
{
    CHECK_AND_RETURN_RET(!burstKey.empty(), E_INVAL_ARG);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    auto resultSet = rdbStore->Query(predicates, {});
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(photoInfoList);
    MEDIA_INFO_LOG("FindAssetsByBurstKey, ret: %{public}d, burstKey: %{public}s, size: %{public}zu",
        ret, burstKey.c_str(), photoInfoList.size());
    return ret;
}
}  // namespace OHOS::Media::Common