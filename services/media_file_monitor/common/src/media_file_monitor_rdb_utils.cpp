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
#include "media_file_monitor_rdb_utils.h"

#include "nlohmann/json.hpp"

#include "dfx_utils.h"
#include "file_const.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "photo_album_column.h"
#include "rdb_helper.h"
#include "rdb_predicates.h"
#include "rdb_utils.h"
#include "thumbnail_service.h"
#include "media_edit_utils.h"
#include "file_manager_parser.h"
#include "medialibrary_tracer.h"

using namespace OHOS::NativeRdb;

namespace OHOS::Media {
constexpr int32_t INVALID_ID = -1;
constexpr int64_t INVALID_DATE_TAKEN = -1;
const std::string ColumnValueParser<int32_t>::typeName = "int32_t";
const std::string ColumnValueParser<int64_t>::typeName = "int64_t";
const std::string ColumnValueParser<std::string>::typeName = "string";

static const std::vector<std::string> ExcludeLPaths = {
    "/Pictures/Screenrecords",
    "/Pictures/Screenshots",
    "/Pictures/hiddenAlbum",
    "/Pictures/其它",
    "/DCIM/Camera",
    "/Pictures/Recover",
    "/Pictures/Users",
};

bool MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::string& storagePath, LakeMonitorQueryResultData &data)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false,
        "QueryDataByDeletedStoragePath failed: rdbStore is nullptr, storagePath: %{public}s",
        DfxUtils::GetSafePath(storagePath).c_str());

    RdbPredicates predicates = BuildDeletePredicatesByStoragePath(storagePath);
    auto resultSet = rdbStore->QueryByStep(predicates,
        { MediaColumn::MEDIA_ID,
          PhotoColumn::PHOTO_OWNER_ALBUM_ID,
          MediaColumn::MEDIA_DATE_TAKEN,
          MediaColumn::MEDIA_FILE_PATH,
          PhotoColumn::PHOTO_POSITION });

    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "QueryDataByDeletedStoragePath failed: resultSet is nullptr, storagePath: %{public}s",
        DfxUtils::GetSafePath(storagePath).c_str());

    int rowCount = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(rowCount) == E_OK && rowCount == 1, false,
        "QueryDataByDeletedStoragePath failed: unexpected rowCount=%{public}d, storagePath: %{public}s",
        rowCount, DfxUtils::GetSafePath(storagePath).c_str());

    CHECK_AND_RETURN_RET_LOG(resultSet->GoToFirstRow() == E_OK, false,
        "QueryDataByDeletedStoragePath failed: GoToFirstRow failed, storagePath: %{public}s",
        DfxUtils::GetSafePath(storagePath).c_str());

    CHECK_AND_RETURN_RET_LOG(FillQueryResultData(resultSet, data), false,
        "QueryDataByDeletedStoragePath failed: invalid result data, storagePath: %{public}s",
        DfxUtils::GetSafePath(storagePath).c_str());

    return true;
}

bool MediaFileMonitorRdbUtils::QueryAlbumIdsByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::string &lPath, std::vector<int32_t> &albumIds)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.BeginWrap();
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lPath)
           ->Or()
           ->Like(PhotoAlbumColumns::ALBUM_LPATH, lPath + "/%");
    rdbPredicates.EndWrap();
    std::vector<std::string> columns = { PhotoAlbumColumns::ALBUM_ID };
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "QueryByStep returned nullptr, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
            resultSet, PhotoAlbumColumns::ALBUM_ID, INVALID_ID);
        CHECK_AND_CONTINUE_ERR_LOG(albumId > 0, "Invalid albumId found for lPath: %{public}s",
            DfxUtils::GetSafePath(lPath).c_str());
        albumIds.push_back(albumId);
    }

    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), false,
        "No valid albumId found for lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    return true;
}

