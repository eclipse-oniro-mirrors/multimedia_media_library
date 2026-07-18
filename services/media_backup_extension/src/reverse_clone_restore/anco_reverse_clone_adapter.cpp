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

#define MLOG_TAG "AncoReverseCloneAdapter"

#include "anco_reverse_clone_adapter.h"

#include <algorithm>
#include <cinttypes>
#include <functional>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "backup_database_utils.h"
#include "backup_file_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_db_const.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdb_transaction.h"
#include "rdb_predicates.h"
#include "rdb_helper.h"
#include "result_set_utils.h"

namespace OHOS {
namespace Media {
namespace {
constexpr int32_t PAGE_SIZE = 200;

struct AncoPhotoRow {
    int32_t fileId = 0;
    std::string storagePath;
    std::string displayName;
    int64_t size = 0;
    int64_t dateModified = 0;
};

struct AncoDeduplicationInfo {
    std::string path;
    std::string newPath;
};

struct AncoRepairUpdate {
    int32_t fileId = 0;
    std::string storagePath;
    std::string displayName;
    std::string title;
    std::string inode;
};

bool StartsWith(const std::string &value, const std::string &prefix)
{
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool ContainsTraversal(const std::string &path)
{
    return path == ".." || StartsWith(path, "../") || path.find("/../") != std::string::npos ||
        (path.size() >= 3 && path.compare(path.size() - 3, 3, "/..") == 0);
}

bool IsFileExistingPath(const std::string &path)
{
    if (path.empty() || ContainsTraversal(path)) {
        return false;
    }
    return MediaFileUtils::IsFileExists(path);
}

bool GetFileStat(const std::string &path, struct stat &statInfo)
{
    if (!IsFileExistingPath(path)) {
        return false;
    }
    return stat(path.c_str(), &statInfo) == 0;
}

void LogLakePathCheck(int32_t fileId, const std::string &stage, const std::string &path)
{
    bool fileExists = IsFileExistingPath(path);
    MEDIA_INFO_LOG("Anco path check, stage=%{public}s, fileId=%{public}d, fileExists=%{public}d, "
        "path=%{public}s", stage.c_str(), fileId, static_cast<int32_t>(fileExists),
        MediaFileUtils::DesensitizePath(path).c_str());
}

std::string ResolveDeduplicationDbPath(const AncoReverseCloneContext &context)
{
    if (!context.deduplicationDbPath.empty()) {
        MEDIA_INFO_LOG("Use custom anco deduplication db path: %{public}s",
            MediaFileUtils::DesensitizePath(context.deduplicationDbPath).c_str());
        return context.deduplicationDbPath;
    }
    std::string defaultPath = AncoReverseCloneAdapter::GetDefaultDeduplicationDbPath();
    MEDIA_INFO_LOG("Use default anco deduplication db path: %{public}s",
        MediaFileUtils::DesensitizePath(defaultPath).c_str());
    return defaultPath;
}

std::vector<AncoPhotoRow> QueryAncoRows(const std::shared_ptr<NativeRdb::RdbStore> &finalDb)
{
    std::vector<AncoPhotoRow> rows;
    CHECK_AND_RETURN_RET_LOG(finalDb != nullptr, rows, "QueryAncoRows: finalDb is null");
    MEDIA_INFO_LOG("QueryAncoRows start");

    const std::string querySql = "SELECT " + MediaColumn::MEDIA_ID + ", " +
        PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_NAME + ", " +
        MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_DATE_MODIFIED +
        " FROM " + PhotoColumn::PHOTOS_TABLE +
        " WHERE " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = ? AND " +
        PhotoColumn::PHOTO_STORAGE_PATH + " IS NOT NULL AND " + PhotoColumn::PHOTO_STORAGE_PATH + " <> ''";
    std::vector<NativeRdb::ValueObject> params = {
        FileSourceType::MEDIA_HO_LAKE,
    };
    auto resultSet = BackupDatabaseUtils::QuerySql(finalDb, querySql, params);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, rows, "QueryAncoRows: query failed");

    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AncoPhotoRow row;
        row.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
        row.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
        row.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
        row.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
        row.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
        rows.emplace_back(row);
    }
    resultSet->Close();
    MEDIA_INFO_LOG("QueryAncoRows finish, rows=%{public}zu", rows.size());
    return rows;
}

std::string BuildPlaceholders(size_t count)
{
    std::string placeholders;
    for (size_t i = 0; i < count; i++) {
        if (i != 0) {
            placeholders += ", ";
        }
        placeholders += "?";
    }
    return placeholders;
}

using AncoRowsByPath = std::unordered_map<std::string, AncoPhotoRow>;

void AddAncoRowByPath(AncoRowsByPath &rowsByPath, const AncoPhotoRow &row, AncoReverseCloneStats &stats)
{
    auto result = rowsByPath.emplace(row.storagePath, row);
    if (!result.second) {
        stats.phaseTwoDuplicateOldPathRows++;
        MEDIA_WARN_LOG("Duplicate anco row matched by storage path, keepFileId=%{public}d, "
            "skipFileId=%{public}d, path=%{public}s", result.first->second.fileId, row.fileId,
            MediaFileUtils::DesensitizePath(row.storagePath).c_str());
    }
}

AncoRowsByPath QueryAncoRowsByPaths(
    const std::shared_ptr<NativeRdb::RdbStore> &finalDb, const std::vector<std::string> &paths,
    AncoReverseCloneStats &stats)
{
    AncoRowsByPath rowsByPath;
    CHECK_AND_RETURN_RET_LOG(finalDb != nullptr, rowsByPath, "QueryAncoRowsByPaths: finalDb is null");
    if (paths.empty()) {
        MEDIA_INFO_LOG("QueryAncoRowsByPaths skipped, pathCount=0");
        return rowsByPath;
    }

    MEDIA_INFO_LOG("QueryAncoRowsByPaths start, pathCount=%{public}zu", paths.size());
    for (size_t offset = 0; offset < paths.size(); offset += PAGE_SIZE) {
        size_t batchSize = std::min(static_cast<size_t>(PAGE_SIZE), paths.size() - offset);
        const std::string querySql = "SELECT " + MediaColumn::MEDIA_ID + ", " +
            PhotoColumn::PHOTO_STORAGE_PATH + ", " + MediaColumn::MEDIA_NAME + ", " +
            MediaColumn::MEDIA_SIZE + ", " + MediaColumn::MEDIA_DATE_MODIFIED +
            " FROM " + PhotoColumn::PHOTOS_TABLE +
            " WHERE " + PhotoColumn::PHOTO_FILE_SOURCE_TYPE + " = ? AND " +
            PhotoColumn::PHOTO_STORAGE_PATH + " IN (" + BuildPlaceholders(batchSize) + ") ORDER BY " +
            MediaColumn::MEDIA_ID + " ASC";
        std::vector<NativeRdb::ValueObject> params = {
            FileSourceType::MEDIA_HO_LAKE,
        };
        for (size_t i = 0; i < batchSize; i++) {
            params.emplace_back(paths[offset + i]);
        }
        MEDIA_INFO_LOG("QueryAncoRowsByPaths batch, offset=%{public}zu, batchSize=%{public}zu",
            offset, batchSize);
        auto resultSet = BackupDatabaseUtils::QuerySql(finalDb, querySql, params);
        if (resultSet == nullptr) {
            MEDIA_WARN_LOG("QueryAncoRowsByPaths batch query failed, offset=%{public}zu", offset);
            continue;
        }
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AncoPhotoRow row;
            row.fileId = GetInt32Val(MediaColumn::MEDIA_ID, resultSet);
            row.storagePath = GetStringVal(PhotoColumn::PHOTO_STORAGE_PATH, resultSet);
            row.displayName = GetStringVal(MediaColumn::MEDIA_NAME, resultSet);
            row.size = GetInt64Val(MediaColumn::MEDIA_SIZE, resultSet);
            row.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
            AddAncoRowByPath(rowsByPath, row, stats);
        }
        resultSet->Close();
    }
    MEDIA_INFO_LOG("QueryAncoRowsByPaths finish, inputPaths=%{public}zu, matchedPaths=%{public}zu",
        paths.size(), rowsByPath.size());
    return rowsByPath;
}

class AncoRestoreInfoRepository {
public:
    bool Load(const std::string &dbPath)
    {
        byPath_.clear();
        deduplicationInfos_.clear();
        failedPaths_.clear();
        MEDIA_INFO_LOG("Load anco restore info start, dbPath=%{public}s",
            MediaFileUtils::DesensitizePath(dbPath).c_str());
        if (!MediaFileUtils::IsFileExists(dbPath)) {
            MEDIA_WARN_LOG("Anco restore info db does not exist: %{public}s",
                MediaFileUtils::DesensitizePath(dbPath).c_str());
            return false;
        }

        CloneFileInfoRestoreDbCallback callback;
        NativeRdb::RdbStoreConfig config("");
        config.SetName(CLONE_FILE_INFO_RESTORE_DB);
        config.SetPath(dbPath);
        int32_t err = 0;
        auto rdbStore = NativeRdb::RdbHelper::GetRdbStore(config, 1, callback, err);
        if (rdbStore == nullptr) {
            MEDIA_ERR_LOG("Open anco restore info db failed, err=%{public}d", err);
            return false;
        }
        MEDIA_INFO_LOG("Open anco restore info db success");
        LoadFailedPaths(rdbStore);
        LoadDeduplicationPaths(rdbStore);
        MEDIA_INFO_LOG("Load anco restore info finish, failPaths=%{public}zu, dedupPaths=%{public}zu",
            failedPaths_.size(), deduplicationInfos_.size());
        return true;
    }

