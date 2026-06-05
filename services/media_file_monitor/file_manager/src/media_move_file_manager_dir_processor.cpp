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
#include "media_move_file_manager_dir_processor.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "album_accurate_refresh.h"
#include "asset_accurate_refresh.h"
#include "dfx_utils.h"
#include "file_const.h"
#if defined(MEDIALIBRARY_FILE_MGR_SUPPORT) || defined(MEDIALIBRARY_LAKE_SUPPORT)
#include "folder_scanner.h"
#endif
#include "media_column.h"
#include "media_file_monitor_rdb_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_utils.h"
#include "photo_album_column.h"
#include "rdb_predicates.h"
#include "value_object.h"

namespace OHOS::Media {
using namespace std;
using namespace OHOS::NativeRdb;
using namespace OHOS::Media::AccurateRefresh;

constexpr int32_t INVALID_ID = -1;
constexpr int32_t INVALID_COUNT = 0;

struct AlbumDetailInfo {
    int32_t albumId = INVALID_ID;
    int32_t albumType = INVALID_COUNT;
    int32_t albumSubtype = INVALID_COUNT;
    std::string albumName;
    std::string albumLPath;
};

struct MoveDirData {
    std::string oldPath;
    std::string newPath;
    std::string oldLPath;
    std::string newLPath;
    std::string newAlbumName;
    std::vector<AlbumDetailInfo> albumDetails;  // old album details
    std::vector<int32_t> oldAlbumIds;           // old album ids
    unordered_map<int32_t, int32_t> albumCounts;
    std::vector<LakeMonitorQueryResultData> dataList;
    std::set<std::string> analysisAlbumIds;
    std::unordered_map<int32_t, int32_t> albumIdMap; // oldAlbumId -> newAlbumId mapping
    std::vector<std::string> newAlbumIdStrings; // new album ids as strings for NotifyAddAlbums
};
// LCOV_EXCL_START
std::string GetLastPathComponent(const std::string &path)
{
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos || pos == path.size() - 1) {
        return path;
    }
    return path.substr(pos + 1);
}

bool QueryAlbumDetailByLPath(const shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::string &lPath, std::vector<AlbumDetailInfo> &albumDetails)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lPath)
           ->Or()
           ->Like(PhotoAlbumColumns::ALBUM_LPATH, lPath + "/%");
    predicates.EndWrap();
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, "2048");

    auto resultSet = rdbStore->QueryByStep(predicates, {
        PhotoAlbumColumns::ALBUM_ID,
        PhotoAlbumColumns::ALBUM_TYPE,
        PhotoAlbumColumns::ALBUM_SUBTYPE,
        PhotoAlbumColumns::ALBUM_NAME,
        PhotoAlbumColumns::ALBUM_LPATH
    });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "QueryByStep returned nullptr, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    while (resultSet->GoToNextRow() == E_OK) {
        AlbumDetailInfo detail;
        int index = -1;
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_ID, index) == E_OK) {
            resultSet->GetInt(index, detail.albumId);
        }
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_TYPE, index) == E_OK) {
            resultSet->GetInt(index, detail.albumType);
        }
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_SUBTYPE, index) == E_OK) {
            resultSet->GetInt(index, detail.albumSubtype);
        }
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_NAME, index) == E_OK) {
            resultSet->GetString(index, detail.albumName);
        }
        if (resultSet->GetColumnIndex(PhotoAlbumColumns::ALBUM_LPATH, index) == E_OK) {
            resultSet->GetString(index, detail.albumLPath);
        }
        CHECK_AND_CONTINUE_ERR_LOG(detail.albumId > 0, "Invalid albumId found for lPath: %{public}s",
            DfxUtils::GetSafePath(lPath).c_str());
        albumDetails.push_back(detail);
    }
    resultSet->Close();
    CHECK_AND_RETURN_RET_LOG(!albumDetails.empty(), false,
        "No valid album found for lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());
    return true;
}

