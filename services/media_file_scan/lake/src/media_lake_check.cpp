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

#define MLOG_TAG "MediaLakeCheck"

#include "media_lake_check.h"

#include <sys/stat.h>
#include <unordered_set>

#include "parameters.h"

#include "asset_accurate_refresh.h"
#include "dfx_utils.h"
#include "file_scan_utils.h"
#include "media_log.h"
#include "media_file_utils.h"
#include "media_file_monitor_rdb_utils.h"
#include "media_lake_album.h"
#include "media_thread.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_unistore_manager.h"
#include "photo_album_column.h"

namespace OHOS::Media {
namespace {
    constexpr int COLUMN_INDEX_MEDIA_ID = 0;
    constexpr int COLUMN_INDEX_STORAGE_PATH = 1;
    constexpr int COLUMN_INDEX_DATE_TAKEN = 2;
}
const char* MEDIA_IN_LAKE_CHECK_TIME = "persist.multimedia.medialibrary.in.lake.check_time"; // s
const char* MEDIA_IN_LAKE_CHECK_PRIVACY_TIME = "persist.multimedia.medialibrary.in.lake.check_privacy_time"; // s
// LCOV_EXCL_START
bool MediaInLakeNeedCheck()
{
    int64_t nowTime = MediaFileUtils::UTCTimeSeconds();
    int64_t defaultValueTime = 0;
    bool isDefaultAccount = FileScanUtils::IsDefaultAccount();
    int64_t lastTime = 0;
    if (isDefaultAccount) {
        lastTime = system::GetIntParameter(MEDIA_IN_LAKE_CHECK_TIME, defaultValueTime);
    } else {
        lastTime = system::GetIntParameter(MEDIA_IN_LAKE_CHECK_PRIVACY_TIME, defaultValueTime);
    }
    if (lastTime == 0) {
        int64_t initCheckTime = 0;
        constexpr int64_t initTimeout = 71 * 60 * 60;
        if (nowTime > initTimeout) {
            initCheckTime = nowTime - initTimeout;
        } else {
            initCheckTime = initTimeout;
        }
        int32_t ret = 0;
        if (isDefaultAccount) {
            ret = system::SetParameter(MEDIA_IN_LAKE_CHECK_TIME, std::to_string(initCheckTime));
        } else {
            ret = system::SetParameter(MEDIA_IN_LAKE_CHECK_PRIVACY_TIME, std::to_string(initCheckTime));
        }
        MEDIA_INFO_LOG("Set init time in lake check, ret:%{public}d, nowTime: %{public}" PRId64 ", checkTime: %{public}" PRId64,
            ret, nowTime, initCheckTime);
        lastTime = initCheckTime;
    }
    constexpr int64_t timeout = 72 * 60 * 60;
    if (nowTime > lastTime + timeout) {
        MEDIA_INFO_LOG("timeout in lake check, nowTime: %{public}" PRId64 ", last checkTime: %{public}" PRId64, nowTime, lastTime);
        return true;
    }
    return false;
}

void MediaInLakeSetCheckFinish()
{
    auto now = MediaFileUtils::UTCTimeSeconds();
    std::string keyValue = std::to_string(now);
    bool isDefaultAccount = FileScanUtils::IsDefaultAccount();
    int32_t ret = 0;
    if (isDefaultAccount) {
        ret = system::SetParameter(MEDIA_IN_LAKE_CHECK_TIME, keyValue);
    } else {
        ret = system::SetParameter(MEDIA_IN_LAKE_CHECK_PRIVACY_TIME, keyValue);
    }
    MEDIA_INFO_LOG("Set in lake check finish, ret: %{public}d", ret);
}

int32_t GetAllInLakeAssetsByAlbumId(int32_t albumId, std::unordered_set<int32_t>& fileIds)
{
    NativeRdb::AbsRdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, FileSourceType::MEDIA_HO_LAKE);
    std::vector<std::string> columns = {MediaColumn::MEDIA_ID};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. resultSet is null");
while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = 0;
        if (resultSet->GetInt(COLUMN_INDEX_MEDIA_ID, fileId) == NativeRdb::E_OK) {
            fileIds.insert(fileId);
        }
    }
    resultSet->Close();

    return E_OK;
}

inline void RemoveScannerFileId(std::unordered_set<int32_t>& fileIds, const std::vector<int32_t>& scannerFileIds)
{
    for (auto fileId: scannerFileIds) {
        fileIds.erase(fileId);
    }
}

std::vector<std::string> GetQueryFileIdsStr(const std::unordered_set<int32_t>& fileIds)
{
    std::vector<std::string> fileIdsStr;
    fileIdsStr.reserve(fileIds.size());
    for (auto fileId : fileIds) {
        fileIdsStr.push_back(std::to_string(fileId));
    }

    return fileIdsStr;
}