    const std::vector<std::string> &GetFailedPaths() const
    {
        return failedPaths_;
    }

    const std::vector<AncoDeduplicationInfo> &GetDeduplicationInfos() const
    {
        return deduplicationInfos_;
    }

private:
    void LoadFailedPaths(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
    {
        const std::string querySql = "SELECT " + PhotoColumn::CLONE_FILE_INFO_PATH +
            " FROM " + LAKE_FILE_INFO_FAIL_TABLE;
        MEDIA_INFO_LOG("Query anco fail info start, sql=%{public}s", querySql.c_str());
        auto resultSet = BackupDatabaseUtils::QuerySql(rdbStore, querySql, {});
        if (resultSet == nullptr) {
            MEDIA_WARN_LOG("Query anco fail info failed or table unavailable");
            return;
        }
        int32_t rowCount = 0;
        std::unordered_set<std::string> uniqueFailedPaths;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            std::string failedPath = BackupFileUtils::ConvertToStoragePath(
                GetStringVal(PhotoColumn::CLONE_FILE_INFO_PATH, resultSet));
            rowCount++;
            if (!failedPath.empty() && uniqueFailedPaths.insert(failedPath).second) {
                failedPaths_.emplace_back(failedPath);
            }
        }
        resultSet->Close();
        MEDIA_INFO_LOG("Loaded anco fail info finish, rows=%{public}d, failedPaths=%{public}zu",
            rowCount, failedPaths_.size());
    }

