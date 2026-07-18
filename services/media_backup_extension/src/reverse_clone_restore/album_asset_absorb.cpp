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

/**
 * album_asset_absorb.cpp
 *
 * 媒体库吸收新设备资产和相册数据适配模块实现
 *
 * 注意：file_id偏移处理已在fast_restore.cpp中通过SQL语句完成，本模块只负责数据吸收。
 */

#include "album_asset_absorb.h"
#include "backup_database_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "media_column.h"
#include "photo_album_column.h"
#include "reverse_clone_resource_inherit_service.h"
#include "vision_column.h"
#include "userfile_manager_types.h"
#include <sstream>
#include <mutex>
#include <unordered_set>

namespace OHOS {
namespace Media {
using namespace std;

const int32_t CLONE_QUERY_COUNT = 200;
const int32_t RELEATED_TO_PHOTO_MAP = 1;

// 列名常量
const std::string ANALYSIS_MAP_ASSET = "map_asset";
const std::string ANALYSIS_MAP_ALBUM = "map_album";

// 判重相关列名常量
const std::string COL_DISPLAY_NAME = "display_name";
const std::string COL_SIZE = "size";
const std::string COL_ORIENTATION = "orientation";
const std::string COL_OWNER_ALBUM_ID = "owner_album_id";
const std::string COL_CLOUD_ID = "cloud_id";
const std::string COL_SOURCE_PATH = "source_path";
const std::string COL_CLEAN_FLAG = "clean_flag";
const std::string COL_HIDDEN = "hidden";
const std::string COL_DATE_TRASHED = "date_trashed";

// 判重 SQL 语句常量（与原克隆逻辑保持一致）
const std::string SQL_FIND_SAME_FILE_WITH_CLOUD_ID = R"(
    SELECT
        P.file_id,
        P.data,
        P.clean_flag,
        P.position,
        P.file_source_type
    FROM Photos AS P
    WHERE file_id >= ? AND
        file_id <= ? AND
        cloud_id = ?
    LIMIT 1;)";

const std::string SQL_FIND_SAME_FILE_IN_ALBUM = R"(
    SELECT
        p.file_id,
        p.data,
        p.clean_flag,
        p.position,
        p.file_source_type
    FROM
        (
            SELECT album_id
            FROM PhotoAlbum
            WHERE LOWER(lpath) = LOWER(?)
        )
        AS a
    INNER JOIN
        (
            SELECT
                file_id,
                data,
                clean_flag,
                position,
                file_source_type,
                size,
                orientation,
                owner_album_id
            FROM Photos
            WHERE file_id >= ? AND
                file_id <= ? AND
                display_name = ? AND
                size = ? AND
                (1 <> ? OR orientation = ?)
        )
        AS p
    ON a.album_id = p.owner_album_id
    ORDER BY p.clean_flag ASC
    LIMIT 1;)";

const std::string SQL_FIND_SAME_FILE_WITHOUT_ALBUM = R"(
    SELECT
        P.file_id,
        P.data,
        P.clean_flag,
        P.position,
        P.file_source_type
    FROM Photos AS P
    WHERE file_id >= ? AND
        file_id <= ? AND
        display_name = ? AND
        size = ? AND
        (owner_album_id IS NULL OR owner_album_id = 0) AND
        (1 <> ? OR orientation = ?)
    ORDER BY clean_flag ASC
    LIMIT 1;)";