int32_t GetAllInLakeAssetsByAlbumId(const std::vector<std::string>& fileIdsStr,
    std::vector<int32_t>& fileIds, std::vector<std::string>& paths)
{
    NativeRdb::AbsRdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(MediaColumn::MEDIA_ID, fileIdsStr);
    std::vector<std::string> columns = {MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_STORAGE_PATH};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. resultSet is null");
while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId = 0;
        std::string path;
        if (resultSet->GetInt(COLUMN_INDEX_MEDIA_ID, fileId) == NativeRdb::E_OK &&
            resultSet->GetString(COLUMN_INDEX_STORAGE_PATH, path) == NativeRdb::E_OK) {
            fileIds.emplace_back(fileId);
            paths.emplace_back(std::move(path));
        }
    }
    resultSet->Close();
    return E_OK;
}

inline int32_t GetThumbByFileId(int32_t albumId, const std::vector<std::string> &fileIdStrs,
    std::vector<int32_t> &fileIds, std::vector<int64_t> &dateTakens, std::vector<std::string> &paths)
{
    NativeRdb::AbsRdbPredicates queryPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.In(MediaColumn::MEDIA_ID, fileIdStrs);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, std::to_string(FileSourceType::MEDIA_HO_LAKE));

    std::vector<std::string> columns = {MediaColumn::MEDIA_ID, PhotoColumn::PHOTO_STORAGE_PATH,
        MediaColumn::MEDIA_DATE_TAKEN};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetAllInLakeAssetsByAlbumId failed. resultSet is null");
    fileIds.reserve(fileIdStrs.size());
    dateTakens.reserve(fileIdStrs.size());
    paths.reserve(fileIdStrs.size());
while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t fileId;
        int64_t dateTaken;
        std::string path;
        if (resultSet->GetInt(COLUMN_INDEX_MEDIA_ID, fileId) == NativeRdb::E_OK &&
            resultSet->GetString(COLUMN_INDEX_STORAGE_PATH, path) == NativeRdb::E_OK &&
            resultSet->GetLong(COLUMN_INDEX_DATE_TAKEN, dateTaken) == NativeRdb::E_OK) {
            paths.emplace_back(std::move(path));
            fileIds.emplace_back(fileId);
            dateTakens.emplace_back(dateTaken);
        }
    }
    resultSet->Close();

    return E_OK;
}

inline void DeleteAssetInfoByFileId(int32_t albumId, const std::vector<std::string> &fileIdStrs)
{
    std::vector<int32_t> fileIds;
    std::vector<int64_t> dateTakens;
    std::vector<std::string> paths;
    auto ret = GetThumbByFileId(albumId, fileIdStrs, fileIds, dateTakens, paths);
    CHECK_AND_RETURN_LOG(ret == E_OK, "GetThumbByFileId failed, ret: %{public}d", ret);
    for (size_t i = 0; i < fileIds.size(); ++i) {
        MediaFileMonitorRdbUtils::DeleteRelatedResource(paths[i],
            std::to_string(fileIds[i]), std::to_string(dateTakens[i]));
    }
}

inline int32_t DeleteDbAssetsByFileIds(int32_t albumId, const std::vector<std::string> &fileIdStrs)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, 0, "DeleteDbAssetsByFileIds failed. rdbStorePtr is null");

    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(MediaColumn::MEDIA_ID, fileIdStrs);
    predicates.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, std::to_string(FileSourceType::MEDIA_HO_LAKE));

    int deletedCount = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh;
    int err = assetRefresh.Delete(deletedCount, predicates);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, 0, "Failed to delete assets, err: %{public}d, fileIds: %{public}zu",
        err, fileIdStrs.size());

    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    MEDIA_INFO_LOG("DeleteDbAssetsByFileIds deleted %{public}d assets for %{public}zu fileIds",
        deletedCount, fileIdStrs.size());
    return deletedCount;
}

inline void NotifyAssetChange(int fileId)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + std::to_string(fileId),
        NotifyType::NOTIFY_REMOVE);
}

int32_t DeleteInLakeAssets(int32_t albumId, const std::unordered_set<int32_t>& fileIds, int32_t& deleteNum)
{
    deleteNum = 0;
    if (fileIds.empty()) {
        return E_OK;
    }

    auto fileIdStrs = GetQueryFileIdsStr(fileIds);
    std::set<std::string> analysisAlbumIds;
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(fileIdStrs, analysisAlbumIds);
    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());

    // 删除缩略图
    DeleteAssetInfoByFileId(albumId, fileIdStrs);

    // 删除并通知
    deleteNum = DeleteDbAssetsByFileIds(albumId, fileIdStrs);
    CHECK_AND_RETURN_RET_LOG(deleteNum > 0, E_ERR, "No valid delete assets");
    for (auto fileId : fileIds) {
        NotifyAssetChange(fileId);
    }
    MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
    return E_OK;
}

inline bool FileExists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0;   // 0 成功 -> 存在
}

