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
 
#include "asset_map_clone.h"
 
#include "backup_database_utils.h"
#include "backup_const_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "result_set_utils.h"
 
namespace OHOS::Media {

const std::string TAB_CLONED_OLD_PHOTOS_TABLE = "tab_cloned_old_photos";
 
AssetMapClone::AssetMapClone(
    const std::shared_ptr<NativeRdb::RdbStore>& sourceRdb,
    const std::shared_ptr<NativeRdb::RdbStore>& destRdb,
    const std::unordered_map<int32_t, PhotoInfo>& photoInfoMap)
    : sourceRdb_(sourceRdb),
      destRdb_(destRdb),
      photoInfoMap_(photoInfoMap)
{
}
 
bool AssetMapClone::CloneAssetMapInfo()
{
    int64_t start = MediaFileUtils::UTCTimeMilliSeconds();
    std::vector<int32_t> oldFileIds;
    oldFileIds.reserve(photoInfoMap_.size());
 
    for (const auto& pair : photoInfoMap_) {
        oldFileIds.push_back(pair.first);
    }
 
    if (oldFileIds.empty()) {
        MEDIA_INFO_LOG("photoInfoMap_ is empty, no asset map entries to clone.");
        migrateTotalTimeCost_ = MediaFileUtils::UTCTimeMilliSeconds() - start;
        return true;
    }
 
    std::optional<int32_t> lastCloneSequenceOpt = GetLastCloneSequence();
    nextCloneSequence_ = lastCloneSequenceOpt.value_or(0) + 1;
    CloneAssetMapInBatches(oldFileIds);
 
    int64_t end = MediaFileUtils::UTCTimeMilliSeconds();
    migrateTotalTimeCost_ += end - start;
    MEDIA_INFO_LOG("CloneAssetMapInfo completed. Changed %{public}lld records. insert %{public}lld records."
        "Total time: %{public}lld ms",
        (long long)migrateNum_, (long long)insertNum_, (long long)migrateTotalTimeCost_);
    return true;
}
 
bool AssetMapClone::CloneAssetMapInBatches(const std::vector<int32_t>& oldFileIds)
{
    for (size_t i = 0; i < oldFileIds.size(); i += SQL_BATCH_SIZE) {
        auto batch_begin = oldFileIds.begin() + i;
        auto batch_end = ((i + SQL_BATCH_SIZE < oldFileIds.size()) ?
            (oldFileIds.begin() + i + SQL_BATCH_SIZE) : oldFileIds.end());
        std::vector<int32_t> batchOldFileIds(batch_begin, batch_end);
 
        if (batchOldFileIds.empty()) {
            continue;
        }
 
        std::string fileIdOldInClause = "(" + BackupDatabaseUtils::JoinValues<int>(batchOldFileIds, ", ") + ")";
        std::vector<AssetMapTbl> assetMapTbls = QueryAssetMapTbl(fileIdOldInClause);
        if (assetMapTbls.empty()) {
            MEDIA_WARN_LOG("Query returned empty result for batch starting at index %{public}zu", i);
            continue;
        }
 
        std::vector<AssetMapTbl> processAssetMaps = ProcessAssetMapTbls(assetMapTbls);
        BatchInsertAssetMaps(processAssetMaps);
    }
    return true;
}
 
std::vector<AssetMapTbl> AssetMapClone::QueryAssetMapTbl(const std::string &fileIdClause)
{
    std::vector<AssetMapTbl> result;
 
    std::string querySql = "SELECT file_id, data FROM Photos ";
    querySql += " WHERE " + ASSET_MAP_COL_FILE_ID + " IN " + fileIdClause;
 
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(sourceRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, result, "Query resultSet is null.");
 
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        AssetMapTbl assetMapTbl;
        ParseAssetMapResultSet(resultSet, assetMapTbl);
        result.emplace_back(assetMapTbl);
    }
 
    resultSet->Close();
    return result;
}
 
void AssetMapClone::ParseAssetMapResultSet(
    const std::shared_ptr<NativeRdb::ResultSet>& resultSet, AssetMapTbl& assetMapTbl)
{
    // Read original values from source database
    assetMapTbl.OldFileId = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ASSET_MAP_COL_FILE_ID);
    assetMapTbl.OldData = BackupDatabaseUtils::GetOptionalValue<std::string>(resultSet, ASSET_MAP_COL_DATA);
 
    // Generate new fileId based on mapping
    if (assetMapTbl.OldFileId.has_value()) {
        int32_t oldFileId = assetMapTbl.OldFileId.value();
        const auto it = photoInfoMap_.find(oldFileId);
        if (it != photoInfoMap_.end()) {
            assetMapTbl.fileId = it->second.fileIdNew;
            assetMapTbl.data = it->second.cloudPath;
        } else {
            MEDIA_WARN_LOG("Original fileId %{public}d not found in photoInfoMap_", oldFileId);
        }
    }
}
 
