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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_photos_service.h"

#include <string>
#include <vector>

#include "cloud_media_dao_utils.h"
#include "cloud_media_sync_utils.h"
#include "dataobs_mgr_changeinfo.h"
#include "directory_ex.h"
#include "file_ex.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "cloud_media_file_utils.h"
#include "cloud_lake_file_handler.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "on_fetch_records_vo.h"
#include "photos_dto.h"
#include "photo_file_utils.h"
#include "result_set_utils.h"
#include "medialibrary_asset_operations.h"
#include "scanner_utils.h"
#include "medialibrary_notify.h"
#include "cloud_media_dao_const.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_operation_code.h"
#include "cloud_media_dfx_service.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
#include "background_cloud_batch_selected_file_processor.h"
#endif
#include "lake_file_utils.h"
#include "album_plugin_config.h"
#include "cloud_media_retain_smart_data.h"
#include "cloud_media_context.h"
#include "cloud_media_asset_retain_compare_dao.h"
#include "medialibrary_unistore_manager.h"
#include "photos_field_iterator.h"
#include "medialibrary_rdb_utils.h"
#include "cloud_media_asset_manager.h"
#include "medialibrary_meta_recovery.h"
#include "thumbnail_service.h"
#include "parameters.h"

using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
namespace OHOS::Media::CloudSync {
int32_t CloudMediaPhotosService::PullDelete(const CloudMediaPullDataDto &data, std::set<std::string> &refreshAlbums,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    std::string cloudId = data.cloudId;
    std::string localPath = data.localPath;
    CHECK_AND_RETURN_RET_INFO_LOG(!cloudId.empty(), E_OK, "cloudId is empty, ignore cloud delete");
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(data.localPosition);
    MEDIA_INFO_LOG("Delete cloudId: %{public}s, isLocal: %{public}d, data: %{public}s",
        cloudId.c_str(),
        isLocal,
        data.ToString().c_str());
    if (isLocal && CloudMediaSyncUtils::IsLocalDirty(data.localDirty, true)) {
        MEDIA_ERR_LOG("local record dirty, ignore cloud delete");
        return this->photosDao_.ClearCloudInfo(cloudId, photoRefresh);
    }

    if (isLocal && CloudMediaFileUtils::LocalWriteOpen(localPath)) {
        int32_t ret = this->photosDao_.SetRetry(cloudId);
        if (ret != E_OK) {
            std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
            REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
        }
        return ret;
    }
    this->RefreshAnalysisAlbum(cloudId);
    int32_t ret = this->photosDao_.DeleteLocalByCloudId(cloudId, photoRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("delete in rdb failed, ret:%{public}d", ret);
        int32_t ret = this->photosDao_.SetRetry(cloudId);
        if (ret != E_OK) {
            std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
            REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
        }
        return ret;
    }
    std::string notifyUri =
        PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(data.localFileId) + '/' + data.localDateAdded;
    MediaGallerySyncNotify::GetInstance().TryNotify(notifyUri, ChangeType::DELETE, std::to_string(data.localFileId));
    refreshAlbums.emplace(std::to_string(data.localOwnerAlbumId));
    this->RemoveLocalFile(data);
    return E_OK;
}

int32_t CloudMediaPhotosService::IsMtimeChanged(const CloudMediaPullDataDto &pullData, bool &changed)
{
    if (!pullData.localDateModified.empty() && pullData.attributesEditedTimeMs > 0) {
        std::string cloudDateModified = std::to_string(pullData.attributesEditedTimeMs);
        MEDIA_INFO_LOG("localDateModified: %{public}s, attributesEditedTimeMs: %{public}s",
            pullData.localDateModified.c_str(),
            cloudDateModified.c_str());
        changed = !(pullData.localDateModified == cloudDateModified);
        return E_OK;
    }

    if (pullData.localDirty == static_cast<int32_t>(DirtyType::TYPE_SDIRTY) &&
        pullData.basicSize == pullData.localSize) {
        changed = false;
        MEDIA_INFO_LOG("file %{public}s in cloud and local with same size: %{public}" PRId64,
            pullData.cloudId.c_str(), pullData.basicSize);
        return E_OK;
    }

    if (pullData.localDateAdded.empty()) {
        MEDIA_INFO_LOG("CloudMediaPhotosService::IsMtimeChanged pullData.localDateAdded empty");
        return E_CLOUDSYNC_INVAL_ARG;
    }

    // Note. basicCreatedTime and localDateAdded should not be compared to identify file changes.
    // basicCreatedTime means date_taken, the file created time.
    // localDateAdded means the db record created time.
    std::string createTime = std::to_string(pullData.basicCreatedTime);
    MEDIA_INFO_LOG(
        "localDateAdded: %{public}s, cloudDateAdded: %{public}s", pullData.localDateAdded.c_str(), createTime.c_str());
    changed = !(pullData.localDateAdded == createTime);
    return E_OK;
}

void CloudMediaPhotosService::ExtractEditDataCamera(const CloudMediaPullDataDto &pullData)
{
    std::string editDataCamera = pullData.attributesEditDataCamera;
    CHECK_AND_RETURN(!editDataCamera.empty());

    std::string editDataCameraPath = PhotoFileUtils::GetEditDataCameraPath(pullData.localPath);
    MEDIA_INFO_LOG("editDataCameraPath: %{public}s , editDataCamera: %{public}s",
        editDataCameraPath.c_str(),
        editDataCamera.c_str());
    ForceCreateDirectory(MediaFileUtils::GetParentPath(editDataCameraPath));
    if (!SaveStringToFile(editDataCameraPath, editDataCamera)) {
        MEDIA_ERR_LOG("save editDataCamera failed");
    }
}

int32_t CloudMediaPhotosService::ClearLocalData(const CloudMediaPullDataDto &pullData,
    std::vector<PhotosDto> &fdirtyData)
{
    MEDIA_INFO_LOG("here ClearLocalData");
    PhotosDto dto;
    dto.cloudId = pullData.cloudId;
    if (!CloudMediaFileUtils::GetParentPathAndFilename(pullData.localPath, dto.path, dto.displayName)) {
        MEDIA_WARN_LOG("Failed to get parent path and real filename.");
    }
    dto.size = pullData.basicSize;
    dto.mediaType = pullData.localMediaType;
    dto.modifiedTime = pullData.modifiedTime;
    dto.originalCloudId = pullData.localOriginalAssetCloudId;
    dto.localPath = pullData.localPath;
    dto.attributesMediaType = pullData.attributesMediaType;
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), false, "localPhotosPoOp has no value");
    PhotosPo localPhotosPo = pullData.localPhotosPoOp.value();
    dto.fileSourceType = localPhotosPo.fileSourceType.value_or(0);
    dto.storagePath = localPhotosPo.storagePath.value_or("");
    CloudMediaSyncUtils::FillPhotosDto(
        dto, pullData.localPath, pullData.localOrientation, pullData.exifRotate, pullData.localThumbState);
    fdirtyData.emplace_back(dto);
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(pullData.localPosition);
    if (isLocal) {
        CloudMediaSyncUtils::RemoveThmParentPath(pullData.localPath, PhotoColumn::FILES_CLOUD_DIR);
        CloudMediaSyncUtils::RemoveMetaDataPath(pullData.localPath);
        CloudMediaSyncUtils::RemoveEditDataPath(pullData.localPath);
        // for cloud enhancement composite photo
        if (CloudMediaSyncUtils::IsCloudEnhancementSupported() &&
            PhotoFileUtils::IsEditDataSourceBackExists(pullData.localPath)) {
            CloudMediaSyncUtils::BackUpEditDataSourcePath(pullData.localPath);
        }
        CloudMediaSyncUtils::RemoveEditDataSourcePath(pullData.localPath);
        CloudMediaSyncUtils::RemoveMovingPhoto(pullData);
    }
    return E_OK;
}

int32_t CloudMediaPhotosService::PullUpdate(CloudMediaPullDataDto &pullData, std::set<std::string> &refreshAlbums,
    std::vector<PhotosDto> &fdirtyData, std::vector<int32_t> &stats,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    std::string cloudId = pullData.cloudId;
    MEDIA_INFO_LOG("Update cloudId: %{public}s.", cloudId.c_str());
    CHECK_AND_RETURN_RET_INFO_LOG(!CloudMediaSyncUtils::IsLocalDirty(pullData.localDirty, false),
        E_OK,
        "local record dirty, ignore cloud update");
    bool mtimeChanged = false;
    int32_t ret = IsMtimeChanged(pullData, mtimeChanged);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("cloudId: %{public}s get mtime changed failed, ret: %{public}d.", cloudId.c_str(), ret);
    }
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(pullData.localPosition);
    // 元数据变更
    if (isLocal && mtimeChanged) {
        if (CloudMediaFileUtils::LocalWriteOpen(pullData.localPath)) {
            ret = this->photosDao_.SetRetry(cloudId);
            if (ret != E_OK) {
                std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
                REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
            }
            return ret;
        }
    }
    // Move to recycle bin or Move out of recycle bin.
    this->PullRecycleUpdate(pullData, photoRefresh);
    // UpdateRecordToDatabase更新成功，stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT]会增加
    int32_t updateCount = stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT];
    ret = this->photosDao_.UpdateRecordToDatabase(pullData, isLocal, mtimeChanged, refreshAlbums, stats, photoRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PullUpdate database Error cloudId: %{public}s, ret: %{public}d.", cloudId.c_str(), ret);
        return ret;
    }
    std::string notifyUri = PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(pullData.localFileId);
    MediaGallerySyncNotify::GetInstance().TryNotify(
        notifyUri, ChangeType::UPDATE, std::to_string(pullData.localFileId));

    refreshAlbums.emplace(std::to_string(pullData.localOwnerAlbumId));
    ExtractEditDataCamera(pullData);
    if (mtimeChanged && (updateCount != stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT])) {
        this->ClearLocalData(pullData, fdirtyData);
    } else {
        // 处理元数据变更
        CloudLakeFileHandler::HandleMetaChanged(pullData.localFileId);
    }
    return E_OK;
}