void CheckAndDeleteAssetRecord(int32_t albumId, std::unordered_set<int32_t>& fileIds, int32_t& deleteNum)
{
    if (fileIds.empty()) {
        return;
    }
    auto fileIdsIn = GetQueryFileIdsStr(fileIds);
    std::vector<int32_t> fileIdsTmp;
    std::vector<std::string> paths;
    fileIdsTmp.reserve(fileIds.size());
    paths.reserve(fileIds.size());
    auto ret = GetAllInLakeAssetsByAlbumId(fileIdsIn, fileIdsTmp, paths);
    CHECK_AND_RETURN_LOG(ret == E_OK, "GetAllInLakeAssetsByAlbumId failed. ret: %{public}d", ret);
    for (size_t i = 0; i < paths.size(); ++i) {
        if (FileExists(paths[i].c_str())) {
            fileIds.erase(fileIdsTmp[i]);
        } else {
            MEDIA_INFO_LOG("Need delete file, id: %{public}d, path: %{public}s", fileIdsTmp[i],
                DfxUtils::GetSafePath(paths[i]).c_str());
        }
    }
    DeleteInLakeAssets(albumId, fileIds, deleteNum);
}

void CheckAndIfNeedDeleteAssets(int32_t albumId, const std::vector<int32_t>& scannerFileIds, int32_t& deleteNum)
{
    std::unordered_set<int32_t> fileIdsInDb;
    fileIdsInDb.reserve(scannerFileIds.size() + 1);
    GetAllInLakeAssetsByAlbumId(albumId, fileIdsInDb);
    MEDIA_INFO_LOG("photo album: %{public}d, assetsInDb: %{public}zu, scannerFiles: %{public}zu", albumId,
        fileIdsInDb.size(), scannerFileIds.size());
    RemoveScannerFileId(fileIdsInDb, scannerFileIds);
    CheckAndDeleteAssetRecord(albumId, fileIdsInDb, deleteNum);
}

int32_t GetAllInLakePhotoAlbum(std::vector<std::string>& paths)
{
    NativeRdb::AbsRdbPredicates queryPredicates(PhotoAlbumColumns::TABLE);
    std::vector<std::string> albumTypes {
        std::to_string(PhotoAlbumType::SOURCE),
        std::to_string(PhotoAlbumType::USER)
    };
    queryPredicates.In(PhotoAlbumColumns::ALBUM_TYPE, albumTypes);
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_LPATH};
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "GetAllInLakeAlbum failed. rdbStorePtr is null");
    auto resultSet = rdbStore->Query(queryPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR, "GetAllInLakeAlbum failed. resultSet is null");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string path;
        if (resultSet->GetString(0, path) == NativeRdb::E_OK) {
            paths.emplace_back(std::move(path));
        }
    }
    resultSet->Close();
    return E_OK;
}

inline std::string LPathToInLakeRealPath(const std::string &uri)
{
    const std::string inLakeRoot = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC";
    return inLakeRoot + uri;
}

bool CheckAndIfNeedDeletePhotoAlbum(int32_t& deletePhotoNum, std::function<bool()> isInterrupted)
{
    MEDIA_INFO_LOG("CheckAndIfNeedDeletePhotoAlbum enter");
    deletePhotoNum = 0;
    std::vector<std::string> paths;
    GetAllInLakePhotoAlbum(paths);
    for (const auto& path: paths) {
        if (isInterrupted()) {
            MEDIA_WARN_LOG("interrupted, stop lake check, lPath: %{public}s", DfxUtils::GetSafePath(path).c_str());
            return false;
        }

        std::string lakeRealPath = LPathToInLakeRealPath(path);
        if (!FileExists(lakeRealPath.c_str())) {
            MEDIA_INFO_LOG("delete photo album, lake_path: %{public}s",
                FileScanUtils::GarbleFilePath(lakeRealPath).c_str());
            // Delete PhotoAblbum
            auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
            int32_t perDelNum = 0;
            MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(lakeRealPath, rdbStore, &perDelNum);
            deletePhotoNum += perDelNum;
        } else {
            MEDIA_DEBUG_LOG("photo album in lake, lPath: %{public}s", MediaFileUtils::DesensitizePath(path).c_str());
        }
    }
    MEDIA_INFO_LOG("CheckAndIfNeedDeletePhotoAlbum exit");
    return true;
}

void ClearLakeAlbum()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "ClearLakeAlbum failed. rdbStore is nullptr");

    NativeRdb::AbsRdbPredicates predicates(LAKE_ALBUM_TABLE);
    std::string whereClause =
        "NOT EXISTS (SELECT 1 FROM PhotoAlbum WHERE PhotoAlbum.album_id = LakeAlbum.album_id)";
    predicates.SetWhereClause(whereClause);
    int32_t deletedRows = 0;
    int32_t ret = rdbStore->Delete(deletedRows, predicates);
    if (ret != E_OK || deletedRows < 0) {
        MEDIA_ERR_LOG("ClearLakeAlbum failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);
        return;
    }
    MEDIA_INFO_LOG("ClearLakeAlbum succeeded, deletedRows: %{public}d", deletedRows);
}
}