std::optional<int32_t> AssetMapClone::GetLastCloneSequence()
{
    std::string querySql = "SELECT " + ASSET_MAP_COL_CLONE_SEQUENCE +
                          " FROM " + TAB_CLONED_OLD_PHOTOS_TABLE +
                          " ORDER BY " + ASSET_MAP_COL_CLONE_SEQUENCE + " DESC LIMIT 1";
 
    auto resultSet = BackupDatabaseUtils::GetQueryResultSet(destRdb_, querySql);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, std::nullopt, "Query resultSet is null for clone sequence");
 
    std::optional<int32_t> lastSequence;
    if (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        lastSequence = BackupDatabaseUtils::GetOptionalValue<int32_t>(resultSet, ASSET_MAP_COL_CLONE_SEQUENCE);
    }
 
    resultSet->Close();
    return lastSequence;
}
 
std::vector<AssetMapTbl> AssetMapClone::ProcessAssetMapTbls(
    const std::vector<AssetMapTbl>& assetMapTbls)
{
    CHECK_AND_RETURN_RET_LOG(!assetMapTbls.empty(), {}, "asset map tbl empty");
 
    std::vector<AssetMapTbl> assetMapNewTbls;
    assetMapNewTbls.reserve(assetMapTbls.size());
    for (const auto& assetMapTbl : assetMapTbls) {
        // Set clone sequence
        AssetMapTbl tbl = assetMapTbl;
        tbl.cloneSequence = nextCloneSequence_;
        assetMapNewTbls.push_back(std::move(tbl));
    }
    return assetMapNewTbls;
}
 
void AssetMapClone::BatchInsertAssetMaps(const std::vector<AssetMapTbl>& assetMapTbls)
{
    std::vector<NativeRdb::ValuesBucket> valuesBucketsToInsert;
    std::unordered_set<int32_t> fileIdsNewlyInserted;
    valuesBucketsToInsert.reserve(assetMapTbls.size());
 
    for (const auto& assetMapTbl : assetMapTbls) {
        if (!assetMapTbl.fileId.has_value()) {
            MEDIA_WARN_LOG("AssetMapTbl has no fileId, skipping.");
            continue;
        }
 
        int32_t currentFileId = assetMapTbl.fileId.value();
        valuesBucketsToInsert.push_back(CreateValuesBucketFromAssetMapTbl(assetMapTbl));
        fileIdsNewlyInserted.insert(currentFileId);
    }
 
    if (valuesBucketsToInsert.empty()) {
        MEDIA_ERR_LOG("No new asset map entries to insert.");
        return;
    }
 
    int64_t changedRows = 0;
    int32_t ret = BatchInsertWithRetry(TAB_CLONED_OLD_PHOTOS_TABLE, valuesBucketsToInsert, changedRows);
    CHECK_AND_RETURN_LOG(ret == E_OK, "Failed to batch insert asset maps");
 
    migrateNum_ += changedRows;
    insertNum_ += static_cast<int64_t>(fileIdsNewlyInserted.size());
}
 
NativeRdb::ValuesBucket AssetMapClone::CreateValuesBucketFromAssetMapTbl(
    const AssetMapTbl& assetMapTbl)
{
    NativeRdb::ValuesBucket values;
 
    PutIfPresent(values, ASSET_MAP_COL_FILE_ID, assetMapTbl.fileId);
    PutIfPresent(values, ASSET_MAP_COL_DATA, assetMapTbl.data);
    PutIfPresent(values, ASSET_MAP_COL_OLD_FILE_ID, assetMapTbl.OldFileId);
    PutIfPresent(values, ASSET_MAP_COL_OLD_DATA, assetMapTbl.OldData);
    PutIfPresent(values, ASSET_MAP_COL_CLONE_SEQUENCE, assetMapTbl.cloneSequence);
 
    return values;
}
 
int32_t AssetMapClone::BatchInsertWithRetry(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values, int64_t &changedRows)
{
    CHECK_AND_RETURN_RET(!values.empty(), E_OK);
    int32_t errCode = E_ERR;
    TransactionOperations trans{ __func__ };
    trans.SetBackupRdbStore(destRdb_);
    std::function<int(void)> func = [&]()->int {
        errCode = trans.BatchInsert(changedRows, tableName, values, NativeRdb::ConflictResolution::ON_CONFLICT_REPLACE);
        CHECK_AND_PRINT_LOG(errCode == E_OK, "InsertSql failed, errCode: %{public}d, rowNum: %{public}ld.",
            errCode, (long)changedRows);
        return errCode;
    };
    errCode = trans.RetryTrans(func, true);
    CHECK_AND_PRINT_LOG(errCode == E_OK, "BatchInsertWithRetry: tans finish fail!, ret:%{public}d", errCode);
    return errCode;
}
} // namespace OHOS::Media