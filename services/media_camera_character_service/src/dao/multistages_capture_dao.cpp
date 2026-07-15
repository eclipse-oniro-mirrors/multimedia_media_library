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

#define MLOG_TAG "MultiStagesCapture"

#include "multistages_capture_dao.h"

#include "database_adapter.h"
#include "file_asset.h"
#include "medialibrary_tracer.h"
#include "medialibrary_command.h"
#include "media_column.h"
#include "medialibrary_operation.h"
#include "media_log.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_request_task_manager.h"
#include "medialibrary_errno.h"
#include "medialibrary_asset_operations.h"
#include "refresh_business_name.h"
#include "rdb_predicates.h"
#include "userfile_manager_types.h"
#include "values_bucket.h"

namespace OHOS::Media {
using namespace NativeRdb;

constexpr uint32_t MANUAL_ENHANCEMENT = 1;
constexpr uint32_t AUTO_ENHANCEMENT = 1 << 1;
constexpr uint32_t MOVINGPHOTO_VIDEO_ENHANCEMENT = 1 << 2;
static constexpr int32_t BOTH = 2;

int32_t MultiStagesCaptureDao::UpdatePhotoDirtyNew(const int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePhotoDirtyNew, fileId: " + std::to_string(fileId));
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, fileId);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, false);
    updateCmd.GetAbsRdbPredicates()->NotEqualTo(
        PhotoColumn::PHOTO_SUBTYPE, std::to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    NativeRdb::ValuesBucket updateValuesDirty;
    updateValuesDirty.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    updateCmd.SetValueBucket(updateValuesDirty);
    auto isDirtyResult = DatabaseAdapter::Update(updateCmd);
    CHECK_AND_PRINT_LOG(isDirtyResult != E_OK, "update dirty flag fail, fileId: %{public}d", fileId);
    return isDirtyResult;
}

std::shared_ptr<FileAsset> MultiStagesCaptureDao::QueryDataByPhotoId(const std::string &videoId,
    const std::vector<std::string> &columns)
{
    int32_t fileId = -1;
    int32_t ret = MultiStagesCaptureRequestTaskManager::GetProcessingFileId(videoId, fileId);
    std::shared_ptr<FileAsset> fileAsset;
    if (ret == E_ERR || fileId == -1) {
        MEDIA_WARN_LOG("get fileId from fileId2PhotoId_ failed");
        fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
            PhotoColumn::PHOTO_ID, videoId, OperationObject::FILESYSTEM_PHOTO, columns);
    } else {
        fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
            PhotoColumn::MEDIA_ID, std::to_string(fileId), OperationObject::FILESYSTEM_PHOTO, columns);
    }
    return fileAsset;
}

// 一阶段: openFd时, 更新timePending
int32_t MultiStagesCaptureDao::UpdateTimePendingForOpenFile(const int32_t &fileId, const int64_t &pendingTime)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateTimePendingForOpenFile");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    ValuesBucket values;
    values.Put(MediaColumn::MEDIA_TIME_PENDING, pendingTime);

    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::CREATE_CAMERA_FILE_FD);
    int32_t updateRows = assetRefresh.UpdateWithDateTime(values, predicates);

    // 刷新相册
    assetRefresh.RefreshAlbum(static_cast<NotifyAlbumType>(SYS_ALBUM | USER_ALBUM | SOURCE_ALBUM));
    assetRefresh.Notify();
    return updateRows;
}

// 二阶段落盘: 图片查询
std::shared_ptr<FileAsset> MultiStagesCaptureDao::QueryForOnProcess(
    const int32_t &fileId, const std::string &photoId, MediaDpsMetadata &metadata)
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::QueryForOnProcess");
    MEDIA_INFO_LOG("QueryForOnProcess begin");

    // todo: 需要新增动态照片的状态修正?
    const std::vector<std::string> QUERY_COLUMNS_FOR_ON_PROCESS_IMAGE = {
        PhotoColumn::PHOTO_IS_TEMP,         // 一阶段落盘
        MediaColumn::MEDIA_DATE_TRASHED,    // 一阶段删除, 影响二阶段
        PhotoColumn::PHOTO_EDIT_TIME,       // 一阶段编辑, 影响二阶段
        PhotoColumn::PHOTO_ORIENTATION,     // 旋转角度(YUV需要)
    };

    // 1.获取数据
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        std::to_string(fileId), OperationObject::FILESYSTEM_PHOTO, QUERY_COLUMNS_FOR_ON_PROCESS_IMAGE);
    if (fileAsset == nullptr) {
        MultiStagesCaptureDfxTotalTime::GetInstance().RemoveStartTime(photoId);

        // When subType query failed, default mediaType is Image
        MultiStagesCaptureDfxResult::Report(photoId, static_cast<int32_t>(MultiStagesCaptureResultErrCode::SQL_ERR),
            static_cast<int32_t>(metadata.dfxMediaType));
        return nullptr;
    }

    // 2.初始化一阶段影响
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    bool isTrashed = fileAsset->GetIsTrash() > 0;

    // 3.dfx打点需要的数据
    metadata.modifyType = isEdited ? FirstStageModifyType::EDITED :
        (isTrashed ? FirstStageModifyType::TRASHED : FirstStageModifyType::NOT_MODIFIED);

    MEDIA_DEBUG_LOG("QueryForOnProcess end");

    return fileAsset;
}