bool MediaFileMonitorRdbUtils::QueryAlbumByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::string &lPath, std::vector<int32_t> &albumIds, std::unordered_map<int32_t, int32_t> &albumCounts)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates rdbPredicates(PhotoAlbumColumns::TABLE);
    rdbPredicates.BeginWrap();
    rdbPredicates.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lPath)
           ->Or()
           ->Like(PhotoAlbumColumns::ALBUM_LPATH, lPath + "/%");
    rdbPredicates.EndWrap();
    std::vector<std::string> columns = { PhotoAlbumColumns::ALBUM_ID, PhotoAlbumColumns::ALBUM_COUNT };
    auto resultSet = rdbStore->QueryByStep(rdbPredicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "QueryByStep returned nullptr, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    while (resultSet->GoToNextRow() == E_OK) {
        int32_t albumId = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
            resultSet, PhotoAlbumColumns::ALBUM_ID, INVALID_ID);
        int32_t counts = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
            resultSet, PhotoAlbumColumns::ALBUM_COUNT, 0);
        CHECK_AND_CONTINUE_ERR_LOG(albumId > 0, "Invalid albumId found for lPath: %{public}s",
            DfxUtils::GetSafePath(lPath).c_str());
        albumIds.push_back(albumId);
        albumCounts.insert(make_pair(albumId, counts));
    }

    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), false,
        "No valid albumId found for lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    return true;
}

bool MediaFileMonitorRdbUtils::QueryDataListByAlbumIds(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<int32_t> &albumIds, std::vector<LakeMonitorQueryResultData> &dataList,
    const FileSourceType &fileSourceType)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates predicates = BuildQueryPredicatesByAlbumIds(albumIds, fileSourceType);
    auto resultSet = rdbStore->QueryByStep(predicates,
        { MediaColumn::MEDIA_ID,
          PhotoColumn::PHOTO_OWNER_ALBUM_ID,
          MediaColumn::MEDIA_DATE_TAKEN,
          MediaColumn::MEDIA_FILE_PATH,
          PhotoColumn::PHOTO_POSITION });
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, false,
        "QueryByStep returned nullptr, albumIds size: %{public}zu", albumIds.size());

    int rowCount = 0;
    CHECK_AND_RETURN_RET_LOG(resultSet->GetRowCount(rowCount) == E_OK && rowCount >= 0, false,
        "Unexpected rowCount: %{public}d, albumIds size: %{public}zu", rowCount, albumIds.size());

    while (resultSet->GoToNextRow() == E_OK) {
        LakeMonitorQueryResultData data;
        CHECK_AND_CONTINUE_ERR_LOG(FillQueryResultData(resultSet, data), "Invalid data found: %{public}s",
            DfxUtils::GetSafePath(data.photoPath).c_str());
        dataList.push_back(data);
    }

    CHECK_AND_PRINT_LOG(!dataList.empty(),
        "No valid data found for albumIds size: %{public}zu", albumIds.size());

    return true;
}

bool MediaFileMonitorRdbUtils::DeleteAssetByStoragePath(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh, const std::string& storagePath)
{
    CHECK_AND_RETURN_RET_LOG(assetRefresh != nullptr, false, "assetRefresh is nullptr");

    RdbPredicates predicates = BuildDeletePredicatesByStoragePath(storagePath);
    int32_t deletedRows = -1;
    int32_t ret = assetRefresh->LogicalDeleteReplaceByUpdate(predicates, deletedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK && deletedRows > 0, false,
        "Delete failed, deletedRows: %{public}d, storagePath: %{public}s", deletedRows,
        DfxUtils::GetSafePath(storagePath).c_str());
    return true;
}

bool MediaFileMonitorRdbUtils::UpdateAlbumInfo(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<int32_t> &albumIds)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");
    std::vector<std::string> targetAlbumIdList;
    targetAlbumIdList.reserve(albumIds.size());
    for (int32_t id : albumIds) {
        targetAlbumIdList.push_back(std::to_string(id));
    }
    MediaLibraryRdbUtils::UpdateCommonAlbumInternal(rdbStore, targetAlbumIdList, true, true);
    MediaLibraryRdbUtils::UpdateSystemAlbumInternal(rdbStore, {}, true);
    return true;
}

