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
#include "medialibrary_ptp_operations.h"

#include <fcntl.h>

#include "dfx_utils.h"
#include "file_asset.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"
#include "result_set_utils.h"
#include "userfile_manager_types.h"
#include "album_accurate_refresh.h"
#include "refresh_business_name.h"
#include "medialibrary_rdb_utils.h"
#include "media_edit_utils.h"

using namespace std;
namespace OHOS::Media {
constexpr int64_t INVALID_SIZE = 0;
// LCOV_EXCL_START
shared_ptr<FileAsset> MediaLibraryPtpOperations::FetchOneFileAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    int32_t currentRowIndex = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowIndex(currentRowIndex) == NativeRdb::E_OK, nullptr, "Cannot get row index of resultset");
    CHECK_AND_RETURN_RET_LOG(currentRowIndex >= 0, nullptr, "Invalid row index");

    auto fileAsset = make_shared<FileAsset>();
    auto &map = fileAsset->GetMemberMap();
    for (const auto &column : columns) {
        int32_t columnIndex = 0;
        CHECK_AND_RETURN_RET_LOG(resultSet->GetColumnIndex(column, columnIndex) == NativeRdb::E_OK,
            nullptr, "Can not get column %{private}s index", column.c_str());
        CHECK_AND_RETURN_RET_LOG(GetFileAssetMemberMap().find(column) != GetFileAssetMemberMap().end(), nullptr,
            "Can not find column %{private}s from member map", column.c_str());
        switch (GetFileAssetMemberMap().at(column)) {
            case MEMBER_TYPE_INT32: {
                int32_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetInt(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get int value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_INT64: {
                int64_t value = 0;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetLong(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get long value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_STRING: {
                string value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetString(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get string value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            case MEMBER_TYPE_DOUBLE: {
                double value;
                CHECK_AND_RETURN_RET_LOG(resultSet->GetDouble(columnIndex, value) == NativeRdb::E_OK, nullptr,
                    "Can not get double value from column %{private}s", column.c_str());
                map[column] = value;
                break;
            }
            default:
                break;
        }
    }
    return fileAsset;
}

shared_ptr<FileAsset> MediaLibraryPtpOperations::GetAssetFromResultSet(
    const shared_ptr<NativeRdb::ResultSet> &resultSet, const vector<string> &columns)
{
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "resultSet is nullptr");
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(count) == NativeRdb::E_OK, nullptr,
        "Cannot get row count of resultset");
    CHECK_AND_RETURN_RET_LOG(count == 1, nullptr, "ResultSet count is %{public}d, not 1", count);
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, nullptr, "Cannot go to first row");
    return FetchOneFileAssetFromResultSet(resultSet, columns);
}

void MediaLibraryPtpOperations::PushMovingPhotoExternalPath(const std::string &path, const std::string &logTarget,
    std::vector<std::string> &attachment)
{
    if (path.empty()) {
        MEDIA_WARN_LOG("%{private}s is invalid.", logTarget.c_str());
        return;
    }
    attachment.push_back(path);
}

int64_t MediaLibraryPtpOperations::GetAssetSize(const std::string &extraPath)
{
    string absExtraPath;
    CHECK_AND_RETURN_RET_LOG(PathToRealPath(extraPath, absExtraPath), static_cast<int64_t>(E_ERR),
        "file is not real path: %{private}s", extraPath.c_str());

    UniqueFd fd(open(absExtraPath.c_str(), O_RDONLY));
    CHECK_AND_RETURN_RET_LOG(fd.Get() != E_ERR, static_cast<int64_t>(E_ERR),
        "failed to open extra file");

    struct stat st;
    CHECK_AND_RETURN_RET_LOG(fstat(fd.Get(), &st) == E_OK, static_cast<int64_t>(E_ERR),
        "failed to get file size");
    off_t fileSize = st.st_size;
    return static_cast<int64_t>(fileSize);
}

void MediaLibraryPtpOperations::GetMovingPhotoExternalInfo(ExternalInfo &exInfo, std::vector<std::string> &attachment)
{
    if (!MovingPhotoFileUtils::IsMovingPhoto(exInfo.subType, exInfo.effectMode, exInfo.originalSubType)) {
        return;
    }
    exInfo.videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(exInfo.path);
    exInfo.extraPath = MovingPhotoFileUtils::GetMovingPhotoExtraDataPath(exInfo.path);
    exInfo.photoImagePath = MovingPhotoFileUtils::GetSourceMovingPhotoImagePath(exInfo.path);
    exInfo.photoVideoPath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(exInfo.path);
    exInfo.cachePath = MovingPhotoFileUtils::GetLivePhotoCachePath(exInfo.path);
    PushMovingPhotoExternalPath(exInfo.videoPath, "videoPath", attachment);
    PushMovingPhotoExternalPath(exInfo.extraPath, "extraPath", attachment);
    PushMovingPhotoExternalPath(exInfo.photoImagePath, "photoImagePath", attachment);
    PushMovingPhotoExternalPath(exInfo.photoVideoPath, "photoVideoPath", attachment);
    PushMovingPhotoExternalPath(exInfo.cachePath, "cachePath", attachment);
    MEDIA_INFO_LOG("videoPath is %{private}s, extraPath is %{private}s, photoImagePath is %{private}s, \
        photoVideoPath is %{private}s, cachePath is %{private}s.", DfxUtils::GetSafePath(exInfo.videoPath).c_str(),
        DfxUtils::GetSafePath(exInfo.extraPath).c_str(), DfxUtils::GetSafePath(exInfo.photoImagePath).c_str(),
        DfxUtils::GetSafePath(exInfo.photoVideoPath).c_str(), DfxUtils::GetSafePath(exInfo.cachePath).c_str());
    exInfo.sizeMp4 = GetAssetSize(exInfo.videoPath);
    exInfo.sizeExtra = GetAssetSize(exInfo.extraPath);
    if (exInfo.sizeMp4 <= INVALID_SIZE) {
        MEDIA_WARN_LOG("failed to get mp4 size.");
    } else {
        exInfo.size += exInfo.sizeMp4;
    }
    if (exInfo.sizeExtra <= INVALID_SIZE) {
        MEDIA_WARN_LOG("failed to get extra size.");
    } else {
        exInfo.size += exInfo.sizeExtra;
    }
    MEDIA_DEBUG_LOG("MovingPhoto size is %{public}" PRId64, exInfo.size);
}

void MediaLibraryPtpOperations::GetEditPhotoExternalInfo(ExternalInfo &exInfo, vector<string> &attachment)
{
    CHECK_AND_RETURN_LOG(exInfo.editTime != 0, "editTime is zero");
    exInfo.editDataPath = MediaEditUtils::GetEditDataPath(exInfo.path);
    exInfo.editDataCameraPath = MediaEditUtils::GetEditDataCameraPath(exInfo.path);
    exInfo.editDataSourcePath = MediaEditUtils::GetEditDataSourcePath(exInfo.path);
    PushMovingPhotoExternalPath(exInfo.editDataPath, "editDataPath", attachment);
    PushMovingPhotoExternalPath(exInfo.editDataCameraPath, "editDataCameraPath", attachment);
    PushMovingPhotoExternalPath(exInfo.editDataSourcePath, "editDataSourcePath", attachment);
    MEDIA_DEBUG_LOG("editDataPath is %{private}s, editDataCameraPath is %{private}s, editDataSourcePath is %{private}s",
        DfxUtils::GetSafePath(exInfo.editDataPath).c_str(), DfxUtils::GetSafePath(exInfo.editDataCameraPath).c_str(),
        DfxUtils::GetSafePath(exInfo.editDataSourcePath).c_str());
}

FileManagement::CloudSync::CleanFileInfo MediaLibraryPtpOperations::GetCleanFileInfo(shared_ptr<FileAsset>&
    fileAssetPtr)
{
    CHECK_AND_RETURN_RET_LOG(fileAssetPtr != nullptr, {}, "GetCleanFileInfo fileAssetPtr is nullptr.");
    FileManagement::CloudSync::CleanFileInfo cleanFileInfo;
    ExternalInfo externalInfo;
    externalInfo.size = fileAssetPtr->GetSize();
    externalInfo.path = fileAssetPtr->GetPath();
    if (externalInfo.size == INVALID_SIZE) {
        externalInfo.size = GetAssetSize(externalInfo.path);
        CHECK_AND_RETURN_RET_LOG(externalInfo.size > INVALID_SIZE, {}, "failed to get asset size.");
    }
    externalInfo.cloudId = fileAssetPtr->GetCloudId();
    externalInfo.subType = fileAssetPtr->GetPhotoSubType();
    externalInfo.effectMode = fileAssetPtr->GetMovingPhotoEffectMode();
    externalInfo.originalSubType = fileAssetPtr->GetOriginalSubType();
    GetMovingPhotoExternalInfo(externalInfo, cleanFileInfo.attachment);
    externalInfo.dateModified = fileAssetPtr->GetDateModified();
    externalInfo.displayName = fileAssetPtr->GetDisplayName();
    externalInfo.editTime = fileAssetPtr->GetPhotoEditTime();
    GetEditPhotoExternalInfo(externalInfo, cleanFileInfo.attachment);
    cleanFileInfo.cloudId = externalInfo.cloudId;
    cleanFileInfo.size = externalInfo.size;
    cleanFileInfo.modifiedTime = externalInfo.dateModified;
    cleanFileInfo.path = externalInfo.path;
    cleanFileInfo.fileName = MediaFileUtils::GetFileName(externalInfo.path);
    return cleanFileInfo;
}

bool MediaLibraryPtpOperations::BatchDeleteLocalAndCloud(const vector<FileManagement::CloudSync::CleanFileInfo>&
    fileInfos, const map<string, int32_t> &notifyMap)
{
    CHECK_AND_RETURN_RET_LOG(!fileInfos.empty(), false, "Batch delete local and cloud fileInfo is empty.");
    vector<string> failCloudId;
    auto ret = FileManagement::CloudSync::CloudSyncManager::GetInstance().BatchCleanFile(fileInfos,
        failCloudId);
    if (ret != 0) {
        MEDIA_ERR_LOG("Failed to delete local and cloud photos permanently.");
        return false;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, false, "watch is nullptr");
    for (const auto &fileInfo : fileInfos) {
        string cloudId = fileInfo.cloudId;
        if (find(failCloudId.begin(), failCloudId.end(), cloudId) != failCloudId.end()) {
            MEDIA_ERR_LOG("Failed to delete, cloudId is %{public}s.", cloudId.c_str());
            continue;
        }
        if (notifyMap.find(cloudId) != notifyMap.end()) {
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(notifyMap.at(cloudId)), NotifyType::NOTIFY_UPDATE);
        }
    }
    if (failCloudId.empty()) {
        MEDIA_DEBUG_LOG("Delete local and cloud photos permanently success");
        return true;
    }
    for (const auto& element : failCloudId) {
        MEDIA_ERR_LOG("Failed to delete, cloudId is %{public}s.", element.c_str());
    }
    return false;
}

int32_t MediaLibraryPtpOperations::DeleteLocalAndCloudPhotos(vector<shared_ptr<FileAsset>> &subFileAsset)
{
    MEDIA_INFO_LOG("DeleteLocalAndCloudPhotos start.");
    vector<FileManagement::CloudSync::CleanFileInfo> fileInfos;
    if (subFileAsset.empty()) {
        MEDIA_INFO_LOG("DeleteLocalAndCloudPhotos subFileAsset is empty.");
        return E_OK;
    }
    map<string, int32_t> notifyMap;
    for (auto& fileAssetPtr : subFileAsset) {
        if (fileAssetPtr == nullptr) {
            continue;
        }
        notifyMap[fileAssetPtr->GetCloudId()] = fileAssetPtr->GetId();
        fileInfos.push_back(GetCleanFileInfo(fileAssetPtr));
    }
    if (!BatchDeleteLocalAndCloud(fileInfos, notifyMap)) {
        MEDIA_ERR_LOG("BatchDeleteLocalAndCloud delete media assert fail.");
        return E_HAS_DB_ERROR;
    }
    return E_OK;
}

int32_t MediaLibraryPtpOperations::GetBurstPhotosInfo(const std::string &burstKey, bool &isLastBurstPhoto,
    vector<int32_t> &burstFileIds)
{
    burstFileIds.clear();
    NativeRdb::RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    vector<string> columns = { MediaColumn::MEDIA_ID };
    queryPredicates.EqualTo(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::MEMBER));
    queryPredicates.EqualTo(PhotoColumn::PHOTO_BURST_KEY, burstKey);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    int32_t count = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_HAS_DB_ERROR, "resultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(
        resultSet->GetRowCount(count) == NativeRdb::E_OK, E_HAS_DB_ERROR, "Cannot get row count of resultset");
    isLastBurstPhoto = count == 1;
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, E_HAS_DB_ERROR, "have no photo asset");
    do {
        burstFileIds.push_back(GetInt32Val(MediaColumn::MEDIA_ID, resultSet));
    } while (resultSet->GoToNextRow() == NativeRdb::E_OK);
    resultSet->Close();
    return NativeRdb::E_OK;
}