static void CheckMovingPhotoFlag(uint32_t cloudImageEnhanceFlag, NativeRdb::ValuesBucket &updateValues)
{
    if (cloudImageEnhanceFlag & MOVINGPHOTO_VIDEO_ENHANCEMENT) {
        updateValues.Put(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
        updateValues.Put(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, BOTH);
    }
    return;
}

static int32_t GetEnhancementStatus(FirstStageModifyType type)
{
    switch (type) {
        case FirstStageModifyType::EDITED:
            return static_cast<int32_t>(CloudEnhancementAvailableType::EDIT);
        case FirstStageModifyType::TRASHED:
            return static_cast<int32_t>(CloudEnhancementAvailableType::TRASH);
        default:
            return static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT);
    }
}

static void UpdateValueForCEAvailable(ValuesBucket &updateValues, const MediaDpsMetadata &metadata)
{
    const int32_t ceAvailable = GetEnhancementStatus(metadata.modifyType);

    uint32_t flag = metadata.cloudImageEnhanceFlag;
    if (flag & AUTO_ENHANCEMENT) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} : AUTO_ENHANCEMENT",
            MLOG_TAG, __FUNCTION__, __LINE__);
        updateValues.Put(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
        updateValues.Put(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
        CheckMovingPhotoFlag(flag, updateValues);
    } else if (flag & MANUAL_ENHANCEMENT) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} : MANUAL_ENHANCEMENT",
            MLOG_TAG, __FUNCTION__, __LINE__);
        updateValues.Put(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
        CheckMovingPhotoFlag(flag, updateValues);
    } else {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} : do not support enhancement",
            MLOG_TAG, __FUNCTION__, __LINE__);
        updateValues.Put(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    }
}

static void UpdateMultiShotValues(ValuesBucket &updateValues, const MediaDpsMetadata &metadata)
{
    uint32_t flag = metadata.captureEnhancementFlag;

    int32_t deferredEffectStatus =  (flag >= static_cast<uint32_t>(DeferredEffects::BLUR)
        && flag <= static_cast<uint32_t>(DeferredEffects::BLUR_SNAP_REMOTE))
        ? static_cast<int32_t>(DeferredEffectsStatus::ASSET_SUPPORT)
        : static_cast<int32_t>(DeferredEffectsStatus::ASSET_NOT_SUPPORT);
    updateValues.Put(PhotoColumn::DEFERRED_EFFECT_STATUS, deferredEffectStatus);
    updateValues.PutInt(PhotoColumn::SUPPORTED_DEFERRED_EFFECTS, static_cast<int32_t>(flag));
}

static void UpdateValuesForMetadata(ValuesBucket &updateValues, const MediaDpsMetadata &metadata)
{
    if (!metadata.isReady) {
        MEDIA_WARN_LOG("This updata does not include metadata.");
        return;
    }

    MEDIA_INFO_LOG("This updata include metadata.");
    // 云增强标志
    if (metadata.cloudImageEnhanceFlag) {
        UpdateValueForCEAvailable(updateValues, metadata);
    }

    // 一拍多得标志
    if (metadata.captureEnhancementFlag) {
        UpdateMultiShotValues(updateValues, metadata);
    }
}

// 二阶段落盘: 图片更新数据(可能会被一阶段调用)
int32_t MultiStagesCaptureDao::UpdateHighQualityInfo(const int32_t &fileId, const MediaDpsMetadata &metadata,
    bool isOnProcess)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateHighQualityInfo");
    MEDIA_INFO_LOG("UpdateHighQualityInfo begin");

    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    // 1.更新到高质量
    ValuesBucket updateValues;
    updateValues.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    if (!isOnProcess) {
        // 一阶段场景下, 可以同时更新 dirty
        updateValues.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    }
    // 2.基于metadata更新values
    UpdateValuesForMetadata(updateValues, metadata);

    int32_t ret = MediaLibraryRdbStore::UpdateWithDateTime(updateValues, predicates);
    CHECK_AND_PRINT_LOG(ret > 0, "Failed to UpdateHighQualityInfo, fileId: %{public}d", fileId);

    MEDIA_INFO_LOG("UpdateHighQualityInfo end");
    return ret;
}

