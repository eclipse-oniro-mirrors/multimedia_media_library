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

#define MLOG_TAG "Media_Cloud_Service"

#include "cloud_media_share_photos_service.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "albums_refresh_manager.h"
#include "cloud_media_sync_utils.h"
#include "dataobs_mgr_changeinfo.h"
#include "media_column.h"
#include "cloud_media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "photos_dto.h"
#include "medialibrary_notify.h"
#include "media_gallery_sync_notify.h"
#include "cloud_media_sync_const.h"
#include "cloud_media_dfx_service.h"
#include "cloud_file_error.h"
#include "cloud_media_context.h"

// LCOV_EXCL_START
using ChangeType = OHOS::AAFwk::ChangeInfo::ChangeType;
namespace OHOS::Media::CloudSync {
int32_t CloudMediaSharePhotosService::OnFetchRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
    std::vector<PhotosDto> &newData, std::vector<PhotosDto> &fdirtyData, std::vector<int32_t> &stats,
    std::vector<std::string> &failedRecords)
{
    int32_t ret = FindLocalPhotoInfo(pullDataList);
    this->photosDao_.ClearAlbumMap();
    MEDIA_INFO_LOG("OnFetchRecords pullDataList: %{public}zu.", pullDataList.size());
    CloudMediaPullDataHandleDto handleDto;
    handleDto.stats = stats;
    ret = HandleRecords(pullDataList, handleDto);
    newData = handleDto.newData;
    fdirtyData = handleDto.fdirtyData;
    stats = handleDto.stats;
    failedRecords = handleDto.failedRecords;
    HandleCloudDeleteRecord(pullDataList);
    return ret;
}

int32_t CloudMediaSharePhotosService::OnDentryFileInsert(
    const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords)
{
    return PullInsert(pullDatas, failedRecords);
}

int32_t CloudMediaSharePhotosService::FindLocalPhotoInfo(std::vector<CloudMediaPullDataDto> &pullDataList)
{
    CHECK_AND_RETURN_RET_LOG(!pullDataList.empty(), E_OK, "pullDataList empty");

    std::vector<std::string> cloudIds;
    this->GetAllCloudIds(pullDataList, cloudIds);
    CHECK_AND_RETURN_RET_LOG(!cloudIds.empty(), E_OK, "cloudIds empty");

    std::vector<PhotosPo> photoInfoList;
    int32_t ret = this->commonDao_.QueryLocalByCloudId(cloudIds, {}, photoInfoList);
    CHECK_AND_RETURN_RET_LOG(!photoInfoList.empty(), E_OK, "photoInfoList empty");

    this->MergePhotoInfoIntoPullData(pullDataList, photoInfoList);
    return E_OK;
}

int32_t CloudMediaSharePhotosService::GetAllCloudIds(std::vector<CloudMediaPullDataDto> &pullDataList,
    std::vector<std::string> &cloudIds)
{
    for (auto &pullData : pullDataList) {
        CHECK_AND_CONTINUE_ERR_LOG(!pullData.cloudId.empty(), "pullData cloudId empty");
        cloudIds.emplace_back(pullData.cloudId);
    }
    return E_OK;
}

int32_t CloudMediaSharePhotosService::MergePhotoInfoIntoPullData(
    std::vector<CloudMediaPullDataDto> &pullDataList, const std::vector<PhotosPo> &photoInfoList)
{
    std::string cloudId;
    std::unordered_map<std::string, PhotosPo> photoInfoMap;
    for (auto &photoInfo : photoInfoList) {
        cloudId = photoInfo.cloudId.value_or("");
        CHECK_AND_CONTINUE_ERR_LOG(!cloudId.empty(), "cloudId empty");
        photoInfoMap[cloudId] = photoInfo;
    }
    
    for (auto &pullData : pullDataList) {
        cloudId = pullData.cloudId;
        CHECK_AND_CONTINUE_ERR_LOG(!cloudId.empty(), "cloudId empty");

        auto it = photoInfoMap.find(cloudId);
        CHECK_AND_CONTINUE(it != photoInfoMap.end());

        this->photosService_.SetPullDataFromPhotosPo(pullData, it->second);
    }
    return E_OK;
}