bool MarkAssetsTimePending(const std::vector<int32_t> &albumIds, int64_t timePendingValue)
{
    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), false, "albumIds is empty");

    AssetAccurateRefresh assetRefresh;
    ValuesBucket values;
    values.PutLong(MediaColumn::MEDIA_TIME_PENDING, timePendingValue);

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> albumIdStrings;
    albumIdStrings.reserve(albumIds.size());
    for (int32_t id : albumIds) {
        albumIdStrings.push_back(to_string(id));
    }
    predicates.In(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumIdStrings);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        to_string(static_cast<int32_t>(FileSourceType::FILE_MANAGER)));

    int32_t changedRows = 0;
    CHECK_AND_RETURN_RET_LOG(
        assetRefresh.Update(changedRows, values, predicates) == E_OK, false,
        "MarkAssetsTimePending Update failed, timePending: %{public}" PRId64, timePendingValue);

    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();
    MEDIA_INFO_LOG("Marked %{public}d assets time_pending=%{public}" PRId64, changedRows, timePendingValue);
    return true;
}

std::string ComputeNewLPath(const std::string &oldLPath, const std::string &detailLPath, const std::string &newLPath)
{
    // Top-level renamed album: lpath fully replaced
    if (detailLPath == oldLPath) {
        return newLPath;
    }
    // Sub-album: only replace lpath prefix, keep suffix unchanged
    if (detailLPath.size() > oldLPath.size() && detailLPath[oldLPath.size()] == '/' &&
        detailLPath.compare(0, oldLPath.size(), oldLPath) == 0) {
        return newLPath + detailLPath.substr(oldLPath.size());
    }
    MEDIA_ERR_LOG("Album lpath mismatch, skip: %{public}s", DfxUtils::GetSafePath(detailLPath).c_str());
    return "";
}

// Top-level album gets new directory name; sub-albums keep original name
std::string ComputeNewAlbumName(const std::string &oldLPath, const std::string &newAlbumName,
    const std::string &detailLPath, const std::string &detailName)
{
    if (detailLPath == oldLPath) {
        return newAlbumName;
    }
    return detailName;
}

bool CreateAlbumsByLPathReplace(MoveDirData &moveDirData)
{
    CHECK_AND_RETURN_RET_LOG(!moveDirData.albumDetails.empty(), false, "albumDetails is empty");

    AlbumAccurateRefresh albumRefresh;

    for (const auto &detail : moveDirData.albumDetails) {
        std::string newLPathForThisAlbum =
            ComputeNewLPath(moveDirData.oldLPath, detail.albumLPath, moveDirData.newLPath);
        if (newLPathForThisAlbum.empty()) {
            continue;
        }

        std::string nameForNewAlbum = ComputeNewAlbumName(moveDirData.oldLPath, moveDirData.newAlbumName,
            detail.albumLPath, detail.albumName);

        ValuesBucket values;
        values.PutString(PhotoAlbumColumns::ALBUM_NAME, nameForNewAlbum);
        values.PutInt(PhotoAlbumColumns::ALBUM_TYPE, detail.albumType);
        values.PutInt(PhotoAlbumColumns::ALBUM_SUBTYPE, detail.albumSubtype);
        values.PutString(PhotoAlbumColumns::ALBUM_LPATH, newLPathForThisAlbum);

        int64_t newRowId = 0;
        int32_t ret = albumRefresh.Insert(newRowId, PhotoAlbumColumns::TABLE, values);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK && newRowId > 0, false,
            "Insert album failed, ret: %{public}d, newRowId: %{public}" PRId64, ret, newRowId);

        moveDirData.albumIdMap[detail.albumId] = static_cast<int32_t>(newRowId);
        moveDirData.newAlbumIdStrings.push_back(to_string(static_cast<int32_t>(newRowId)));
        MEDIA_INFO_LOG("New album created, oldId: %{public}d -> newId: %{public}d, "
            "lpath: %{public}s -> %{public}s, name: %{public}s -> %{public}s",
            detail.albumId, static_cast<int32_t>(newRowId),
            DfxUtils::GetSafePath(detail.albumLPath).c_str(),
            DfxUtils::GetSafePath(newLPathForThisAlbum).c_str(),
            detail.albumName.c_str(), nameForNewAlbum.c_str());
    }
    CHECK_AND_RETURN_RET_LOG(!moveDirData.albumIdMap.empty(), false, "albumIdMap is empty after creation");
    CHECK_AND_RETURN_RET_LOG(albumRefresh.NotifyAddAlbums(moveDirData.newAlbumIdStrings) == E_OK, false,
        "AlbumAccurateRefresh NotifyAddAlbums failed");
    return true;
}