const std::string SQL_FIND_SAME_FILE_BY_SOURCE_PATH = R"(
    SELECT
        file_id,
        data,
        clean_flag,
        position,
        file_source_type
    FROM
        (
            SELECT file_id,
                data,
                clean_flag,
                position,
                file_source_type,
                display_name,
                size,
                orientation,
                date_trashed,
                source_path
            FROM Photos
                LEFT JOIN PhotoAlbum
                ON Photos.owner_album_id = PhotoAlbum.album_id
            WHERE PhotoAlbum.album_id IS NULL AND
                COALESCE(Photos.source_path, '') <> '' AND
                (
                    COALESCE(Photos.hidden, 0) = 1 OR
                    COALESCE(Photos.date_trashed, 0) <> 0
                )
        ) AS MISS
    LEFT JOIN
        (
            SELECT
                ? AS source_path,
                ? AS min_dest_db_file_id,
                ? AS max_file_id,
                ? AS display_name,
                ? AS size,
                ? AS picture_flag,
                ? AS orientation
        ) AS INPUT
    ON 1 = 1
    WHERE MISS.file_id >= INPUT.min_dest_db_file_id AND
        MISS.file_id <= INPUT.max_file_id AND
        MISS.display_name = INPUT.display_name AND
        MISS.size = INPUT.size AND
        (1 <> INPUT.picture_flag OR MISS.orientation = INPUT.orientation) AND
        LOWER(MISS.source_path) = LOWER(INPUT.source_path)
    ORDER BY MISS.clean_flag ASC
    LIMIT 1;)";

// 辅助函数
// LCOV_EXCL_START
int32_t AlbumAssetAbsorb::QueryMaxFileId(const shared_ptr<NativeRdb::RdbStore> &rdb)
{
    std::string sql = "SELECT MAX(" + string(MediaColumn::MEDIA_ID) + ") FROM " + string(PhotoColumn::PHOTOS_TABLE);
    auto resultSet = BackupDatabaseUtils::QuerySql(rdb, sql, {});
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, 0, "QueryMaxFileId: query failed");

    int32_t maxFileId = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, maxFileId);
    }
    resultSet->Close();

    return maxFileId;
}

// 判重辅助方法（与原克隆逻辑保持一致）

/**
 * @brief 根据 cloud_id 查找重复照片（云照片优先）
 */
int32_t AlbumAssetAbsorb::FindSameFileWithCloudId(
    const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
    int32_t maxFileId, int32_t minDestDbFileId)
{
    if (fileInfo.cloudUniqueId.empty() || maxFileId <= 0) {
        return 0;
    }

    const vector<NativeRdb::ValueObject> params = {minDestDbFileId, maxFileId, fileInfo.cloudUniqueId};
    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, SQL_FIND_SAME_FILE_WITH_CLOUD_ID, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);

    int32_t fileId = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, fileId);
    }
    resultSet->Close();
    return fileId;
}

/**
 * @brief 根据 lPath + display_name + size + orientation 查找重复照片（在相册中）
 */
int32_t AlbumAssetAbsorb::FindSameFileInAlbum(
    const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
    int32_t maxFileId, int32_t minDestDbFileId)
{
    if (fileInfo.lPath.empty() || maxFileId <= 0) {
        return 0;
    }

    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;
    const vector<NativeRdb::ValueObject> params = {fileInfo.lPath, minDestDbFileId, maxFileId,
                                                   fileInfo.displayName, fileInfo.fileSize, pictureFlag,
                                                   fileInfo.orientation};

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, SQL_FIND_SAME_FILE_IN_ALBUM, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);

    int32_t fileId = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, fileId);
    }
    resultSet->Close();
    return fileId;
}

/**
 * @brief 根据 display_name + size + orientation 查找重复照片（不在相册中）
 */
int32_t AlbumAssetAbsorb::FindSameFileWithoutAlbum(
    const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
    int32_t maxFileId, int32_t minDestDbFileId)
{
    if (maxFileId <= 0) {
        return 0;
    }

    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;
    const vector<NativeRdb::ValueObject> params = {
        minDestDbFileId, maxFileId, fileInfo.displayName, fileInfo.fileSize, pictureFlag, fileInfo.orientation};

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, SQL_FIND_SAME_FILE_WITHOUT_ALBUM, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);

    int32_t fileId = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, fileId);
    }
    resultSet->Close();
    return fileId;
}

/**
 * @brief 根据 source_path + display_name + size + orientation 查找重复照片（隐藏/回收站）
 */