    void LoadDeduplicationPaths(const std::shared_ptr<NativeRdb::RdbStore> &rdbStore)
    {
        const std::string querySql = "SELECT " + PhotoColumn::CLONE_FILE_INFO_PATH + ", " +
            PhotoColumn::CLONE_FILE_INFO_NEW_PATH + " FROM " + LAKE_FILE_INFO_DEDUPLICATION_TABLE;
        MEDIA_INFO_LOG("Query anco deduplication info start, sql=%{public}s", querySql.c_str());
        auto resultSet = BackupDatabaseUtils::QuerySql(rdbStore, querySql, {});
        if (resultSet == nullptr) {
            MEDIA_WARN_LOG("Query anco deduplication info failed or table unavailable");
            return;
        }
        int32_t rowCount = 0;
        while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
            AncoDeduplicationInfo info;
            info.path = BackupFileUtils::ConvertToStoragePath(
                GetStringVal(PhotoColumn::CLONE_FILE_INFO_PATH, resultSet));
            info.newPath = BackupFileUtils::ConvertToStoragePath(
                GetStringVal(PhotoColumn::CLONE_FILE_INFO_NEW_PATH, resultSet));
            AddInfo(info);
            rowCount++;
        }
        resultSet->Close();
        MEDIA_INFO_LOG("Loaded anco deduplication info finish, rows=%{public}d, byPath=%{public}zu",
            rowCount, byPath_.size());
    }

    void AddInfo(const AncoDeduplicationInfo &info)
    {
        if (!info.path.empty() && byPath_.find(info.path) == byPath_.end()) {
            byPath_[info.path] = info;
            deduplicationInfos_.emplace_back(info);
        }
    }

