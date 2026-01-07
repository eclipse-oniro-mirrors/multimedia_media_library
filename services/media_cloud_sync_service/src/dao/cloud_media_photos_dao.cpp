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

#define MLOG_TAG "Media_Cloud_Dao"

#include "cloud_media_photos_dao.h"

#include "photo_map_column.h"
#include "cloud_media_sync_utils.h"
#include "cloud_media_operation_code.h"
#include "exif_rotate_utils.h"
#include "medialibrary_unistore_manager.h"
#include "result_set_reader.h"
#include "photos_po_writer.h"
#include "photo_album_po_writer.h"
#include "cloud_sync_convert.h"
#include "photo_map_column.h"
#include "medialibrary_rdb_transaction.h"
#include "medialibrary_rdb_utils.h"
#include "scanner_utils.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_dao_utils.h"
#include "media_file_utils.h"
#include "cloud_media_context.h"
#include "photos_field_iterator.h"
#include "hi_audit.h"
#include "photo_owner_album_id_operation.h"
#include "lake_file_utils.h"
#include "photo_album_upload_status_operation.h"

namespace OHOS::Media::CloudSync {
using ChangeType = AAFwk::ChangeInfo::ChangeType;
// LCOV_EXCL_START
void NotifyDateTakenChanged(const CloudMediaPullDataDto &pullData)
{
    MEDIA_INFO_LOG("NotifyDateTakenChanged localTime: %{public}ld, cloudTime: %{public}ld, fileId: %{public}d",
        (long)pullData.dateTaken,
        (long)pullData.basicCreatedTime,
        pullData.localFileId);
    if (pullData.dateTaken != pullData.basicCreatedTime) {
        std::string uri = PhotoColumn::PHOTO_CLOUD_URI_PREFIX + std::to_string(pullData.localFileId) + '/' +
                          std::to_string(pullData.dateTaken) + '/' + std::to_string(pullData.basicCreatedTime);
        MediaGallerySyncNotify::GetInstance().TryNotify(
            uri, static_cast<ChangeType>(ExtraChangeType::PHOTO_TIME_UPDATE), std::to_string(pullData.localFileId));
    }
}

int32_t CloudMediaPhotosDao::BatchInsertFile(std::map<std::string, int> &recordAnalysisAlbumMaps,
    std::map<std::string, std::set<int>> &recordAlbumMaps, std::vector<NativeRdb::ValuesBucket> &insertFiles,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret;
    if (!insertFiles.empty() || !recordAlbumMaps.empty() || !recordAnalysisAlbumMaps.empty()) {
        int64_t rowId = 0;
        std::vector<NativeRdb::ValuesBucket> insertFilesTmp(insertFiles);
        ret = BatchInsertQuick(rowId, PhotoColumn::PHOTOS_TABLE, insertFiles, photoRefresh);
        CHECK_AND_RETURN_RET_LOG(ret != E_STOP, ret, "BatchInsertFile E_STOP failed");
        if (ret != E_OK) {
            MEDIA_ERR_LOG("BatchInsertFile batch insert failed return %{public}d", ret);
            /* 打点 UpdateMetaStat(INDEX_DL_META_ERROR_RDB, records->size() - params.insertFiles.size()); */
            return ret;
        } else {
            /* 打点 UpdateMetaStat(INDEX_DL_META_SUCCESS, params.insertFiles.size(), META_DL_INSERT); */
            BatchInsertAssetMaps(recordAlbumMaps);
            BatchInsertAssetAnalysisMaps(recordAnalysisAlbumMaps);
        }
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::BatchInsertAssetMaps(std::map<std::string, std::set<int32_t>> &recordAlbumMaps)
{
    auto recordIds = vector<string>();
    for (const auto &it : recordAlbumMaps) {
        recordIds.push_back(it.first);
    }
    std::vector<PhotosPo> photos;
    int32_t ret = this->commonDao_.QueryLocalByCloudId(recordIds, PULL_QUERY_COLUMNS, photos);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BatchInsertAssetMaps QueryLocalByCloudId failed");
        return E_RDB;
    }
    std::vector<NativeRdb::ValuesBucket> valuesList;
    for (const auto &it : recordAlbumMaps) {
        for (auto &photo : photos) {
            if (it.first != photo.cloudId.value_or("")) {
                continue;
            }
            for (auto albumId : it.second) {
                NativeRdb::ValuesBucket values;
                values.PutInt(PhotoMap::ALBUM_ID, albumId);
                values.PutInt(PhotoMap::ASSET_ID, photo.fileId.value_or(-1));
                values.PutInt(PhotoMap::DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SDIRTY));
                valuesList.emplace_back(values);
            }
        }
    }
    int64_t rowId = 0;
    ret = this->BatchInsert(rowId, PhotoMap::TABLE, valuesList);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("fail to insert album mapping, ret %{public}d", ret);
        return ret;
    }
    MEDIA_INFO_LOG("add mapping success");
    return E_OK;
}

int32_t CloudMediaPhotosDao::BatchInsertAssetAnalysisMaps(std::map<std::string, int32_t> recordAnalysisAlbumMaps)
{
    auto recordIds = vector<string>();
    for (const auto &it : recordAnalysisAlbumMaps) {
        recordIds.push_back(it.first);
    }
    if (recordIds.empty()) {
        MEDIA_DEBUG_LOG("No shooting mode file");
        return E_OK;
    }
    std::vector<PhotosPo> photos;
    int32_t ret = this->commonDao_.QueryLocalByCloudId(recordIds, PULL_QUERY_COLUMNS, photos);
    if (ret != E_OK) {
        return E_ERR;
    }
    std::vector<NativeRdb::ValuesBucket> batchValues;
    for (const auto &it : recordAnalysisAlbumMaps) {
        for (auto &photo : photos) {
            if (it.first != photo.cloudId.value_or("")) {
                continue;
            }
            NativeRdb::ValuesBucket values;
            values.PutInt(PhotoMap::ALBUM_ID, it.second);
            values.PutInt(PhotoMap::ASSET_ID, photo.fileId.value_or(-1));
            batchValues.push_back(values);
        }
    }
    int64_t rowId;
    auto result = this->BatchInsert(rowId, ANALYSIS_PHOTO_MAP_TABLE, batchValues);
    MEDIA_INFO_LOG("BatchInsertAssetAnalysisMaps result:%{public}d", result);
    return E_OK;
}

int32_t CloudMediaPhotosDao::BatchInsertQuick(int64_t &outRowId, const std::string &table,
    std::vector<NativeRdb::ValuesBucket> &initialBatchValues,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "BatchInsertQuick Failed to get rdbStore.");
    std::vector<NativeRdb::ValuesBucket> succeedValues;
    if (initialBatchValues.size() == 0) {
        return E_OK;
    }

    std::function<int(void)> transFunc = [&]()->int {
        int rdbError = 0;
        auto retInner = photoRefresh->BatchInsert(outRowId, PhotoColumn::PHOTOS_TABLE, initialBatchValues, rdbError,
            NativeRdb::ConflictResolution::ON_CONFLICT_REPLACE);
        CHECK_AND_RETURN_RET_LOG(
            retInner == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
            rdbError,
            "Failed to BatchInsertQuick func, ret=%{public}d, rdbError=%{public}d",
            retInner, rdbError);
        return retInner;
    };
    int32_t ret = E_ERR;
    if (photoRefresh->GetTransaction() != nullptr) {
        ret = photoRefresh->GetTransaction()->RetryTrans(transFunc);
    } else {
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        CHECK_AND_RETURN_RET_LOG(trans != nullptr, E_RDB_STORE_NULL, "BatchInsertQuick Failed to get trans.");
        ret = trans->RetryTrans(transFunc);
    }
    CHECK_AND_RETURN_RET_LOG(
        ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret, "Failed to BatchInsertQuick, ret=%{public}d", ret);
    return ret;
}

int32_t CloudMediaPhotosDao::BatchInsert(
    int64_t &outRowId, const std::string &table, std::vector<NativeRdb::ValuesBucket> &initialBatchValues)
{
    const uint32_t TRY_TIMES = 15;
    std::vector<NativeRdb::ValuesBucket> succeedValues;
    int32_t ret = E_OK;
    uint32_t tryCount = 0;
    if (initialBatchValues.size() == 0) {
        return ret;
    }
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    while (tryCount <= TRY_TIMES) {
        for (auto val = initialBatchValues.begin(); val != initialBatchValues.end();) {
            {
                ret = albumRefresh.Insert(outRowId, table, *val);
            }
            if (ret == E_OK) {
                succeedValues.push_back(*val);
                val = initialBatchValues.erase(val);
            } else {
                val++;
            }
        }
        if (initialBatchValues.empty()) {
            break;
        } else {
            MEDIA_INFO_LOG("batch insert fail try next time, retry time is tryCount %{public}d", tryCount);
            tryCount++;
        }
    }
    if (!initialBatchValues.empty()) {
        MEDIA_ERR_LOG("batch insert fail, try too many times, %{public}zu is not inserted", initialBatchValues.size());
    }
    if (!succeedValues.empty()) {
        albumRefresh.Notify();
        ret = E_OK;
    }
    initialBatchValues.swap(succeedValues);
    return ret;
}

int32_t CloudMediaPhotosDao::UpdateAssetInPhotoMap(const int32_t &fileId, set<int> cloudMapIds)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    std::map<int32_t, int32_t> localMapIds;
    this->commonDao_.QueryLocalMap(fileId, localMapIds);
    std::vector<NativeRdb::ValuesBucket> pmList;
    std::vector<NativeRdb::ValueObject> bindArgs;
    std::stringstream ss;
    for (const auto cloudAlbum : cloudMapIds) {
        if (localMapIds.find(cloudAlbum) == localMapIds.end()) {
            NativeRdb::ValuesBucket values;
            values.PutInt(PhotoMap::ALBUM_ID, cloudAlbum);
            values.PutInt(PhotoMap::ASSET_ID, fileId);
            values.PutInt(PhotoMap::DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SDIRTY));
            pmList.emplace_back(values);
        }
        if (ss.tellp() != 0) {
            ss << ",";
        }
        ss << " ? ";
        bindArgs.emplace_back(cloudAlbum);
    }
    int64_t rowId = 0;
    int32_t ret = this->BatchInsert(rowId, PhotoMap::TABLE, pmList);
    MEDIA_ERR_LOG("fileId: %{public}d insert photomap, ret: %{public}d", fileId, ret);
    std::string deleteSql = "Delete FROM PhotoMap WHERE map_asset = " + to_string(fileId) +
                            " AND dirty != 0 AND map_album NOT IN ( " + ss.str() + " )";
    ret = rdbStore->ExecuteSql(deleteSql, bindArgs);
    MEDIA_ERR_LOG("fileId: %{public}d delete photomap, ret: %{public}d, deleteSql: %{public}s",
        fileId, ret, deleteSql.c_str());
    return E_OK;
}

int32_t CloudMediaPhotosDao::UpdatePhotosSynced(
    const NativeRdb::AbsRdbPredicates &predicates, const int32_t &dirtyValue)
{
    MEDIA_INFO_LOG("UpdatePhotosSynced v2 enter");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::ValuesBucket values;
    // 根据调用方场景，重置脏状态；非恒定为 DirtyType::TYPE_SYNCED
    values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyValue);
    int32_t changedRows = DEFAULT_VALUE;
    return rdbStore->Update(changedRows, values, predicates);
}

int32_t CloudMediaPhotosDao::UpdatePhotosSynced(const std::string &table, const std::string &whereClause,
    const std::vector<std::string> &args, const int32_t &dirtyValue)
{
    MEDIA_INFO_LOG("UpdatePhotosSynced v1 enter");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyValue);
    int32_t changedRows = DEFAULT_VALUE;
    return rdbStore->Update(changedRows, table, values, whereClause, args);
}

int32_t CloudMediaPhotosDao::GetFieldIntValue(
    const NativeRdb::ValuesBucket &values, const std::string &fieldName, const int32_t &defaultFieldValue)
{
    NativeRdb::ValueObject val;
    int32_t fieldValue = defaultFieldValue;
    bool isSuccess = values.GetObject(fieldName, val) && val.GetInt(fieldValue) == E_OK;
    CHECK_AND_RETURN_RET_LOG(
        isSuccess, defaultFieldValue, "Failed to get field value, fieldName: %{public}s.", fieldName.c_str());
    return fieldValue;
}