int32_t AlbumAssetAbsorb::FindSameFileBySourcePath(
    const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
    int32_t maxFileId, int32_t minDestDbFileId)
{
    if (fileInfo.lPath.empty() || maxFileId <= 0) {
        return 0;
    }

    // pictureFlag: 0 for video, 1 for photo; Only search for photo in this case.
    int pictureFlag = fileInfo.fileType == MEDIA_TYPE_VIDEO ? 0 : 1;

    // 构造 sourcePath（与原克隆逻辑保持一致）
    const std::string SOURCE_PATH_PREFIX = "/storage/emulated/0";
    std::string sourcePath = SOURCE_PATH_PREFIX + fileInfo.lPath + "/" + fileInfo.displayName;

    const vector<NativeRdb::ValueObject> params = {sourcePath, minDestDbFileId, maxFileId,
                                                   fileInfo.displayName, fileInfo.fileSize, pictureFlag,
                                                   fileInfo.orientation};

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, SQL_FIND_SAME_FILE_BY_SOURCE_PATH, params);
    CHECK_AND_RETURN_RET(resultSet != nullptr, 0);

    int32_t fileId = 0;
    if (resultSet->GoToFirstRow() == NativeRdb::E_OK) {
        resultSet->GetInt(0, fileId);
    }
    resultSet->Close();
    return fileId;
}

/**
 * @brief 查找重复照片（与原克隆逻辑保持一致的判重流程）
 * 优先级：cloud_id > lPath + displayName + size + orientation >
 *         displayName + size + orientation (owner_album_id IS NULL) >
 *         source_path + displayName + size + orientation
 */
int32_t AlbumAssetAbsorb::FindSameFile(
    const shared_ptr<NativeRdb::RdbStore> &destRdb, const FileInfo &fileInfo,
    int32_t maxFileId, int32_t minDestDbFileId)
{
    int32_t duplicateFileId = 0;

    // 1. 优先根据 cloud_id 判重（云照片）
    if (!fileInfo.cloudUniqueId.empty()) {
        duplicateFileId = FindSameFileWithCloudId(destRdb, fileInfo, maxFileId, minDestDbFileId);
        if (duplicateFileId > 0) {
            MEDIA_INFO_LOG("FindSameFile: found duplicate by cloud_id, fileId=%{public}d", duplicateFileId);
            return duplicateFileId;
        }
    }

    // 2. 如果 lPath 为空，使用 FindSameFileWithoutAlbum
    if (fileInfo.lPath.empty()) {
        duplicateFileId = FindSameFileWithoutAlbum(destRdb, fileInfo, maxFileId, minDestDbFileId);
        if (duplicateFileId > 0) {
            MEDIA_INFO_LOG("FindSameFile: found duplicate without album, fileId=%{public}d", duplicateFileId);
        }
        return duplicateFileId;
    }

    // 3. 先根据 lPath + displayName + size + orientation 判重（在相册中）
    duplicateFileId = FindSameFileInAlbum(destRdb, fileInfo, maxFileId, minDestDbFileId);
    if (duplicateFileId > 0) {
        MEDIA_INFO_LOG("FindSameFile: found duplicate in album, fileId=%{public}d", duplicateFileId);
        return duplicateFileId;
    }

    // 4. 再根据 sourcePath + displayName + size + orientation 判重（不在相册中）
    duplicateFileId = FindSameFileBySourcePath(destRdb, fileInfo, maxFileId, minDestDbFileId);
    if (duplicateFileId > 0) {
        MEDIA_INFO_LOG("FindSameFile: found duplicate by source_path, fileId=%{public}d", duplicateFileId);
    }

    return duplicateFileId;
}