int32_t CloudMediaPhotosService::GetCloudKeyData(const CloudMediaPullDataDto &pullData, KeyData &keyData)
{
    bool ret = (!pullData.hasAttributes || pullData.basicFileName.empty() || pullData.basicSize == -1);
    CHECK_AND_RETURN_RET_LOG(!ret, E_CLOUDSYNC_INVAL_ARG, "PullData cannot find attributes or filename or size");

    keyData.displayName = pullData.basicFileName;

    int64_t singleEditTime = pullData.attributesMetaDateModified;
    int64_t dualEditTime = pullData.basicEditedTime;
    keyData.modifyTime = dualEditTime > singleEditTime ? dualEditTime : singleEditTime;

    keyData.isize = pullData.basicSize;
    keyData.createTime = pullData.basicCreatedTime;
    keyData.dateTrashed = pullData.basicRecycledTime;

    int32_t exifRotateValue = ORIENTATION_NORMAL;
    if (pullData.propertiesRotate != -1) {
        exifRotateValue = pullData.propertiesRotate;
    }
    keyData.exifRotateValue = exifRotateValue;
    keyData.lPath = CloudMediaSyncUtils::GetLpath(pullData);
    int32_t fileType = pullData.basicFileType;
    CHECK_AND_RETURN_RET_LOG(fileType != -1, E_CLOUDSYNC_INVAL_ARG, "Cannot find basic::fileType");

    keyData.mediaType = fileType == FILE_TYPE_VIDEO ? static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)
                                                    : static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE);
    return E_OK;
}

void CloudMediaPhotosService::GetMergeDataMap(
    const std::vector<CloudMediaPullDataDto> &pullDatas, std::map<std::string, KeyData> &mergeDataMap)
{
    KeyData mergeData;
    for (auto &pullData : pullDatas) {
        auto ret = GetCloudKeyData(pullData, mergeData);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("GetCloudKey data failed");
            continue;
        }
        mergeDataMap.insert(std::make_pair(pullData.cloudId, mergeData));
    }
}

int32_t CloudMediaPhotosService::DoDataMerge(const CloudMediaPullDataDto &pullData, const KeyData &localKeyData,
    const KeyData &cloudKeyData, std::set<std::string> &refreshAlbums,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    set<int32_t> cloudMapIds;
    bool cloudStd = (localKeyData.modifyTime != 0) && (cloudKeyData.modifyTime >= localKeyData.modifyTime);
    MEDIA_INFO_LOG("cloudStd %{public}d, cloud modifyTime: %{public}" PRId64 ", local modifyTime: %{public}" PRId64,
        cloudStd, cloudKeyData.modifyTime, localKeyData.modifyTime);

    int32_t ret = this->photosDao_.ConflictDataMerge(
        pullData, localKeyData.filePath, cloudStd, cloudMapIds, refreshAlbums, photoRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Conflict dataMerge failed");
        return ret;
    }
    ret = this->photosDao_.UpdateAssetInPhotoMap(pullData.attributesFileId, cloudMapIds);

    if (cloudStd && localKeyData.createTime != cloudKeyData.createTime) {
        std::string notifyUri = PhotoColumn::PHOTO_CLOUD_URI_PREFIX + to_string(pullData.attributesFileId) + '/' +
                                to_string(localKeyData.createTime) + '/' + to_string(cloudKeyData.createTime);
        MediaGallerySyncNotify::GetInstance().TryNotify(notifyUri,
            static_cast<ChangeType>(ExtraChangeType::PHOTO_TIME_UPDATE),
            to_string(pullData.attributesFileId));
    }
    return E_OK;
}

int32_t CloudMediaPhotosService::PullRecordsDataMerge(std::vector<CloudMediaPullDataDto> &allPullDatas,
    const KeyData &localKeyData, std::map<std::string, KeyData> &mergeDataMap, DataMergeResult &mergeResult,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    KeyData cloudKeyData;
    std::set<std::string> refreshAlbums;
    for (auto mergeData = allPullDatas.begin(); mergeData != allPullDatas.end();) {
        // Check the cloud data map (cache) is consistent with the cloud data list (allPullDatas)
        std::string id = mergeData->cloudId;
        if (mergeDataMap.find(id) == mergeDataMap.end()) {
            MEDIA_INFO_LOG("PullRecordsConflictProc GetLocalKey Data failed: %{public}s", id.c_str());
            ++mergeData;
            continue;
        }
        cloudKeyData = mergeDataMap[id];
        auto isMatchConflict = this->photosDao_.JudgeConflict(*mergeData, localKeyData, cloudKeyData);
        if (isMatchConflict) {
            // Check the cloud data is in Recycle-Bin, local data is not, and target Album is not upload.
            this->FindPhotoAlbum(*mergeData);
            CHECK_AND_BREAK_INFO_LOG(!this->IsIgnoreMatch(*mergeData, cloudKeyData, localKeyData),
                "ignore conflict data, recordId:%{public}s",
                id.c_str());
            // Merge the cloud data into the local.
            MEDIA_INFO_LOG("PullRecordsConflictProc merge record, recordId:%{public}s", id.c_str());
            mergeResult.mergeCount = 1;
            int32_t ret = DoDataMerge(*mergeData, localKeyData, cloudKeyData, refreshAlbums, photoRefresh);
            CHECK_AND_EXECUTE(ret == E_OK, mergeResult.failCloudId = mergeData->cloudId);
            mergeData = allPullDatas.erase(mergeData);  // 把合一数据剔除，剩下的就是纯新增数据
            break;
        } else {
            ++mergeData;
        }
    }
    if (!refreshAlbums.empty()) {
        std::set<std::string>::iterator it = refreshAlbums.begin();
        mergeResult.refreshAlbumId = *it;
    }
    return E_OK;
}

int32_t CloudMediaPhotosService::PullRecordsConflictProc(std::vector<CloudMediaPullDataDto> &allPullDatas,
    std::set<std::string> &refreshAlbums, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("PullRecordsConflictProc enter");
    int32_t rowCount = 0;
    auto resultSet = this->photosDao_.BatchQueryLocal(allPullDatas, PULL_QUERY_COLUMNS, rowCount);
    CHECK_AND_RETURN_RET_LOG(
        resultSet != nullptr, E_CLOUDSYNC_RDB_QUERY_FAILED, "PullRecordsConflictProc Failed to query.");
    CHECK_AND_RETURN_RET_INFO_LOG(rowCount != 0, E_OK, "PullRecordsConflictProc Normal download process.");
    MEDIA_INFO_LOG("PullRecordsConflictProc same displayName files num: %{public}d", rowCount);

    std::map<std::string, KeyData> mergeDataMap;  // {cloudId: cloudkeydata}
    GetMergeDataMap(allPullDatas, mergeDataMap);

    KeyData localKeyData;
    while (resultSet->GoToNextRow() == 0) {
        this->photosDao_.GetLocalKeyData(localKeyData, resultSet);
        DataMergeResult mergeResult;
        this->PullRecordsDataMerge(allPullDatas, localKeyData, mergeDataMap, mergeResult, photoRefresh);
        CHECK_AND_EXECUTE(
            mergeResult.mergeCount == 0, stats[StatsIndex::MERGE_RECORDS_COUNT] += mergeResult.mergeCount);
        CHECK_AND_EXECUTE(mergeResult.failCloudId.empty(), failedRecords.emplace_back(mergeResult.failCloudId));
        CHECK_AND_EXECUTE(mergeResult.refreshAlbumId.empty(), refreshAlbums.emplace(mergeResult.refreshAlbumId));
    }
    MEDIA_INFO_LOG("PullRecordsConflictProc end");
    return E_OK;
}

