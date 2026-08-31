/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Service"

#include "media_share_assets_service.h"

#include <thread>

#include "cloud_media_asset_manager.h"
#include "hi_audit.h"
#include "media_assets_utils.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_tracer.h"
#include "notify/asset_batch_notify.h"
#include "operation/photo_file_operation.h"
#include "parameters.h"
#include "thumbnail_service.h"

namespace OHOS::Media {

using namespace FileManagement::CloudSync;

constexpr int32_t CYCLE_NUMBER = 5000;
constexpr int32_t SLEEP_FOR_DELETE = 600;
const std::string START_QUERY_ZERO = "0";

int32_t MediaShareAssetsService::MarkShareAssetsToRemove()
{
    MediaLibraryTracer tracer;
    tracer.Start("CLOUD_EXIT: MarkShareAssetsToRemove");
    int32_t cycleNumber = 0;
    std::string lastFileId = START_QUERY_ZERO;
    std::vector<std::string> updateFileIds;
    int32_t ret = E_OK;
    MEDIA_INFO_LOG("begin MarkShareAssetsToRemove");
    AssetBatchNotify batchAssetNotify;
    bool hasRecords = this->shareAssetsDao_.HasShareAssetToMarkDeleted(updateFileIds, lastFileId);
    while (hasRecords && cycleNumber++ <= CYCLE_NUMBER) {
        ret = this->shareAssetsDao_.MarkDeletedAndClearCloudInfo(updateFileIds);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "MarkDeletedAndClearCloudInfo failed, ret: %{public}d", ret);
        lastFileId = updateFileIds.back();
        batchAssetNotify.TryNotifyAssetsChange(updateFileIds);
        hasRecords = this->shareAssetsDao_.HasShareAssetToMarkDeleted(updateFileIds, lastFileId);
    }
    batchAssetNotify.FinalNotifyAssetsChange();
    MEDIA_INFO_LOG("end MarkShareAssetsToRemove, ret: %{public}d", ret);
    return ret;
}

bool MediaShareAssetsService::RemoveShareAssets(const std::vector<PhotosPo> &photoInfoList)
{
    MediaLibraryTracer tracer;
    tracer.Start("CLOUD_EXIT: RemoveShareAssets");
    PhotoFileOperation fileOperation;
    std::vector<std::string> fileIds;
    std::vector<std::string> paths;
    std::vector<std::string> dateTakens;
    for (const PhotosPo &photoInfo : photoInfoList) {
        int32_t ret = fileOperation.DeletePhoto(photoInfo);
        CHECK_AND_CONTINUE_ERR_LOG(ret == E_OK, "DeletePhoto fail, ret: %{public}d", ret);
        fileIds.emplace_back(std::to_string(photoInfo.fileId.value_or(-1)));
        paths.emplace_back(photoInfo.data.value_or(""));
        dateTakens.emplace_back(std::to_string(photoInfo.dateTaken.value_or(0)));
    }

    CHECK_AND_RETURN_RET_LOG(!fileIds.empty(), true, "no photo deleted in this batch.");
    int32_t ret = this->shareAssetsDao_.DeleteShareAssets(fileIds);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, false, "DeleteShareAssets failed!");
    // 批量删除缩略图
    CHECK_AND_PRINT_LOG(ThumbnailService::GetInstance()->BatchDeleteThumbnailDirAndAstc(
        PhotoColumn::PHOTOS_TABLE, fileIds, paths, dateTakens), "DeleteThumbnailDirAndAstc error.");
    return true;
}