static void SolveDuplicate(AlbumAssetAbsorb::DuplicateCount &duplicateCount, ReverseCloneResourcePlan &plan)
{
    // 统计东湖资产数量
    if (plan.donor.fileSourceType == FileSourceType::MEDIA_HO_LAKE) {
        if (plan.donor.fingerprint.fileType == MediaType::MEDIA_TYPE_VIDEO) {
            duplicateCount.hoLakeVideo++;
        } else if (plan.donor.fingerprint.fileType == MediaType::MEDIA_TYPE_IMAGE) {
            duplicateCount.hoLakeImage++;
        }
    } else {
        // 统计非东湖资产的视频和图片数量
        if (plan.donor.fingerprint.fileType == MediaType::MEDIA_TYPE_VIDEO) {
            duplicateCount.nonHoLakeVideo++;
        } else if (plan.donor.fingerprint.fileType == MediaType::MEDIA_TYPE_IMAGE) {
            duplicateCount.nonHoLakeImage++;
        }
    }
}

void AlbumAssetAbsorb::CheckAndRemoveDuplicatePhotos(const shared_ptr<NativeRdb::RdbStore> &destRdb,
    vector<FileInfo> &fileInfos, int32_t maxFileId, int32_t minDestDbFileId,
    vector<ReverseCloneResourcePlan> &resourcePlans, const unordered_set<int32_t> &originalPureCloudFileIds,
    DuplicateCount &duplicateCount)
{
    if (fileInfos.empty()) {
        return;
    }

    if (maxFileId <= 0) {
        MEDIA_WARN_LOG("CheckAndRemoveDuplicatePhotos: invalid maxFileId=%{public}d", maxFileId);
        return;
    }

    MEDIA_INFO_LOG(
        "CheckAndRemoveDuplicatePhotos: maxFileId=%{public}d, minDestDbFileId=%{public}d, fileInfos.size=%{public}zu",
        maxFileId, minDestDbFileId, fileInfos.size());

    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    ReverseCloneResourceInheritService resourceInheritService;
    for (auto &fileInfo : fileInfos) {
        int32_t duplicateFileId = FindSameFile(destRdb, fileInfo, maxFileId, minDestDbFileId);
        if (duplicateFileId <= 0) {
            continue;
        }

        ReverseCloneResourcePlan plan =
            resourceInheritService.BuildDuplicatePlanByFileId(
                fileInfo, duplicateFileId, destRdb, originalPureCloudFileIds);
        if (plan.donor.fileId <= 0) {
            MEDIA_WARN_LOG("CheckAndRemoveDuplicatePhotos: build duplicate resource plan failed, fileId=%{public}d",
                duplicateFileId);
            continue;
        }
        SolveDuplicate(duplicateCount, plan);
        MEDIA_INFO_LOG("CheckAndRemoveDuplicatePhotos: duplicate photo found, displayName=%{public}s, "
                       "size=%{public}ld, orientation=%{public}d, srcdbFileId=%{public}d",
            fileInfo.displayName.c_str(), fileInfo.fileSize, fileInfo.orientation, duplicateFileId);

        // Mark the donor for post-insert deletion; do not delete before the absorbed row is inserted.
        fileInfo.deletedSrcdbFileId = duplicateFileId;
        resourcePlans.emplace_back(plan);
        duplicateCount.total++;
    }

    int64_t cost = MediaFileUtils::UTCTimeMilliSeconds() - start;
    MEDIA_INFO_LOG("CheckAndRemoveDuplicatePhotos: resolved %{public}d duplicate photos, cost=%{public}lld",
        duplicateCount.total, static_cast<long long>(cost));
}

void AlbumAssetAbsorb::UpdateDuplicateAssetMapForDuplicates(
    vector<FileInfo> &fileInfos, unordered_map<int32_t, int32_t> &duplicateAssetMap, std::mutex *mutex)
{
    // 加锁保护（如果提供了互斥锁）
    if (mutex != nullptr) {
        mutex->lock();
    }

    for (const auto &fileInfo : fileInfos) {
        // 只处理有删除记录的照片
        if (fileInfo.deletedSrcdbFileId <= 0) {
            continue;
        }

        // 记录映射：被删除的旧机file_id -> 新机file_id
        duplicateAssetMap[fileInfo.deletedSrcdbFileId] = fileInfo.fileIdOld;

        MEDIA_INFO_LOG(
            "UpdateDuplicateAssetMapForDuplicates: added mapping, oldFileId=%{public}d -> newFileId=%{public}d",
            fileInfo.deletedSrcdbFileId,
            fileInfo.fileIdOld);
    }

    MEDIA_INFO_LOG("UpdateDuplicateAssetMapForDuplicates: updated duplicateAssetMap_");
    MEDIA_INFO_LOG("UpdateDuplicateAssetMapForDuplicates: duplicateAssetMap_ after update: size=%{public}zu",
                   duplicateAssetMap.size());

    // 解锁
    if (mutex != nullptr) {
        mutex->unlock();
    }
}