    std::unordered_map<std::string, AncoDeduplicationInfo> byPath_;
    std::vector<AncoDeduplicationInfo> deduplicationInfos_;
    std::vector<std::string> failedPaths_;
};

std::vector<std::string> BuildDeduplicationPaths(const std::vector<AncoDeduplicationInfo> &infos)
{
    std::vector<std::string> paths;
    paths.reserve(infos.size());
    for (const auto &info : infos) {
        if (!info.path.empty()) {
            paths.emplace_back(info.path);
        }
    }
    return paths;
}

bool CheckNewPathReady(const AncoPhotoRow &row, const AncoDeduplicationInfo &info, std::string &inode,
    AncoReverseCloneStats &stats)
{
    // Keep forward clone semantics: trust the concrete new_path from clone_file_info_restore.db,
    // then accept it only when the file exists and its size matches the retained Photos row.
    struct stat statInfo {};
    if (!GetFileStat(info.newPath, statInfo)) {
        stats.invalidNewPathRows++;
        LogLakePathCheck(row.fileId, "invalid_new_path", info.newPath);
        MEDIA_WARN_LOG("PhaseTwo skip update for invalid new path, fileId=%{public}d, newPath=%{public}s",
            row.fileId, MediaFileUtils::DesensitizePath(info.newPath).c_str());
        return false;
    }
    if (row.size > 0 && static_cast<int64_t>(statInfo.st_size) != row.size) {
        stats.invalidNewPathRows++;
        MEDIA_WARN_LOG("PhaseTwo skip update for size mismatch, fileId=%{public}d, expected=%{public}" PRId64
            ", actual=%{public}" PRId64 ", newPath=%{public}s", row.fileId, row.size,
            static_cast<int64_t>(statInfo.st_size), MediaFileUtils::DesensitizePath(info.newPath).c_str());
        return false;
    }
    inode = std::to_string(statInfo.st_ino);
    MEDIA_INFO_LOG("PhaseTwo new path validated, fileId=%{public}d, inode=%{public}s, "
        "size=%{public}" PRId64 ", newPath=%{public}s", row.fileId, inode.c_str(),
        static_cast<int64_t>(statInfo.st_size), MediaFileUtils::DesensitizePath(info.newPath).c_str());
    return true;
}

AncoRepairUpdate BuildUpdate(const AncoPhotoRow &row, const AncoDeduplicationInfo &info, const std::string &inode)
{
    AncoRepairUpdate update;
    update.fileId = row.fileId;
    update.storagePath = info.newPath;
    update.displayName = MediaFileUtils::GetFileName(info.newPath);
    update.title = MediaFileUtils::GetTitleFromDisplayName(update.displayName);
    update.inode = inode;
    MEDIA_INFO_LOG("Build anco update, fileId=%{public}d, displayName=%{public}s, title=%{public}s, "
        "inode=%{public}s, newPath=%{public}s", update.fileId, update.displayName.c_str(),
        update.title.c_str(), update.inode.c_str(), MediaFileUtils::DesensitizePath(update.storagePath).c_str());
    return update;
}

bool ExecuteUpdates(const std::shared_ptr<NativeRdb::RdbStore> &finalDb,
    const std::vector<AncoRepairUpdate> &updates, AncoReverseCloneStats &stats)
{
    if (updates.empty()) {
        MEDIA_INFO_LOG("Execute anco updates skipped, count=0");
        return true;
    }
    MEDIA_INFO_LOG("Execute anco updates start, count=%{public}zu", updates.size());
    TransactionOperations trans { __func__ };
    trans.SetBackupRdbStore(finalDb);
    const std::string updateSql = "UPDATE " + PhotoColumn::PHOTOS_TABLE + " SET " +
        PhotoColumn::PHOTO_STORAGE_PATH + " = ?, " + MediaColumn::MEDIA_NAME + " = ?, " +
        MediaColumn::MEDIA_TITLE + " = ?, " + PhotoColumn::PHOTO_FILE_INODE + " = ? WHERE " +
        MediaColumn::MEDIA_ID + " = ?";
    std::function<int()> updateFunc = [&]() -> int {
        for (const auto &update : updates) {
            int32_t ret = trans.ExecuteSql(updateSql, {
                update.storagePath, update.displayName, update.title, update.inode, update.fileId
            });
            if (ret != NativeRdb::E_OK) {
                stats.updateFailedRows++;
                MEDIA_ERR_LOG("Update anco row failed, fileId=%{public}d, ret=%{public}d, "
                    "path=%{public}s", update.fileId, ret,
                    MediaFileUtils::DesensitizePath(update.storagePath).c_str());
                return ret;
            }
            MEDIA_INFO_LOG("Update anco row success, fileId=%{public}d, path=%{public}s",
                update.fileId, MediaFileUtils::DesensitizePath(update.storagePath).c_str());
        }
        return NativeRdb::E_OK;
    };
    int32_t ret = trans.RetryTrans(updateFunc, true);
    CHECK_AND_PRINT_LOG(ret == NativeRdb::E_OK, "ExecuteUpdates failed, ret=%{public}d", ret);
    MEDIA_INFO_LOG("Execute anco updates finish, count=%{public}zu, ret=%{public}d", updates.size(), ret);
    return ret == NativeRdb::E_OK;
}