static void UpdateAndNotifyBurstAlbum()
{
    vector<string> albumIdsStr;
    int32_t albumId;
    CHECK_AND_RETURN_LOG(MediaLibraryRdbUtils::QueryShootingModeAlbumIdByType(
        ShootingModeAlbumType::BURST_MODE_ALBUM, albumId), "Failed to query BURST album id");
    albumIdsStr.push_back(to_string(albumId));
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "rdbStore is nullptr");
    MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIdsStr);
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const string& albumId : albumIdsStr) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

int32_t MediaLibraryPtpOperations::DeletePtpPhoto(NativeRdb::RdbPredicates &rdbPredicate)
{
    int32_t ret = 0;
    std::vector<std::shared_ptr<FileAsset>> fileAssetVector;
    std::shared_ptr<FileAsset> fileAsset = QueryPhotoInfo(rdbPredicate);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_HAS_DB_ERROR, "fileAsset is nullptr");
    int32_t position = fileAsset->GetPosition();
    if (position == static_cast<int32_t>(PhotoPositionType::CLOUD) ||
        position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
        fileAssetVector.push_back(fileAsset);
        return DeleteLocalAndCloudPhotos(fileAssetVector);
    }
    int32_t subType = fileAsset->GetPhotoSubType();
    int32_t burstCoverLevel = fileAsset->GetBurstCoverLevel();
    string burstKey = fileAsset->GetBurstKey();
    bool isBurstCover = false;
    bool isLastBurstPhoto = false;
    vector<int32_t> burstFileIds;
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>(
        AccurateRefresh::DELETE_PERMANENTLY_BUSSINESS_NAME);
    if (subType == static_cast<int32_t>(PhotoSubType::BURST)) {
        isBurstCover = burstCoverLevel == static_cast<int32_t>(BurstCoverLevelType::COVER);
        ret = GetBurstPhotosInfo(burstKey, isLastBurstPhoto, burstFileIds);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR, "GetBurstPhotosInfo fail.");
        bool needClearBurst = isLastBurstPhoto || isBurstCover;
        ret = UpdateBurstPhotoInfo(burstKey, needClearBurst, rdbPredicate, assetRefresh);
        CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, E_HAS_DB_ERROR, "UpdateBurstPhotoInfo fail.");
        UpdateAndNotifyBurstAlbum();
    }
    if (isBurstCover) {
        auto watch = MediaLibraryNotify::GetInstance();
        CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
        for (const auto &fileId : burstFileIds) {
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_ADD);
            watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + to_string(fileId), NotifyType::NOTIFY_THUMB_ADD);
        }
    }
    return MediaLibraryAssetOperations::DeletePermanently(rdbPredicate, true, assetRefresh);
}