// Delete old albums after new albums are created
bool DeleteAlbumsByIds(const std::vector<int32_t> &albumIds)
{
    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), false, "albumIds is empty");

    std::vector<std::string> albumIdStrings;
    albumIdStrings.reserve(albumIds.size());
    for (int32_t id : albumIds) {
        albumIdStrings.push_back(to_string(id));
    }

    RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, albumIdStrings);

    AlbumAccurateRefresh albumRefresh;
    int32_t deletedRows = 0;
    int32_t ret = albumRefresh.LogicalDeleteReplaceByUpdate(predicates, deletedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false,
        "DeleteAlbumsByIds failed, ret: %{public}d, deletedRows: %{public}d", ret, deletedRows);

    albumRefresh.Notify();

    MEDIA_INFO_LOG("Deleted %{public}d old albums", deletedRows);
    return true;
}

bool RefreshAssetsForDirMove(const unordered_map<int32_t, int32_t> &albumIdMap,
    const std::string &oldPathPrefix, const std::string &newPathPrefix)
{
    CHECK_AND_RETURN_RET_LOG(!albumIdMap.empty(), false, "albumIdMap is empty");

    AssetAccurateRefresh assetRefresh;

    for (const auto &[oldAlbumId, newAlbumId] : albumIdMap) {
        std::string sql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET "
            + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = ?, "
            + PhotoColumn::PHOTO_STORAGE_PATH + " = REPLACE("
                + PhotoColumn::PHOTO_STORAGE_PATH + ", ?, ?), "
            + MediaColumn::MEDIA_TIME_PENDING + " = 0 "
            + "WHERE " + PhotoColumn::PHOTO_OWNER_ALBUM_ID + " = ? AND "
            + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = ?";

        std::vector<ValueObject> bindArgs;
        bindArgs.push_back(ValueObject(static_cast<int32_t>(newAlbumId)));
        bindArgs.push_back(ValueObject(oldPathPrefix));
        bindArgs.push_back(ValueObject(newPathPrefix));
        bindArgs.push_back(ValueObject(static_cast<int32_t>(oldAlbumId)));
        bindArgs.push_back(ValueObject(to_string(static_cast<int32_t>(FileSourceType::FILE_MANAGER))));

        CHECK_AND_RETURN_RET_LOG(
            assetRefresh.ExecuteSql(sql, bindArgs, RDB_OPERATION_UPDATE) == E_OK, false,
            "RefreshAssetsForDirMove ExecuteSql failed, oldId:%{public}d->newId:%{public}d",
            oldAlbumId, newAlbumId);
    }

    assetRefresh.RefreshAlbum();
    assetRefresh.Notify();

    MEDIA_INFO_LOG("RefreshAssetsForDirMove completed, %{public}zu mappings, "
        "oldPrefix: %{public}s -> newPrefix: %{public}s",
        albumIdMap.size(),
        DfxUtils::GetSafePath(oldPathPrefix).c_str(), DfxUtils::GetSafePath(newPathPrefix).c_str());
    return true;
}

bool QueryMoveDirData(const shared_ptr<MediaLibraryRdbStore> &rdbStore, MoveDirData &moveDirData)
{
    CHECK_AND_RETURN_RET_LOG(QueryAlbumDetailByLPath(rdbStore, moveDirData.oldLPath, moveDirData.albumDetails),
        false, "QueryAlbumDetailByLPath failed");

    CHECK_AND_RETURN_RET_LOG(
        MediaFileMonitorRdbUtils::QueryAlbumByLPath(
            rdbStore, moveDirData.oldLPath, moveDirData.oldAlbumIds, moveDirData.albumCounts),
        false, "QueryAlbumByLPath failed");

    CHECK_AND_RETURN_RET_LOG(
        MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(rdbStore, moveDirData.oldAlbumIds, moveDirData.dataList,
            FileSourceType::FILE_MANAGER),
        false, "QueryDataListByAlbumIds failed");

    std::vector<std::string> fileIds;
    for (auto &data : moveDirData.dataList) {
        fileIds.emplace_back(to_string(data.fileId));
    }
    if (!fileIds.empty()) {
        MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(fileIds, moveDirData.analysisAlbumIds);
    }
    return true;
}