bool ExecuteDeletes(const std::vector<int32_t> &fileIds, AncoReverseCloneStats &stats)
{
    if (fileIds.empty()) {
        MEDIA_INFO_LOG("Execute anco deletes skipped, count=0");
        return true;
    }
    MEDIA_INFO_LOG("Execute anco permanent deletes start, count=%{public}zu", fileIds.size());
    int32_t acceptedRows = 0;
    bool success = true;
    for (size_t offset = 0; offset < fileIds.size(); offset += PAGE_SIZE) {
        size_t batchSize = std::min(static_cast<size_t>(PAGE_SIZE), fileIds.size() - offset);
        std::vector<std::string> batchIds;
        batchIds.reserve(batchSize);
        for (size_t i = 0; i < batchSize; i++) {
            int32_t fileId = fileIds[offset + i];
            batchIds.emplace_back(std::to_string(fileId));
            MEDIA_INFO_LOG("Permanent delete anco row prepared, fileId=%{public}d", fileId);
        }
        NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
        predicates.In(MediaColumn::MEDIA_ID, batchIds);
        // RepairFinalDb is invoked after finalDb has become the active media library store.
        // Keep DeletePermanently here to preserve full asset cleanup semantics, including files,
        // thumbnails, edit data, moving-photo sidecars, download tasks, notifications and album refresh.
        int32_t ret = MediaLibraryAssetOperations::DeletePermanently(predicates, true);
        if (ret != E_OK) {
            stats.deleteFailedRows += static_cast<int32_t>(batchSize);
            MEDIA_ERR_LOG("Permanent delete anco rows failed, offset=%{public}zu, batchSize=%{public}zu, "
                "ret=%{public}d", offset, batchSize, ret);
            success = false;
            continue;
        }
        acceptedRows += static_cast<int32_t>(batchSize);
        stats.deletedRows += static_cast<int32_t>(batchSize);
        MEDIA_INFO_LOG("Permanent delete anco rows accepted by DeletePermanently, "
            "offset=%{public}zu, batchSize=%{public}zu", offset, batchSize);
    }
    MEDIA_INFO_LOG("Execute anco permanent deletes finish, count=%{public}zu, acceptedRows=%{public}d, "
        "failedRows=%{public}d", fileIds.size(), acceptedRows, stats.deleteFailedRows);
    return success;
}

class AncoRepairBatch {
public:
    AncoRepairBatch(const std::shared_ptr<NativeRdb::RdbStore> &finalDb, AncoReverseCloneStats &stats)
        : finalDb_(finalDb), stats_(stats)
    {}

    const std::shared_ptr<NativeRdb::RdbStore> &GetFinalDb() const
    {
        return finalDb_;
    }

    AncoReverseCloneStats &GetStats()
    {
        return stats_;
    }

    bool IsMarkedForDelete(int32_t fileId) const
    {
        return deleteFileIdSet_.find(fileId) != deleteFileIdSet_.end();
    }

    bool AddUpdate(const AncoRepairUpdate &update)
    {
        updates_.emplace_back(update);
        return FlushUpdatesIfNeeded();
    }

    void AddDelete(int32_t fileId)
    {
        if (deleteFileIdSet_.insert(fileId).second) {
            deleteFileIds_.emplace_back(fileId);
        }
    }