bool MediaFileMonitorRdbUtils::DeleteAssetsByOwnerAlbumIds(std::shared_ptr<MediaLibraryRdbStore> rdbStore,
    const std::vector<int32_t> &albumIds, const FileSourceType &fileSourceType)
{
    CHECK_AND_RETURN_RET_LOG(!albumIds.empty(), false, "Function called with empty albumIds");
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    RdbPredicates assetPredicates = BuildQueryPredicatesByAlbumIds(albumIds, fileSourceType);
    int deletedCount = 0;
    int err = rdbStore->Delete(deletedCount, assetPredicates);
    CHECK_AND_RETURN_RET_LOG(err == E_OK, false, "Failed to delete assets, err: %{public}d, albumCount: %{public}zu",
        err, albumIds.size());

    MEDIA_INFO_LOG("Batch deleted %{public}d assets for %{public}zu albumIds", deletedCount, albumIds.size());
    return true;
}

RdbPredicates MediaFileMonitorRdbUtils::BuildDeletePredicatesByStoragePath(const std::string& storagePath)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_STORAGE_PATH, storagePath);

    // 删除或隐藏场景: rename不支持, 基于先新增再删除的方式移动湖内照片到湖外, 此时湖内文件删除事件湖外不响应
    predicates.And()->EqualTo(MediaColumn::MEDIA_DATE_TRASHED, "0")
                    ->EqualTo(MediaColumn::MEDIA_HIDDEN, "0");
    // 对端设备编辑下行后, 文件删除事件湖外不响应
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    return predicates;
}

RdbPredicates MediaFileMonitorRdbUtils::BuildQueryPredicatesByAlbumIds(const std::vector<int32_t> &albumIds,
    const FileSourceType &fileSourceType)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> albumIdStrings;
    albumIdStrings.reserve(albumIds.size());
    for (int32_t id : albumIds) {
        albumIdStrings.push_back(std::to_string(id));
    }
    predicates.In(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumIdStrings);
    predicates.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, std::to_string(fileSourceType));
    return predicates;
}

RdbPredicates BuildDeletePredicatesByLPath(const std::string &lPath)
{
    NativeRdb::RdbPredicates delPred(PhotoAlbumColumns::TABLE);
    delPred.BeginWrap();
    delPred.EqualTo(PhotoAlbumColumns::ALBUM_LPATH, lPath)
           ->Or()
           ->Like(PhotoAlbumColumns::ALBUM_LPATH, lPath + "/%");
    delPred.EndWrap();
    delPred.EqualTo(PhotoAlbumColumns::ALBUM_COUNT, "0");
    
    delPred.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, "2048");
    for (const auto &excludePath : ExcludeLPaths) {
        delPred.NotEqualTo(PhotoAlbumColumns::ALBUM_LPATH, excludePath);
    }
    return delPred;
}

bool MediaFileMonitorRdbUtils::DeleteEmptyAlbumsByLPath(const std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    const std::string &lPath)
{
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr");

    // 构造条件: ALBUM_LPATH LIKE 'lPath%' AND COUNT = 0
    NativeRdb::RdbPredicates delPred = BuildDeletePredicatesByLPath(lPath);

    int deletedRows = rdbStore->Delete(delPred);
    CHECK_AND_RETURN_RET_LOG(deletedRows > 0, false,
        "Delete failed, deletedRows: %{public}d, lPath: %{public}s", deletedRows, DfxUtils::GetSafePath(lPath).c_str());

    MEDIA_INFO_LOG("Deleted empty albums for root lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());
    return true;
}

bool MediaFileMonitorRdbUtils::FillQueryResultData(const std::shared_ptr<ResultSet> &resultSet,
    LakeMonitorQueryResultData &data)
{
    data.fileId = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
        resultSet, MediaColumn::MEDIA_ID, INVALID_ID);

    data.albumId = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
        resultSet, PhotoColumn::PHOTO_OWNER_ALBUM_ID, INVALID_ID);

    data.dateTaken = MediaFileMonitorRdbUtils::GetColumnValue<int64_t>(
        resultSet, MediaColumn::MEDIA_DATE_TAKEN, INVALID_DATE_TAKEN);

    data.photoPath = MediaFileMonitorRdbUtils::GetColumnValue<std::string>(
        resultSet, MediaColumn::MEDIA_FILE_PATH, "");

    data.position = MediaFileMonitorRdbUtils::GetColumnValue<int32_t>(
        resultSet, PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::INVALID));

    return CheckValidData(data); // 保证必要字段有效
}