int CloudMediaPhotosDao::UpdateProxy(int &changedRows, const NativeRdb::ValuesBucket &row,
    const NativeRdb::AbsRdbPredicates &predicates, const std::string &cloudId,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "UpdateProxy-update Failed to get rdbStore.");
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);

    std::function<int(void)> func = [&]() -> int {
        int32_t retInner = photoRefresh->Update(changedRows, row, predicates);
        CHECK_AND_RETURN_RET_LOG(
            retInner == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
            retInner,
            "Failed to UpdateProxy func, ret=%{public}d",
            retInner);
        CHECK_AND_RETURN_RET_INFO_LOG(changedRows > 0, E_OK, "No photo need to update dirty.");
        // 获取 NativeRdb::ValuesBucket 的 dirty 值，重置该 dirty；需要根据入参，不能直接修改为 0/同步状态
        NativeRdb::AbsRdbPredicates dirtyPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
        dirtyPredicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
        int32_t dirtyValue =
            this->GetFieldIntValue(row, PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        NativeRdb::ValuesBucket values;
        // 根据调用方场景，重置脏状态；非恒定为 DirtyType::TYPE_SYNCED
        values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyValue);
        int32_t dirtyChangedRows = DEFAULT_VALUE;
        return photoRefresh->Update(dirtyChangedRows, values, dirtyPredicates);
    };
    int ret = trans->RetryTrans(func);
    CHECK_AND_RETURN_RET_LOG(
        ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret, "Failed to UpdateProxy, ret=%{public}d", ret);
    return E_OK;
}

int CloudMediaPhotosDao::UpdateProxy(int &changedRows, const std::string &table, const NativeRdb::ValuesBucket &row,
    const std::string &whereClause, const std::vector<std::string> &args,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "UpdateProxy-merge Failed to get rdbStore.");
 
    std::function<int(void)> func = [&]() -> int {
        int32_t retInner = photoRefresh->Update(changedRows, table, row, whereClause, args);
        CHECK_AND_RETURN_RET_LOG(
            retInner == AccurateRefresh::ACCURATE_REFRESH_RET_OK,
            retInner,
            "Failed to UpdateProxy func, ret=%{public}d",
            retInner);
        CHECK_AND_RETURN_RET_INFO_LOG(changedRows > 0, E_OK, "No photo need to update dirty.");
        // 获取 NativeRdb::ValuesBucket 的 dirty 值，重置该 dirty；需要根据入参，不能直接修改为 0/同步状态
        int32_t dirtyValue =
                this->GetFieldIntValue(row, PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
        NativeRdb::ValuesBucket values;
        values.PutInt(PhotoColumn::PHOTO_DIRTY, dirtyValue);
        int32_t dirtyChangedRows = DEFAULT_VALUE;
        return photoRefresh->Update(dirtyChangedRows, table, values, whereClause, args);
    };
    int ret = E_ERR;
    if (photoRefresh->GetTransaction() != nullptr) {
        ret = photoRefresh->GetTransaction()->RetryTrans(func);
    } else {
        std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
        CHECK_AND_RETURN_RET_LOG(trans != nullptr, E_RDB_STORE_NULL, "UpdateProxy-merge Failed to get trans.");
        ret = trans->RetryTrans(func);
    }
    CHECK_AND_RETURN_RET_LOG(
        ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret, "Failed to UpdateProxy, ret=%{public}d", ret);
    return E_OK;
}

void CloudMediaPhotosDao::GetUpdateRecordValues(const CloudMediaPullDataDto &pullData, NativeRdb::ValuesBucket &values)
{
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, pullData.basicCloudVersion);
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));

    CloudSyncConvert().RecordToValueBucket(pullData, values);
    return;
}

void CloudMediaPhotosDao::GetUpdateRecordCondition(const std::string &cloudId,
    NativeRdb::AbsRdbPredicates &predicates)
{
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
    predicates.BeginWrap();
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    predicates.Or()->EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SDIRTY));
    predicates.Or()->EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_RETRY));
    predicates.EndWrap();
}

void CloudMediaPhotosDao::GetUpdateRecordConditionForRecycleUpdate(const std::string &cloudId,
    NativeRdb::AbsRdbPredicates &predicates)
{
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);
}

void UpdateTransCode(const CloudMediaPullDataDto &pullData, NativeRdb::ValuesBucket &values, bool mtimeChanged)
{
    if (MediaFileUtils::GetExtensionFromPath(pullData.localDisplayName) != "heif" &&
        MediaFileUtils::GetExtensionFromPath(pullData.localDisplayName) != "heic") {
        MEDIA_INFO_LOG("cloudId: %{public}s Display name is not heif", pullData.cloudId.c_str());
        return;
    }

    if (mtimeChanged) {
        CloudMediaSyncUtils::RemoveTransCodePath(pullData.localPath);
        values.PutLong(PhotoColumn::PHOTO_TRANSCODE_TIME, 0);
        values.PutLong(PhotoColumn::PHOTO_TRANS_CODE_FILE_SIZE, 0);
        values.PutLong(PhotoColumn::PHOTO_EXIST_COMPATIBLE_DUPLICATE, 0);
    }
    return;
}

void CloudMediaPhotosDao::UpdateRecordToDatabasePrepare(const CloudMediaPullDataDto &pullData, bool isLocal,
    bool mtimeChanged, NativeRdb::ValuesBucket &values)
{
    if (mtimeChanged) {
        HandleExifRotateDownloadAsset(pullData, values);
    } else {
        HandleExifRotateDownloadMetaData(pullData, values);
    }
    if (isLocal && mtimeChanged) {
        values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(CloudFilePosition::POSITION_CLOUD));
        values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, CloudMediaContext::GetInstance().GetCloudType());
        values.PutInt(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, static_cast<int32_t>(FileSourceType::MEDIA));
    }
    this->FillThumbStatus(values, mtimeChanged);
}

int32_t CloudMediaPhotosDao::UpdateRecordToDatabase(const CloudMediaPullDataDto &pullData, bool isLocal,
    bool mtimeChanged, std::set<std::string> &refreshAlbums, std::vector<int32_t> &stats,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    NativeRdb::ValuesBucket values;
    this->GetUpdateRecordValues(pullData, values);
    this->UpdateRecordToDatabasePrepare(pullData, isLocal, mtimeChanged, values);
    int32_t albumId = 0;
    std::set<int32_t> albumIds;
    UpdateFixDB(pullData, values, albumId, albumIds, refreshAlbums);
    if (albumId != 0) {
        values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    }
    NativeRdb::ValueObject val;
    int32_t subtype = 0;
    if (isLocal && !mtimeChanged && values.GetObject(PhotoColumn::PHOTO_SUBTYPE, val) && val.GetInt(subtype) == E_OK &&
        subtype == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        values.Delete(PhotoColumn::PHOTO_SUBTYPE);
    }
    bool isThumbFailDirty = pullData.localDirty == static_cast<int32_t>(DirtyType::TYPE_TDIRTY);
    if (!(isThumbFailDirty && !mtimeChanged)) {
        MEDIA_DEBUG_LOG("upate TYPE_SYNC when not TDIRTY");
        values.Delete(PhotoColumn::PHOTO_DIRTY);
        values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }
    UpdateTransCode(pullData, values, mtimeChanged);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    if (pullData.GetIsRecycleUpdated()) {
        this->GetUpdateRecordConditionForRecycleUpdate(pullData.cloudId, predicates);
    } else {
        this->GetUpdateRecordCondition(pullData.cloudId, predicates);
    }
    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = this->UpdateProxy(changedRows, values, predicates, pullData.cloudId, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Failed to UpdateRecordToDatabase, ret: %{public}d", ret);
    NotifyDateTakenChanged(pullData);
    MEDIA_INFO_LOG("changedRows %{public}d, IsRecycleUpdated %{public}d", changedRows, pullData.GetIsRecycleUpdated());
    if (changedRows > 0) {
        if (mtimeChanged) {  // 文件修改与元数据同时修改，算文件修改
            stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT]++;
        } else {
            stats[StatsIndex::META_MODIFY_RECORDS_COUNT]++;
        }
    }
    UpdateAssetInPhotoMap(pullData.attributesFileId, albumIds);
    IsCoverContentChange(changedRows, mtimeChanged, pullData.localFileId);
    return ret;
}

int32_t CloudMediaPhotosDao::ConflictDataMerge(const CloudMediaPullDataDto &pullData, const std::string fullPath,
    const bool cloudStd, std::set<int32_t> albumIds, std::set<std::string> &refreshAlbums,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int updateRows;
    string filePath = fullPath;
    NativeRdb::ValuesBucket values;
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, pullData.cloudId);
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPosition::POSITION_BOTH));
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, CloudMediaContext::GetInstance().GetCloudType());
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(Clean::NOT_NEED_CLEAN));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, pullData.basicCloudVersion);
    bool needFix = false;
    int32_t albumId = 0;
    if (cloudStd) {
        CloudSyncConvert().RecordToValueBucket(pullData, values);
        UpdateFixDB(pullData, values, albumId, albumIds, refreshAlbums);
        values.Delete(PhotoColumn::PHOTO_SUBTYPE);
        if (albumId != 0) {
            values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
        }
        needFix = IsNeededFix(pullData);
        if (needFix) {
            values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SDIRTY));
        } else {
            values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED));
        }
    } else {
        values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SDIRTY));
    }
    string whereClause = PhotoColumn::MEDIA_FILE_PATH + " = ? AND position = 1";
    int32_t ret = this->UpdateProxy(
        updateRows, PhotoColumn::PHOTOS_TABLE, values, whereClause, {filePath}, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_RDB, "ConflictDataMerge failed, ret: %{public}d, filePath: %{public}s",
        ret, filePath.c_str());
    MEDIA_INFO_LOG("merge update done."
        "ret: %{public}d, updateRows: %{public}d,"
        "filePath: %{public}s, cloudStd: %{public}d, needFix: %{public}d, pullData: %{public}s",
        ret, updateRows, filePath.c_str(), cloudStd, needFix, pullData.ToString().c_str());
    return E_OK;
}

int32_t CloudMediaPhotosDao::GetInsertParams(const CloudMediaPullDataDto &pullData,
    std::map<std::string, int> &recordAnalysisAlbumMaps, std::map<std::string, std::set<int>> &recordAlbumMaps,
    std::set<std::string> &refreshAlbums, std::vector<NativeRdb::ValuesBucket> &insertFiles)
{
    NativeRdb::ValuesBucket values;
    auto ret = CloudSyncConvert().RecordToValueBucket(pullData, values);
    HandleExifRotateDownloadAsset(pullData, values);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("record to valuebucket failed, ret=%{public}d", ret);
        return ret;
    }
    int32_t albumId = 0;
    set<int32_t> albumIds;
    UpdateFixDB(pullData, values, albumId, albumIds, refreshAlbums);
    recordAlbumMaps[pullData.cloudId] = albumIds;
    values.PutString(PhotoColumn::MEDIA_FILE_PATH, pullData.localPath);
    values.PutString(PhotoColumn::PHOTO_MEDIA_SUFFIX, ScannerUtils::GetFileExtension(pullData.basicDisplayName));
    values.PutInt(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED));
    values.PutInt(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(Clean::NOT_NEED_CLEAN));
    values.PutInt(PhotoColumn::PHOTO_POSITION, PhotoPosition::POSITION_CLOUD);
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, CloudMediaContext::GetInstance().GetCloudType());
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, pullData.basicCloudVersion);
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::TO_DOWNLOAD));
    values.PutString(PhotoColumn::PHOTO_CLOUD_ID, pullData.cloudId);
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    HandleShootingMode(pullData.cloudId, values, recordAnalysisAlbumMaps);
    insertFiles.push_back(values);
    return E_OK;
}

int32_t CloudMediaPhotosDao::GetSourceAlbumForMerge(const CloudMediaPullDataDto &pullData,
    std::vector<std::string> &albumCloudIds, SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap)
{
    /* if (GetSourceAlbumList(record, list) != E_OK) 变成在序列化反序列化中获取 pullData.attributesSrcAlbumIds */
    if (pullData.attributesSrcAlbumIds.size() <= 0) {
        MEDIA_INFO_LOG("attributesSrcAlbumIds size 0");
        return E_INVAL_ARG;
    }
    bool isHidden = false;
    for (auto attributesSrcAlbumId : pullData.attributesSrcAlbumIds) {
        if (attributesSrcAlbumId == HIDDEN_ALBUM_CLOUD_ID) {
            isHidden = true;
        }
        albumCloudIds.emplace_back(attributesSrcAlbumId);
    }
    if (albumCloudIds.size() <= 0) {
        MEDIA_INFO_LOG("albumCloudIds size 0");
        return E_INVAL_ARG;
    }
    if (isHidden) {
        MEDIA_INFO_LOG("FixData:ishidden");
        std::string lPath = CloudMediaSyncUtils::GetLpath(pullData);
        std::transform(lPath.begin(), lPath.end(), lPath.begin(), ::tolower);
        MEDIA_INFO_LOG("FixData:GetLpath %{public}s", lPath.c_str());
        std::pair<int32_t, std::string> val;
        if (lpathToIdMap.Find(lPath, val)) {
            MEDIA_INFO_LOG("FixData: findlpath %{public}s", (val.second).c_str());
            albumCloudIds.push_back(val.second);
        }
    }
    return E_OK;
}