int32_t CloudMediaSharePhotosService::HandleRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
    CloudMediaPullDataHandleDto &handleDto)
{
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Photos OnFetchRecords Failed to get photoRefresh.");

    uint64_t rdbFail = 0;
    int32_t ret = E_OK;
    NotifyType notifyType = NotifyType::NOTIFY_INVALID;
    for (auto &pullData : pullDataList) {
        MEDIA_INFO_LOG("pullData: %{public}s, sceneType(share): %{public}d", pullData.ToString().c_str(),
            CloudMediaContext::GetInstance().GetSceneType());
        ret = this->HandleUpdateOrDeleteRecord(pullData, handleDto, notifyType);
        if (ret == FileManagement::E_STOP) {
            MEDIA_ERR_LOG("HandleRecord stop sync cloudId: %{public}s, error: %{public}d",
                pullData.cloudId.c_str(), ret);
            return ret;
        }
        if (ret != E_OK) {
            MEDIA_ERR_LOG("HandleRecord cloudId: %{public}s, error: %{public}d", pullData.cloudId.c_str(), ret);
            if (ret == E_RDB) {
                rdbFail++;
                continue;
            }
            ret = E_OK;
        }
    }
    CloudMediaDfxService::UpdateMetaStat(INDEX_DL_META_ERROR_RDB, rdbFail);
    HandleMergeOrNewRecords(pullDataList, handleDto, photoRefresh);
    photoRefresh->RefreshAlbumNoDateModified();
    photoRefresh->Notify();
    AlbumsRefreshManager::GetInstance().SendNotifyInfoOfAssetAndAlbum(notifyType, {}, handleDto.refreshAlbums);
    MediaGallerySyncNotify::GetInstance().FinalNotify();
    return ret;
}

int32_t CloudMediaSharePhotosService::HandleUpdateOrDeleteRecord(
    const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto, NotifyType &notifyType)
{
    const bool hasLocalInfo = pullData.localPhotosPoOp.has_value();
    CHECK_AND_RETURN_RET(hasLocalInfo, E_OK);

    const bool isUpdate = hasLocalInfo && !pullData.basicIsDelete;
    const bool isDelete = hasLocalInfo && pullData.basicIsDelete;
    int32_t ret = E_OK;
    if (isUpdate) {
        ret = this->PullUpdate(pullData, handleDto);
        notifyType = NotifyType::NOTIFY_UPDATE;
    } else if (isDelete) {
        ret = this->PullDelete(pullData, handleDto);
        notifyType = NotifyType::NOTIFY_REMOVE;
        handleDto.stats[StatsIndex::DELETE_RECORDS_COUNT]++;
    }
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HandleRecord cloudId: %{public}s, error: %{public}d", pullData.cloudId.c_str(), ret);
        handleDto.failedRecords.emplace_back(pullData.cloudId);
    }
    return ret;
}

int32_t CloudMediaSharePhotosService::HandleMergeOrNewRecords(const std::vector<CloudMediaPullDataDto> &pullDataList,
    CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    // Find records for new or merge.
    std::vector<CloudMediaPullDataDto> newOrMergePullDataList;
    for (const auto &pullData : pullDataList) {
        CHECK_AND_EXECUTE(pullData.localPhotosPoOp.has_value(), newOrMergePullDataList.emplace_back(pullData));
    }
    CHECK_AND_RETURN_RET(!newOrMergePullDataList.empty(), E_OK);

    // Handle merge records.
    this->HandleMergeRecords(newOrMergePullDataList, handleDto, photoRefresh);
    
    // Find records for new records.
    std::vector<CloudMediaPullDataDto> newPullDataList;
    for (const auto &pullData : newOrMergePullDataList) {
        CHECK_AND_EXECUTE(pullData.localPhotosPoOp.has_value(), newPullDataList.emplace_back(pullData));
    }

    // Handle new records.
    this->HandleNewRecords(newPullDataList, handleDto, photoRefresh);
    return E_OK;
}