    bool Finish()
    {
        MEDIA_INFO_LOG("FinishRepairBatch start, updateCount=%{public}zu, deleteCount=%{public}zu",
            updates_.size(), deleteFileIds_.size());
        bool success = FlushUpdates();
        success = ExecuteDeletes(deleteFileIds_, stats_) && success;
        if (!success) {
            MEDIA_ERR_LOG("FinishRepairBatch failed, deleteCount=%{public}zu", deleteFileIds_.size());
        }
        MEDIA_INFO_LOG("FinishRepairBatch finish, deleteCount=%{public}zu", deleteFileIds_.size());
        deleteFileIds_.clear();
        deleteFileIdSet_.clear();
        return success;
    }

private:
    bool FlushUpdatesIfNeeded()
    {
        if (updates_.size() < PAGE_SIZE) {
            return true;
        }
        return FlushUpdates();
    }

    bool FlushUpdates()
    {
        if (updates_.empty()) {
            return true;
        }
        MEDIA_INFO_LOG("FlushRepairUpdates start, count=%{public}zu", updates_.size());
        bool success = ExecuteUpdates(finalDb_, updates_, stats_);
        if (!success) {
            MEDIA_ERR_LOG("FlushRepairUpdates failed, size=%{public}zu", updates_.size());
        }
        MEDIA_INFO_LOG("FlushRepairUpdates finish, count=%{public}zu, success=%{public}d",
            updates_.size(), static_cast<int32_t>(success));
        updates_.clear();
        return success;
    }

    std::shared_ptr<NativeRdb::RdbStore> finalDb_;
    AncoReverseCloneStats &stats_;
    std::vector<AncoRepairUpdate> updates_;
    std::vector<int32_t> deleteFileIds_;
    std::unordered_set<int32_t> deleteFileIdSet_;
};

void AddDeleteIfMissing(const AncoPhotoRow &row, AncoRepairBatch &batch)
{
    bool fileExists = IsFileExistingPath(row.storagePath);
    if (fileExists) {
        batch.GetStats().phaseOneRowsKept++;
        LogLakePathCheck(row.fileId, "keep_existing_origin", row.storagePath);
        MEDIA_INFO_LOG("Keep anco row because origin exists, fileId=%{public}d, path=%{public}s",
            row.fileId, MediaFileUtils::DesensitizePath(row.storagePath).c_str());
        return;
    }
    LogLakePathCheck(row.fileId, "delete_missing_origin", row.storagePath);
    MEDIA_WARN_LOG("Mark anco row deleted because origin missing, fileId=%{public}d, path=%{public}s",
        row.fileId, MediaFileUtils::DesensitizePath(row.storagePath).c_str());
    batch.AddDelete(row.fileId);
}

bool RepairPhaseOneRows(const std::shared_ptr<NativeRdb::RdbStore> &finalDb,
    const std::vector<AncoPhotoRow> &rows, AncoReverseCloneStats &stats)
{
    MEDIA_INFO_LOG("RepairPhaseOneRows start, rows=%{public}zu", rows.size());
    AncoRepairBatch batch(finalDb, stats);
    for (const auto &row : rows) {
        AddDeleteIfMissing(row, batch);
    }
    bool success = batch.Finish();
    MEDIA_INFO_LOG("RepairPhaseOneRows finish, rows=%{public}zu, kept=%{public}d, deleted=%{public}d",
        rows.size(), stats.phaseOneRowsKept, stats.deletedRows);
    return success;
}

bool RepairPhaseOne(const std::shared_ptr<NativeRdb::RdbStore> &finalDb, AncoReverseCloneStats &stats)
{
    std::vector<AncoPhotoRow> rows = QueryAncoRows(finalDb);
    stats.totalRows = static_cast<int32_t>(rows.size());
    if (rows.empty()) {
        MEDIA_INFO_LOG("RepairFinalDb: no anco rows");
        return true;
    }
    MEDIA_INFO_LOG("RepairFinalDb use phase one repair");
    return RepairPhaseOneRows(finalDb, rows, stats);
}