bool CloudMediaPhotosDao::IsHiddenAsset(const CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_INFO_LOG(
        pullData.attributesSrcAlbumIds.size() > 0, false, "attributesSrcAlbumIds size 0, not hidden");
    for (auto cloudId : pullData.attributesSrcAlbumIds) {
        CHECK_AND_RETURN_RET_INFO_LOG(
            cloudId != HIDDEN_ALBUM_CLOUD_ID, true, "isHidden: true, cloudId %{public}s", cloudId.c_str());
    }
    return false;
}

// 无归属相册的资产下行时，如果是回收站或者隐藏相册，则归属虚拟相册回收站(-3)或者隐藏相册(-4)
int32_t CloudMediaPhotosDao::FixEmptyAlbumId(const CloudMediaPullDataDto &data, int32_t &albumId)
{
    CHECK_AND_RETURN_RET(albumId == 0, E_OK);
    if (data.basicRecycledTime != 0) {
        albumId = ALBUM_ID_RECYCLE;
    } else if (this->IsHiddenAsset(data)) {
        albumId = ALBUM_ID_HIDDEN;
    } else if (!data.propertiesSourcePath.empty()) {
        albumId = ALBUM_ID_NEED_REBUILD;
        int32_t buildAlbumId = PhotoOwnerAlbumIdOperation().BuildAlbumBySourcePath(data.propertiesSourcePath);
        CHECK_AND_EXECUTE(buildAlbumId <= 0, albumId = buildAlbumId);
    } else {
        this->FixAlbumIdToBeOtherAlbumId(albumId);
    }
    MEDIA_WARN_LOG("FixEmptyAlbumId, fixed-albumId: %{public}d, data: %{public}s.", albumId, data.ToString().c_str());
    return E_OK;
}

int32_t CloudMediaPhotosDao::FixAlbumIdToBeOtherAlbumId(int32_t &albumId)
{
    CHECK_AND_RETURN_RET(albumId == 0, E_OK);
    std::string lPath = "/Pictures/其它";
    std::transform(lPath.begin(), lPath.end(), lPath.begin(), ::tolower);
    std::pair<int32_t, std::string> val;
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap = GetAlbumLPathToIdMap();
    if (lpathToIdMap.Find(lPath, val)) {
        albumId = val.first;
    }
    MEDIA_INFO_LOG("FixAlbumId Lpath %{public}s, albumId %{public}d", lPath.c_str(), albumId);
    return E_OK;
}

void CloudMediaPhotosDao::GetSourceAlbumFromPath(const CloudMediaPullDataDto &pullData, int32_t &albumId,
    std::set<int32_t> &cloudMapIds, SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap)
{
    std::string lPath = CloudMediaSyncUtils::GetLpath(pullData);
    std::transform(lPath.begin(), lPath.end(), lPath.begin(), ::tolower);
    std::pair<int32_t, std::string> val;
    if (lpathToIdMap.Find(lPath, val)) {
        if (albumId != 0 && albumId != val.first) {
            cloudMapIds.insert(albumId);
        }
        albumId = val.first;
    }
    MEDIA_INFO_LOG("GetSourceAlbumFromPath Lpath %{public}s, albumId %{public}d", lPath.c_str(), albumId);
}

std::shared_ptr<NativeRdb::ResultSet> CloudMediaPhotosDao::GetAllSysAlbumsQuery(
    NativeRdb::AbsRdbPredicates &predicates, const std::vector<std::string> &columns)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_INFO_LOG(rdbStore != nullptr, nullptr, "GetAllSysAlbumsQuery Failed to get rdbStore.");

    /* build all-table vector */
    std::string tableName = predicates.GetTableName();
    std::vector<std::string> joinTables = predicates.GetJoinTableNames();
    joinTables.push_back(tableName);
    /* add filters */
    std::string filters;
    for (auto t : joinTables) {
        std::string filter = PhotoAlbumColumns::TABLE + "." + PhotoAlbumColumns::ALBUM_DIRTY +
                             " != " + to_string(static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
        if (filter.empty()) {
            continue;
        }
        if (filters.empty()) {
            filters += filter;
        } else {
            filters += " AND " + filter;
        }
    }
    if (filters.empty()) {
        return nullptr;
    }

    /* rebuild */
    std::string queryCondition = predicates.GetWhereClause();
    queryCondition = queryCondition.empty() ? filters : filters + " AND " + queryCondition;
    predicates.SetWhereClause(queryCondition);
    return rdbStore->Query(predicates, columns);
}

std::shared_ptr<NativeRdb::ResultSet> CloudMediaPhotosDao::GetAllSysAlbums(
    const std::vector<std::string> &subtypes, const std::vector<std::string> &columns)
{
    NativeRdb::AbsRdbPredicates predicates(PhotoAlbumColumns::TABLE);
    if (subtypes.empty()) {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, ALL_SYSTEM_PHOTO_ALBUM);
    } else {
        predicates.In(PhotoAlbumColumns::ALBUM_SUBTYPE, subtypes);
    }

    return GetAllSysAlbumsQuery(predicates, columns);
}

int32_t CloudMediaPhotosDao::GetSourceAlbum(const CloudMediaPullDataDto &pullData, int32_t &albumId,
    std::set<int32_t> &cloudMapIds, bool &isHidden, SafeMap<std::string, int32_t> &cloudToLocalMap)
{
    bool firstAlbum = true;
    bool physical = false;
    int32_t tmpAlbum = 0;
    int32_t mediaType =
        pullData.basicFileType == FILE_TYPE_VIDEO ? MediaType::MEDIA_TYPE_VIDEO : MediaType::MEDIA_TYPE_IMAGE;

    if (pullData.attributesSrcAlbumIds.size() <= 0) {
        MEDIA_ERR_LOG("GetSourceAlbum MDKRecord albumIds is empty");
        return E_INVAL_ARG;
    }
    for (auto attributesSrcAlbumId : pullData.attributesSrcAlbumIds) {
        std::string cloudId = attributesSrcAlbumId;
        if (cloudId == HIDDEN_ALBUM_CLOUD_ID) {
            isHidden = true;
            continue;
        }
        if ((cloudId == SCREENSHOT_ALBUM_CLOUD_ID) && (mediaType == MediaType::MEDIA_TYPE_VIDEO)) {
            cloudId = SCREENSHOT_ALBUM_CLOUD_ID + "V";
        }
        if (!physical && (cloudId.substr(0, PYHSICAL_ALBUM_CLOUD_ID_PREFIX.size()) == PYHSICAL_ALBUM_CLOUD_ID_PREFIX)) {
            if (cloudToLocalMap.Find(cloudId, albumId)) {
                physical = true;
            }
            continue;
        }
        if (firstAlbum) {
            if (cloudToLocalMap.Find(cloudId, tmpAlbum)) {
                firstAlbum = false;
            }
        } else {
            int32_t localId = 0;
            if (cloudToLocalMap.Find(cloudId, localId)) {
                cloudMapIds.insert(localId);
            }
        }
    }
    if (albumId == 0) {
        albumId = tmpAlbum;
    } else if (tmpAlbum != 0) {
        cloudMapIds.insert(tmpAlbum);
    }
    MEDIA_INFO_LOG("GetSourceAlbum, firstAlbum: %{public}d, albumId: %{public}d, "
                   "physical: %{public}d, tmpAlbum: %{public}d, mediaType: %{public}d.",
        firstAlbum,
        albumId,
        physical,
        tmpAlbum,
        mediaType);
    return E_OK;
}

int32_t CloudMediaPhotosDao::UpdateFixDB(const CloudMediaPullDataDto &data, NativeRdb::ValuesBucket &values,
    int32_t &albumId, std::set<int32_t> &albumIds, std::set<std::string> &refreshAlbums)
{
    SafeMap<std::string, int32_t> albumCloudToLocalMap = GetAlbumCloudToLocalMap();
    MEDIA_ERR_LOG("UpdateFixDB: %{public}s, %{public}d", data.cloudId.c_str(), albumCloudToLocalMap.Size());
    bool isHide = false;
    int32_t ret = E_OK;
    ret = GetSourceAlbum(data, albumId, albumIds, isHide, albumCloudToLocalMap);
    CHECK_AND_PRINT_LOG(
        ret == E_OK, "UpdateFixDB cloudId: %{public}s cannot get sourceAlbum", data.cloudId.c_str());
    MEDIA_INFO_LOG("UpdateFixDB cloudId: %{public}s, isHide: %{public}d, album: %{public}d",
        data.cloudId.c_str(),
        isHide,
        albumId);

    NativeRdb::ValueObject val;
    int32_t hidden = 0;
    bool needFix = IsNeededFix(data);
    if (needFix) {
        values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SDIRTY));
    }
    SafeMap<std::string, std::pair<int32_t, std::string>> lpathToIdMap = GetAlbumLPathToIdMap();
    if (values.GetObject(PhotoColumn::MEDIA_HIDDEN, val) && val.GetInt(hidden) == E_OK && hidden == 1) {
        values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SDIRTY));
    } else if (isHide) {
        int64_t hiddenTime = 0;
        if (values.HasColumn(PhotoColumn::PHOTO_HIDDEN_TIME)) {
            NativeRdb::ValueObject hiddenValue;
            values.GetObject(PhotoColumn::PHOTO_HIDDEN_TIME, hiddenValue);
            hiddenValue.GetLong(hiddenTime);
        }
        if (hiddenTime == 0) {
            MEDIA_INFO_LOG("FixData: fix hidenTime by setting createTime");
            values.Delete(PhotoColumn::PHOTO_HIDDEN_TIME);
            values.PutLong(PhotoColumn::PHOTO_HIDDEN_TIME, static_cast<int64_t>(data.basicCreatedTime));
        }
        values.Delete(PhotoColumn::MEDIA_HIDDEN);
        values.PutInt(PhotoColumn::MEDIA_HIDDEN, 1);
        GetSourceAlbumFromPath(data, albumId, albumIds, lpathToIdMap);
    }
    if (albumId == 0 && !isHide) {
        GetSourceAlbumFromPath(data, albumId, albumIds, lpathToIdMap);
    }
    this->FixEmptyAlbumId(data, albumId);
    refreshAlbums.emplace(std::to_string(albumId));
    MEDIA_INFO_LOG(
        "UpdateFixDB needFix %{public}d  %{public}d  %{public}d %{public}d", needFix, isHide, hidden, albumId);
    return E_OK;
}

bool CloudMediaPhotosDao::IsNeededFix(const CloudMediaPullDataDto &data)
{
    int32_t fixVersion = data.attributesFixVersion;
    if (fixVersion < 0) {
        return true;
    }
    return false;
}

void CloudMediaPhotosDao::HandleExifRotateDownloadAsset(const CloudMediaPullDataDto &data,
    NativeRdb::ValuesBucket &valuebucket)
{
    int32_t exifRotate = data.exifRotate;
    CHECK_AND_RETURN_WARN_LOG(exifRotate >= 0,
        "Download with asset, exifRotate: %{public}d is invalid.", exifRotate);
    int32_t rotate = data.propertiesRotate;
    int32_t mediaType = data.attributesMediaType;
    if (mediaType == -1) {
        mediaType = data.basicFileType == FILE_TYPE_VIDEO ? static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)
                                                          : static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    }

    if (mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)) {
        if (rotate != ROTATE_ANGLE_0) {
            ExifRotateUtils::ConvertOrientationToExifRotate(rotate, exifRotate);
        } else if (exifRotate == static_cast<int32_t>(ExifRotateType::RIGHT_TOP) ||
            exifRotate == static_cast<int32_t>(ExifRotateType::BOTTOM_RIGHT) ||
            exifRotate == static_cast<int32_t>(ExifRotateType::LEFT_BOTTOM)) {
            exifRotate = 0;
        }
    }
    valuebucket.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, exifRotate);
}

void CloudMediaPhotosDao::HandleExifRotateDownloadMetaData(const CloudMediaPullDataDto &data,
    NativeRdb::ValuesBucket &valuebucket)
{
    CHECK_AND_RETURN(data.localExifRotate != data.exifRotate);
    int32_t mediaType = data.attributesMediaType;
    if (mediaType == -1) {
        mediaType = data.basicFileType == FILE_TYPE_VIDEO ? static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)
                                                          : static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    }

    CHECK_AND_RETURN(CloudMediaSyncUtils::CanUpdateExifRotateOnly(
        mediaType, data.localExifRotate, data.exifRotate));
    valuebucket.PutInt(PhotoColumn::PHOTO_EXIF_ROTATE, data.exifRotate);
}