int32_t CloudMediaPhotosService::PullInsert(
    const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!pullDatas.empty(), E_OK, "PullInsert No need to pull insert.");
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>(trans);
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnDentryFileInsert Failed to get photoRefresh.");
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> insertFiles;

    int32_t ret;
    std::vector<CloudMediaPullDataDto> allPullDatas = pullDatas;
    PullRecordsConflictProc(allPullDatas, refreshAlbums, stats, failedRecords, photoRefresh);

    assetCompareDao_.SetRdbStore(MediaLibraryUnistoreManager::GetInstance().GetRdbStore());

    if (IsNeedRecoverSmartData()) {
        MEDIA_INFO_LOG("HDC Home Storage Switching detected, using duplicate detection logic");
        return ProcessHdcHomeStorageSwitching(allPullDatas, failedRecords, refreshAlbums, stats, photoRefresh);
    }

    for (auto insertData : allPullDatas) {
        MEDIA_DEBUG_LOG("PullInsert insert of record %{public}s", insertData.cloudId.c_str());
        ExtractEditDataCamera(insertData);
        ret = this->photosDao_.GetInsertParams(
            insertData, recordAnalysisAlbumMaps, recordAlbumMaps, refreshAlbums, insertFiles);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("PullInsert Get insert params failed %{public}d", ret);
            failedRecords.emplace_back(insertData.cloudId);
            continue;
        }
    }

    MEDIA_INFO_LOG("PullInsert insert %{public}zu, update %{public}d, delete %{public}d, map %{public}zu",
        insertFiles.size(),
        stats[StatsIndex::META_MODIFY_RECORDS_COUNT],
        stats[StatsIndex::DELETE_RECORDS_COUNT],
        recordAlbumMaps.size());
    ret = this->photosDao_.BatchInsertFile(recordAnalysisAlbumMaps, recordAlbumMaps, insertFiles, photoRefresh);
    photoRefresh->RefreshAlbumNoDateModified();
    photoRefresh->Notify();
    this->photosDao_.UpdateAlbumInternal(refreshAlbums);
    NotifyPhotoInserted(insertFiles);
    return ret;
}

void CloudMediaPhotosService::NotifyPhotoInserted(const std::vector<NativeRdb::ValuesBucket> &insertFiles)
{
    MEDIA_INFO_LOG("NotifyPhotoInserted enter %{public}zu", insertFiles.size());
    std::vector<std::string> cloudIds;
    bool ret = false;
    for (auto const &value : insertFiles) {
        if (value.HasColumn(Media::PhotoColumn::PHOTO_CLOUD_ID)) {
            std::string cloudId;
            NativeRdb::ValueObject obj;
            ret = value.GetObject(Media::PhotoColumn::PHOTO_CLOUD_ID, obj);
            if (!ret || obj.GetType() != NativeRdb::ValueObject::TypeId::TYPE_STRING) {
                MEDIA_ERR_LOG("NotifyPhotoInserted ret: %{public}d, type: %{public}d", ret, obj.GetType());
                continue;
            }
            obj.GetString(cloudId);
            MEDIA_INFO_LOG("NotifyPhotoInserted CloudId %{public}s", cloudId.c_str());
            MediaGallerySyncNotify::GetInstance().AddNotify(
                PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + cloudId, ChangeType::INSERT, cloudId);
        } else {
            MEDIA_ERR_LOG("NotifyPhotoInserted no CloudId");
        }
    }
    MediaGallerySyncNotify::GetInstance().FinalNotify();
}

void CloudMediaPhotosService::Notify(const std::string &uri, NotifyType type)
{
    auto watcher = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_INFO_LOG(watcher != nullptr, "watcher is nullptr");
    watcher->Notify(uri, type);
}

int32_t CloudMediaPhotosService::UpdateMetaStat(const std::vector<NativeRdb::ValuesBucket> &insertFiles,
    const std::vector<CloudMediaPullDataDto> &allPullDatas, const uint64_t dataFail)
{
    if (insertFiles.size() != allPullDatas.size()) {
        CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_ERROR_RDB, allPullDatas.size() - insertFiles.size());
    }
    if (!insertFiles.empty()) {
        CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_SUCCESS, insertFiles.size(), META_DL_INSERT);
    }
    if (dataFail > 0) {
        CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_ERROR_DATA, dataFail);
    }
    return E_OK;
}

int32_t CloudMediaPhotosService::CreateEntry(const std::vector<CloudMediaPullDataDto> &pullDatas,
    std::set<std::string> &refreshAlbums, std::vector<PhotosDto> &newData, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!pullDatas.empty(), E_OK, "CreateEntry No need to pull insert.");

    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> insertFiles;

    uint64_t dataFail = 0;
    int32_t uniqueId = 0;
    int32_t ret;
    std::vector<CloudMediaPullDataDto> allPullDatas = pullDatas;
    PullRecordsConflictProc(allPullDatas, refreshAlbums, stats, failedRecords, photoRefresh);
    ret = MediaLibraryAssetOperations::CreateAssetUniqueIds(MediaType::MEDIA_TYPE_IMAGE, allPullDatas.size(), uniqueId);
    if (ret != E_OK) {
        uniqueId = 0;
    }
    for (auto insertData : allPullDatas) {
        MEDIA_DEBUG_LOG("CreateEntry insert of record %{public}s", insertData.cloudId.c_str());
        int32_t mediaType =
            (insertData.basicFileType == FILE_TYPE_VIDEO) ? MediaType::MEDIA_TYPE_VIDEO : MediaType::MEDIA_TYPE_IMAGE;
        std::string extension = ScannerUtils::GetFileExtension(insertData.basicFileName);
        ret = MediaLibraryAssetOperations::CreateAssetPathById(uniqueId, mediaType, extension, insertData.localPath);
        uniqueId++;
        if (ret != E_OK) {
            MEDIA_ERR_LOG("CreateEntry Generate File Path err %{public}d", ret);
            dataFail++;
            continue;
        }
        ExtractEditDataCamera(insertData);
        ret = this->photosDao_.GetInsertParams(
            insertData, recordAnalysisAlbumMaps, recordAlbumMaps, refreshAlbums, insertFiles);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("CreateEntry Get insert params failed %{public}d", ret);
            dataFail++;
            continue;
        }
        PhotosDto dto;
        dto.cloudId = insertData.cloudId;
        if (!CloudMediaFileUtils::GetParentPathAndFilename(insertData.localPath, dto.path, dto.fileName)) {
            MEDIA_WARN_LOG("CreateEntry Failed to get parent path and real filename.");
        }
        dto.size = insertData.localSize;
        dto.mediaType = mediaType;
        dto.modifiedTime = insertData.attributesEditedTimeMs;
        // new data 不更新 originalCloudId (无)
        CloudMediaSyncUtils::FillPhotosDto(dto, insertData);
        MEDIA_DEBUG_LOG("CreateEntry NewData: %{public}s", dto.ToString().c_str());
        newData.emplace_back(dto);
    }
    this->UpdateMetaStat(insertFiles, allPullDatas, dataFail);
    return E_OK;
}

int32_t CloudMediaPhotosService::HandleRecord(const std::vector<std::string> &cloudIds,
    std::map<std::string, CloudMediaPullDataDto> &cloudIdRelativeMap, std::vector<PhotosDto> &newData,
    std::vector<PhotosDto> &fdirtyData, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnFetchRecords Failed to get photoRefresh.");

    std::set<std::string> refreshAlbums;
    std::vector<CloudMediaPullDataDto> insertPullDatas;
    uint64_t rdbFail = 0;
    int32_t ret = E_OK;
    for (auto &cloudId : cloudIds) {
        ChangeType changeType = ChangeType::INVAILD;

        CloudMediaPullDataDto pullData = cloudIdRelativeMap.at(cloudId);
        std::string dateAdded = pullData.localDateAdded;
        MEDIA_INFO_LOG("pulldata HandleRecord pullData: %{public}s", pullData.ToString().c_str());
        // 下行机已经有数据并且不是被删除
        if (pullData.localPath.empty() && !pullData.basicIsDelete) {
            insertPullDatas.emplace_back(pullData);
            stats[StatsIndex::NEW_RECORDS_COUNT]++;     // crash, stats的长度组要手动设置
        } else if (!pullData.localPath.empty()) {
            if (this->photosDeleteService_.IsClearCloudInfoOnly(pullData)) {
                ret = this->photosDeleteService_.PullClearCloudInfo(pullData, refreshAlbums, stats, photoRefresh);
                changeType = ChangeType::DELETE;
            } else if (pullData.basicIsDelete) {
                ret = PullDelete(pullData, refreshAlbums, photoRefresh);
                changeType = ChangeType::DELETE;
                stats[StatsIndex::DELETE_RECORDS_COUNT]++;
            } else {
                // 需要更新
                ret = PullUpdate(pullData, refreshAlbums, fdirtyData, stats, photoRefresh);
                changeType = ChangeType::UPDATE;
            }
        }
        if (ret == E_STOP) {
            MEDIA_ERR_LOG("HandleRecord stop sync cloudId: %{public}s, error: %{public}d", cloudId.c_str(), ret);
            return ret;
        }
        if (ret != E_OK) {
            MEDIA_ERR_LOG("HandleRecord cloudId: %{public}s, error: %{public}d", cloudId.c_str(), ret);
            if (ret == E_RDB) {
                rdbFail++;
                continue;
            }
            /* might need to specifiy which type error */
            failedRecords.emplace_back(pullData.cloudId);
            ret = E_OK;
        }
    }
    CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_ERROR_RDB, rdbFail);
    ret = CreateEntry(insertPullDatas, refreshAlbums, newData, stats, failedRecords, photoRefresh);
    photoRefresh->RefreshAlbumNoDateModified();
    photoRefresh->Notify();
    this->photosDao_.UpdateAlbumInternal(refreshAlbums);
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return ret;
}

