/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MultiStagesCaptureDeferredVideoProcSessionCallback"

#include "multistages_capture_deferred_video_proc_session_callback.h"

#include "media_log.h"
#include "database_adapter.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_manager.h"
#include "multistages_capture_notify_info.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "medialibrary_notify.h"
#include "medialibrary_notify_new.h"
#include "medialibrary_object_utils.h"
#include "result_set_utils.h"
#include "dfx_manager.h"
#include "multistages_capture_request_task_manager.h"
#include "multistages_capture_dao.h"
#include "multistages_capture_notify.h"

using namespace std;
using namespace OHOS::CameraStandard;
using namespace OHOS::Media::Notification;
const std::string HIGH_TEMPERATURE_VIDEO = "high_temperature_video";
namespace OHOS {
namespace Media {
MultiStagesCaptureDeferredVideoProcSessionCallback::MultiStagesCaptureDeferredVideoProcSessionCallback()
{}
 
MultiStagesCaptureDeferredVideoProcSessionCallback::~MultiStagesCaptureDeferredVideoProcSessionCallback()
{}

void MultiStagesCaptureDeferredVideoProcSessionCallback::NotifyIfTempFile(
    const std::shared_ptr<FileAsset> &fileAsset, bool isError)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr.");
        return;
    }
    std::string displayName = fileAsset->GetDisplayName();
    std::string filePath = fileAsset->GetFilePath();
    int32_t fileId = fileAsset->GetId();
    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "get instance notify failed NotifyIfTempFile abortion");

    std::string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    auto notifyUri = MediaFileUtils::GetUriByExtrConditions(ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(
        MediaType::MEDIA_TYPE_VIDEO, MEDIA_API_VERSION_V10) + "/", to_string(fileId), extrUri);
    notifyUri = MediaFileUtils::GetUriWithoutDisplayname(notifyUri);
    CHECK_AND_EXECUTE(!isError, notifyUri += HIGH_TEMPERATURE_VIDEO);
    MEDIA_ERR_LOG("MultistagesVideo notify: %{public}s", notifyUri.c_str());
    watch->Notify(notifyUri, NOTIFY_UPDATE);
}

int32_t MultiStagesCaptureDeferredVideoProcSessionCallback::UpdateVideoQuality(
    const int32_t &fileId, const std::shared_ptr<FileAsset> &fileAsset, bool isSuccess)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr.");
        return E_ERR;
    }
    int32_t subType = fileAsset->GetPhotoSubType();
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    if (subType == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
        updateValues.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    } else {
        if ((fileAsset->GetPosition()) != static_cast<int32_t>(PhotoPositionType::LOCAL)) {
            updateValues.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_FDIRTY));
        }
    }
    if (subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        updateValues.Put(PhotoColumn::STAGE_VIDEO_TASK_STATUS,
            isSuccess ? static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_SUCCESS) :
            static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_FAIL));
    } else {
        updateValues.Put(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    }

    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, fileId);
    return DatabaseAdapter::Update(updateCmd);
}

int32_t GetDfxCaptureMediaType(const std::shared_ptr<FileAsset> &fileAsset)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr.");
        return static_cast<int32_t>(MultiStagesCaptureMediaType::VIDEO);
    }
    bool isMovingPhoto =
        (fileAsset->GetStageVideoTaskStatus()) == static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED);
    if (isMovingPhoto) {
        return static_cast<int32_t>(MultiStagesCaptureMediaType::MOVING_PHOTO_VIDEO);
    }
    bool isCinematicVideo = (fileAsset->GetPhotoSubType()) == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO);
    if (isCinematicVideo) {
        return static_cast<int32_t>(MultiStagesCaptureMediaType::CINEMATIC_VIDEO);
    }
    return static_cast<int32_t>(MultiStagesCaptureMediaType::VIDEO);
}