void CloudMediaPhotosDao::HandleShootingMode(const std::string &cloudId, const NativeRdb::ValuesBucket &valuebucket,
    std::map<std::string, int> &recordAnalysisAlbumMaps)
{
    string shootingMode = "";
    NativeRdb::ValueObject valueObject;
    if (valuebucket.GetObject(PhotoColumn::PHOTO_SHOOTING_MODE, valueObject)) {
        valueObject.GetString(shootingMode);
    }
    bool isNum = std::all_of(shootingMode.begin(), shootingMode.end(), ::isdigit);
    if (!isNum) {
        MEDIA_ERR_LOG("shootingMode %{public}s is invailed string", shootingMode.c_str());
        return;
    }
    if (!shootingMode.empty()) {
        bool isNum = std::all_of(shootingMode.begin(), shootingMode.end(), ::isdigit);
        if (!isNum) {
            MEDIA_ERR_LOG("shootingMode %{public}s convert to int failed", shootingMode.c_str());
            return;
        }
        recordAnalysisAlbumMaps[cloudId] = CloudMediaDaoUtils::ToInt32(shootingMode);
    }
}

std::shared_ptr<NativeRdb::ResultSet> CloudMediaPhotosDao::BatchQueryLocal(
    const std::vector<CloudMediaPullDataDto> &datas, const std::vector<std::string> &columns, int32_t &rowCount,
    CleanType cleanType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, nullptr, "BatchQueryLocal Failed to get rdbStore.");

    std::vector<std::string> displayNames;
    for (auto &data : datas) {
        std::string displayName = data.basicFileName;
        MEDIA_DEBUG_LOG("BatchQueryLocal displayName: %{public}s.", displayName.c_str());
        if (displayName.empty()) {
            continue;
        }
        displayNames.emplace_back(displayName);
    }
    CHECK_AND_RETURN_RET_LOG(!displayNames.empty(), nullptr, "BatchQueryLocal record not include displayname.");
    MEDIA_INFO_LOG("BatchQueryLocal num: %{public}d.", static_cast<int32_t>(displayNames.size()));

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(CloudFilePosition::POSITION_LOCAL));
    predicates.In(PhotoColumn::MEDIA_NAME, displayNames);
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(cleanType));

    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, nullptr, "BatchQueryLocal Failed to query.");

    int32_t ret = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG((ret == E_OK && rowCount >= 0),
        nullptr,
        "BatchQueryLocal, ret: %{public}d, rowCount: %{public}d.",
        ret,
        rowCount);
    return resultSet;
}

int32_t CloudMediaPhotosDao::GetLocalKeyData(KeyData &localKeyData, std::shared_ptr<NativeRdb::ResultSet> &resultSet)
{
    localKeyData.displayName = Media::GetStringVal(PhotoColumn::MEDIA_NAME, resultSet);
    localKeyData.filePath = Media::GetStringVal(PhotoColumn::MEDIA_FILE_PATH, resultSet);
    localKeyData.isize = Media::GetInt64Val(PhotoColumn::MEDIA_SIZE, resultSet);
    localKeyData.createTime = Media::GetInt64Val(PhotoColumn::MEDIA_DATE_TAKEN, resultSet);
    localKeyData.modifyTime = Media::GetInt64Val(PhotoColumn::PHOTO_META_DATE_MODIFIED, resultSet);
    localKeyData.exifRotateValue = Media::GetInt32Val(PhotoColumn::PHOTO_ORIENTATION, resultSet);
    localKeyData.dateTrashed = Media::GetInt64Val(MediaColumn::MEDIA_DATE_TRASHED, resultSet);
    int32_t albumid = Media::GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, resultSet);
    pair<string, string> val;
    SafeMap<int32_t, std::pair<std::string, std::string>> localToCloudMap = GetAlbumLocalToCloudMap();
    if (!localToCloudMap.Find(albumid, val)) {
        localKeyData.sourceAlbum = hiddenAlbumId_ == albumid ? HIDDEN_ALBUM_CLOUD_ID : "";
        MEDIA_ERR_LOG("FixData: Get sourceAlbum %{public}s", localKeyData.sourceAlbum.c_str());
        std::string sourcePath = Media::GetStringVal(PhotoColumn::PHOTO_SOURCE_PATH, resultSet);
        localKeyData.lPath = CloudMediaSyncUtils::GetLpathFromSourcePath(sourcePath);
        MEDIA_ERR_LOG(
            "sourcePath %{public}s, localKeyData.lPath %{public}s", sourcePath.c_str(), localKeyData.lPath.c_str());
    } else {
        localKeyData.sourceAlbum = val.first;
        localKeyData.lPath = val.second;
    }
    return E_OK;
}

bool CloudMediaPhotosDao::JudgeConflict(
    const CloudMediaPullDataDto &pullData, const KeyData &localKeyData, const KeyData &cloudKeyData)
{
    if (localKeyData.displayName != cloudKeyData.displayName) {
        return false;
    }
    MEDIA_INFO_LOG("Judge duplicate files %{public}s", localKeyData.displayName.c_str());
    if ((localKeyData.isize != cloudKeyData.isize) ||
        ((cloudKeyData.mediaType == MediaType::MEDIA_TYPE_IMAGE) &&
            (localKeyData.exifRotateValue != cloudKeyData.exifRotateValue))) {
        MEDIA_INFO_LOG("local isize %{public}ld, cloud isize %{public}ld, local exifRotateValue %{public}d,\
            cloud exifRotateValue %{public}d",
            (long)localKeyData.isize,
            (long)cloudKeyData.isize,
            localKeyData.exifRotateValue,
            cloudKeyData.exifRotateValue);
        return false;
    }
    if (localKeyData.sourceAlbum.empty()) {
        if (localKeyData.lPath == cloudKeyData.lPath) {
            return true;
        } else {
            MEDIA_INFO_LOG("JudgeConflict sourcePath not equal local lPath: %{public}s, cloud lPath: %{public}s",
                localKeyData.lPath.c_str(),
                cloudKeyData.lPath.c_str());
            return false;
        }
    } else {
        SafeMap<std::string, std::pair<int32_t, std::string>> albumLPathToIdMap = GetAlbumLPathToIdMap();
        std::vector<std::string> albumCloudIds;
        if (GetSourceAlbumForMerge(pullData, albumCloudIds, albumLPathToIdMap) != E_OK) {
            MEDIA_INFO_LOG("get source album for merge error");
            return false;
        }
        for (const auto &albumCloudId : albumCloudIds) {
            if (albumCloudId == localKeyData.sourceAlbum) {
                return true;
            }
        }
        if (localKeyData.lPath == cloudKeyData.lPath) {
            return true;
        }
        MEDIA_INFO_LOG("local source album %{public}s", localKeyData.sourceAlbum.c_str());
        for (const auto &albumCloudId : albumCloudIds) {
            MEDIA_INFO_LOG("album cloud ids %{public}s", albumCloudId.c_str());
        }
        return false;
    }
    return true;  // never execute
}

int32_t CloudMediaPhotosDao::GetRetryRecords(std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("GetRetryRecords enter");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "GetRetryRecords Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_RETRY));
    predicates.EqualTo(PhotoColumn::PHOTO_CLEAN_FLAG, static_cast<int32_t>(CleanType::TYPE_NOT_CLEAN));
    predicates.Limit(LIMIT_SIZE);

    const std::vector<std::string> columns = {PhotoColumn::PHOTO_CLOUD_ID};
    auto resultSet = rdbStore->Query(predicates, columns);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "GetRetryRecords Failed to query.");
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG((ret == 0 && rowCount >= 0), E_RDB, "GetRetryRecords Failed to Get Count.");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        std::string cloudId = GetStringVal(PhotoColumn::PHOTO_CLOUD_ID, resultSet);
        if (cloudId.empty()) {
            continue;
        }
        MEDIA_DEBUG_LOG("GetRetryRecords result cloudId:%{public}s", cloudId.c_str());
        cloudIds.push_back(cloudId);
    }
    resultSet->Close();
    return E_OK;
}

std::vector<PhotosPo> CloudMediaPhotosDao::GetCheckRecords(const std::vector<std::string> cloudIds)
{
    MEDIA_INFO_LOG("enter GetCheckRecords");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, {}, "GetCheckRecords Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.In(PhotoColumn::PHOTO_CLOUD_ID, cloudIds);
    auto resultSet = rdbStore->Query(predicates, PULL_QUERY_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, {}, "resultset is null");
    auto resultList = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
    for (auto &record : resultList) {
        MEDIA_DEBUG_LOG("GetCheckRecords Record: %{public}s", record.ToString().c_str());
    }
    return resultList;
}

/**
 * @brief 查询新增上行的数据
 * @param size 查询的条数；注：查询出的集合数据应等于预期查询的数量（非最后一页）
 * @return 查询的结果；E_DB_FAIL 数据库操作失败，E_OK 成功
 */
int32_t CloudMediaPhotosDao::GetCreatedRecords(int32_t size, std::vector<PhotosPo> &createdRecords)
{
    MEDIA_INFO_LOG("enter GetCreatedRecords");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "GetCreatedRecords Failed to get rdbStore.");
    /* build predicates */
    std::string fileIdNotIn = CloudMediaDaoUtils::ToStringWithComma(this->photoCreateFailSet_.ToVector());
    MEDIA_INFO_LOG("GetCreatedRecords fileIdNotIn:%{public}s", fileIdNotIn.c_str());
    bool notSupport = !PhotoAlbumUploadStatusOperation::IsSupportUploadStatus();
    std::vector<std::string> params = {
        fileIdNotIn,
        std::to_string(notSupport)
    };
    std::vector<NativeRdb::ValueObject> bindArgs = {size};
    std::string execSql = CloudMediaDaoUtils::FillParams(this->SQL_PHOTOS_GET_CREATE_RECORDS, params);
    /* query */
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, E_DB_FAIL, "GetCreatedRecords Failed to query RowCount.");
    MEDIA_INFO_LOG("GetCreatedRecords RowCount:%{public}d", rowCount);
    // Notify caller if no data is returned, it means all data has been processed.
    CHECK_AND_RETURN_RET_LOG(rowCount > 0, E_OK, "GetCreatedRecords Empty Result.");
    createdRecords = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
    for (auto &record : createdRecords) {
        MEDIA_DEBUG_LOG("Media_Trace: GetCreatedRecords Record: %{public}s", record.ToString().c_str());
    }
    return E_OK;
}

/**
 * @brief 查询元数据修改上行的数据
 * @param size 查询的条数；注：查询出的集合数据应等于预期查询的数量（非最后一页）
 * @return 查询的结果；E_DB_FAIL 数据库操作失败，E_OK 成功
 */
int32_t CloudMediaPhotosDao::GetMetaModifiedRecords(
    int32_t size, std::vector<PhotosPo> &cloudRecordPoList, int32_t dirtyType)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "GetMetaModifiedRecords Failed to get rdbStore.");
    /* build predicates */
    std::string cloudIdNotIn = CloudMediaDaoUtils::ToStringWithCommaAndQuote(this->photoModifyFailSet_.ToVector());
    MEDIA_INFO_LOG("GetMetaModifiedRecords cloudIdNotIn:%{public}s", cloudIdNotIn.c_str());
    std::vector<NativeRdb::ValueObject> bindArgs = {dirtyType, size};
    std::string execSql = CloudMediaDaoUtils::FillParams(this->SQL_PHOTOS_GET_META_MODIFIED_RECORDS, {cloudIdNotIn});
    /* query */
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    // Notify caller if no data is returned, it means all data has been processed.
    std::vector<PhotosPo> tempList;
    int32_t ret = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords(tempList);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "GetMetaModifiedRecords Failed to query, ret: %{public}d", ret);
    int32_t ownerAlbumId;
    int32_t fileId;
    bool isValid;
    for (auto &record : tempList) {
        ownerAlbumId = record.ownerAlbumId.value_or(-1);
        fileId = record.fileId.value_or(-1);
        isValid = fileId > 0;
        CHECK_AND_RETURN_RET_LOG(isValid, E_HAS_DB_ERROR, "GetMetaModifiedRecords Invalid Data.");
        ret = this->AddRemoveAlbumCloudId(rdbStore, fileId, ownerAlbumId, record);
        CHECK_AND_PRINT_LOG(ret == E_OK, "AddRemoveAlbumCloudId failed. ret: %{public}d", ret);
        MEDIA_DEBUG_LOG("Media_Trace: GetMetaModifiedRecords Record: %{public}s", record.ToString().c_str());
        cloudRecordPoList.emplace_back(move(record));
        MEDIA_INFO_LOG("GetMetaModifiedRecords dirtyType: %{public}d, size: %{public}d,"
                        "rowCount: %{public}zu, resultCount: %{public}zu",
            dirtyType,
            size,
            tempList.size(),
            cloudRecordPoList.size());
    }
    return E_OK;
}

/**
 * @brief 查询元数据修改上行的数据
 * @param size 查询的条数；注：查询出的集合数据应等于预期查询的数量（非最后一页）
 * @return 查询的结果；E_DB_FAIL 数据库操作失败，E_OK 成功
 */