void CloudMediaPhotosService::ConvertPullDataToPhotosDto(const CloudMediaPullDataDto &data, PhotosDto &dto)
{
    MEDIA_INFO_LOG("ConvertPullDataToPhotosDto enter: %{public}s", data.cloudId.c_str());
    int32_t mediaType =
        (data.basicFileType == FILE_TYPE_VIDEO) ? MediaType::MEDIA_TYPE_VIDEO : MediaType::MEDIA_TYPE_IMAGE;
    dto.cloudId = data.cloudId;
    if (!CloudMediaFileUtils::GetParentPathAndFilename(data.localPath, dto.path, dto.fileName)) {
        MEDIA_WARN_LOG("ConvertPullDataToPhotosDto Failed to get parent path and real filename.");
    }
    dto.size = data.localSize;
    dto.mediaType = mediaType;
    dto.modifiedTime = data.attributesEditedTimeMs;
    // new data 不更新 originalCloudId (无)
    CloudMediaSyncUtils::FillPhotosDto(dto, data.localPath, data.propertiesRotate,
        data.exifRotate, data.localThumbState);
    MEDIA_INFO_LOG("ConvertPullDataToPhotosDto NewData: %{public}s", dto.cloudId.c_str());
}

void CloudMediaPhotosService::SetPullDataFromPhotosPo(CloudMediaPullDataDto &pullData, const PhotosPo &photo)
{
    pullData.localFileId = photo.fileId.value_or(-1);
    pullData.localPath = photo.data.value_or("");
    pullData.localSize = photo.size.value_or(0);
    pullData.localMediaType = photo.mediaType.value_or(-1);
    pullData.localDateAdded = std::to_string(photo.dateAdded.value_or(-1));
    pullData.localDateModified = std::to_string(photo.dateModified.value_or(-1));
    pullData.localDirty = photo.dirty.value_or(-1);
    pullData.localPosition = photo.position.value_or(-1);
    pullData.localOwnerAlbumId = photo.ownerAlbumId.value_or(-1);
    pullData.localOrientation = photo.orientation.value_or(-1);
    pullData.localThumbState = photo.thumbStatus.value_or(-1);
    pullData.modifiedTime = photo.dateModified.value_or(-1);
    pullData.dateTaken = photo.dateTaken.value_or(0);
    pullData.localOriginalAssetCloudId = photo.originalAssetCloudId.value_or("");
    pullData.localExifRotate = photo.exifRotate.value_or(0);
    pullData.attributesSubtype = photo.subtype.value_or(0);
    pullData.attributesMovingPhotoEffectMode = photo.movingPhotoEffectMode.value_or(0);
    pullData.attributesOriginalSubtype = photo.originalSubtype.value_or(0);
    pullData.localDisplayName = photo.displayName.value_or("");
    pullData.localPhotosPoOp = photo;
}

int32_t CloudMediaPhotosService::OnFetchRecords(const std::vector<std::string> &cloudIds,
    std::map<std::string, CloudMediaPullDataDto> &cloudIdRelativeMap, std::vector<PhotosDto> &newData,
    std::vector<PhotosDto> &fdirtyData, std::vector<int32_t> &stats, std::vector<std::string> &failedRecords)
{
    MEDIA_INFO_LOG("OnFetchRecords: %{public}d.", static_cast<int32_t>(cloudIds.size()));
    std::vector<PhotosPo> photos;
    int32_t ret = this->commonDao_.QueryLocalByCloudId(cloudIds, {}, photos);
    if (ret != E_OK) {
        /* 打点 UpdateMetaStat(INDEX_DL_META_ERROR_RDB, recordIds.size());  */
        CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_ERROR_RDB, cloudIds.size() - photos.size());
        MEDIA_ERR_LOG("OnFetchRecords query error.");
        return E_CLOUDSYNC_RDB_QUERY_FAILED;
    }
    this->photosDao_.ClearAlbumMap();
    // cloudIdRelativeMap是待下行数据，photos是新机已有的数据
    for (auto it = cloudIdRelativeMap.begin(); it != cloudIdRelativeMap.end(); it++) {
        bool found = false;
        for (auto &photo : photos) {
            std::string cloudId = photo.cloudId.value_or("");
            if (cloudId.empty()) {
                MEDIA_WARN_LOG("OnFetchRecords cloudId: %{public}s.", cloudId.c_str());
                continue;
            }
            if (it->first != cloudId) {
                continue;
            }
            found = true;
            CloudMediaPullDataDto pullData = cloudIdRelativeMap.at(cloudId);
            // 根据本机的photo设置pullData（一些字段）
            SetPullDataFromPhotosPo(pullData, photo);
            pullData.cloudId = cloudId;

            cloudIdRelativeMap[cloudId] = pullData;
            MEDIA_DEBUG_LOG("OnFetchRecords CloudMediaPullData: %{public}s.", pullData.ToString().c_str());
        }
        if (!found) {
            MEDIA_WARN_LOG("OnFetchRecords Record need insert cloudId: %{public}s.", it->first.c_str());
            it->second.localPath = "";
        }
    }
    MEDIA_INFO_LOG("OnFetchRecords cloudIdRelativeMap: %{public}zu.", cloudIdRelativeMap.size());
    ret = HandleRecord(cloudIds, cloudIdRelativeMap, newData, fdirtyData, stats, failedRecords);
    HandleCloudDeleteRecord(cloudIdRelativeMap);
    return ret;
}

int32_t CloudMediaPhotosService::HandleCloudDeleteRecord(
    const std::map<std::string, CloudMediaPullDataDto> &cloudIdRelativeMap)
{
    std::vector<std::string> cloudDeleteFileIds;  // 云侧被删除的文件
    bool isValid = false;
    for (const auto &it : cloudIdRelativeMap) {
        CloudMediaPullDataDto pullData = it.second;
        isValid = !pullData.localPath.empty();
        isValid = isValid && pullData.basicIsDelete;
        CHECK_AND_CONTINUE(isValid);
        cloudDeleteFileIds.emplace_back(std::to_string(pullData.localFileId));
    }
    CHECK_AND_RETURN_RET_INFO_LOG(!cloudDeleteFileIds.empty(), E_OK, "CloudDeleteFileIds List Empty.");
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("BatchSelectFileDownload HandleCloudDeleteRecord size: %{public}zu", cloudDeleteFileIds.size());
    // 检查点 批量下载 云删除 通知应用 notify type 3
    BackgroundCloudBatchSelectedFileProcessor::TriggerCancelBatchDownloadProcessor(cloudDeleteFileIds, true);
#endif
    return E_OK;
}

int32_t CloudMediaPhotosService::OnDentryFileInsert(
    const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords)
{
    return PullInsert(pullDatas, failedRecords);
}

int32_t CloudMediaPhotosService::GetRetryRecords(std::vector<std::string> &cloudIds)
{
    int32_t ret = this->photosDao_.GetRetryRecords(cloudIds);
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetRetryRecords end, "
                   "ret: %{public}d, size: %{public}zu",
        ret,
        cloudIds.size());
    if (ret != E_OK) {
        std::string errMsg = "gallery data syncer pull file err";
        REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::QUERY_DATABASE, ret, errMsg});
    }
    return ret;
}

std::vector<PhotosDto> CloudMediaPhotosService::GetCheckRecords(const std::vector<std::string> &cloudIds)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetCheckRecords enter");
    // get & cache local photos data. cloudId -> photosDto
    std::vector<PhotosPo> photosPos = this->photosDao_.GetCheckRecords(cloudIds);
    std::vector<PhotosDto> result = this->processor_.GetPhotosDtos(photosPos);
    return result;
}

int32_t CloudMediaPhotosService::GetCreatedRecords(int32_t size, std::vector<PhotosPo> &createdRecords)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetCreatedRecords enter %{public}d", size);
    int32_t ret = this->photosDao_.GetCreatedRecords(size, createdRecords);
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetCreatedRecords end, ret: %{public}d, size: %{public}zu",
        ret,
        createdRecords.size());
    return ret;
}

int32_t CloudMediaPhotosService::GetMetaModifiedRecords(
    int32_t size, std::vector<PhotosPo> &modifiedRecords, int32_t dirtyType)
{
    int32_t ret = this->photosDao_.GetMetaModifiedRecords(size, modifiedRecords, dirtyType);
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetMetaModifiedRecords end,"
                    "ret: %{public}d, size: %{public}zu, dirtyType: %{public}d",
        ret,
        modifiedRecords.size(),
        dirtyType);
    return ret;
}

int32_t CloudMediaPhotosService::GetFileModifiedRecords(int32_t size, std::vector<PhotosPo> &modifiedRecords)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetFileModifiedRecords enter %{public}d", size);
    int32_t ret = this->photosDao_.GetFileModifiedRecords(size, modifiedRecords);
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetFileModifiedRecords end, ret: %{public}d, size: %{public}zu",
        ret,
        modifiedRecords.size());
    return ret;
}

std::vector<PhotosPo> CloudMediaPhotosService::GetDeletedRecords(int32_t size)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetDeletedRecords enter %{public}d", size);
    std::vector<PhotosPo> cloudRecordPoList;
    int32_t ret = this->photosDao_.GetDeletedRecordsAsset(size, cloudRecordPoList);
    if (ret < 0) {
        MEDIA_ERR_LOG("CloudMediaPhotosService::GetDeletedRecords failed to convert");
    }
    return cloudRecordPoList;
}