bool MediaFileMonitorRdbUtils::CheckValidData(const LakeMonitorQueryResultData &data)
{
    return data.fileId > 0 && data.albumId > 0 && data.dateTaken > 0 && !data.photoPath.empty();
}

int ColumnValueParser<int32_t>::ParseValue(ResultSet &rs, int index, int32_t &value)
{
    CHECK_AND_RETURN_RET_LOG(rs.GetInt(index, value) == E_OK, E_ERROR,
        "Failed to get int32_t value from index %{public}d", index);
    return E_OK;
}

int ColumnValueParser<int64_t>::ParseValue(ResultSet &rs, int index, int64_t &value)
{
    CHECK_AND_RETURN_RET_LOG(rs.GetLong(index, value) == E_OK, E_ERROR,
        "Failed to get int64_t value from index %{public}d", index);
    return E_OK;
}

int ColumnValueParser<std::string>::ParseValue(ResultSet &rs, int index, std::string &value)
{
    CHECK_AND_RETURN_RET_LOG(rs.GetString(index, value) == E_OK, E_ERROR,
        "Failed to get string value from index %{public}d", index);
    return E_OK;
}

template <typename T>
T MediaFileMonitorRdbUtils::GetColumnValue(const std::shared_ptr<NativeRdb::ResultSet> &rs,
    const std::string &colName, const T &defaultValue)
{
    CHECK_AND_RETURN_RET_LOG(rs != nullptr, defaultValue,
        "ResultSet is nullptr, colName: %{public}s", colName.c_str());

    int index = -1;
    CHECK_AND_RETURN_RET_LOG(rs->GetColumnIndex(colName, index) == E_OK, defaultValue,
        "Failed to get column index, colName: %{public}s", colName.c_str());

    bool isNull = true;
    CHECK_AND_RETURN_RET_LOG(rs->IsColumnNull(index, isNull) == E_OK && !isNull, defaultValue,
        "Column is null or failed to check null, colName: %{public}s", colName.c_str());

    T value = defaultValue;
    CHECK_AND_RETURN_RET_LOG(ColumnValueParser<T>::ParseValue(*rs, index, value) == E_OK, defaultValue,
        "Fail to get %{public}s value, colName: %{public}s", ColumnValueParser<T>::typeName.c_str(), colName.c_str());

    return value;
}

inline void NotifyAssetChange(int fileId)
{
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    watch->Notify(PhotoColumn::PHOTO_URI_PREFIX + std::to_string(fileId),
        NotifyType::NOTIFY_REMOVE);
}

std::string MediaFileMonitorRdbUtils::RemovePrefix(const std::string &uri, const std::string &prefix)
{
    if (uri.compare(0, prefix.size(), prefix) != 0) {
        MEDIA_ERR_LOG("Invalid path: %{public}s", DfxUtils::GetSafePath(uri).c_str());
        return "";
    }
    return uri.substr(prefix.size());
}

inline int32_t DeleteEditdata(const std::string &path)
{
    std::string editDataDirPath = MediaEditUtils::GetEditDataDir(path);
    CHECK_AND_RETURN_RET_LOG(!editDataDirPath.empty(), E_ERR, "Cannot get editPath, path: %{private}s", path.c_str());
    if (MediaFileUtils::IsFileExists(editDataDirPath)) {
        CHECK_AND_RETURN_RET_LOG(MediaFileUtils::DeleteDir(editDataDirPath), E_ERR,
            "Failed to delete edit data, path: %{private}s", editDataDirPath.c_str());
    }
    return E_OK;
}