int32_t CloudMediaPhotosDao::GetFileModifiedRecords(int32_t size, std::vector<PhotosPo> &cloudRecordPoList)
{
    MEDIA_INFO_LOG("enter GetFileModifiedRecords");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "GetFileModifiedRecords Failed to get rdbStore.");
    /* build predicates */
    std::string cloudIdNotIn = CloudMediaDaoUtils::ToStringWithCommaAndQuote(this->photoModifyFailSet_.ToVector());
    MEDIA_INFO_LOG("GetFileModifiedRecords cloudIdNotIn:%{public}s", cloudIdNotIn.c_str());
    std::vector<NativeRdb::ValueObject> bindArgs = {size};
    std::string execSql = CloudMediaDaoUtils::FillParams(this->SQL_PHOTOS_GET_FILE_MODIFIED_RECORDS, {cloudIdNotIn});
    /* query */
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, E_DB_FAIL, "GetFileModifiedRecords Failed to query.");
    MEDIA_INFO_LOG("GetFileModifiedRecords RowCount:%{public}d", rowCount);
    // Notify caller if no data is returned, it means all data has been processed.
    CHECK_AND_RETURN_RET_LOG(rowCount > 0, E_OK, "GetFileModifiedRecords Empty Result.");
    std::vector<PhotosPo> tempList = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
    MEDIA_INFO_LOG("GetFileModifiedRecords Counts: %{public}zu", tempList.size());
    int32_t ownerAlbumId;
    int32_t fileId;
    bool isValid;
    for (auto &record : tempList) {
        ownerAlbumId = record.ownerAlbumId.value_or(-1);
        fileId = record.fileId.value_or(-1);
        isValid = fileId > 0;
        CHECK_AND_RETURN_RET_LOG(isValid, E_HAS_DB_ERROR, "GetFileModifiedRecords Invalid Data.");
        ret = this->AddRemoveAlbumCloudId(rdbStore, fileId, ownerAlbumId, record);
        CHECK_AND_PRINT_LOG(ret == E_OK, "AddRemoveAlbumCloudId failed. ret: %{public}d", ret);
        MEDIA_DEBUG_LOG("Media_Trace: GetFileModifiedRecords Record: %{public}s", record.ToString().c_str());
        cloudRecordPoList.emplace_back(move(record));
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::GetDeletedRecordsAsset(int32_t size, std::vector<PhotosPo> &cloudRecordPoList)
{
    /* build predicates */
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "GetDeletedRecordsAsset Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates queryPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    queryPredicates.EqualTo(PhotoColumn::PHOTO_DIRTY, to_string(static_cast<int32_t>(Media::DirtyType::TYPE_DELETED)));
    queryPredicates.And()->NotEqualTo(PhotoColumn::PHOTO_CLOUD_ID, "");
    queryPredicates.And()->IsNotNull(PhotoColumn::PHOTO_CLOUD_ID);
    if (!photoModifyFailSet_.Empty()) {
        queryPredicates.And()->NotIn(PhotoColumn::PHOTO_CLOUD_ID, photoModifyFailSet_.ToVector());
    }
    queryPredicates.Limit(size);
    std::string cloudIdNotIn = CloudMediaDaoUtils::ToStringWithCommaAndQuote(this->photoModifyFailSet_.ToVector());
    MEDIA_INFO_LOG("GetDeletedRecordsAsset cloudIdNotIn:%{public}s", cloudIdNotIn.c_str());
    /* query */
    std::vector<string> queryColums = {};
    auto resultSet = rdbStore->Query(queryPredicates, queryColums);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "GetDeletedRecordsAsset Failed to query.");
    cloudRecordPoList = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
    for (auto &record : cloudRecordPoList) {
        MEDIA_DEBUG_LOG("GetDeletedRecords Record: %{public}s", record.ToString().c_str());
    }
    return E_OK;
}

/**
 * @brief 查询云复制上行的数据
 * @param size 查询的条数；注：查询出的集合数据应等于预期查询的数量（非最后一页）
 * @return 查询的结果；E_DB_FAIL 数据库操作失败，E_OK 成功
 */
int32_t CloudMediaPhotosDao::GetCopyRecords(int32_t size, std::vector<PhotosPo> &copyRecords)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "GetCopyRecords Failed to get rdbStore.");
    /* build predicates */
    std::string fileIdNotIn = CloudMediaDaoUtils::ToStringWithComma(this->photoCopyFailSet_.ToVector());
    MEDIA_DEBUG_LOG("GetCopyRecords fileIdNotIn:%{public}s", fileIdNotIn.c_str());
    std::vector<NativeRdb::ValueObject> bindArgs = {size};
    std::string execSql = CloudMediaDaoUtils::FillParams(this->SQL_PHOTOS_GET_COPY_RECORDS, {fileIdNotIn});
    /* query */
    auto resultSet = rdbStore->QuerySql(execSql, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    int32_t rowCount = 0;
    int32_t ret = resultSet->GetRowCount(rowCount);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, E_DB_FAIL, "GetCopyRecords Failed to query RowCount.");
    // Notify caller if no data is returned, it means all data has been processed.
    std::vector<PhotosPo> tempList = ResultSetReader<PhotosPoWriter, PhotosPo>(resultSet).ReadRecords();
    MEDIA_INFO_LOG("GetCopyRecords Counts: %{public}zu", tempList.size());
    int32_t ownerAlbumId;
    int32_t fileId;
    bool isValid;
    for (auto &record : tempList) {
        ownerAlbumId = record.ownerAlbumId.value_or(-1);
        fileId = record.fileId.value_or(-1);
        isValid = fileId > 0;
        CHECK_AND_RETURN_RET_LOG(isValid, E_HAS_DB_ERROR, "GetCopyRecords Invalid Data.");
        ret = this->AddRemoveAlbumCloudId(rdbStore, fileId, ownerAlbumId, record);
        CHECK_AND_PRINT_LOG(ret == E_OK, "AddRemoveAlbumCloudId failed. ret: %{public}d", ret);
        MEDIA_DEBUG_LOG("Media_Trace: GetCopyRecords Record: %{public}s", record.ToString().c_str());
        copyRecords.emplace_back(move(record));
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::AddRemoveAlbumCloudId(
    std::shared_ptr<MediaLibraryRdbStore> rdbStore, const int32_t fileId, const int32_t ownerAlbumId, PhotosPo &record)
{
    const std::string dirty = std::to_string(static_cast<int32_t>(DirtyType::TYPE_DELETED));
    const std::string queryAlbumCloudId =
        "SELECT " + PhotoAlbumColumns::ALBUM_CLOUD_ID + " FROM " + PhotoAlbumColumns::TABLE + " WHERE " +
        PhotoAlbumColumns::ALBUM_ID + " IN(SELECT " + PhotoMap::ALBUM_ID + " FROM " + PhotoMap::TABLE + " WHERE " +
        PhotoMap::ASSET_ID + " = " + std::to_string(fileId) + " AND " + PhotoMap::DIRTY + " = " + dirty + " AND " +
        PhotoMap::ALBUM_ID + " <> " + std::to_string(ownerAlbumId) + ")";

    /* query map */
    auto resultSet = rdbStore->QuerySql(queryAlbumCloudId);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_OK, "resultset is null");
    while (resultSet->GoToNextRow() == E_OK) {
        std::string removeAlbumCloudId = GetStringVal(PhotoAlbumColumns::ALBUM_CLOUD_ID, resultSet);
        record.removeAlbumCloudId.push_back(removeAlbumCloudId);
    }
    return E_OK;
}

static int32_t BuildInfoMap(const shared_ptr<NativeRdb::ResultSet> resultSet,
    std::unordered_map<std::string, LocalInfo> &infoMap, const std::string &type)
{
    MEDIA_INFO_LOG("BuildInfoMap enter");
    int32_t idIndex = -1;
    int32_t mtimeIndex = -1;
    int32_t metatimeIndex = -1;
    int32_t ret = resultSet->GetColumnIndex(type, idIndex);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BuildInfoMap Get type Index Error");
        return ret;
    }
    ret = resultSet->GetColumnIndex(PhotoColumn::MEDIA_DATE_MODIFIED, mtimeIndex);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BuildInfoMap Get date modified Index Error");
        return ret;
    }
    ret = resultSet->GetColumnIndex(PhotoColumn::PHOTO_META_DATE_MODIFIED, metatimeIndex);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("BuildInfoMap Get meta date modified Index Error");
        return ret;
    }

    /* iterate all rows compare mtime metatime */
    while (resultSet->GoToNextRow() == 0) {
        std::string idValue;
        int64_t mtime;
        int64_t metatime;
        if (resultSet->GetString(idIndex, idValue) == 0 && resultSet->GetLong(mtimeIndex, mtime) == 0 &&
            resultSet->GetLong(metatimeIndex, metatime) == 0) {
            infoMap.insert({idValue, {"", "", metatime, mtime, 0}});
        }
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::GetPhotoLocalInfo(
    const std::vector<PhotosDto> &records, std::unordered_map<std::string, LocalInfo> &infoMap, const std::string &type)
{
    MEDIA_INFO_LOG("GetPhotoLocalInfo enter");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "GetPhotoLocalInfo Failed to get rdbStore.");
    std::vector<std::string> path;
    for (auto &record : records) {
        if (type == PhotoColumn::PHOTO_CLOUD_ID) {
            path.push_back(record.cloudId);
        } else {
            path.push_back(std::to_string(record.fileId));
        }
    }

    NativeRdb::AbsRdbPredicates createPredicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    createPredicates.And()->In(type, path);
    auto resultSet = rdbStore->Query(createPredicates, ON_UPLOAD_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "resultset is null");

    return BuildInfoMap(move(resultSet), infoMap, type);
}

int32_t CloudMediaPhotosDao::UpdateLocalAlbumMap(const std::string &cloudId)
{
    MEDIA_INFO_LOG("enter UpdateLocalAlbumMap %{public}s", cloudId.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateLocalAlbumMap get store failed.");
    /* update deleted */
    std::string deleteSql = "DELETE FROM " + PhotoMap::TABLE + " WHERE " + PhotoMap::DIRTY + " = " +
                            std::to_string(static_cast<int32_t>(Media::DirtyType::TYPE_DELETED)) + " AND " +
                            PhotoMap::ASSET_ID + " IN (SELECT " + PhotoColumn::MEDIA_ID + " FROM " +
                            PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::PHOTO_CLOUD_ID + " = '" + cloudId +
                            "')";
    int32_t ret = rdbStore->ExecuteSql(deleteSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpdateLocalAlbumMap delete local album map err %{public}d, %{public}s", ret, cloudId.c_str());
        return ret;
    }

    /* update new */
    std::string newSql = "\
        UPDATE PhotoMap \
            SET dirty = 0 \
        WHERE map_asset IN ( \
            SELECT file_id \
            FROM Photos \
            WHERE cloud_id = ? AND \
                dirty != 6 \
            );";
    std::vector<NativeRdb::ValueObject> bindArgs = {cloudId};
    ret = rdbStore->ExecuteSql(newSql, bindArgs);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("UpdateLocalAlbumMap Update local album map err %{public}d, %{public}s", ret, cloudId.c_str());
        return ret;
    }
    MEDIA_INFO_LOG("UpdateLocalAlbumMap success");
    return ret;
}

bool CloudMediaPhotosDao::IsTimeChanged(const PhotosDto &record,
    const std::unordered_map<std::string, LocalInfo> &localMap, const std::string &fileId, const std::string &type)
{
    int64_t cloudtime = 0;
    int64_t localtime = 0;
    auto it = localMap.find(fileId);
    if (it == localMap.end()) {
        MEDIA_INFO_LOG("IsTimeChanged cloudId: %{public}s cause not int local map", record.cloudId.c_str());
        return true;
    }

    /* get mtime or metatime */
    if (type == PhotoColumn::MEDIA_DATE_MODIFIED) {
        localtime = it->second.fdirtyTime;
        cloudtime = record.editedTimeMs;
        if (cloudtime <= 0) {
            MEDIA_INFO_LOG("IsTimeChanged cloudId: %{public}s cause MEDIA_DATE_MODIFIED err", record.cloudId.c_str());
            return false;
        }
    } else {
        localtime = it->second.mdirtyTime;
        cloudtime = record.metaDateModified;
        if (cloudtime <= 0) {
            MEDIA_INFO_LOG(
                "IsTimeChanged cloudId: %{public}s cause PHOTO_META_DATE_MODIFIED err", record.cloudId.c_str());
            return false;
        }
    }
    MEDIA_INFO_LOG("IsTimeChanged cloudId: %{public}s lt: %{public}ld, ct: %{public}ld",
        record.cloudId.c_str(),
        (long)localtime,
        (long)cloudtime);
    if (localtime == cloudtime) {
        return false;
    }
    return true;
}