int32_t CloudMediaPhotosService::GetCopyRecords(int32_t size, std::vector<PhotosPo> &copyRecords)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::GetCopyRecords enter %{public}d", size);
    int32_t ret = this->photosDao_.GetCopyRecords(size, copyRecords);
    MEDIA_INFO_LOG(
        "CloudMediaPhotosService::GetCopyRecords end, ret: %{public}d, size: %{public}zu", ret, copyRecords.size());
    return ret;
}

int32_t CloudMediaPhotosService::OnCreateRecords(std::vector<PhotosDto> &records, int32_t &failedSize)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnCreateRecords Failed to get photoRefresh.");

    std::unordered_map<std::string, LocalInfo> localMap;
    int32_t ret = this->photosDao_.GetPhotoLocalInfo(records, localMap, PhotoColumn::MEDIA_ID);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "OnCreateRecords get local match info err %{public}d", ret);

    for (auto &record : records) {
        MEDIA_DEBUG_LOG("OnCreateRecords Record: %{public}s", record.ToString().c_str());
        int32_t err;
        if (record.isSuccess) {
            err = OnCreateRecordSuccess(record, localMap, photoRefresh);
        } else {
            err = OnRecordFailed(record, photoRefresh);
            CloudMediaDfxService::UpdateUploadDetailError(err);
            this->photosDao_.UpdateFailRecordsCloudId(record, localMap, photoRefresh);
            failedSize++;
        }
        if (err != E_OK) {
            this->photosDao_.InsertPhotoCreateFailedRecord(record.fileId);
            MEDIA_ERR_LOG(
                "OnCreateRecords create record fail: file path %{public}d, err %{public}d", record.fileId, err);
            if (record.isSuccess) {
                failedSize++;
            }
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return ret;
}

int32_t CloudMediaPhotosService::OnCreateRecordSuccess(
    const PhotosDto &record, const std::unordered_map<std::string, LocalInfo> &localMap,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t localId = record.localId;
    if (localId < 0) {
        MEDIA_ERR_LOG("OnCreateRecordSuccess invalid local id %{public}d", localId);
        return E_INVAL_ARG;
    }
    /* local file deleted */
    if (localMap.find(std::to_string(localId)) == localMap.end()) {
        MEDIA_INFO_LOG("OnCreateRecordSuccess local file is deleted %{public}d", localId);
        CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_DATA, 1);
        return E_OK;
    }

    int32_t ret = this->photosDao_.UpdateLocalAlbumMap(record.cloudId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnCreateRecordSuccess update local album map err %{public}d, %{public}d", ret, localId);
        CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_RDB, 1);
    }
    ret = this->photosDao_.UpdatePhotoCreatedRecord(record, localMap, photoRefresh);
    if (ret != 0) {
        MEDIA_ERR_LOG("OnCreateRecordSuccess update synced err %{public}d, %{public}d", ret, localId);
        CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_RDB, 1);
        return ret;
    }
    std::string fileId = std::to_string(record.fileId);
    MediaGallerySyncNotify::GetInstance().TryNotify(
        PhotoColumn::PHOTO_CLOUD_URI_PREFIX + fileId, ChangeType::UPDATE, fileId);
    return E_OK;
}