void HandleAnalysisAlbum(std::shared_ptr<MediaLibraryRdbStore> rdbStore, std::set<std::string>& analysisAlbumIds)
{
    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    if (!albumIds.empty() && rdbStore != nullptr) {
        MediaLibraryRdbUtils::UpdateAnalysisAlbumInternal(rdbStore, albumIds);
        MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
    }
}

bool MediaFileMonitorRdbUtils::DeleteLakeDirByLakePath(const std::string &path,
    std::shared_ptr<MediaLibraryRdbStore> &rdbStore, int32_t *delNum)
{
    // 1. 去除湖外前缀
    std::string lPath = RemovePrefix(path, LAKE_SCAN_DIR);
    CHECK_AND_RETURN_RET_LOG(!lPath.empty(), false,
        "Invalid path after prefix removal, path: %{public}s", DfxUtils::GetSafePath(path).c_str());

    // 2. 查出该目录及子目录的 album_id
    std::vector<int32_t> albumIds;
    CHECK_AND_RETURN_RET_LOG(QueryAlbumIdsByLPath(rdbStore, lPath, albumIds),
        false, "QueryAlbumIdsByLPath failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 3. 查出对应的所有湖内文件的相关数据用于后续处理
    std::vector<LakeMonitorQueryResultData> dataList;
    CHECK_AND_RETURN_RET_LOG(QueryDataListByAlbumIds(rdbStore, albumIds, dataList, FileSourceType::MEDIA_HO_LAKE),
        false, "QueryDataListByAlbumIds failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 4. 查出湖内资产对应的智慧相册
    std::vector<std::string> fileIds;
    std::set<std::string> analysisAlbumIds;
    for (auto data : dataList) {
        fileIds.emplace_back(std::to_string(data.fileId));
    }
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets(fileIds, analysisAlbumIds);

    // 5. 批量删除资产
    CHECK_AND_PRINT_LOG(DeleteAssetsByOwnerAlbumIds(rdbStore, albumIds, FileSourceType::MEDIA_HO_LAKE),
        "DeleteAssetsByOwnerAlbumIds failed");

    // 6. 删除该图片对应湖外资源
    for (auto data : dataList) {
        DeleteRelatedResource(data.photoPath, std::to_string(data.fileId), std::to_string(data.dateTaken));
    }
    if (delNum != nullptr) {
        *delNum = static_cast<int32_t>(dataList.size());
    }
    // 7. 刷新相册并发送相册通知
    UpdateAlbumInfo(rdbStore, albumIds);
    HandleAnalysisAlbum(rdbStore, analysisAlbumIds);

    // 8. 删除空相册
    CHECK_AND_RETURN_RET_LOG(DeleteEmptyAlbumsByLPath(rdbStore, lPath),
        false, "DeleteEmptyAlbumsByLPath failed");

    // 9. 发送资产变更通知
    for (auto data : dataList) {
        NotifyAssetChange(data.fileId);
    }
    return true;
}

void MediaFileMonitorRdbUtils::DeleteRelatedResource(const std::string &photoPath, const std::string &fileId,
    const std::string &dateTaken)
{
    ThumbnailService::GetInstance()->DeleteThumbnailDirAndAstc(fileId,
        PhotoColumn::PHOTOS_TABLE, photoPath, dateTaken);
    CHECK_AND_PRINT_LOG(DeleteEditdata(photoPath) == E_OK, "DeleteEditdata failed.");
}

void MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(const std::vector<std::string>& albumIds)
{
    if (albumIds.empty()) {
        return;
    }
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "Can not get MediaLibraryNotify Instance");
    for (const auto& albumId : albumIds) {
        watch->Notify(MediaFileUtils::GetUriByExtrConditions(
            PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, albumId), NotifyType::NOTIFY_UPDATE);
    }
}

bool MediaFileMonitorRdbUtils::DeleteLakeAlbumByLakePath(const std::string &path,
    std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    // 1. 去除湖外前缀
    std::string lPath = RemovePrefix(path, LAKE_SCAN_DIR);
    CHECK_AND_RETURN_RET_LOG(!lPath.empty(), false,
        "Invalid path after prefix removal, path: %{public}s", DfxUtils::GetSafePath(path).c_str());

    // 2. 查出该目录及子目录的 album_id
    std::vector<int32_t> albumIds;
    std::unordered_map<int32_t, int32_t> albumCounts;
    CHECK_AND_RETURN_RET_LOG(QueryAlbumByLPath(rdbStore, lPath, albumIds, albumCounts),
        false, "QueryAlbumByLPath failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 3. 查出对应的所有湖内文件的相关数据用于后续处理
    std::vector<LakeMonitorQueryResultData> dataList;
    CHECK_AND_RETURN_RET_LOG(QueryDataListByAlbumIds(rdbStore, albumIds, dataList, FileSourceType::MEDIA_HO_LAKE),
        false, "QueryDataListByAlbumIds failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 4. 在相册表删除非融合的纯湖内相册
    CHECK_AND_RETURN_RET_LOG(DeleteLakeAlbums(rdbStore, albumCounts, dataList),
        false, "DeleteLakeAlbums failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 5. 删除空相册
    CHECK_AND_RETURN_RET_LOG(DeleteEmptyAlbumsByLPath(rdbStore, lPath),
        false, "DeleteEmptyAlbumsByLPath failed");

    return true;
}

bool MediaFileMonitorRdbUtils::DeleteLakeAlbums(std::shared_ptr<MediaLibraryRdbStore> &rdbStore,
    std::unordered_map<int32_t, int32_t>& albumCounts, std::vector<LakeMonitorQueryResultData>& dataList)
{
    // 1.入参校验
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, false, "rdbStore is nullptr, cannot delete lake albums");
    CHECK_AND_RETURN_RET_LOG(!albumCounts.empty(), false, "albumCounts is empty, cannot delete lake albums");
    CHECK_AND_RETURN_RET_LOG(!dataList.empty(), false, "dataList is empty, cannot delete lake albums");

    // 2.统计dataList中每个albumId对应的记录数量
    std::unordered_map<int32_t, int32_t> albumIdCountMap;
    for (const auto& data : dataList) {
        // 过滤无效albumId
        if (data.albumId <= 0) {
            MEDIA_ERR_LOG("Skip invalid albumId: %{public}d in dataList, fileId:%{public}d",
                data.albumId, data.fileId);
            continue;
        }
        albumIdCountMap[data.albumId]++;
    }

    // 3.筛选数量匹配的albumId（count == albumCounts中对应值）
    std::vector<string> matchedAlbumIds;
    for (const auto& [albumId, realCount] : albumIdCountMap) {
        auto it = albumCounts.find(albumId);
        if (it == albumCounts.end()) {
            MEDIA_DEBUG_LOG("AlbumId %{public}d not found in albumCounts, skip", albumId);
            continue;
        }

        int32_t expectedCount = it->second;
        if (realCount == expectedCount) {
            matchedAlbumIds.push_back(to_string(albumId));
            MEDIA_DEBUG_LOG("AlbumId %{public}d matched count: real=%{public}d, expected=%{public}d",
                albumId, realCount, expectedCount);
        } else {
            MEDIA_DEBUG_LOG("AlbumId %{public}d count not matched: real=%{public}d, expected=%{public}d",
                albumId, realCount, expectedCount);
        }
    }

    CHECK_AND_RETURN_RET_LOG(!matchedAlbumIds.empty(), false,
        "No albumId matched count condition, skip delete");

    // 5.删除对应album记录
    NativeRdb::RdbPredicates predicates(PhotoAlbumColumns::TABLE);
    predicates.In(PhotoAlbumColumns::ALBUM_ID, matchedAlbumIds);
    int32_t deletedRow = -1;
    auto ret = rdbStore->Delete(deletedRow, predicates);
    bool cond = (ret != NativeRdb::E_OK || deletedRow < 0);
    CHECK_AND_PRINT_LOG(!cond, "Story Delete db failed, errCode = %{public}d", ret);
    MEDIA_INFO_LOG("album Delete retVal:%{public}d, deletedRow:%{public}d", ret, deletedRow);
    return !cond;
}

bool MediaFileMonitorRdbUtils::HandleFileManagerCloudAssets(
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh,
    std::vector<LakeMonitorQueryResultData> dataList)
{
    CHECK_AND_RETURN_RET_LOG(assetRefresh != nullptr, false, "assetRefresh is nullptr");

    std::vector<std::string> updateFileIds;
    std::vector<std::string> deleteFileIds;
    std::vector<LakeMonitorQueryResultData> deleteDataList;

    for (auto data : dataList) {
        if (data.position == static_cast<int32_t>(PhotoPositionType::LOCAL)) {
            deleteDataList.emplace_back(data);
            deleteFileIds.emplace_back(std::to_string(data.fileId));
        } else if (data.position == static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD)) {
            updateFileIds.emplace_back(std::to_string(data.fileId));
        }
    }

    // 处理端云合一图：更新 position 为 CLOUD
    if (!updateFileIds.empty()) {
        NativeRdb::AbsRdbPredicates updatePredicates(PhotoColumn::PHOTOS_TABLE);
        updatePredicates.In(MediaColumn::MEDIA_ID, updateFileIds);
        FileManagerParser::HandleUpdateCloudAsset(updatePredicates, PhotoPositionType::CLOUD);
    }

    // 处理本地图：删除数据库记录
    if (!deleteFileIds.empty()) {
        NativeRdb::AbsRdbPredicates deletePredicates(PhotoColumn::PHOTOS_TABLE);
        deletePredicates.In(MediaColumn::MEDIA_ID, deleteFileIds);
        int32_t deletedRows = -1;
        int32_t ret = assetRefresh->Delete(deletedRows, deletePredicates);
        CHECK_AND_RETURN_RET_LOG(ret == E_OK && deletedRows > 0, false,
            "Delete failed, deletedRows: %{public}d", deletedRows);

        // 删除本地图片对应资源
        for (auto data : deleteDataList) {
            DeleteRelatedResource(data.photoPath, std::to_string(data.fileId), std::to_string(data.dateTaken));
        }
    }

    return true;
}

bool MediaFileMonitorRdbUtils::DeleteFileManagerDirByFileManagerPath(const std::string &path,
    std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaFileMonitorRdbUtils::DeleteFileManagerDirByFileManagerPath");
    // 文管回收站彻底删除文件夹不做处理
    if (path.find(FILE_MANAGER_TRASH_PATH) == 0) {
        MEDIA_DEBUG_LOG("Delete trashed file assets, path: %{public}s", DfxUtils::GetSafePath(path).c_str());
        return true;
    }

    // 1. 去除文管前缀
    std::string lPath = FILE_MANAGER_LPATH_PREFIX + RemovePrefix(path, FILE_MANAGER_SCAN_DIR);
    CHECK_AND_RETURN_RET_LOG(!lPath.empty(), false,
        "Invalid path after prefix removal, path: %{public}s", DfxUtils::GetSafePath(path).c_str());

    // 2. 查出该目录及子目录的 album_id
    std::vector<int32_t> albumIds;
    CHECK_AND_RETURN_RET_LOG(QueryAlbumIdsByLPath(rdbStore, lPath, albumIds),
        false, "QueryAlbumIdsByLPath failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 3. 查出对应的所有文管文件的相关数据用于后续处理
    std::vector<LakeMonitorQueryResultData> dataList;
    CHECK_AND_RETURN_RET_LOG(QueryDataListByAlbumIds(rdbStore, albumIds, dataList, FileSourceType::FILE_MANAGER),
        false, "QueryDataListByAlbumIds failed, lPath: %{public}s", DfxUtils::GetSafePath(lPath).c_str());

    // 4. 处理文管资产：删除本地图，更新端云合一图position为CLOUD
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(HandleFileManagerCloudAssets(assetRefresh, dataList),
        false, "HandleFileManagerCloudAssets failed");

    // 5. 更新相册信息并发送相册更新通知
    assetRefresh->RefreshAlbum();
    assetRefresh->Notify();

    // 6. 删除空相册
    CHECK_AND_RETURN_RET_LOG(DeleteEmptyAlbumsByLPath(rdbStore, lPath),
        false, "DeleteEmptyAlbumsByLPath failed");

    return true;
}

bool MediaFileMonitorRdbUtils::DeleteFileByLakePath(const std::string &path,
    std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    // 1. 构造查询条件并查询 albumId 与 file_id; 根据 file_id 查询对应的 AnalysisAlbum id
    LakeMonitorQueryResultData data;
    CHECK_AND_RETURN_RET_LOG(MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(rdbStore, path, data),
        false, "Failed to get valid albumId, path: %{public}s", DfxUtils::GetSafePath(path).c_str());
    std::set<std::string> analysisAlbumIds;
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets({to_string(data.fileId)}, analysisAlbumIds);

    // 2. 删除该路径对应的资产
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(MediaFileMonitorRdbUtils::DeleteAssetByStoragePath(assetRefresh, path), false,
        "Failed to DeleteAssetByStoragePath, path: %{public}s", DfxUtils::GetSafePath(path).c_str());

    // 3. 删除该图片对应湖外资源
    MediaFileMonitorRdbUtils::DeleteRelatedResource(data.photoPath,
        std::to_string(data.fileId), std::to_string(data.dateTaken));

    // 4. 更新相册信息并发送相册更新通知
    assetRefresh->RefreshAlbum();
    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    if (!albumIds.empty() && rdbStore != nullptr) {
        MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
    }

    // 5. 发送资产更新通知
    assetRefresh->Notify();
    NotifyAssetChange(data.fileId);

    return true;
}

bool MediaFileMonitorRdbUtils::DeleteFileByFileManagerPath(const std::string &path,
    std::shared_ptr<MediaLibraryRdbStore> &rdbStore)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaFileMonitorRdbUtils::DeleteFileByFileManagerPath");
    // 1. 文管回收站彻底删除资产不做处理
    if (path.find(FILE_MANAGER_TRASH_PATH) == 0) {
        MEDIA_DEBUG_LOG("Delete trashed directory, path: %{public}s", DfxUtils::GetSafePath(path).c_str());
        return true;
    }
    // 2. 构造查询条件并查询 albumId 与 file_id; 根据 file_id 查询对应的 AnalysisAlbum id
    LakeMonitorQueryResultData data;
    CHECK_AND_RETURN_RET_LOG(MediaFileMonitorRdbUtils::QueryDataByDeletedStoragePath(rdbStore, path, data),
        false, "Failed to get valid albumId, path: %{public}s", DfxUtils::GetSafePath(path).c_str());
    std::set<std::string> analysisAlbumIds;
    MediaLibraryRdbUtils::QueryAnalysisAlbumIdOfAssets({to_string(data.fileId)}, analysisAlbumIds);

    // 3. 删除本地图记录，刷新端云合一图position
    auto assetRefresh = std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::vector<LakeMonitorQueryResultData> dataList {data};
    CHECK_AND_RETURN_RET_LOG(HandleFileManagerCloudAssets(assetRefresh, dataList),
        false, "HandleFileManagerCloudAssets failed");

    // 4. 更新相册信息并发送相册更新通知，智慧相册单独触发通知
    assetRefresh->RefreshAlbum();
    assetRefresh->Notify();

    std::vector<std::string> albumIds(analysisAlbumIds.begin(), analysisAlbumIds.end());
    if (!albumIds.empty() && rdbStore != nullptr) {
        MediaFileMonitorRdbUtils::NotifyAnalysisAlbum(albumIds);
    }

    return true;
}
} // namespace OHOS::Media