void MediaShareAssetsService::RemoveShareAssetsTask()
{
    MediaLibraryTracer tracer;
    tracer.Start("CLOUD_EXIT: RemoveShareAssetsTask");
    TimeLogger timeLogger;
    timeLogger.Start("RemoveShareAssetsTask");

    // 防重入：已有异步清理任务在进行时直接返回，避免重复清理
    std::unique_lock<std::mutex> lock(updateMutex_, std::defer_lock);
    CHECK_AND_RETURN_WARN_LOG(lock.try_lock(),
        "share assets is cleaning, skipping this operation");

    std::vector<PhotosPo> photoInfoList;

    int32_t cycleNumber = 0;
    while (cycleNumber++ <= CYCLE_NUMBER) {
        int32_t ret = this->shareAssetsDao_.GetShareAssetToRemove(photoInfoList);
        if (ret != E_OK || photoInfoList.empty()) {
            MEDIA_WARN_LOG("GetShareAssetToRemove failed or no assets left, ret: %{public}d", ret);
            break;
        }

        if (!RemoveShareAssets(photoInfoList)) {
            MEDIA_ERR_LOG("RemoveShareAssets failed, exiting operation.");
            break;
        }

        MEDIA_INFO_LOG("Processed batch. loop: %{public}d, asset count: %{public}zu",
            cycleNumber, photoInfoList.size());

        photoInfoList.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_FOR_DELETE));
    }
}

void MediaShareAssetsService::StartRemoveShareAssetsTask()
{
    MediaLibraryTracer tracer;
    tracer.Start("CLOUD_EXIT: StartRemoveShareAssetsTask");
    std::thread deleteThread(&MediaShareAssetsService::RemoveShareAssetsTask, this);
    deleteThread.detach();
}

void MediaShareAssetsService::CleanShareAssetsDownloadTasksTable()
{
    MEDIA_ERR_LOG("refer to CleanDownloadTasksTable() and CancelDownloadCloudAsset()");
}

int32_t MediaShareAssetsService::RemoveShareAssetsInner()
{
    int32_t ret = MarkShareAssetsToRemove();
    CHECK_AND_PRINT_LOG(ret == E_OK, "MarkShareAssetsToRemove failed. ret: %{public}d", ret);

    ret = this->shareAssetsDao_.DeleteShareAlbums();
    CHECK_AND_PRINT_LOG(ret == E_OK, "DeleteEmptyCloudAlbums failed. ret: %{public}d", ret);

    StartRemoveShareAssetsTask();
    return E_OK;
}

void MediaShareAssetsService::BeforeRemoveShareAlbumAndAsset()
{
    HiAudit::GetInstance().WriteForCloudExit(MediaLibraryBundleManager::GetInstance()->GetClientBundleName(),
        static_cast<int32_t>(CloudMediaRetainType::SHARE_RETAIN_FORCE), "start");

    // 主动停止端云同步
    this->cloudShareSyncFoundationService_.StopSync();

    // 备份/恢复需要特殊处理，等待备份和恢复完成再清除;
    CloudMediaAssetManager::WaitIfBackUpingOrRestoring();

    // 清除批量下载任务列表
    CleanShareAssetsDownloadTasksTable();
}

void MediaShareAssetsService::AfterRemoveShareAlbumAndAsset()
{
    // 重置端云同步水位
    this->cloudShareSyncFoundationService_.ResetCursor();

    // 尝试启动端云同步
    this->cloudShareSyncFoundationService_.TryToStartSync();

    HiAudit::GetInstance().WriteForCloudExit(MediaLibraryBundleManager::GetInstance()->GetClientBundleName(),
        static_cast<int32_t>(CloudMediaRetainType::SHARE_RETAIN_FORCE), "success");
}

int32_t MediaShareAssetsService::RemoveShareAlbumAndAsset()
{
    MediaLibraryTracer tracer;
    tracer.Start("CLOUD_EXIT: RemoveShareAlbumAndAsset");
    TimeLogger timeLogger;
    timeLogger.Start("RemoveShareAlbumAndAsset");

    BeforeRemoveShareAlbumAndAsset();

    int32_t ret = RemoveShareAssetsInner();
    CHECK_AND_PRINT_LOG(ret == E_OK, "RemoveShareAssetsInner failed. ret: %{public}d", ret);

    AfterRemoveShareAlbumAndAsset();
    return ret;
}
} // namespace OHOS::Media