int32_t CloudMediaPhotosService::OnMdirtyRecords(std::vector<PhotosDto> &records, int32_t &failedSize)
{
    MEDIA_INFO_LOG("OnMdirtyRecords enter");
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnMdirtyRecords Failed to get photoRefresh.");
    int32_t ret = E_OK;
    for (auto &photo : records) {
        int32_t err;
        if (photo.isSuccess) {
            err = this->photosDao_.OnModifyPhotoRecord(photo, photoRefresh);
        } else {
            err = OnRecordFailed(photo, photoRefresh);
            CloudMediaDfxService::UpdateUploadDetailError(err);
        }
        if (err == E_OK) {
            Notify(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(photo.fileId) + "/" +
                       std::to_string(photo.dateAdded),
                NotifyType::NOTIFY_UPDATE);
        }
        if (err != E_OK) {
            failedSize++;
            MEDIA_ERR_LOG("OnMdirtyRecords error %{public}d", err);
            this->photosDao_.InsertPhotoModifyFailedRecord(photo.cloudId);
            CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_RDB, 1);
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosService::OnFdirtyRecords(std::vector<PhotosDto> &records, int32_t &failedSize)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnMdirtyRecords Failed to get photoRefresh.");

    std::unordered_map<std::string, LocalInfo> localMap;

    int32_t ret = this->photosDao_.GetPhotoLocalInfo(records, localMap, PhotoColumn::PHOTO_CLOUD_ID);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "OnFdirtyRecords get local match info err %{public}d", ret);

    for (auto &record : records) {
        MEDIA_DEBUG_LOG("OnFdirtyRecords Record: %{public}s", record.ToString().c_str());
        int32_t err;
        if (record.isSuccess) {
            err = OnFdirtyRecordSuccess(record, localMap, photoRefresh);
        } else {
            err = OnRecordFailed(record, photoRefresh);
            CloudMediaDfxService::UpdateUploadDetailError(err);
        }
        if (err != E_OK) {
            failedSize++;
            MEDIA_ERR_LOG("OnFdirtyRecords Error cloudId: %{public}s", record.cloudId.c_str());
            this->photosDao_.InsertPhotoModifyFailedRecord(record.cloudId);
        }
        if (ret == E_OK) {
            Notify(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(record.fileId) + "/" +
                       std::to_string(record.dateAdded),
                NotifyType::NOTIFY_UPDATE);
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosService::OnFdirtyRecordSuccess(
    const PhotosDto &record, const std::unordered_map<string, LocalInfo> &localMap,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("OnFdirtyRecordSuccess enter");
    std::string cloudId = record.cloudId;

    /* Fix me: might need a transaction to do an atomic update for files and their album maps */
    int32_t ret = this->photosDao_.UpdateLocalAlbumMap(cloudId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OnFdirtyRecordSuccess update local album map err %{public}d", ret);
        CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_RDB, 1);
        return ret;
    }
    if (record.metaDateModified == -1) {
        MEDIA_ERR_LOG("OnFdirtyRecordSuccess metaDateModified error");
        return E_INVAL_ARG;
    }
    ret = this->photosDao_.UpdateFdirtyVersion(record, photoRefresh);
    if (ret != E_OK) {
        CloudMediaDfxService::UpdateMetaStat(INDEX_UL_META_ERROR_RDB, 1);
    }
    return ret;
}

int32_t CloudMediaPhotosService::OnDeleteRecords(std::vector<PhotosDto> &records, int32_t &failSize)
{
    MEDIA_INFO_LOG("CloudMediaPhotosService::OnDeleteRecords");
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnMdirtyRecords Failed to get photoRefresh.");
    int32_t ret = E_OK;
    for (auto &photo : records) {
        int32_t err;
        MEDIA_DEBUG_LOG("CloudMediaPhotosService::OnDeleteRecords isSuccess: %{public}d", photo.isSuccess);
        if (photo.isSuccess) {
            err = this->photosDao_.OnDeleteRecordsAsset(photo, photoRefresh);
        } else {
            err = OnRecordFailed(photo, photoRefresh);
            CloudMediaDfxService::UpdateUploadDetailError(err);
        }
        if (err != E_OK) {
            this->photosDao_.InsertPhotoModifyFailedRecord(photo.cloudId);
            std::string errMsg = "on delete records update err : " + to_string(err);
            REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::DELETE_DATABASE, err, errMsg});
        }
        if (ret == E_OK) {
            Notify(PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(photo.fileId) + "/" +
                       std::to_string(photo.dateAdded),
                NotifyType::NOTIFY_REMOVE);
        }
        if (err != E_OK) {
            failSize++;
            MEDIA_ERR_LOG("delete record fail: cloud id: %{private}s", photo.dkRecordId.c_str());
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosService::OnCopyRecords(std::vector<PhotosDto> &records, int32_t &failedSize)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnMdirtyRecords Failed to get photoRefresh.");

    int32_t ret = E_OK;
    for (auto &photo : records) {
        int32_t err;
        MEDIA_INFO_LOG("OnCopyRecords photo: %{public}s", photo.ToString().c_str());
        if (photo.isSuccess) {
            err = this->photosDao_.OnCopyPhotoRecord(photo, photoRefresh);
        } else {
            err = OnRecordFailed(photo, photoRefresh);
            CloudMediaDfxService::UpdateUploadDetailError(err);
        }
        if (err != E_OK) {
            int32_t fileId = photo.fileId;
            if (err != E_OK) {
                failedSize++;
                this->photosDao_.InsertPhotoCopyFailedRecord(photo.fileId);
            }
            if (fileId == 0) {
                MEDIA_ERR_LOG("on copy record fail: file id %{private}d", fileId);
            }
        }
        if (err == E_SYNC_STOP || err == E_SYNC_FAILED_NETWORK_NOT_AVAILABLE || err == E_CLOUD_STORAGE_FULL ||
            err == E_STOP || err == E_BUSINESS_MODE_CHANGED) {
            ret = err;
        }
    }
    photoRefresh->RefreshAlbumNoDateModified(static_cast<NotifyAlbumType>(NotifyAlbumType::SYS_ALBUM |
        NotifyAlbumType::USER_ALBUM | NotifyAlbumType::SOURCE_ALBUM));
    photoRefresh->Notify();
    return ret;
}

int32_t CloudMediaPhotosService::OnRecordFailedErrorDetails(
    PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    ErrorType errorType = photo.errorType;
    if (photo.errorDetails.size() == 0 && errorType != ErrorType::TYPE_NOT_NEED_RETRY) {
        MEDIA_ERR_LOG("errorDetails is empty and errorType is invalid, errorType:%{public}d", errorType);
        return E_INVAL_ARG;
    } else if (photo.errorDetails.size() != 0) {
        auto errorDetailcode = static_cast<ErrorDetailCode>(photo.errorDetails[0].detailCode);
        if (errorDetailcode == ErrorDetailCode::SPACE_FULL) {
            /* Stop sync */
            return E_CLOUD_STORAGE_FULL;
        }
        if (errorDetailcode == ErrorDetailCode::BUSINESS_MODEL_CHANGE_DATA_UPLOAD_FORBIDDEN) {
            MEDIA_ERR_LOG("Business Mode Change, Upload Fail");
            /* Stop sync */
            return E_BUSINESS_MODE_CHANGED;
        }
        if (errorDetailcode == ErrorDetailCode::SAME_FILENAME_NOT_ALLOWED) {
            return HandleSameNameUploadFail(photo, photoRefresh);
        }
        if (errorDetailcode == ErrorDetailCode::CONTENT_NOT_FIND) {
            return HandleNoContentUploadFail(photo, photoRefresh);
        }
        if (errorType != ErrorType::TYPE_NOT_NEED_RETRY) {
            MEDIA_ERR_LOG("unknown error code record failed, errorDetailcode = %{public}d", errorDetailcode);
            return HandleDetailcode(errorDetailcode);
        }
        MEDIA_ERR_LOG("errorDetailcode = %{public}d, errorType = %{public}d, no need retry",
            errorDetailcode,
            static_cast<int32_t>(errorType));
        return E_STOP;
    } else {
        MEDIA_ERR_LOG("errorType = %{public}d, no need retry", static_cast<int32_t>(errorType));
        return E_STOP;
    }
    return E_UNKNOWN;
}

std::string CloudMediaPhotosService::GetCloudPath(const std::string &filePath)
{
    const std::string sandboxPrefix = "/storage/cloud";
    const std::string dfsPrefix = "/mnt/hmdfs/account/device_view/cloud";
    size_t pos = filePath.find(sandboxPrefix);
    bool isInValid = (pos != 0 || pos == std::string::npos);
    CHECK_AND_RETURN_RET_LOG(!isInValid, "", "GetCloudPath, invalid path %{public}s",
        MediaFileUtils::DesensitizePath(filePath).c_str());
    return dfsPrefix + filePath.substr(sandboxPrefix.length());
}

// failure scenario handler for repush duplicate resource, never return E_OK. E_DATA when process success.
int32_t CloudMediaPhotosService::HandleDuplicatedResource(const PhotosDto &photo)
{
    int32_t ret = this->photosDao_.RepushDuplicatedPhoto(photo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_RDB, "HandleDuplicatedResource, err: %{public}d, photo: %{public}s",
        ret, photo.ToString().c_str());
    return E_DATA;
}

// failure scenario handler for same cloud resource, never return E_OK. E_DATA when process success.
int32_t CloudMediaPhotosService::HandleSameCloudResource(const PhotosDto &photo)
{
    std::string path = photo.path;
    CHECK_AND_RETURN_RET_LOG(!path.empty(), E_INVAL_ARG, "HandleSameCloudResource data invalid, photo: %{public}s",
        photo.ToString().c_str());

    bool isLake = photo.fileSourceType == static_cast<int32_t>(FileSourceType::MEDIA_HO_LAKE);
    std::string localPath = isLake ? CloudMediaSyncUtils::GetLocalPath(path) : photo.storagePath;
    MEDIA_INFO_LOG("HandleSameCloudResource isLake is %{public}d, localPath is %{public}s",
        isLake, localPath.c_str());
    bool isFileExists = (access(localPath.c_str(), F_OK) == 0);
    CHECK_AND_EXECUTE(!isFileExists, this->photosDao_.RenewSameCloudResource(photo));
    CHECK_AND_RETURN_RET_LOG(!isFileExists, E_DATA, "HandleSameCloudResource push again %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    int32_t ret = this->photosDao_.DeleteLocalFileNotExistRecord(photo);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_RDB, "DeleteLocalFileNotExistRecord err: %{public}d, path: %{public}s",
        ret, MediaFileUtils::DesensitizePath(path).c_str());
    std::string cloudPath = GetCloudPath(path);
    CHECK_AND_RETURN_RET_LOG(!cloudPath.empty(), E_DATA, "cloudPath is empty, path: %{public}s",
        MediaFileUtils::DesensitizePath(path).c_str());
    bool isValid = (unlink(cloudPath.c_str()) >= 0);
    CHECK_AND_PRINT_LOG(isValid, "unlink fail, path: %{public}s, err: %{public}d",
        MediaFileUtils::DesensitizePath(cloudPath).c_str(), errno);
    CloudMediaSyncUtils::RemoveThmParentPath(path, PhotoColumn::FILES_CLOUD_DIR);
    return E_DATA;
}

int32_t CloudMediaPhotosService::OnRecordFailed(
    PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t serverErrorCode = photo.serverErrorCode;
    if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::NETWORK_ERROR)) {
        MEDIA_ERR_LOG("Network Error or Response Time Out");
        return E_SYNC_FAILED_NETWORK_NOT_AVAILABLE;
    } else if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::UID_EMPTY) ||
               (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::SWITCH_OFF)) {
        MEDIA_ERR_LOG("switch off or uid empty");
        return E_STOP;
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::INVALID_LOCK_PARAM) {
        MEDIA_ERR_LOG("Invalid lock param ");
        return E_STOP;
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RESPONSE_TIME_OUT) {
        MEDIA_ERR_LOG("on record failed response time out");
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RESOURCE_INVALID) {
        MEDIA_ERR_LOG("resource invalid");
        return HandleDuplicatedResource(photo);
    } else if (static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::RENEW_RESOURCE) {
        MEDIA_ERR_LOG("renew resource");
        return HandleSameCloudResource(photo);
    }
    if ((static_cast<ServerErrorCode>(serverErrorCode) == ServerErrorCode::ALBUM_NOT_EXIST)) {
        MEDIA_ERR_LOG("ALBUM NOT EXIST PhotosService");
        // Query AlbumId by file id, Set PhotoAlbum record dirty status to new
        return this->photosDao_.HandleNotExistAlbumRecord(photo);
    }
    return this->OnRecordFailedErrorDetails(photo, photoRefresh);
}

int32_t CloudMediaPhotosService::HandleNoContentUploadFail(
    const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("HandleNoContentUploadFail");
    std::string path = photo.path;
    std::string tmpPath = LakeFileUtils::GetAssetRealPath(path);
    int32_t ret = E_OK;
    if (access(tmpPath.c_str(), F_OK) == 0) {
        // 本地有图
        MEDIA_INFO_LOG("HandleNoContentUploadFail file found");
        ret = this->photosDao_.ClearCloudInfo(photo.cloudId, photoRefresh);
        return E_RDB;
    }
    ret = this->photosDao_.DeleteFileNotExistPhoto(path, photoRefresh);
    MEDIA_INFO_LOG("HandleNoContentUploadFail ret is %{public}d", ret);
    if (ret != E_OK) {
        return E_RDB;
    }
    CloudMediaSyncUtils::RemoveThmParentPath(path, "");
    return ret;
}

int32_t CloudMediaPhotosService::HandleDetailcode(ErrorDetailCode &errorCode)
{
    /* Only one record failed, not stop sync */
    return E_UNKNOWN;
}

int32_t CloudMediaPhotosService::HandleSameNameUploadFail(
    const PhotosDto &photo, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    std::string fileId = std::to_string(photo.fileId);
    std::string path = photo.data;
    int32_t ret = this->photosDao_.HandleSameNameRename(photo, photoRefresh);
    if (ret == E_OK) {
        std::string uri = PhotoColumn::PHOTO_CLOUD_URI_PREFIX + to_string(photo.fileId);
        MediaGallerySyncNotify::GetInstance().TryNotify(uri, ChangeType::UPDATE, to_string(photo.fileId));
        MediaGallerySyncNotify::GetInstance().FinalNotify();
    }
    return ret;
}

int32_t CloudMediaPhotosService::OnStartSync()
{
    auto cloudType = CloudMediaContext::GetInstance().GetCloudType();
    needRecoverSmartData_ = false;
    if (cloudType == static_cast<int32_t>(SouthDeviceType::SOUTH_DEVICE_CLOUD)) {
        if (IsNeedRecoverSmartData()) {
            needRecoverSmartData_ = true;
        }
    }

    CloudMediaDfxService::SyncStart("", 1);
    MediaGallerySyncNotify::GetInstance().NotifyProgressBegin();
    return this->photosDao_.ClearPhotoFailedRecords();
}