bool SwapAlbums(MoveDirData &moveDirData)
{
    CHECK_AND_RETURN_RET_LOG(
        CreateAlbumsByLPathReplace(moveDirData),
        false, "CreateAlbumsByLPathReplace failed");

    CHECK_AND_RETURN_RET_LOG(DeleteAlbumsByIds(moveDirData.oldAlbumIds),
        false, "DeleteAlbumsByIds failed");
    return true;
}

void NotifyMoveDirResult(const MoveDirData &moveDirData)
{
    if (!moveDirData.analysisAlbumIds.empty()) {
        std::vector<std::string> albumIdsVec(moveDirData.analysisAlbumIds.begin(), moveDirData.analysisAlbumIds.end());
        MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIdsVec);
    }
}

bool MoveFileManagerDir(const std::string &oldPath, const std::string &newPath,
    shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    MoveDirData moveDirData;
    moveDirData.oldPath = oldPath;
    moveDirData.newPath = newPath;
    moveDirData.oldLPath =
        FILE_MANAGER_LPATH_PREFIX + MediaFileMonitorRdbUtils::RemovePrefix(oldPath, FILE_MANAGER_SCAN_DIR);
    moveDirData.newLPath =
        FILE_MANAGER_LPATH_PREFIX + MediaFileMonitorRdbUtils::RemovePrefix(newPath, FILE_MANAGER_SCAN_DIR);
    CHECK_AND_RETURN_RET_LOG(!moveDirData.oldLPath.empty() && !moveDirData.newLPath.empty(), false,
        "Invalid lpath, oldLPath: %{public}s, newLPath: %{public}s",
        DfxUtils::GetSafePath(moveDirData.oldLPath).c_str(), DfxUtils::GetSafePath(moveDirData.newLPath).c_str());
    moveDirData.newAlbumName = GetLastPathComponent(moveDirData.newLPath);

    // 查询文件夹变化前后数据库相册和资产信息
    CHECK_AND_RETURN_RET_LOG(QueryMoveDirData(rdbStore, moveDirData), false,
        "Query file_manager albums and assets failed");
    // 变更前设置所有相关资产time_pending=-1不可见
    CHECK_AND_RETURN_RET_LOG(MarkAssetsTimePending(moveDirData.oldAlbumIds, -1), false,
        "Mark assets time_pending failed");
    // 新增、删除相册
    CHECK_AND_RETURN_RET_LOG(SwapAlbums(moveDirData), false, "Swap albums failed");
    // 刷新数据库资产（owner_album_id, storage_path, time_pending）
    CHECK_AND_RETURN_RET_LOG(RefreshAssetsForDirMove(moveDirData.albumIdMap, moveDirData.oldPath, moveDirData.newPath),
        false, "Refresh file_manager assets failed");
    NotifyMoveDirResult(moveDirData);

    return true;
}

void MediaMoveFileManagerDirProcessor::Process(const MediaNotifyInfo &notifyInfo)
{
    CHECK_AND_RETURN_LOG(!notifyInfo.beforePath.empty() && !notifyInfo.afterPath.empty(),
        "Invalid path in MediaMoveFileManagerDirProcessor.");
    MEDIA_INFO_LOG("Process in MediaMoveFileManagerDirProcessor, before: %{public}s, after: %{public}s",
        DfxUtils::GetSafePath(notifyInfo.beforePath).c_str(), DfxUtils::GetSafePath(notifyInfo.afterPath).c_str());

    // Move to trash
    if (notifyInfo.afterPath.find(FILE_MANAGER_TRASH_PATH) == 0) {
        CHECK_AND_RETURN_LOG(
            MediaFileMonitorRdbUtils::DeleteFileManagerDirByFileManagerPath(notifyInfo.beforePath, rdbStore_),
            "DeleteFileManagerDirByFileManagerPath failed");
        return;
    }

    // Recover from trash
    if (notifyInfo.beforePath.find(FILE_MANAGER_TRASH_PATH) == 0) {
        MEDIA_DEBUG_LOG("Recover from trash, skip dir move");
        FolderScanner fs(notifyInfo);
        fs.Run();
        return;
    }

    // Normal rename/move
    CHECK_AND_RETURN_LOG(MoveFileManagerDir(notifyInfo.beforePath, notifyInfo.afterPath, rdbStore_),
        "MoveFileManagerDir failed");
}
// LCOV_EXCL_STOP
} // namespace OHOS::Media