// merge cloud with local by album,name,size,orientaion,creatorId.
int32_t CloudMediaSharePhotosService::HandleMergeRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
    CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    MEDIA_ERR_LOG("To merge cloud with local by album,name,size,orientaion,creatorId.");
    return E_OK;
}

int32_t CloudMediaSharePhotosService::HandleNewRecords(std::vector<CloudMediaPullDataDto> &pullDataList,
    CloudMediaPullDataHandleDto &handleDto, std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> &photoRefresh)
{
    int32_t ret = this->photosService_.CreateEntry(pullDataList, handleDto.refreshAlbums,
        handleDto.newData, handleDto.stats, handleDto.failedRecords, photoRefresh);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "CreateEntry failed, ret: %{public}d.", ret);
    return E_OK;
}

int32_t CloudMediaSharePhotosService::HandleCloudDeleteRecord(std::vector<CloudMediaPullDataDto> &pullDataList)
{
    std::map<std::string, CloudMediaPullDataDto> deletedPullDataList;
    bool isValid = false;
    for (const auto &pullData : pullDataList) {
        isValid = pullData.localPhotosPoOp.has_value();
        isValid = isValid & pullData.basicIsDelete;
        CHECK_AND_EXECUTE(!isValid, deletedPullDataList[pullData.cloudId] = pullData);
    }
    CHECK_AND_RETURN_RET(!deletedPullDataList.empty(), E_OK);

    return this->photosService_.HandleCloudDeleteRecord(deletedPullDataList);
}

int32_t CloudMediaSharePhotosService::PullUpdate(
    const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto)
{
    std::set<std::string> &refreshAlbums = handleDto.refreshAlbums;
    std::vector<PhotosDto> &fdirtyData = handleDto.fdirtyData;
    std::vector<int32_t> &stats = handleDto.stats;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    const std::string cloudId = pullData.cloudId;
    MEDIA_DEBUG_LOG("Update cloudId: %{public}s.", cloudId.c_str());
    CHECK_AND_RETURN_RET_INFO_LOG(!CloudMediaSyncUtils::IsLocalDirty(pullData.localDirty, false),
        E_OK, "local record dirty, ignore cloud update");
    bool mtimeChanged = false;
    int32_t ret = this->photosService_.IsMtimeChanged(pullData, mtimeChanged);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("cloudId: %{public}s get mtime changed failed, ret: %{public}d.", cloudId.c_str(), ret);
    }
    CHECK_AND_RETURN_RET_LOG(pullData.localPhotosPoOp.has_value(), E_ERR, "localPhotosPoOp has no value");
    const PhotosPo &photoInfo = pullData.localPhotosPoOp.value();
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(photoInfo.position.value_or(-1));
    if (isLocal && mtimeChanged) {
        std::string fileStoragePath = CloudMediaSyncUtils::FindFileStoragePath(photoInfo);
        if (CloudMediaFileUtils::LocalWriteOpen(fileStoragePath)) {
            ret = this->photosDao_.SetRetry(cloudId);
            if (ret != E_OK) {
                std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
                REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
            }
            return ret;
        }
    }
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
    this->photosService_.Notify(PhotoColumn::PHOTO_URI_PREFIX + std::to_string(pullData.localFileId),
        NotifyType::NOTIFY_UPDATE);

    refreshAlbums.emplace(std::to_string(pullData.localOwnerAlbumId));
    this->photosService_.ExtractEditDataCamera(pullData);
    if (mtimeChanged && (updateCount != stats[StatsIndex::FILE_MODIFY_RECORDS_COUNT])) {
        this->photosService_.ClearLocalData(pullData, fdirtyData);
    } else {
        this->photosService_.PullUpdateEndWithNoFdirty(pullData, fdirtyData);
    }
    return E_OK;
}