int32_t CloudMediaPhotosService::OnCompleteSync()
{
    CloudMediaDfxService::SyncEnd(0);
    MediaGallerySyncNotify::GetInstance().NotifyProgressEnd();
    return 0;
}

int32_t CloudMediaPhotosService::OnCompletePull(const MediaOperateResult &optRet)
{
    const std::string CLOUDSYNC_SWITCH_STATUS_KEY = "persist.kernel.cloudsync.switch_status";
    auto cloudSyncStatus = system::GetIntParameter(CLOUDSYNC_SWITCH_STATUS_KEY, 0);

    CHECK_AND_RETURN_RET_LOG(optRet.errorCode == 0, E_OK, "photo OnCompletePull errCode:%{public}d", optRet.errorCode);

    if ((cloudSyncStatus == 0) && needRecoverSmartData_ &&
        GetSmartDataUpdateState() > static_cast<int64_t>(UpdateSmartDataState::IDLE)) {
        int32_t ret = DoUpdateSmartDataAlbum();
        CHECK_AND_PRINT_LOG(ret == E_OK, "Failed to schedule DoUpdateSmartDataAlbum task");
        auto watch = MediaLibraryNotify::GetInstance();
        if (watch == nullptr) {
            MEDIA_ERR_LOG("Can not get MediaLibraryNotify Instance");
            return E_FAIL;
        }
        watch->Notify(PhotoAlbumColumns::ANALYSIS_ALBUM_URI_PREFIX, NotifyType::NOTIFY_UPDATE);
        SetSmartDataUpdateState(UpdateSmartDataState::IDLE);
    }
    return this->photosDao_.UpdatePhotoVisible();
}

int32_t CloudMediaPhotosService::OnCompletePush()
{
    return this->photosDao_.ClearPhotoFailedRecords();
}

int32_t CloudMediaPhotosService::OnCompleteCheck()
{
    MEDIA_INFO_LOG("enter CloudMediaPhotosService::OnCompleteCheck");
    return 0;
}

int32_t CloudMediaPhotosService::NotifyUploadErr(const int32_t errorCode, const std::string fileId)
{
    CloudSyncErrType errType;
    switch (errorCode) {
        case E_THM_SOURCE_BASIC + ENOENT: {
            errType = CloudSyncErrType::THM_NOT_FOUND;
            break;
        }
        case E_LCD_SOURCE_BASIC + ENOENT: {
            errType = CloudSyncErrType::LCD_NOT_FOUND;
            break;
        }
        case E_DB_SIZE_IS_ZERO: {
            errType = CloudSyncErrType::CONTENT_SIZE_IS_ZERO;
            break;
        }
        case E_LCD_IS_TOO_LARGE: {
            errType = CloudSyncErrType::LCD_SIZE_IS_TOO_LARGE;
            break;
        }
        case E_DB_ALBUM_NOT_FOUND: {
            errType = CloudSyncErrType::ALBUM_NOT_FOUND;
            break;
        }
        default: {
            errType = CloudSyncErrType::OTHER_ERROR;
        }
    }
    // not suport
    CHECK_AND_RETURN_RET_LOG(
        errType != CloudSyncErrType::OTHER_ERROR, -1, "not support error type, errorCode: %{public}d", errorCode);
    /* notify */
    MediaGallerySyncNotify::GetInstance().TryNotify(
        PhotoColumn::PHOTO_CLOUD_GALLERY_REBUILD_URI_PREFIX, static_cast<ChangeType>(errType), fileId);
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return E_OK;
}

int32_t CloudMediaPhotosService::ReportFailure(const ReportFailureDto &failureDto)
{
    int32_t apiCode = failureDto.apiCode;
    MEDIA_INFO_LOG("enter CloudMediaPhotosService::ReportFailure: %{public}s", failureDto.ToString().c_str());
    switch (apiCode) {
        case static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_CREATED_RECORDS):
            this->photosDao_.InsertPhotoCreateFailedRecord(failureDto.fileId);
            break;
        case static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_META_MODIFIED_RECORDS):
            this->photosDao_.InsertPhotoModifyFailedRecord(failureDto.cloudId);
            break;
        case static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_FILE_MODIFIED_RECORDS):
            this->photosDao_.InsertPhotoModifyFailedRecord(failureDto.cloudId);
            break;
        case static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_DELETED_RECORDS):
            this->photosDao_.InsertPhotoModifyFailedRecord(failureDto.cloudId);
            break;
        case static_cast<int32_t>(CloudMediaPhotoOperationCode::CMD_GET_COPY_RECORDS):
            this->photosDao_.RemovePhotoCopyFailedRecord(failureDto.fileId);
            break;
        default:
            break;
    }
    return this->NotifyUploadErr(failureDto.errorCode, std::to_string(failureDto.fileId));
}

void CloudMediaPhotosService::RefreshAnalysisAlbum(const std::string &cloudId)
{
    std::vector<std::string> analysisAlbumIds;
    int32_t ret = this->photosDao_.QueryAnalysisAlbum(cloudId, analysisAlbumIds);
    CHECK_AND_RETURN_LOG(!analysisAlbumIds.empty(), "RefreshAnalysisAlbum analysisAlbumIds is null");
    ret = photosDao_.UpdateAlbumReplacedSignal(analysisAlbumIds);
    MEDIA_INFO_LOG("RefreshAnalysisAlbum size %{public}zu, ret %{public}d", analysisAlbumIds.size(), ret);
}

int32_t CloudMediaPhotosService::RemoveLocalFile(const CloudMediaPullDataDto &pullData)
{
    std::string prefixCloud = "";

    // 根据是不是湖内文件来获得不同的路径
    std::string localPath;
    int32_t ret =  CloudMediaDaoUtils::GetLocalPathByPullData(pullData, localPath);
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(pullData.localPosition);
    if (!isLocal) {
        localPath = CloudMediaSyncUtils::GetCloudPath(pullData.localPath, prefixCloud);
    }
    MEDIA_INFO_LOG("RemoveLocalFile isLocal is %{public}d, localPath is %{public}s",
        isLocal, localPath.c_str());
    ret = unlink(localPath.c_str());
    CHECK_AND_PRINT_LOG(ret == E_OK,
        "unlink local failed, mergePath: %{public}s, ret: %{public}d.",
        MediaFileUtils::DesensitizePath(localPath).c_str(),
        ret);
    CloudMediaSyncUtils::RemoveThmParentPath(pullData.localPath, PhotoColumn::FILES_CLOUD_DIR);
    CloudMediaSyncUtils::RemoveEditDataParentPath(pullData.localPath);
    CloudMediaSyncUtils::RemoveMetaDataPath(pullData.localPath);
    CloudMediaSyncUtils::InvalidVideoCache(pullData.localPath);
    CloudMediaSyncUtils::RemoveEditDataPath(pullData.localPath);
    CloudMediaSyncUtils::RemoveMovingPhoto(pullData);
    CloudMediaSyncUtils::RemoveTransCodePath(pullData.localPath);
    return E_OK;
}

int32_t CloudMediaPhotosService::FindPhotoAlbum(CloudMediaPullDataDto &pullData)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!pullData.albumInfoOp.has_value(), E_OK, "has album info, skip.");
    std::string albumCloudId;
    std::string lPath;
    pullData.FindAlbumCloudId(albumCloudId);
    if (albumCloudId == SCREENSHOT_ALBUM_CLOUD_ID && pullData.IsVideoAsset()) {
        albumCloudId = "";
        lPath = AlbumPlugin::LPATH_SCREEN_RECORDS;
    }
    std::optional<PhotoAlbumPo> photoAlbumPoOp;
    int32_t ret =
        this->photosDao_.FindPhotoAlbumInCache(albumCloudId, lPath, pullData.propertiesSourcePath, photoAlbumPoOp);
    bool isValid = ret == E_OK && photoAlbumPoOp.has_value();
    MEDIA_INFO_LOG("FindPhotoAlbum ret: %{public}d, PhotoAlbumPo has_value: %{public}d, IsVideoAsset: %{public}d",
        ret,
        photoAlbumPoOp.has_value(),
        pullData.IsVideoAsset());
    CHECK_AND_RETURN_RET(isValid, ret);
    pullData.albumInfoOp = photoAlbumPoOp;
    return E_OK;
}

int32_t CloudMediaPhotosService::PullRecycleUpdate(
    CloudMediaPullDataDto &pullData, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = E_OK;
    if (this->photosDeleteService_.IsMoveOnlyCloudAssetIntoTrash(pullData)) {
        pullData.SetIsRecycleUpdated(true);
        ret = this->photosDeleteService_.CopyAndMoveCloudAssetToTrash(pullData, photoRefresh);
    } else if (this->photosDeleteService_.IsMoveOutFromTrash(pullData)) {
        pullData.SetIsRecycleUpdated(true);
        ret = this->photosDeleteService_.MoveOutTrashAndMergeWithSameAsset(photoRefresh, pullData);
    }
    return ret;
}