std::shared_ptr<FileAsset> MediaLibraryPtpOperations::QueryPhotoInfo(NativeRdb::RdbPredicates &rdbPredicate)
{
    vector<string> columns = {
        PhotoColumn::PHOTO_CLOUD_ID,
        MediaColumn::MEDIA_SIZE,
        MediaColumn::MEDIA_DATE_MODIFIED,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_POSITION,
        PhotoColumn::PHOTO_BURST_KEY,
        MediaColumn::MEDIA_TYPE,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_ORIGINAL_SUBTYPE,
        PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_OWNER_ALBUM_ID,
        PhotoColumn::PHOTO_BURST_COVER_LEVEL,
    };
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "rdbStore is null");
    auto resultSet = rdbStore->Query(rdbPredicate, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "QueryPhotoInfo fail");
    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == NativeRdb::E_OK, nullptr, "have no photos");
    return GetAssetFromResultSet(resultSet, columns);
}

int32_t MediaLibraryPtpOperations::UpdateBurstPhotoInfo(const std::string &burstKey, const bool isCover,
    NativeRdb::RdbPredicates &rdbPredicate, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    CHECK_AND_RETURN_RET_LOG(assetRefresh != nullptr, E_HAS_DB_ERROR, "assetRefresh is null");
    vector<string> whereArgs;
    string whereClause;
    int32_t changeRows;
    int32_t updateCount = 0;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_BURST_COVER_LEVEL, static_cast<int32_t>(BurstCoverLevelType::COVER));
    values.PutString(PhotoColumn::PHOTO_BURST_KEY, "");
    values.PutInt(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::DEFAULT));
    if (isCover) {
        whereClause = PhotoColumn::PHOTO_BURST_KEY + " = ?";
        whereArgs = {burstKey};
        changeRows = assetRefresh->Update(updateCount, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    } else {
        changeRows = assetRefresh->Update(updateCount, values, rdbPredicate);
    }
    MEDIA_INFO_LOG("UpdateBurstPhotoInfo end updateRows:%{public}d", updateCount);
    CHECK_AND_RETURN_RET_LOG(changeRows == NativeRdb::E_OK && updateCount >= 0, E_HAS_DB_ERROR,
        "Update failed. changeRows:%{public}d, updateRows:%{public}d", changeRows, updateCount);
    return E_OK;
}