bool ProcessPhaseTwoFailedRows(const AncoRestoreInfoRepository &repo, AncoRepairBatch &batch)
{
    auto &stats = batch.GetStats();
    const auto &failedPaths = repo.GetFailedPaths();
    MEDIA_INFO_LOG("ProcessPhaseTwoFailedRows start, failedPathCount=%{public}zu", failedPaths.size());
    auto rowsByPath = QueryAncoRowsByPaths(batch.GetFinalDb(), failedPaths, stats);
    for (const auto &path : failedPaths) {
        auto it = rowsByPath.find(path);
        if (it == rowsByPath.end()) {
            stats.phaseTwoFailedRowsNotMatched++;
            MEDIA_INFO_LOG("Fail info not matched final Photos, path=%{public}s",
                MediaFileUtils::DesensitizePath(path).c_str());
            continue;
        }
        const auto &row = it->second;
        stats.phaseTwoFailedRowsMatched++;
        MEDIA_WARN_LOG("Mark anco row deleted because File Manager transfer failed, fileId=%{public}d, "
            "path=%{public}s", row.fileId, MediaFileUtils::DesensitizePath(row.storagePath).c_str());
        batch.AddDelete(row.fileId);
    }
    MEDIA_INFO_LOG("ProcessPhaseTwoFailedRows finish, matched=%{public}d, notMatched=%{public}d",
        stats.phaseTwoFailedRowsMatched, stats.phaseTwoFailedRowsNotMatched);
    return true;
}

bool ProcessPhaseTwoDeduplicationRows(const AncoRestoreInfoRepository &repo, AncoRepairBatch &batch)
{
    auto &stats = batch.GetStats();
    const auto &infos = repo.GetDeduplicationInfos();
    MEDIA_INFO_LOG("ProcessPhaseTwoDeduplicationRows start, dedupCount=%{public}zu", infos.size());
    auto rowsByPath = QueryAncoRowsByPaths(batch.GetFinalDb(), BuildDeduplicationPaths(infos), stats);
    for (const auto &info : infos) {
        auto it = rowsByPath.find(info.path);
        if (it == rowsByPath.end()) {
            stats.phaseTwoRowsNotMatched++;
            MEDIA_INFO_LOG("Deduplication info not matched final Photos, oldPath=%{public}s, "
                "newPath=%{public}s", MediaFileUtils::DesensitizePath(info.path).c_str(),
                MediaFileUtils::DesensitizePath(info.newPath).c_str());
            continue;
        }
        const auto &row = it->second;
        stats.phaseTwoRowsMatched++;
        MEDIA_INFO_LOG("Deduplication info matched by path, fileId=%{public}d, oldPath=%{public}s, "
            "newPath=%{public}s", row.fileId, MediaFileUtils::DesensitizePath(row.storagePath).c_str(),
            MediaFileUtils::DesensitizePath(info.newPath).c_str());
        if (batch.IsMarkedForDelete(row.fileId)) {
            MEDIA_WARN_LOG("Skip anco dedup update because row already marked deleted, fileId=%{public}d",
                row.fileId);
            continue;
        }
        std::string inode;
        if (!CheckNewPathReady(row, info, inode, stats)) {
            MEDIA_WARN_LOG("PhaseTwo delete anco row because matched new path is invalid, "
                "fileId=%{public}d, oldPath=%{public}s, newPath=%{public}s", row.fileId,
                MediaFileUtils::DesensitizePath(row.storagePath).c_str(),
                MediaFileUtils::DesensitizePath(info.newPath).c_str());
            batch.AddDelete(row.fileId);
            continue;
        }
        if (!batch.AddUpdate(BuildUpdate(row, info, inode))) {
            MEDIA_ERR_LOG("ProcessPhaseTwoDeduplicationRows abort: update flush failed");
            return false;
        }
    }
    MEDIA_INFO_LOG("ProcessPhaseTwoDeduplicationRows finish, matched=%{public}d, notMatched=%{public}d",
        stats.phaseTwoRowsMatched, stats.phaseTwoRowsNotMatched);
    return true;
}

bool RepairPhaseTwoRows(const std::shared_ptr<NativeRdb::RdbStore> &finalDb,
    const AncoRestoreInfoRepository &repo, AncoReverseCloneStats &stats)
{
    MEDIA_INFO_LOG("RepairPhaseTwoRows start, failedPaths=%{public}zu, dedupPaths=%{public}zu",
        repo.GetFailedPaths().size(), repo.GetDeduplicationInfos().size());
    AncoRepairBatch batch(finalDb, stats);
    bool success = ProcessPhaseTwoFailedRows(repo, batch);
    success = ProcessPhaseTwoDeduplicationRows(repo, batch) && success;
    success = batch.Finish() && success;
    MEDIA_INFO_LOG("RepairPhaseTwoRows finish, matched=%{public}d, notMatched=%{public}d, "
        "failMatched=%{public}d, failNotMatched=%{public}d, invalidPath=%{public}d, deleted=%{public}d",
        stats.phaseTwoRowsMatched, stats.phaseTwoRowsNotMatched, stats.phaseTwoFailedRowsMatched,
        stats.phaseTwoFailedRowsNotMatched, stats.invalidNewPathRows, stats.deletedRows);
    return success;
}
} // namespace