void AlbumAssetAbsorb::ExtractOldFileIds(const unordered_map<int32_t, int32_t> &reverseDupMap,
                                         vector<int32_t> &oldFileIds,
                                         unordered_map<int32_t, int32_t> &oldFileIdToNewFileIdMap)
{
    for (const auto &entry : reverseDupMap) {
        int32_t newFileId = entry.first;
        int32_t oldFileId = entry.second;

        if (newFileId != oldFileId) {
            oldFileIds.push_back(oldFileId);
            oldFileIdToNewFileIdMap[oldFileId] = newFileId;
        }
    }
}

unordered_map<int32_t, string> AlbumAssetAbsorb::QueryExistingRecords(const shared_ptr<NativeRdb::RdbStore> &destRdb,
                                                                      const vector<int32_t> &oldFileIds)
{
    unordered_map<int32_t, string> existingRecords;

    if (oldFileIds.empty()) {
        return existingRecords;
    }

    // 使用 JoinSQLValues 拼接
    string fileIdsStr = BackupDatabaseUtils::JoinSQLValues(oldFileIds, ",");
    string querySql = "SELECT file_id, data FROM tab_cloned_old_photos WHERE file_id IN (" + fileIdsStr + ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, querySql, {});
    if (resultSet == nullptr) {
        MEDIA_WARN_LOG("QueryExistingRecords: query failed");
        return existingRecords;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t oldFileId = 0;
        string data;
        resultSet->GetInt(0, oldFileId);
        resultSet->GetString(1, data);
        existingRecords[oldFileId] = data;
    }
    resultSet->Close();

    return existingRecords;
}

unordered_map<int32_t, string> AlbumAssetAbsorb::QueryPhotosData(const shared_ptr<NativeRdb::RdbStore> &destRdb,
                                                                 const vector<int32_t> &newFileIds)
{
    unordered_map<int32_t, string> newFileIdToDataMap;

    if (newFileIds.empty()) {
        return newFileIdToDataMap;
    }

    // 使用 JoinSQLValues 拼接
    string fileIdsStr = BackupDatabaseUtils::JoinSQLValues(newFileIds, ",");
    string querySql = "SELECT file_id, data FROM Photos WHERE file_id IN (" + fileIdsStr + ")";

    auto resultSet = BackupDatabaseUtils::QuerySql(destRdb, querySql, {});
    if (resultSet == nullptr) {
        MEDIA_WARN_LOG("QueryPhotosData: query failed");
        return newFileIdToDataMap;
    }

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        int32_t newFileId = 0;
        string data;
        resultSet->GetInt(0, newFileId);
        resultSet->GetString(1, data);
        newFileIdToDataMap[newFileId] = data;
    }
    resultSet->Close();

    return newFileIdToDataMap;
}