int32_t MediaLibraryPtpOperations::DeletePtpAlbum(NativeRdb::RdbPredicates &predicates)
{
    std::vector<string> queryAlbumColumns = {PhotoAlbumColumns::ALBUM_ID};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "rdbStore is nullptr");
    auto albumResultSet = rdbStore->Query(predicates, queryAlbumColumns);
    CHECK_AND_RETURN_RET_LOG(albumResultSet != nullptr, E_ERR, "albumResultSet is nullptr");
    CHECK_AND_RETURN_RET_LOG(albumResultSet->GoToFirstRow() == NativeRdb::E_OK, E_ERR, "get albumResultSet failed");
    int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, albumResultSet);
    albumResultSet->Close();
    NativeRdb::RdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    NativeRdb::RdbPredicates deletePhotoPredicates(PhotoColumn::PHOTOS_TABLE);
    deletePhotoPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(albumId));
    std::vector<std::string> photosCloumns = {PhotoColumn::PHOTO_POSITION};
    auto photosResultSet = rdbStore->Query(deletePhotoPredicates, photosCloumns);
    CHECK_AND_RETURN_RET_LOG(photosResultSet != nullptr, E_ERR, "photosResultSet is nullptr");
    CHECK_AND_PRINT_LOG(photosResultSet->GoToFirstRow() == NativeRdb::E_OK, "photosResultSet do not have photo asset");
    while (photosResultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t position = GetInt32Val(PhotoColumn::PHOTO_POSITION, photosResultSet);
        if (position == static_cast<int32_t>(PhotoPositionType::CLOUD) ||
            position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
            MediaLibraryAssetOperations::DeletePermanently(deletePhotoPredicates, true);
            return E_OK;
        }
    }
    MediaLibraryAssetOperations::DeletePermanently(deletePhotoPredicates, true);
    AccurateRefresh::AlbumAccurateRefresh albumRefresh(AccurateRefresh::DELETE_PTP_ALBUM_BUSSINESS_NAME);
    int deleteRow = -1;
    albumRefresh.LogicalDeleteReplaceByUpdate(predicates, deleteRow);
    CHECK_AND_RETURN_RET_LOG(deleteRow > 0, E_ERR, "delete album fail");
    albumRefresh.Notify();
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_RET_LOG(watch != nullptr, E_ERR, "Can not get MediaLibraryNotify Instance");
    watch->Notify(MediaFileUtils::GetUriByExtrConditions(PhotoAlbumColumns::ALBUM_URI_PREFIX,
        to_string(albumId)), NotifyType::NOTIFY_REMOVE);
    return E_OK;
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media