int32_t CloudMediaPhotosDao::GetSameNamePhotoCount(const PhotosDto &photo, bool isHide, int32_t count)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "get same name photo get store failed.");
    NativeRdb::AbsRdbPredicates sameFilePred = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    sameFilePred.EqualTo(PhotoColumn::MEDIA_NAME, photo.displayName)
        ->EqualTo(PhotoColumn::MEDIA_SIZE, to_string(photo.size))
        ->EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, to_string(photo.ownerAlbumId))
        ->EqualTo(PhotoColumn::MEDIA_HIDDEN, to_string(isHide));
    if (photo.mediaType == MediaType::MEDIA_TYPE_IMAGE) {
        sameFilePred.And()->EqualTo(PhotoColumn::PHOTO_ORIENTATION, photo.rotation);
    }
    auto results = rdbStore->Query(sameFilePred, {PhotoColumn::PHOTO_CLOUD_ID});
    return results->GetRowCount(count);
}

int32_t CloudMediaPhotosDao::HandleNotExistAlbumRecord(const PhotosDto &record)
{
    MEDIA_INFO_LOG("enter HandleNotExistAlbumRecord");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL,
        "OnCreateNotExistRecord Album Failed to get rdbStore.");
    std::string fileId = to_string(record.fileId);
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);
    auto results = rdbStore->Query(predicates, {PhotoColumn::PHOTO_OWNER_ALBUM_ID});
    if (results == nullptr) {
        MEDIA_ERR_LOG("HandleNotExistAlbumRecord Query results is null");
        return E_RDB;
    }
    auto ret = results->GoToNextRow();
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HandleNotExistAlbumRecord GoToNextRow, ret is %{public}d", ret);
        return E_DATA;
    }
    int32_t albumId = GetInt32Val(PhotoColumn::PHOTO_OWNER_ALBUM_ID, results);
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutInt(PhotoAlbumColumns::ALBUM_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_NEW));
    string whereClause = PhotoAlbumColumns::ALBUM_ID + " = ? AND " + PhotoAlbumColumns::ALBUM_DIRTY + " != ?";
    vector<std::string> whereArgs = {to_string(albumId), to_string(static_cast<int32_t>(Media::DirtyType::TYPE_NEW))};
    int32_t changeRows;
    ret = rdbStore->Update(changeRows, PhotoAlbumColumns::TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("HandleNotExistAlbumRecord ret:%{public}d, changedRows:%{public}d, cloudId:%{public}s",
        ret,
        changeRows,
        record.cloudId.c_str());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("album dirty change failed, ret is %{public}d", ret);
        return E_RDB;
    }
    if (changeRows == 0) {
        MEDIA_ERR_LOG("HandleNotExistAlbumRecord Album Failed to UpdateAlbumAfterUpload.");
    }
    return E_DATA;
}

int32_t CloudMediaPhotosDao::UpdatePhotoCreatedRecord(
    const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("enter UpdatePhotoCreatedRecord");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "UpdatePhotoCreatedRecord get store failed.");
    std::string fileId = to_string(record.fileId);
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(PhotoColumn::PHOTO_CLOUD_ID, record.cloudId);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, PhotoPosition::POSITION_BOTH);
    valuesBucket.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, CloudMediaContext::GetInstance().GetCloudType());
    valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.version);
    if (IsTimeChanged(record, localMap, fileId, PhotoColumn::MEDIA_DATE_MODIFIED)) {
        valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
    } else if (IsTimeChanged(record, localMap, fileId, PhotoColumn::PHOTO_META_DATE_MODIFIED)) {
        valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_MDIRTY));
    } else {
        valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_SYNCED));
    }

    int32_t changedRows;
    std::string whereClause = PhotoColumn::MEDIA_ID + " = ? AND " + PhotoColumn::PHOTO_DIRTY + " = ?";
    std::vector<std::string> whereArgs = {fileId, std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW))};
    int32_t ret = photoRefresh->Update(changedRows, PhotoColumn::PHOTOS_TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("UpdatePhotoCreatedRecord ret:%{public}d, changedRows:%{public}d, cloudId:%{public}s",
        ret,
        changedRows,
        record.cloudId.c_str());
    return ret;
}

int32_t CloudMediaPhotosDao::OnModifyPhotoRecord(
    const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("OnModifyPhotoRecord enter %{public}s", record.ToString().c_str());
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "on modify photo get store failed.");
    if (record.cloudId.empty()) {
        MEDIA_ERR_LOG("OnModifyPhotoRecord cloudId is empty");
        return E_ERR;
    }
    int32_t ret = UpdateLocalAlbumMap(record.cloudId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnModifyPhotoRecord update local album map error %{public}d", ret);
    }
    int32_t changedRows;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.version);
    valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED));
    /* mdirty -> synced: only if no change in meta_date_modified */
    ret = photoRefresh->Update(changedRows,
        PhotoColumn::PHOTOS_TABLE,
        valuesBucket,
        PhotoColumn::PHOTO_CLOUD_ID + " = ? AND " + PhotoColumn::PHOTO_META_DATE_MODIFIED + " = ?",
        {record.cloudId, std::to_string(record.metaDateModified)});
    MEDIA_INFO_LOG("OnModifyPhotoRecord Update Dirty %{public}d, ret: %{public}d", changedRows, ret);
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
        MEDIA_ERR_LOG("OnModifyPhotoRecord update synced err %{public}d", ret);
        /* update record version anyway */
        valuesBucket.Clear();
        valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.version);
        ret = photoRefresh->Update(changedRows,
            PhotoColumn::PHOTOS_TABLE,
            valuesBucket,
            PhotoColumn::PHOTO_CLOUD_ID + " = ?",
            {record.cloudId});
        MEDIA_INFO_LOG("OnModifyPhotoRecord Update Version %{public}d", changedRows);
        if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
            MEDIA_ERR_LOG("OnModifyPhotoRecord update record version err %{public}d", ret);
            return ret;
        }
    }
    return ret;
}

int32_t CloudMediaPhotosDao::UpdateFdirtyVersion(
    const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("UpdateFdirtyVersion enter %{public}s", record.ToString().c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateFdirtyVersion Failed to get rdbStore.");
    int32_t changedRows;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.version);
    valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED));
    this->OnFdirtyHandlePosition(record, valuesBucket);
    /**
     * fdirty -> synced: only if no change in meta_date_modified.
     * Fix me: if date_modified unchanged, update fdirty -> mdirty
     */
    int32_t ret = photoRefresh->Update(changedRows,
        PhotoColumn::PHOTOS_TABLE,
        valuesBucket,
        PhotoColumn::PHOTO_CLOUD_ID + " = ? AND " + PhotoColumn::PHOTO_META_DATE_MODIFIED + " = ?",
        {record.cloudId, std::to_string(record.metaDateModified)});
    MEDIA_INFO_LOG("UpdateFdirtyVersion completed, "
                   "rows: %{public}d, ret: %{public}d, IsLocalFileExists: %{public}d",
                   changedRows, ret, this->IsLocalFileExists(record));
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
        MEDIA_ERR_LOG("UpdateFdirtyVersion update synced err %{public}d", ret);
        /* update record version anyway */
        valuesBucket.Clear();
        valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.version);
        ret = photoRefresh->Update(changedRows,
            PhotoColumn::PHOTOS_TABLE,
            valuesBucket,
            PhotoColumn::PHOTO_CLOUD_ID + " = ?",
            {record.cloudId});
        MEDIA_INFO_LOG(
            "UpdateFdirtyVersion Update CloudVersion Update Rows: %{public}d, Ret: %{public}d", changedRows, ret);
        if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
            MEDIA_ERR_LOG("UpdateFdirtyVersion update record version err %{public}d", ret);
            return ret;
        }
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::OnDeleteRecordsAsset(
    const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("OnDeleteRecordsAsset");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "OnDeleteRecordsAsset Failed to get rdbStore.");
    string whereClause = PhotoColumn::PHOTO_CLOUD_ID + " = ?";
    std::vector<std::string> whereArgs = {record.dkRecordId};
    int32_t deletedRows = -1;
    int32_t ret = photoRefresh->Delete(deletedRows, PhotoColumn::PHOTOS_TABLE, whereClause, whereArgs);
    MEDIA_INFO_LOG("OnDeleteRecordsAsset Result: %{public}d, delete rows: %{public}d", ret, deletedRows);
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK || deletedRows <= 0) {
        MEDIA_ERR_LOG("OnDeleteRecordsAsset fail err %{public}d, delete rows: %{public}d", ret, deletedRows);
        return ret;
    }
    return E_OK;
}

int32_t CloudMediaPhotosDao::OnCopyPhotoRecord(
    const PhotosDto &record, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "OnCopyPhotoRecord get store failed.");
    int32_t fileId = record.fileId;
    string sandboxPath = record.path;
    int32_t changedRows;
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, record.cloudVersion);
    valuesBucket.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_SYNCED));
    valuesBucket.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    valuesBucket.PutString(PhotoColumn::PHOTO_CLOUD_ID, record.cloudId);
    valuesBucket.PutNull(PhotoColumn::PHOTO_ORIGINAL_ASSET_CLOUD_ID);
    int32_t ret = photoRefresh->Update(
        changedRows, PhotoColumn::PHOTOS_TABLE, valuesBucket, PhotoColumn::MEDIA_ID + " = ? ", {to_string(fileId)});
    MEDIA_INFO_LOG("OnCopyPhotoRecord changedRows: %{public}d, meida id: %{public}d", changedRows, fileId);
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
        MEDIA_ERR_LOG("OnCopyPhotoRecord update synced err %{public}d", ret);
        return ret;
    }
    return ret;
}

int32_t CloudMediaPhotosDao::ClearCloudInfo(
    const std::string &cloudId, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "ClearCloudInfo Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    NativeRdb::ValuesBucket values;
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(CloudFilePosition::POSITION_LOCAL));
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = photoRefresh->Update(changedRows, values, predicates);
    MEDIA_INFO_LOG("ClearCloudInfo Update Ret: %{public}d, ChangedRows: %{public}d", ret, changedRows);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_CLOUDSYNC_RDB_UPDATE_FAILED, "Failed to ClearCloudInfo.");
    CHECK_AND_RETURN_RET_WARN_LOG(changedRows > 0, ret, "ClearCloudInfo Check updateRows: %{public}d.", changedRows);
    return ret;
}

int32_t CloudMediaPhotosDao::DeleteFileNotExistPhoto(
    std::string &path, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("enter DeleteFileNotExistPhoto %{public}s", path.c_str());
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "DeleteFileNotExistPhoto get store failed.");
    int32_t deletedRows = 0;
    std::string whereClause = PhotoColumn::MEDIA_FILE_PATH + " = ?";
    int32_t ret = photoRefresh->Delete(deletedRows, PhotoColumn::PHOTOS_TABLE, whereClause, {path});
    if (ret != AccurateRefresh::ACCURATE_REFRESH_RET_OK || deletedRows <= 0) {
        MEDIA_INFO_LOG("DeleteFileNotExistPhoto failed ret: %{public}d", ret);
        return E_RDB;
    }
    MEDIA_INFO_LOG("DeleteFileNotExistPhoto deletedRows: %{public}d, ret: %{public}d", deletedRows, ret);
    return ret;
}

int32_t CloudMediaPhotosDao::HandleSameNameRename(
    const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "rename same name get store failed.");
    size_t dotPos = photo.fileName.rfind('.');
    if (dotPos == std::string::npos) {
        MEDIA_ERR_LOG("fileName have no suffix, %{private}s.", photo.fileName.c_str());
        dotPos = photo.fileName.length();
    }
    std::string fileName = photo.fileName.substr(0, dotPos);
    std::string fileExtension = photo.fileName.substr(dotPos);
    NativeRdb::ValuesBucket values;
    if (fileName.size() == 0) {
        MEDIA_ERR_LOG("file name is too short!");
        return E_DATA;
    }
    size_t len = fileName.size() - 1;
    std::string tmpName;
    if (len >= 1 && fileName[len - 1] == '_' && isdigit(fileName[len])) {
        tmpName = fileName.substr(0, len) + to_string(static_cast<int>(fileName[len]) - '0' + 1);
    } else {
        tmpName = fileName + "_1";
    }
    std::string storagePath = photo.storagePath;
    if (!storagePath.empty()) {
        auto index = storagePath.rfind('/');
        if (index != std::string::npos) {
            std::string newStoragePath = storagePath.substr(0, index) + "/" + tmpName + fileExtension;
            if (rename(storagePath.c_str(), newStoragePath.c_str()) != 0) {
                MEDIA_ERR_LOG("Failed to rename local anco file, file path: %{public}s, errno: %{public}d",
                    storagePath.c_str(), errno);
            } else {
                MEDIA_INFO_LOG("the new storagePath is %{public}s", newStoragePath.c_str());
                values.PutString(PhotoColumn::PHOTO_STORAGE_PATH, newStoragePath);
            }
        }
    }
    values.PutString(PhotoColumn::MEDIA_TITLE, tmpName);
    values.PutString(PhotoColumn::MEDIA_NAME, tmpName + fileExtension);
    int32_t changedRows = -1;
    std::vector<std::string> whereArgs = {to_string(photo.fileId)};
    int ret =
        photoRefresh->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values, PhotoColumn::MEDIA_ID + " = ?", whereArgs);
    if (ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK && changedRows > 0) {
        MEDIA_ERR_LOG("HandleSameNameRename Success %{public}d, %{public}s", photo.fileId, photo.cloudId.c_str());
        photoCreateFailSet_.Remove(std::to_string(photo.fileId));
        photoModifyFailSet_.Remove(photo.cloudId);
        photoCopyFailSet_.Remove(std::to_string(photo.fileId));
        return E_OK;
    } else {
        MEDIA_ERR_LOG("HandleSameNameRename update database fail, ret is %{public}d", ret);
        return E_RDB;
    }
}