vector<tuple<int32_t, int32_t, string>> AlbumAssetAbsorb::BuildUpdateCandidates(
    const unordered_map<int32_t, string> &existingRecords, const unordered_map<int32_t, string> &newFileIdToDataMap,
    const unordered_map<int32_t, int32_t> &oldFileIdToNewFileIdMap)
{
    vector<tuple<int32_t, int32_t, string>> updates;

    for (const auto &entry : oldFileIdToNewFileIdMap) {
        int32_t oldFileId = entry.first;
        int32_t newFileId = entry.second;

        // 检查 oldFileId 是否在 tab_cloned_old_photos 表中存在
        if (existingRecords.count(oldFileId) == 0) {
            continue;
        }

        // 检查 newFileId 是否在 Photos 表中存在
        if (newFileIdToDataMap.count(newFileId) == 0) {
            MEDIA_WARN_LOG("BuildUpdateCandidates: newFileId not found in Photos, newFileId=%{public}d", newFileId);
            continue;
        }

        updates.emplace_back(make_tuple(oldFileId, newFileId, newFileIdToDataMap.at(newFileId)));
        MEDIA_INFO_LOG("BuildUpdateCandidates: will update, targetFileId=%{public}d, newFileId=%{public}d", oldFileId,
            newFileId);
    }

    return updates;
}

int32_t AlbumAssetAbsorb::UpdateClonedPhotosRecords(const shared_ptr<NativeRdb::RdbStore> &destRdb,
                                                    const vector<tuple<int32_t, int32_t, string>> &updates)
{
    int32_t updateCount = 0;
    for (const auto &update : updates) {
        int32_t targetFileId = get<0>(update);
        int32_t newFileId = get<1>(update);
        const string &newData = get<2>(update);

        if (newData.empty()) {
            MEDIA_WARN_LOG("UpdateClonedPhotosRecords: skipping update due to empty data, targetFileId=%{public}d",
                targetFileId);
            continue;
        }

        // 更新 file_id 和 data 字段
        std::string updateSql = "UPDATE tab_cloned_old_photos SET file_id = ?, data = ? WHERE file_id = ?";
        int32_t result = BackupDatabaseUtils::ExecuteSQL(destRdb, updateSql,
            {to_string(newFileId), newData, to_string(targetFileId)});
        if (result == NativeRdb::E_OK) {
            updateCount++;
            MEDIA_INFO_LOG("UpdateClonedPhotosRecords: updated record, targetFileId=%{public}d -> newFileId=%{public}d",
                targetFileId, newFileId);
        } else {
            MEDIA_ERR_LOG("UpdateClonedPhotosRecords: failed to update record, targetFileId=%{public}d", targetFileId);
        }
    }

    return updateCount;
}

void AlbumAssetAbsorb::UpdateTabClonedOldPhotos(const shared_ptr<NativeRdb::RdbStore> &destRdb,
                                                const unordered_map<int32_t, int32_t> &reverseDupMap)
{
    if (reverseDupMap.empty()) {
        return;
    }

    // 1. 提取 key != value 的 oldFileId
    vector<int32_t> oldFileIds;
    unordered_map<int32_t, int32_t> oldFileIdToNewFileIdMap;
    ExtractOldFileIds(reverseDupMap, oldFileIds, oldFileIdToNewFileIdMap);

    if (oldFileIds.empty()) {
        MEDIA_INFO_LOG("UpdateTabClonedOldPhotos: no records need update (all key == value)");
        return;
    }

    // 2. 查询 tab_cloned_old_photos 表中存在的记录
    auto existingRecords = QueryExistingRecords(destRdb, oldFileIds);

    // 3. 收集 newFileId 列表
    vector<int32_t> newFileIds;
    for (const auto &entry : oldFileIdToNewFileIdMap) {
        newFileIds.push_back(entry.second);
    }

    // 4. 查询 Photos 表中 newFileId 对应的 data
    auto newFileIdToDataMap = QueryPhotosData(destRdb, newFileIds);

    // 5. 构建待更新列表
    auto updates = BuildUpdateCandidates(existingRecords, newFileIdToDataMap, oldFileIdToNewFileIdMap);

    // 6. 批量更新 tab_cloned_old_photos 表
    int32_t updateCount = UpdateClonedPhotosRecords(destRdb, updates);

    MEDIA_INFO_LOG("UpdateTabClonedOldPhotos: updated %{public}d records", updateCount);
}
// LCOV_EXCL_STOP
}  // namespace Media
}  // namespace OHOS