// 二阶段: 仅更新photo_quality = 高
int32_t MultiStagesCaptureDao::UpdatePhotoQuality(const int32_t &fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::UpdatePhotoQuality " + std::to_string(fileId));

    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);

    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    updateCmd.SetValueBucket(updateValues);

    int32_t updatePhotoQualityResult = DatabaseAdapter::Update(updateCmd);
    return updatePhotoQualityResult;
}

// 二阶段: 周同步任务查询
std::vector<std::shared_ptr<FileAsset>> MultiStagesCaptureDao::QueryForSessionSyncImage()
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::QueryForSessionSyncImage");
    MEDIA_INFO_LOG("QueryForSessionSyncImage begin.");

    const std::vector<std::string> QUERY_COLUMNS = {
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_SUBTYPE,

        MediaColumn::MEDIA_OWNER_PACKAGE,
        MediaColumn::MEDIA_DATE_TRASHED,
        PhotoColumn::PHOTO_DEFERRED_PROC_TYPE,
        PhotoColumn::COMPRESSION_QUALITY,
    };

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(
        MediaColumn::MEDIA_TYPE, std::to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE)));
    cmd.GetAbsRdbPredicates()->GreaterThan(PhotoColumn::PHOTO_QUALITY, 0);
    cmd.GetAbsRdbPredicates()->IsNotNull(PhotoColumn::PHOTO_ID);

    std::vector<std::shared_ptr<FileAsset>> fileAssetVec;
    int32_t errCode = MediaLibraryAssetOperations::GetFileAssetVectorFromDb(fileAssetVec, cmd, QUERY_COLUMNS);
    CHECK_AND_RETURN_RET_LOG(errCode == E_OK, fileAssetVec, "Failed to query file asset, err: %{public}d", errCode);

    MEDIA_INFO_LOG("QueryForSessionSyncImage end.");
    return fileAssetVec;
}

// 二阶段落盘前: 相机框架需要获取一些信息
std::shared_ptr<FileAsset> MultiStagesCaptureDao::QueryForDeferredPictureInfo(int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::QueryForDeferredPictureInfo");
    MEDIA_INFO_LOG("QueryForDeferredPictureInfo begin");

    // todo: 需要新增动态照片的状态修正?
    const std::vector<std::string> QUERY_COLUMNS_FOR_DEFERRED_PICTURE_INFO = {
        MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_ORIENTATION,
    };

    // 1.获取数据
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        std::to_string(fileId), OperationObject::FILESYSTEM_PHOTO, QUERY_COLUMNS_FOR_DEFERRED_PICTURE_INFO);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to QueryForDeferredPictureInfo");
        return nullptr;
    }

    MEDIA_DEBUG_LOG("QueryForDeferredPictureInfo end");
    return fileAsset;
}

// 异常流程中, 需要恢复pipeline
std::shared_ptr<FileAsset> MultiStagesCaptureDao::RecoverPipelineByFileId(int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::RecoverPipelineByFileId");
    MEDIA_INFO_LOG("RecoverPipelineByFileId begin, fileId: %{public}d.", fileId);

    const std::vector<std::string> QUERY_COLUMNS = {
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::COMPRESSION_QUALITY,
    };

    // 1.获取数据
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(MediaColumn::MEDIA_ID,
        std::to_string(fileId), OperationObject::FILESYSTEM_PHOTO, QUERY_COLUMNS);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to RecoverPipelineByFileId");
        return nullptr;
    }

    MEDIA_DEBUG_LOG("RecoverPipelineByFileId end");
    return fileAsset;
}

std::shared_ptr<FileAsset> MultiStagesCaptureDao::RecoverPipelineByPhotoId(const std::string& photoId)
{
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesCaptureDao::RecoverPipelineByPhotoId");
    MEDIA_INFO_LOG("RecoverPipelineByPhotoId begin, photoId: %{public}s.", photoId.c_str());

    const std::vector<std::string> QUERY_COLUMNS = {
        MediaColumn::MEDIA_ID,
        PhotoColumn::PHOTO_ID,
        MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME,
        MediaColumn::MEDIA_TYPE,
        MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_SUBTYPE,
        PhotoColumn::COMPRESSION_QUALITY,
    };

    // 1.获取数据
    auto fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(PhotoColumn::PHOTO_ID,
        photoId, OperationObject::FILESYSTEM_PHOTO, QUERY_COLUMNS);
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("Failed to RecoverPipelineByPhotoId");
        return nullptr;
    }

    MEDIA_DEBUG_LOG("RecoverPipelineByPhotoId end");
    return fileAsset;
}
}  // namespace OHOS::Media