int32_t CloudMediaPhotosDao::UpdatePhotoVisible()
{
    MEDIA_INFO_LOG("enter UpdatePhotoVisible");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdatePhotoVisible get store failed.");
    int updateRows;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_SYNC_STATUS, static_cast<int32_t>(SyncStatusType::TYPE_VISIBLE));
    std::string whereClause = PhotoColumn::PHOTO_SYNC_STATUS + " = ?";
    int32_t ret = rdbStore->Update(updateRows,
        PhotoColumn::PHOTOS_TABLE,
        values,
        whereClause,
        {std::to_string(static_cast<int32_t>(SyncStatusType::TYPE_DOWNLOAD))});
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdatePhotoVisible failed, ret=%{public}d", ret);
    }
    return ret;
}

void CloudMediaPhotosDao::UpdateAlbumInternal(std::set<std::string> &refreshAlbums)
{
    std::vector<std::string> albums(refreshAlbums.begin(), refreshAlbums.end());
    refreshAlbums.clear();
    UpdateAllAlbumsCountForCloud(albums);
}

int32_t CloudMediaPhotosDao::UpdateAlbumReplacedSignal(const std::vector<std::string> &albumIdVector)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_INFO_LOG(rdbStore != nullptr, E_RDB, "UpdateAlbumReplacedSignal Failed to get rdbStore.");

    auto albumIds = CloudMediaDaoUtils::GetNumbers(albumIdVector);
    if (albumIds.empty()) {
        return E_OK;
    }

    NativeRdb::ValuesBucket refreshValues;
    std::string insertRefreshTableSql = "INSERT OR REPLACE INTO RefreshAlbum (REFRESH_ALBUM_ID, STATUS) VALUES ";
    for (size_t i = 0; i < albumIds.size(); ++i) {
        if (i != albumIds.size() - 1) {
            insertRefreshTableSql += "(" + albumIds[i] + ", 0), ";
        } else {
            insertRefreshTableSql += "(" + albumIds[i] + ", 0);";
        }
    }
    MEDIA_INFO_LOG("output insertRefreshTableSql:%{public}s", insertRefreshTableSql.c_str());

    int32_t ret = rdbStore->ExecuteSql(insertRefreshTableSql);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Can not insert refreshed table, ret:%{public}d", ret);
        return E_RDB;
    }
    return E_OK;
}

void CloudMediaPhotosDao::UpdateAllAlbumsCountForCloud(const std::vector<std::string> &albums)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateAllAlbumsCountForCloud Failed to get rdbStore.");

    std::vector<std::string> allRefreshAlbum = {ALL_SYSTEM_PHOTO_ALBUM};
    if (albums.empty()) {
        allRefreshAlbum.push_back(to_string(PhotoAlbumSubType::USER_GENERIC));
        allRefreshAlbum.push_back(to_string(PhotoAlbumSubType::SOURCE_GENERIC));
        UpdateAlbumCountInternal(allRefreshAlbum);
    } else {
        UpdateAlbumCountInternal(ALL_SYSTEM_PHOTO_ALBUM);
        UpdateAlbumReplacedSignal(albums);
    }
    std::vector<std::string> subtype = {"4101"};
    MediaLibraryRdbUtils::UpdateAnalysisAlbumCountInternal(rdbStore, subtype);
}

void CloudMediaPhotosDao::UpdateAlbumCountInternal(const std::vector<std::string> &subtypes)
{
    std::vector<std::string> columns = {PhotoAlbumColumns::ALBUM_ID};
    auto albumResult = GetAllSysAlbums(subtypes, columns);
    if (albumResult == nullptr) {
        return;
    }

    std::vector<std::string> replaceSignalAlbumVector;
    while (albumResult->GoToNextRow() == NativeRdb::E_OK) {
        int32_t albumId = GetInt32Val(PhotoAlbumColumns::ALBUM_ID, albumResult);
        if (albumId < 0) {
            MEDIA_ERR_LOG("Can not get albumId:%{public}d", albumId);
        } else {
            replaceSignalAlbumVector.push_back(to_string(albumId));
        }
    }
    if (!replaceSignalAlbumVector.empty()) {
        int32_t ret = UpdateAlbumReplacedSignal(replaceSignalAlbumVector);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("Update sysalbum replaced signal failed ret:%{public}d", ret);
        }
    }
}

int32_t CloudMediaPhotosDao::SetRetry(const std::string &cloudId)
{
    MEDIA_INFO_LOG("Set retry cloudId: %{public}s.", cloudId.c_str());
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "Failed to get rdbStore.");

    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_RETRY));

    int32_t changedRows = DEFAULT_VALUE;
    int32_t ret = rdbStore->Update(changedRows, values, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_CLOUDSYNC_RDB_UPDATE_FAILED, "Failed to SetRetry.");
    CHECK_AND_RETURN_RET_WARN_LOG(changedRows > 0, ret, "Check updateRows: %{public}d.", changedRows);

    // 添加大数据打点
    return ret;
}

int32_t CloudMediaPhotosDao::DeleteLocalByCloudId(
    const std::string &cloudId, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "DeleteLocalByCloudId get store failed.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::PHOTO_CLOUD_ID, cloudId);

    int32_t deletedRows = DEFAULT_VALUE;
    int32_t ret = photoRefresh->Delete(deletedRows, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK && deletedRows > 0,
 
        E_CLOUDSYNC_RDB_DELETE_FAILED,
        "Failed to DeleteLocalByCloudId, ret: %{public}d, deletedRows: %{public}d.",
        ret,
        deletedRows);
    AuditLog auditLog = { true, "USER BEHAVIOR", "DELETE", "io", 1, "running", "ok" };
    auditLog.size = deletedRows;
    auditLog.id = cloudId;
    HiAudit::GetInstance().Write(auditLog);
    return ret;
}
 
int32_t CloudMediaPhotosDao::UpdateFailRecordsCloudId(
    const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    bool isValid = record.serverErrorCode != static_cast<int32_t>(ServerErrorCode::RENEW_RESOURCE);
    CHECK_AND_RETURN_RET_INFO_LOG(isValid, E_OK, "Skip UpdateFailRecordsCloudId");
    CHECK_AND_RETURN_RET_LOG(photoRefresh != nullptr, E_RDB_STORE_NULL, "UpdateFailRecordsCloudId get store failed.");
    std::string fileId = to_string(record.fileId);
    if (localMap.find(fileId) == localMap.end()) {
        MEDIA_INFO_LOG("UpdateFailRecordsCloudId fileId not exist");
        return E_OK;
    }
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(PhotoColumn::PHOTO_CLOUD_ID, record.cloudId);
    int32_t changedRows;
    std::string whereClause = "file_id = ? AND dirty = ? AND COALESCE(cloud_id, '') <> ?";
    std::vector<std::string> whereArgs = {
        fileId, std::to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)), record.cloudId};
    int32_t ret = photoRefresh->Update(changedRows, PhotoColumn::PHOTOS_TABLE, valuesBucket, whereClause, whereArgs);
    MEDIA_INFO_LOG("UpdateFailRecordsCloudId ret:%{public}d, changedRows:%{public}d, cloudId:%{public}s",
        ret,
        changedRows,
        record.cloudId.c_str());
    return ret;
}

void CloudMediaPhotosDao::InsertPhotoCreateFailedRecord(int32_t fileId)
{
    photoCreateFailSet_.PushBack(std::to_string(fileId));
}

void CloudMediaPhotosDao::InsertPhotoModifyFailedRecord(const std::string &cloudId)
{
    photoModifyFailSet_.PushBack(cloudId);
}

void CloudMediaPhotosDao::InsertPhotoCopyFailedRecord(int32_t fileId)
{
    photoCopyFailSet_.PushBack(std::to_string(fileId));
}

void CloudMediaPhotosDao::RemovePhotoCreateFailedRecord(int32_t fileId)
{
    photoCreateFailSet_.Remove(std::to_string(fileId));
}

void CloudMediaPhotosDao::RemovePhotoModifyFailedRecord(const std::string &cloudId)
{
    photoModifyFailSet_.Remove(cloudId);
}

void CloudMediaPhotosDao::RemovePhotoCopyFailedRecord(int32_t fileId)
{
    photoCopyFailSet_.Remove(std::to_string(fileId));
}

int32_t CloudMediaPhotosDao::ClearPhotoFailedRecords()
{
    photoCopyFailSet_.Clear();
    photoCreateFailSet_.Clear();
    photoModifyFailSet_.Clear();
    return E_OK;
}

void CloudMediaPhotosDao::LoadAlbumMap()
{
    // use localToCloudMap_ to identify the cache loaded or not.
    CHECK_AND_RETURN_INFO_LOG(localToCloudMap_.IsEmpty(),
        "Album Map is not empty. localToCloudMap: %{public}d,"
        "cloudToLocalMap: %{public}d, lpathToIdMap: %{public}d",
        localToCloudMap_.Size(),
        cloudToLocalMap_.Size(),
        lpathToIdMap_.Size());
    return PrepareAlbumMap(localToCloudMap_, cloudToLocalMap_, lpathToIdMap_);
}

void CloudMediaPhotosDao::ClearAlbumMap()
{
    MEDIA_INFO_LOG("ClearAlbumMap. localToCloudMap: %{public}d,"
                   "cloudToLocalMap: %{public}d, lpathToIdMap: %{public}d",
        localToCloudMap_.Size(),
        cloudToLocalMap_.Size(),
        lpathToIdMap_.Size());
    localToCloudMap_.Clear();
    cloudToLocalMap_.Clear();
    lpathToIdMap_.Clear();
    this->albumCache_.ClearAlbumCache();
}

void CloudMediaPhotosDao::PrepareAlbumMap(SafeMap<int32_t, std::pair<std::string, std::string>> &localToCloudMap,
    SafeMap<std::string, int32_t> &cloudToLocalMap, SafeMap<std::string, std::pair<int32_t, std::string>> &lpathToIdMap,
    bool isUpload)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "PrepareAlbumMap Failed to get rdbStore.");
    NativeRdb::AbsRdbPredicates predicates = NativeRdb::AbsRdbPredicates(PhotoAlbumColumns::TABLE);
    predicates.EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(Media::PhotoAlbumType::USER))
        ->Or()
        ->EqualTo(PhotoAlbumColumns::ALBUM_TYPE, std::to_string(Media::PhotoAlbumType::SOURCE));
    auto results = rdbStore->Query(predicates, {});
    std::vector<PhotoAlbumPo> albumInfoList;
    int32_t ret = ResultSetReader<PhotoAlbumPoWriter, PhotoAlbumPo>(results).ReadRecords(albumInfoList);
    CHECK_AND_RETURN_LOG(ret == E_OK, "PrepareAlbumMap read records failed");
    this->albumCache_.SetAlbumCache(albumInfoList);
    int32_t albumId;
    std::string cloudId;
    std::string lPath;
    for (const auto &albumInfo : albumInfoList) {
        albumId = albumInfo.albumId.value_or(0);
        cloudId = albumInfo.cloudId.value_or("");
        lPath = albumInfo.lpath.value_or("");
        if (cloudId == HIDDEN_ALBUM_CLOUD_ID) {
            hiddenAlbumId_ = albumId;
            localToCloudMap.EnsureInsert(albumId, std::make_pair(cloudId, lPath));
            continue;
        }
        MEDIA_DEBUG_LOG(
            "FixData:path %{public}s cloudid %{public}s albumId %{public}d", lPath.c_str(), cloudId.c_str(), albumId);
        if (!IsAlbumCloud(isUpload, albumInfo)) {
            localToCloudMap.EnsureInsert(albumId, std::make_pair("", lPath));
        } else {
            localToCloudMap.EnsureInsert(albumId, std::make_pair(cloudId, lPath));
        }

        if (!cloudId.empty()) {
            if (cloudId == SCREENSHOT_ALBUM_CLOUD_ID && lPath.find("records") != std::string::npos) {
                cloudToLocalMap.EnsureInsert(cloudId + "V", albumId);
            } else {
                cloudToLocalMap.EnsureInsert(cloudId, albumId);
            }
        }
        std::transform(lPath.begin(), lPath.end(), lPath.begin(), ::tolower);
        lpathToIdMap.EnsureInsert(lPath, std::make_pair(albumId, cloudId));
    }
}