bool CloudMediaPhotosService::IsIgnoreMatch(
    const CloudMediaPullDataDto &mergeData, const KeyData &cloudKeyData, const KeyData &localKeyData)
{
    // Check: the switch (Photos.upload_status) is off.
    bool isValid = !mergeData.FindAlbumUploadStatus();
    CHECK_AND_RETURN_RET(isValid, false);
    // Scenario 1, cloud data is trashed, and local data is not trashed.
    bool isIgnoreMatchOfCloudRecycle = (cloudKeyData.dateTrashed != 0) && (localKeyData.dateTrashed == 0);
    // Scenario 2, cloud data is not trashed, and local data is trashed.
    bool isIgnoreMatchOfLocalRecycle = (cloudKeyData.dateTrashed == 0) && (localKeyData.dateTrashed != 0);
    bool isIgnoreMatch = isIgnoreMatchOfCloudRecycle || isIgnoreMatchOfLocalRecycle;
    CHECK_AND_PRINT_INFO_LOG(!isIgnoreMatch,
        "IsIgnoreMatch, recordId: %{public}s, "
        "isIgnoreMatchOfCloudRecycle: %{public}d, isIgnoreMatchOfLocalRecycle: %{public}d",
        mergeData.cloudId.c_str(),
        isIgnoreMatchOfCloudRecycle,
        isIgnoreMatchOfLocalRecycle);
    return isIgnoreMatch;
}

int32_t CloudMediaPhotosService::ProcessHdcHomeStorageSwitching(
    const std::vector<CloudMediaPullDataDto> &pullDatas,
    std::vector<std::string> &failedRecords,
    std::set<std::string> &refreshAlbums,
    std::vector<int32_t> &stats,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_INFO_LOG("ProcessHdcHomeStorageSwitching: processing %zu photos", pullDatas.size());

    auto maxFileId = assetCompareDao_.GetMaxFileIdBeforeCompare();
    std::vector<NativeRdb::ValuesBucket> insertFiles;
    std::vector<NativeRdb::ValuesBucket> updateFiles;
    std::vector<int32_t> cloudFileIdlist;
    std::vector<DuplicatePhotoInfo> cloudFileInfoList;
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;

    for (const auto &insertData : pullDatas) {
        ProcessSinglePhotoRecord(insertData, maxFileId, insertFiles, updateFiles, cloudFileIdlist,
            cloudFileInfoList, refreshAlbums, recordAnalysisAlbumMaps, recordAlbumMaps, failedRecords);
    }

    CleanDuplicatePhotosAssets(cloudFileInfoList);
    MEDIA_INFO_LOG("insert %zu, update %zu, map %zu", insertFiles.size(), updateFiles.size(), recordAlbumMaps.size());

    int32_t ret = ApplyBatchDatabaseChanges(insertFiles, updateFiles, cloudFileIdlist,
        recordAnalysisAlbumMaps, recordAlbumMaps, photoRefresh);
    if (ret != E_OK) {
        return ret;
    }

    FinalizeAndNotifyChanges(refreshAlbums, insertFiles, updateFiles, photoRefresh);
    return E_OK;
}

void CloudMediaPhotosService::ProcessSinglePhotoRecord(const CloudMediaPullDataDto &data, int64_t maxFileId,
    std::vector<NativeRdb::ValuesBucket> &inserts, std::vector<NativeRdb::ValuesBucket> &updates,
    std::vector<int32_t> &cloudFileIds, std::vector<DuplicatePhotoInfo> &duplicateInfos,
    std::set<std::string> &refreshAlbums, std::map<std::string, int> &analysisAlbumMaps,
    std::map<std::string, std::set<int>> &recordAlbumMaps, std::vector<std::string> &failedRecords)
{
    MEDIA_DEBUG_LOG("Processing record %s", data.cloudId.c_str());
    ExtractEditDataCamera(data);

    DuplicatePhotoInfo duplicateInfo = assetCompareDao_.FindDuplicatePhoto(data, maxFileId);
    if (duplicateInfo.isValid) {
        if (duplicateInfo.cleanFlag == 1) {
            MEDIA_INFO_LOG("Found duplicate photo with clean_flag=1, fileId: %d", duplicateInfo.fileId);
            if (ProcessDuplicatePhoto(data, duplicateInfo, refreshAlbums, updates) != E_OK) {
                MEDIA_ERR_LOG("ProcessDuplicatePhoto failed for %s", data.cloudId.c_str());
                failedRecords.emplace_back(data.cloudId);
            }
            cloudFileIds.push_back(duplicateInfo.fileId);
            duplicateInfos.push_back(duplicateInfo);
        } else if (duplicateInfo.cleanFlag == 0) {
            return ;
        }
    } else {
        if (photosDao_.GetInsertParams(data, analysisAlbumMaps, recordAlbumMaps, refreshAlbums, inserts) != E_OK) {
            MEDIA_ERR_LOG("GetInsertParams failed for %s", data.cloudId.c_str());
            failedRecords.emplace_back(data.cloudId);
        }
    }
}

int32_t CloudMediaPhotosService::ApplyBatchDatabaseChanges(
    std::vector<NativeRdb::ValuesBucket> &inserts, std::vector<NativeRdb::ValuesBucket> &updates,
    const std::vector<int32_t> &cloudFileIds, std::map<std::string, int> &analysisAlbumMaps,
    std::map<std::string, std::set<int>> &recordAlbumMaps,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = E_OK;
    if (!updates.empty()) {
        ret = photosDao_.BatchUpdateFile(analysisAlbumMaps, recordAlbumMaps, updates, photoRefresh, cloudFileIds);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("BatchUpdateFile failed %d", ret);
            return ret;
        }
    }

    if (!inserts.empty()) {
        ret = this->photosDao_.BatchInsertFile(analysisAlbumMaps, recordAlbumMaps, inserts, photoRefresh);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("BatchInsertFile failed %d", ret);
            return ret;
        }
    }
    return ret;
}

void CloudMediaPhotosService::FinalizeAndNotifyChanges(std::set<std::string> &refreshAlbums,
    const std::vector<NativeRdb::ValuesBucket> &inserts, const std::vector<NativeRdb::ValuesBucket> &updates,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    photoRefresh->RefreshAlbumNoDateModified();
    photoRefresh->Notify();
    this->photosDao_.UpdateAlbumInternal(refreshAlbums);

    if (!inserts.empty()) {
        NotifyPhotoInserted(inserts);
    }
    if (!updates.empty()) {
        NotifyPhotoInserted(updates); // Assuming updates should also trigger insertion notifications
    }

    this->photosDao_.UpdateMediaAnalysisHdcData();
    return ;
}

int32_t CloudMediaPhotosService::CleanDuplicatePhotosAssets(const std::vector<DuplicatePhotoInfo> &cloudFileInfolist)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_ERR, "CleanDuplicatePhotosAssets failed. rdbStore is null");

    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;

    for (const auto &info : cloudFileInfolist) {
        if (info.real_lcd_visit_time != REAL_LCD_VISIT_TIME_INVALID) {
            continue;
        }
        std::string fileIdStr = std::to_string(info.fileId);
        fileIds.push_back(fileIdStr);
        paths.push_back(info.data);
        dateTakens.push_back("");

        // Delete edit data, metadata, and clean gallery dentry file for each path
        CHECK_AND_PRINT_LOG(CloudMediaAssetManager::DeleteEditdata(info.data) == OHOS::Media::E_OK, "Delete edit err.");
#ifdef META_RECOVERY_SUPPORT
        CHECK_AND_PRINT_LOG(MediaLibraryMetaRecovery::DeleteMetaDataByPath(info.data) == E_OK,
            "DeleteMetaDataByPath error.");
#endif
        CloudSyncManager::GetInstance().CleanGalleryDentryFile(info.data);
    }
    // Delete thumbnails and ASTC files
    MEDIA_INFO_LOG("Deleting thumbnail files.");
    CHECK_AND_PRINT_LOG(ThumbnailService::GetInstance()->BatchDeleteThumbnailDirAndAstc(PhotoColumn::PHOTOS_TABLE,
        fileIds, paths, dateTakens), "BatchDeleteThumbnailDirAndAstc error.");
    return E_OK;
}

int32_t CloudMediaPhotosService::ProcessDuplicatePhoto(const CloudMediaPullDataDto &pullData,
    const DuplicatePhotoInfo &duplicateInfo, std::set<std::string> &refreshAlbums,
    std::vector<NativeRdb::ValuesBucket> &updateFiles)
{
    MEDIA_INFO_LOG("ProcessDuplicatePhoto: replacing fileId %{public}d with cloudId %{public}s",
        duplicateInfo.fileId, pullData.cloudId.c_str());

    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> tempInsertFiles;
    int32_t ret = this->photosDao_.GetInsertParams(
        pullData, recordAnalysisAlbumMaps, recordAlbumMaps, refreshAlbums, tempInsertFiles);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetInsertParams failed for duplicate photo %{public}d", ret);
        return ret;
    }

    if (tempInsertFiles.empty()) {
        MEDIA_ERR_LOG("No insert files generated for duplicate photo");
        return E_INVAL_ARG;
    }

    for (auto &values : tempInsertFiles) {
        values.PutInt(PhotoColumn::MEDIA_ID, duplicateInfo.fileId);
        PhotosFieldIterator::ResetLocalFields(values);
        updateFiles.push_back(values);
        MEDIA_INFO_LOG("Added duplicate photo for update: fileId %{public}d", duplicateInfo.fileId);
    }

    return E_OK;
}
}  // namespace OHOS::Media::CloudSync