std::vector<std::string> GetColumns()
{
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_TYPE, PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::STAGE_VIDEO_TASK_STATUS, PhotoColumn::PHOTO_POSITION, PhotoColumn::MOVING_PHOTO_EFFECT_MODE,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_IS_TEMP};
    return columns;
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::OnProcessVideoDone(const std::string& videoId)
{
    CHECK_AND_RETURN_LOG(!videoId.empty(), "OnProcessVideoDone, videoId is empty");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} OnProcessVideoDone, videoId: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, videoId.c_str());
    const std::vector<std::string> columns = GetColumns();
    auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(videoId, columns);
    if (fileAsset == nullptr) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} result set is empty", MLOG_TAG, __FUNCTION__, __LINE__);
        MultiStagesCaptureDfxTotalTime::GetInstance().RemoveStartTime(videoId);
        // When subType query failed, default dfxCaptureMediaType is Video
        MultiStagesCaptureDfxResult::Report(videoId, static_cast<int32_t>(MultiStagesCaptureResultErrCode::SQL_ERR),
            static_cast<int32_t>(MultiStagesCaptureMediaType::VIDEO));
        return;
    }
    int32_t dfxCaptureMediaType = GetDfxCaptureMediaType(fileAsset);    // 获取打点的media类型
    MEDIA_ERR_LOG("dfxCaptureMediaType: %{public}d.", dfxCaptureMediaType);

    int ret = MediaLibraryPhotoOperations::ProcessMultistagesVideo(fileAsset);
    if (ret != E_OK) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} "
            "Save 110 quality video failed. ret: %{public}d, errno: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, ret, errno);
        MultiStagesCaptureDfxResult::Report(videoId,
            static_cast<int32_t>(MultiStagesCaptureResultErrCode::SAVE_VIDEO_FAIL), dfxCaptureMediaType);
        return;
    }
    int32_t fileId = fileAsset->GetId();
    UpdateVideoQuality(fileId, fileAsset, true);
    if (fileAsset->GetPhotoIsTemp()) {
        MEDIA_WARN_LOG("MultistagesCapture, this video is temp");
    } else {
        MediaLibraryObjectUtils::ScanFileAsync(
            fileAsset->GetFilePath(), to_string(fileAsset->GetId()), MediaLibraryApi::API_10);
        if (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
            MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_VIDEO_DONE);
            NotifyIfTempFile(fileAsset);
        }
    }
    MultiStagesVideoCaptureManager::GetInstance().ClearCinematicProgressMap(videoId);
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(videoId, dfxCaptureMediaType);
    MultiStagesCaptureDfxResult::Report(videoId,
        static_cast<int32_t>(MultiStagesCaptureResultErrCode::SUCCESS), dfxCaptureMediaType);
    MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(videoId, false);
    MultiStagesVideoCaptureManager::GetInstance().RemoveVideoInfo(videoId);
    if (dfxCaptureMediaType == static_cast<int32_t>(MultiStagesCaptureMediaType::CINEMATIC_VIDEO)) {
        DfxManager::GetInstance()->HandleCinematicVideoAddEndTime(CinematicWaitType::PROCESS_CINEMATIC, videoId);
        DfxManager::GetInstance()->HandleCinematicVideoMultistageResult(true);
    }
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} OnProcessVideoDone, success videoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, videoId.c_str());
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::HandleVideoProcFailedAndInvalid(const std::string& videoId,
    const CameraStandard::DpsErrorCode errorCode, const std::shared_ptr<FileAsset> &fileAsset)
{
    MultiStagesVideoCaptureManager::GetInstance().RemoveVideo(videoId, false);
    MultiStagesVideoCaptureManager::GetInstance().RemoveVideoInfo(videoId);
    MultiStagesVideoCaptureManager::GetInstance().ClearCinematicProgressMap(videoId);
    if (fileAsset != nullptr) {
        int32_t fileId = fileAsset->GetId();
        UpdateVideoQuality(fileId, fileAsset, false);
        if (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
            NotifyIfTempFile(fileAsset, true);
            MEDIA_ERR_LOG("cinematic video process failed, videoId: %{public}s", videoId.c_str());
        }
    }

    MEDIA_ERR_LOG("error %{public}d, videoId: %{public}s", static_cast<int32_t>(errorCode),
        videoId.c_str());
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::HandleVideoProcInterrupted(const std::string& videoId,
    const CameraStandard::DpsErrorCode errorCode, const std::shared_ptr<FileAsset> &fileAsset)
{
    MultiStagesVideoCaptureManager::GetInstance().ClearCinematicProgressMap(videoId);
    if (fileAsset != nullptr) {
        if (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
            MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_ERROR_VIDEO);
            NotifyIfTempFile(fileAsset, true);
            MEDIA_ERR_LOG("cinematic video process failed, videoId: %{public}s", videoId.c_str());
        }
    }
    
    MEDIA_ERR_LOG("error %{public}d, videoId: %{public}s", static_cast<int32_t>(errorCode),
        videoId.c_str());
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::VideoFaileProcAsync(AsyncTaskData *data)
{
    auto *taskData = static_cast<VideoFaileProcTaskData *>(data);
    CHECK_AND_RETURN_LOG(taskData != nullptr, "taskData is null");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} OnError, errorCode: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, taskData->errorCode_);
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, MediaColumn::MEDIA_NAME };
    auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(taskData->videoId_, columns);
    switch (taskData->errorCode_) {
        case ERROR_SESSION_SYNC_NEEDED:
            MultiStagesVideoCaptureManager::GetInstance().SyncWithDeferredVideoProcSession();
            break;
        case ERROR_VIDEO_PROC_INVALID_VIDEO_ID:
        case ERROR_VIDEO_PROC_FAILED: {
            HandleVideoProcFailedAndInvalid(taskData->videoId_, taskData->errorCode_, fileAsset);
            break;
        }
        case ERROR_VIDEO_PROC_INTERRUPTED:
            HandleVideoProcInterrupted(taskData->videoId_, taskData->errorCode_, fileAsset);
            break;
        default:
            MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(taskData->videoId_);
            break;
    }

    if (taskData->errorCode_ != ERROR_SESSION_SYNC_NEEDED) {
        int32_t mediaType = static_cast<int32_t>(MultiStagesCaptureMediaType::VIDEO);
        int32_t photoSubType = MultiStagesVideoCaptureManager::QuerySubType(taskData->videoId_);
        auto itr = SUBTYPR_TO_MEDIATYPE_MAP.find(photoSubType);
        if (itr != SUBTYPR_TO_MEDIATYPE_MAP.end()) {
            mediaType = static_cast<int32_t>(itr->second);
        }
        MultiStagesCaptureDfxResult::Report(taskData->videoId_, static_cast<int32_t>(taskData->errorCode_), mediaType);
    }
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::AsyncOnErrorProc(const std::string& videoId,
    const CameraStandard::DpsErrorCode errorCode)
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "Can not get asyncWorker");

    AsyncTaskData* taskData = new (std::nothrow) VideoFaileProcTaskData(videoId, errorCode);
    CHECK_AND_RETURN_LOG(taskData != nullptr, "Failed to new taskData");

    shared_ptr<MediaLibraryAsyncTask> asyncTask =
        make_shared<MediaLibraryAsyncTask>(VideoFaileProcAsync, taskData);
    CHECK_AND_RETURN_LOG(asyncTask != nullptr, "Can not get asyncWorker");

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} AsyncOnErrorProc add task success",
        MLOG_TAG, __FUNCTION__, __LINE__);
    asyncWorker->AddTask(asyncTask, false);
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::OnError(const std::string& videoId,
    const CameraStandard::DpsErrorCode errorCode)
{
    AsyncOnErrorProc(videoId, errorCode);
    DfxManager::GetInstance()->HandleCinematicVideoMultistageResult(false);
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::OnStateChanged(const CameraStandard::DpsStatusCode state)
{
    MEDIA_INFO_LOG("status: %{public}d", state);
}

void MultiStagesCaptureDeferredVideoProcSessionCallback::OnProcessingProgress(const string& videoId, float progress)
{
    if (videoId.empty()) {
        MEDIA_ERR_LOG("videoId is empty.");
        return;
    }

    MEDIA_INFO_LOG("OnProcessingProgress enter, videoId: %{public}s, progress: %{public}f.", videoId.c_str(), progress);
    MultiStagesVideoCaptureManager::GetInstance().InsertCinematicProgress(videoId, progress);
}
} // namespace Media
} // namespace OHOS