SafeMap<int32_t, std::pair<std::string, std::string>> &CloudMediaPhotosDao::GetAlbumLocalToCloudMap()
{
    CHECK_AND_EXECUTE(!localToCloudMap_.IsEmpty(), LoadAlbumMap());
    return localToCloudMap_;
}

SafeMap<std::string, int32_t> &CloudMediaPhotosDao::GetAlbumCloudToLocalMap()
{
    CHECK_AND_EXECUTE(!cloudToLocalMap_.IsEmpty(), LoadAlbumMap());
    return cloudToLocalMap_;
}

SafeMap<std::string, std::pair<int32_t, std::string>> &CloudMediaPhotosDao::GetAlbumLPathToIdMap()
{
    CHECK_AND_EXECUTE(!lpathToIdMap_.IsEmpty(), LoadAlbumMap());
    return lpathToIdMap_;
}

bool CloudMediaPhotosDao::IsAlbumCloud(bool isUpload, const PhotoAlbumPo &albumInfo)
{
    if (!isUpload) {
        return true;
    }
    int32_t dirty = albumInfo.dirty.value_or(1);
    if (dirty == static_cast<int32_t>(DirtyType::TYPE_NEW)) {
        MEDIA_ERR_LOG("IsAlbumCloud album is uploading");
        return false;
    }
    return true;
}

int32_t CloudMediaPhotosDao::UpdatePhoto(const std::string &whereClause, const std::vector<std::string> &whereArgs,
    NativeRdb::ValuesBucket &values, int32_t &changedRows)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB, "UpdatePhoto get store failed.");
    return rdbStore->Update(changedRows, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
}

void CloudMediaPhotosDao::IsCoverContentChange(int32_t changedRows, bool mtimeChanged, int32_t dataFileId)
{
    CHECK_AND_RETURN(changedRows > 0 && mtimeChanged);
    CHECK_AND_RETURN_LOG(dataFileId > 0, "fileId is invalid");
    string fileId = to_string(dataFileId);
    AccurateRefresh::AlbumAccurateRefresh albumRefresh;
    if (albumRefresh.IsCoverContentChange(fileId)) {
        MEDIA_INFO_LOG("album cover content has Changed, fileId: %{public}d", dataFileId);
    }
}

int32_t CloudMediaPhotosDao::RepushDuplicatedPhoto(const PhotosDto &photo)
{
    int32_t changeRows;
    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(Media::DirtyType::TYPE_FDIRTY));
    values.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    std::string whereClause = MediaColumn::MEDIA_ID + " = ?";
    std::vector<std::string> whereArgs = {std::to_string(photo.fileId)};
    int32_t ret = UpdatePhoto(whereClause, whereArgs, values, changeRows);
    MEDIA_INFO_LOG("RepushDuplicatedPhoto,"
        "ret: %{public}d, changeRows: %{public}d, fileId: %{public}s",
        ret, changeRows, std::to_string(photo.fileId).c_str());
    return ret;
}

int32_t CloudMediaPhotosDao::RenewSameCloudResource(const PhotosDto &photo)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos RenewSameCloudResource Failed to get photoRefresh.");
    NativeRdb::ValuesBucket values;
    values.PutNull(PhotoColumn::PHOTO_CLOUD_ID);
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    values.PutInt(PhotoColumn::PHOTO_POSITION, PhotoPosition::POSITION_LOCAL);
    values.PutInt(PhotoColumn::PHOTO_SOUTH_DEVICE_TYPE, static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_NULL));
    values.PutLong(PhotoColumn::PHOTO_CLOUD_VERSION, 0);
    std::string whereClause = PhotoColumn::PHOTO_CLOUD_ID + " = ?";
    std::vector<std::string> whereArgs = {photo.cloudId};
    int32_t changeRows = -1;
    int32_t ret = photoRefresh->Update(changeRows, PhotoColumn::PHOTOS_TABLE, values, whereClause, whereArgs);
    MEDIA_INFO_LOG("RenewSameCloudResource,"
        "ret: %{public}d, changeRows: %{public}d, cloudId: %{public}s",
        ret, changeRows, photo.cloudId.c_str());
    CHECK_AND_RETURN_RET(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret);
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosDao::DeleteLocalFileNotExistRecord(const PhotosDto &photo)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos DeleteLocalFileNotExistRecord Failed to get photoRefresh.");
    int32_t deletedRows = -1;
    std::string whereClause = MediaColumn::MEDIA_FILE_PATH + " = ?";
    std::vector<std::string> whereArgs = {photo.path};
    int32_t ret = photoRefresh->Delete(deletedRows, PhotoColumn::PHOTOS_TABLE, whereClause, whereArgs);
    MEDIA_INFO_LOG("DeleteLocalFileNotExistRecord,"
        "ret: %{public}d, deletedRows: %{public}d, path: %{public}s",
        ret, deletedRows, MediaFileUtils::DesensitizePath(photo.path).c_str());
    CHECK_AND_RETURN_RET(ret == AccurateRefresh::ACCURATE_REFRESH_RET_OK, ret);
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosDao::QueryAnalysisAlbum(const std::string &cloudId, std::vector<std::string> &analysisAlbumIds)
{
    CHECK_AND_RETURN_RET_LOG(!cloudId.empty(), E_ERR, "cloudId is null");
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_DB_FAIL, "QueryAnalysisAlbum Failed to get rdbStore.");
    std::vector<NativeRdb::ValueObject> bindArgs = {cloudId};
    /* query */
    auto resultSet = rdbStore->QuerySql(SQL_QUERY_ANALYSIS_ALBUM, bindArgs);
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_RESULT_SET_NULL, "Failed to query.");
    // Notify caller if no data is returned, it means all data has been processed.
    int32_t analysisAlbumId = 0;
    while (resultSet->GoToNextRow() == E_OK) {
        analysisAlbumId = GetInt32Val("album_id", resultSet);
        analysisAlbumIds.emplace_back(std::to_string(analysisAlbumId));
    }
    resultSet->Close();
    return E_OK;
}

int32_t CloudMediaPhotosDao::FindPhotoAlbumInCache(const std::string &albumCloudId, const std::string &lPath,
    const std::string &sourcePath, std::optional<PhotoAlbumPo> &photoAlbumPoOp)
{
    CHECK_AND_EXECUTE(!this->albumCache_.IsEmpty(), LoadAlbumMap());
    int32_t ret = this->albumCache_.QueryAlbumByCloudId(albumCloudId, photoAlbumPoOp);
    bool isValid = ret == E_OK && photoAlbumPoOp.has_value();
    CHECK_AND_RETURN_RET(!isValid, ret);
    ret = this->albumCache_.QueryAlbumBylPath(lPath, photoAlbumPoOp);
    isValid = ret == E_OK && photoAlbumPoOp.has_value();
    CHECK_AND_RETURN_RET(!isValid, ret);
    return this->albumCache_.QueryAlbumBySourcePath(sourcePath, photoAlbumPoOp);
}

int32_t CloudMediaPhotosDao::UpdateFileRecordsInTransaction(const std::vector<NativeRdb::ValuesBucket> &updateFiles,
    const std::vector<int32_t> &cloudFileIdlist, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    const auto loop_count = std::min(updateFiles.size(), cloudFileIdlist.size());
    std::function<int(void)> transFunc = [&]() -> int {
        int32_t totalChangedRows = 0;
        for (size_t idx = 0; idx < loop_count; ++idx) {
            const auto& valuesBucket = updateFiles[idx];
            const auto& cloudFileId = cloudFileIdlist[idx];
            std::string whereClause = MediaColumn::MEDIA_ID + " = ?";
            std::vector<std::string> whereArgs = {std::to_string(cloudFileId)};

            int32_t changedRows = 0;
            auto retInner = photoRefresh->Update(changedRows, PhotoColumn::PHOTOS_TABLE,
                valuesBucket, whereClause, whereArgs);
            if (retInner != AccurateRefresh::ACCURATE_REFRESH_RET_OK) {
                MEDIA_ERR_LOG("Failed to update file at index %{public}zu, ret=%{public}d", idx, retInner);
                return retInner;
            }
            totalChangedRows += changedRows;
        }

        MEDIA_INFO_LOG("BatchUpdateFile: updated %{public}d total rows in batch.", totalChangedRows);
        return AccurateRefresh::ACCURATE_REFRESH_RET_OK;
    };

    int32_t transRet = E_ERR;
    if (photoRefresh->GetTransaction() != nullptr) {
        transRet = photoRefresh->GetTransaction()->RetryTrans(transFunc);
    } else {
        auto trans = std::make_shared<TransactionOperations>(__func__);
        CHECK_AND_RETURN_RET_LOG(trans != nullptr, E_RDB_STORE_NULL, "BatchUpdate Failed to get trans.");
        transRet = trans->RetryTrans(transFunc);
    }

    if (transRet != E_OK) {
        MEDIA_ERR_LOG("BatchUpdate transaction failed, ret=%{public}d", transRet);
    }

    return transRet;
}

int32_t CloudMediaPhotosDao::BatchUpdateFile(std::map<std::string, int> &recordAnalysisAlbumMaps,
    std::map<std::string, std::set<int>> &recordAlbumMaps, std::vector<NativeRdb::ValuesBucket> &updateFiles,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh, std::vector<int32_t> cloudFileIdlist)
{
    int32_t ret = E_OK;
    if (!updateFiles.empty()) {
        ret = UpdateFileRecordsInTransaction(updateFiles, cloudFileIdlist, photoRefresh);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("UpdateFileRecordsInTransaction failed, ret=%{public}d", ret);
            return ret;
        }
    }

    if (!recordAlbumMaps.empty()) {
        BatchInsertAssetMaps(recordAlbumMaps);
    }

    if (!recordAnalysisAlbumMaps.empty()) {
        BatchInsertAssetAnalysisMaps(recordAnalysisAlbumMaps);
    }

    return ret;
}

void CloudMediaPhotosDao::UpdateMediaAnalysisHdcData()
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_LOG(rdbStore != nullptr, "UpdateInvalidHighlightInfo Failed to get rdbStore.");
    
    // 更新非前台推送和删除的时刻状态为-4，智慧分析会清理时刻状态为-4的数据
    std::string updateSql = "\
        UPDATE tab_highlight_album \
            SET highlight_status = -4 \
        WHERE highlight_status != 1 \
            AND highlight_status != -3 ";
    int32_t highlighRet = rdbStore->ExecuteSql(updateSql);
    if (highlighRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update invalid highlight err %{public}d", highlighRet);
    }

    // 更新时刻封面状态为1，智慧分析根据状态位刷新时刻封面
    std::string updateHighlightCoverSql = "\
        UPDATE tab_highlight_cover_info \
            SET status = 1 ";
    int32_t highlighCoverRet = rdbStore->ExecuteSql(updateHighlightCoverSql);
    if (highlighCoverRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update invalid highlight cover err %{public}d", highlighCoverRet);
    }

    // 更新人像相册、合影相册的analysis_status为1，智慧分析根据状态位刷新相册显示状态
    std::string updatePortraitSql = "\
        UPDATE AnalysisAlbum \
            SET analysis_status = 1 \
        WHERE (album_subtype = 4102 \
            OR album_subtype = 4103) \
            AND user_display_level < 0 ";
    int32_t portraitRet = rdbStore->ExecuteSql(updatePortraitSql);
    if (portraitRet != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Update portrait analysis status err %{public}d", portraitRet);
    }
}

bool CloudMediaPhotosDao::IsLocalFileExists(const PhotosDto &record)
{
    std::string cloudFilePath = LakeFileUtils::GetAssetRealPath(record.path);
    bool isValid = !cloudFilePath.empty();
    CHECK_AND_RETURN_RET_LOG(isValid, false, "cloudFilePath empty, record: %{public}s", record.ToString().c_str());
    std::string localFilePath = CloudMediaSyncUtils::GetLocalPath(cloudFilePath);
    return MediaFileUtils::IsFileExists(localFilePath);
}

int32_t CloudMediaPhotosDao::OnFdirtyHandlePosition(const PhotosDto &record, NativeRdb::ValuesBucket &valuesBucket)
{
    CHECK_AND_RETURN_RET(this->IsLocalFileExists(record), E_OK);
    valuesBucket.PutInt(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL_AND_CLOUD));
    return E_OK;
}

int32_t CloudMediaPhotosDao::FillThumbStatus(NativeRdb::ValuesBucket &values, const bool mtimeChanged)
{
    CHECK_AND_RETURN_RET(mtimeChanged, E_OK);
    values.PutInt(PhotoColumn::PHOTO_THUMB_STATUS, static_cast<int32_t>(ThumbState::TO_DOWNLOAD));
    return E_OK;
}
// LCOV_EXCL_STOP
}  // namespace OHOS::Media::CloudSync