int32_t AncoReverseCloneAdapter::RepairFinalDb(const std::shared_ptr<NativeRdb::RdbStore> &finalDb,
    const AncoReverseCloneContext &context)
{
    stats_ = {};
    CHECK_AND_RETURN_RET_LOG(finalDb != nullptr, E_ERR, "RepairFinalDb: finalDb is null");
    AncoReverseClonePhase phase = DecidePhase(context);
    MEDIA_INFO_LOG("RepairFinalDb start, dstAncoFileTransfer=%{public}d, phase=%{public}d, "
        "customDeduplicationDbPath=%{public}d",
        static_cast<int32_t>(context.dstConfig.ancoFileTransfer), static_cast<int32_t>(phase),
        static_cast<int32_t>(!context.deduplicationDbPath.empty()));

    bool success = true;
    if (phase == AncoReverseClonePhase::PHASE_TWO) {
        AncoRestoreInfoRepository repo;
        if (repo.Load(ResolveDeduplicationDbPath(context))) {
            stats_.totalRows =
                static_cast<int32_t>(repo.GetFailedPaths().size() + repo.GetDeduplicationInfos().size());
            MEDIA_INFO_LOG("RepairFinalDb use phase two repair");
            success = RepairPhaseTwoRows(finalDb, repo, stats_);
            if (!success) {
                MEDIA_ERR_LOG("RepairFinalDb: phase two repair failed");
            }
        } else {
            MEDIA_WARN_LOG("RepairFinalDb: phase two restore info unavailable, fallback to phase one repair");
            success = RepairPhaseOne(finalDb, stats_);
            if (!success) {
                MEDIA_ERR_LOG("RepairFinalDb: fallback phase one repair failed");
            }
        }
    } else {
        success = RepairPhaseOne(finalDb, stats_);
        if (!success) {
            MEDIA_ERR_LOG("RepairFinalDb: phase one repair failed");
        }
    }

    MEDIA_INFO_LOG("RepairFinalDb finished: total=%{public}d, matched=%{public}d, notMatched=%{public}d, "
        "failMatched=%{public}d, failNotMatched=%{public}d, duplicateOldPath=%{public}d, "
        "invalidPath=%{public}d, kept=%{public}d, deleted=%{public}d, updateFailed=%{public}d, "
        "deleteFailed=%{public}d, success=%{public}d",
        stats_.totalRows, stats_.phaseTwoRowsMatched, stats_.phaseTwoRowsNotMatched,
        stats_.phaseTwoFailedRowsMatched, stats_.phaseTwoFailedRowsNotMatched,
        stats_.phaseTwoDuplicateOldPathRows, stats_.invalidNewPathRows, stats_.phaseOneRowsKept,
        stats_.deletedRows, stats_.updateFailedRows, stats_.deleteFailedRows, static_cast<int32_t>(success));
    return (success && stats_.updateFailedRows == 0 && stats_.deleteFailedRows == 0) ? E_OK : E_ERR;
}

const AncoReverseCloneStats &AncoReverseCloneAdapter::GetStats() const
{
    return stats_;
}

AncoReverseClonePhase AncoReverseCloneAdapter::DecidePhase(const AncoReverseCloneContext &context)
{
    bool destSupportsPhaseTwo =
        context.dstConfig.ancoFileTransfer == AncoFileTransfer::ANCO_FILE_TRANSFER_SUPPORTED;
    return destSupportsPhaseTwo ? AncoReverseClonePhase::PHASE_TWO : AncoReverseClonePhase::PHASE_ONE;
}

std::string AncoReverseCloneAdapter::GetDefaultDeduplicationDbPath()
{
    return OTHER_CLONE_PATH + CLONE_FILE_INFO_RESTORE_DB;
}
} // namespace Media
} // namespace OHOS