int32_t CloudMediaSharePhotosService::PullDelete(
    const CloudMediaPullDataDto &pullData, CloudMediaPullDataHandleDto &handleDto)
{
    std::set<std::string> &refreshAlbums = handleDto.refreshAlbums;
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>();
    std::string cloudId = pullData.cloudId;
    std::string localPath = pullData.localPath;
    CHECK_AND_RETURN_RET_INFO_LOG(!cloudId.empty(), E_OK, "cloudId is empty, ignore cloud delete");
    bool isLocal = CloudMediaSyncUtils::FileIsLocal(pullData.localPosition);
    MEDIA_INFO_LOG("Delete cloudId: %{public}s, isLocal: %{public}d, pullData: %{public}s",
        cloudId.c_str(),
        isLocal,
        pullData.ToString().c_str());
    if (isLocal && CloudMediaSyncUtils::IsLocalDirty(pullData.localDirty, true)) {
        MEDIA_ERR_LOG("local record dirty, ignore cloud delete");
        return this->photosDao_.ClearCloudInfo(cloudId, photoRefresh);
    }
    int32_t ret = E_OK;
    if (isLocal && CloudMediaFileUtils::LocalWriteOpen(localPath)) {
        ret = this->photosDao_.SetRetry(cloudId);
        if (ret != E_OK) {
            std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
            REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
        }
        return ret;
    }
    ret = this->photosDao_.DeleteLocalByCloudId(pullData, photoRefresh);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("delete in rdb failed, ret:%{public}d", ret);
        ret = this->photosDao_.SetRetry(cloudId);
        if (ret != E_OK) {
            std::string errMsg = "update retry flag failed, ret = " + to_string(ret);
            REPORT_SYNC_FAULT({FaultScenario::CLOUD_SYNC_PULL, FaultType::MODIFY_DATABASE, ret, errMsg});
        }
        return ret;
    }
    std::string notifyUri = PhotoColumn::PHOTO_GALLERY_CLOUD_URI_PREFIX + std::to_string(pullData.localFileId) + '/' +
                            pullData.localDateAdded;
    MediaGallerySyncNotify::GetInstance().TryNotify(
        notifyUri, ChangeType::DELETE, std::to_string(pullData.localFileId));
    this->photosService_.Notify(PhotoColumn::PHOTO_URI_PREFIX + std::to_string(pullData.localFileId),
        NotifyType::NOTIFY_REMOVE);
    refreshAlbums.emplace(std::to_string(pullData.localOwnerAlbumId));
    this->photosService_.RemoveLocalFile(pullData);
    return E_OK;
}

int32_t CloudMediaSharePhotosService::PullInsert(
    const std::vector<CloudMediaPullDataDto> &pullDatas, std::vector<std::string> &failedRecords)
{
    CHECK_AND_RETURN_RET_INFO_LOG(!pullDatas.empty(), E_OK, "PullInsert No need to pull insert.");
    std::shared_ptr<TransactionOperations> trans = make_shared<TransactionOperations>(__func__);
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> photoRefresh =
        std::make_shared<AccurateRefresh::AssetAccurateRefresh>(trans);
    CHECK_AND_RETURN_RET_LOG(
        photoRefresh != nullptr, E_RDB_STORE_NULL, "Share Photos OnDentryFileInsert Failed to get photoRefresh.");
    std::set<std::string> refreshAlbums;
    std::vector<int32_t> stats = {0, 0, 0, 0, 0};
    std::map<std::string, int> recordAnalysisAlbumMaps;
    std::map<std::string, std::set<int>> recordAlbumMaps;
    std::vector<NativeRdb::ValuesBucket> insertFiles;

    int32_t ret;
    for (auto insertData : pullDatas) {
        MEDIA_DEBUG_LOG("PullInsert insert of record %{public}s", insertData.cloudId.c_str());
        this->photosService_.ExtractEditDataCamera(insertData);
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
    this->photosService_.NotifyPhotoInserted(insertFiles, refreshAlbums);
    return ret;
}
}  // namespace OHOS::Media::CloudSync
// LCOV_EXCL_STOP
