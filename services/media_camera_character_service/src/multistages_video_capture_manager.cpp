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

#define MLOG_TAG "MultiStagesVideoCaptureManager"

#include "multistages_video_capture_manager.h"

#include <fcntl.h>
#include <string>

#include "asset_accurate_refresh.h"
#include "database_adapter.h"
#include "dfx_utils.h"
#include "dfx_manager.h"
#include "directory_ex.h"
#include "userfilemgr_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_async_worker.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_tracer.h"
#include "medialibrary_type_const.h"
#include "moving_photo_file_utils.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_request_task_manager.h"
#include "medialibrary_bundle_manager.h"
#include "refresh_business_name.h"
#include "result_set_utils.h"
#include "request_policy.h"
#include "medialibrary_asset_operations.h"

using namespace std;
#ifdef ABILITY_CAMERA_SUPPORT
using namespace OHOS::CameraStandard;
#endif

using namespace OHOS::NativeRdb;
namespace OHOS {
namespace Media {
const int32_t MULTISTAGES_OPERATION_CMD_SIZE = 2;
std::map<std::string, VideoInfo> MultiStagesVideoCaptureManager::videoInfoMap_ = {};

MultiStagesVideoCaptureManager::MultiStagesVideoCaptureManager()
{
    deferredProcSession_ = make_shared<DeferredVideoProcessingAdapter>();
}

MultiStagesVideoCaptureManager::~MultiStagesVideoCaptureManager() {}

MultiStagesVideoCaptureManager& MultiStagesVideoCaptureManager::GetInstance()
{
    static MultiStagesVideoCaptureManager instance;
    return instance;
}

void MultiStagesVideoCaptureManager::AddVideoInfo(const std::string &videoId, VideoInfo &videoInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("Begin AddVideoInfo, videoId: %{public}s", videoId.c_str());
    videoInfoMap_[videoId] = videoInfo;
}

void MultiStagesVideoCaptureManager::RemoveVideoInfo(const std::string &videoId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("Begin RemoveVideoInfo, videoId: %{public}s", videoId.c_str());
    auto iter = videoInfoMap_.find(videoId);
    if (iter != videoInfoMap_.end()) {
        MEDIA_INFO_LOG("Remove videoInfo success");
        videoInfoMap_.erase(iter);
    }
}

void MultiStagesVideoCaptureManager::GetVideoInfo(const std::string &videoId, VideoInfo &videoInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = videoInfoMap_.find(videoId);
    if (iter != videoInfoMap_.end()) {
        videoInfo = iter->second;
    }
}

static std::string ConvertFileId2VideoId(const int32_t fileId)
{
    std::vector<std::string> fileAssetColumns = {PhotoColumn::MEDIA_ID, PhotoColumn::PHOTO_ID};
    shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(fileId), OperationObject::FILESYSTEM_PHOTO, fileAssetColumns);
    return fileAsset->GetPhotoId();
}

shared_ptr<OHOS::NativeRdb::ResultSet> MultiStagesVideoCaptureManager::HandleMultiStagesOperation(
    MediaLibraryCommand &cmd, const std::vector<std::string> &columns)
{
    switch (cmd.GetOprnType()) {
        case OperationType::CANCEL_PROCESS_VIDEO: {
            CHECK_AND_RETURN_RET_LOG(columns.size() >= 1, nullptr, "columns is empty");
            int32_t fileId = ToInt32(columns[0]); // 0 indicates file id
            MEDIA_INFO_LOG("cancel request fileId: %{public}d", fileId);
            std::string videoId = ConvertFileId2VideoId(fileId);
            CHECK_AND_RETURN_RET_LOG(!videoId.empty(), nullptr, "video id is empty.");
            CancelProcessRequest(videoId);
            break;
        }
        default:
            break;
    }
    return nullptr;
}

void MultiStagesVideoCaptureManager::AddSingleVideo(const std::string &videoId,
    VideoInfo &videoInfo, bool isMovingPhoto)
{
    const mode_t fileMode = 0644;
    int32_t srcFd = open(videoInfo.absSrcFilePath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_LOG(srcFd >= 0, "open file fail, srcFd = %{public}d, errno:%{public}d", srcFd, errno);

    std::string dirPath = videoInfo.filePath.substr(0, videoInfo.filePath.rfind('/'));
    char realDirPath[PATH_MAX] = {0};
    if (realpath(dirPath.c_str(), realDirPath) == nullptr) {
        MEDIA_ERR_LOG("check dirPath fail, dirPath = %{public}s", dirPath.c_str());
        close(srcFd);
        return;
    }

    if (isMovingPhoto) {
        videoInfo.videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(videoInfo.filePath);
    }

    std::string tempPath = realDirPath + videoInfo.videoPath.substr(videoInfo.videoPath.rfind('/'),
        videoInfo.videoPath.rfind('.') - videoInfo.videoPath.rfind('/')) + "_tmp" +
        videoInfo.videoPath.substr(videoInfo.videoPath.rfind('.'));
    int32_t dstFd = open(tempPath.c_str(), O_CREAT|O_WRONLY|O_TRUNC, fileMode);
    if (dstFd < 0) {
        MEDIA_ERR_LOG("open file fail, dstFd = %{public}d, errno:%{public}d", dstFd, errno);
        close(srcFd);
        return;
    }

    deferredProcSession_->AddVideo(videoId, srcFd, dstFd);
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(videoId);
}

bool MultiStagesVideoCaptureManager::Openfd4AddDoubleVideo(const std::string &effectVideoPath,
    VideoInfo &videoInfo, int32_t &lowSrcFd, int32_t &srcFd, int32_t &srcFdCopy)
{
    lowSrcFd = open(effectVideoPath.c_str(), O_RDONLY);
    CHECK_AND_RETURN_RET_LOG(lowSrcFd >= 0, false, "open file fail, lowSrcFd = %{public}d", lowSrcFd);
    srcFd = open(videoInfo.absSrcFilePath.c_str(), O_RDONLY);
    if (srcFd < 0) {
        MEDIA_WARN_LOG("open file fail, srcFd = %{public}d, lowSrcFd = %{public}d", srcFd, lowSrcFd);
        close(lowSrcFd);
        return false;
    }

    srcFdCopy = open(videoInfo.absSrcFilePath.c_str(), O_RDONLY);
    if (srcFdCopy < 0) {
        MEDIA_WARN_LOG("open file fail, srcFd = %{public}d, srcFdCopy = %{public}d, lowSrcFd = %{public}d",
            srcFd, srcFdCopy, lowSrcFd);
        close(lowSrcFd);
        close(srcFd);
        return false;
    }

    return true;
}

void MultiStagesVideoCaptureManager::AddDoubleVideo(const std::string &videoId,
    VideoInfo &videoInfo, bool isMovingPhoto)
{
    const mode_t fileMode = 0644;
    std::string effectVideoPath = videoInfo.videoPath;
    CHECK_AND_RETURN_LOG(videoInfo.videoPath.size() >= MEDIA_EDIT_DATA_DIR.size(),
        "videoPath is too short, video Path: %{private}s", videoInfo.videoPath.c_str());
    CHECK_AND_RETURN_LOG(videoInfo.videoPath.find(ROOT_MEDIA_DIR) == 0,
        "videoPath does not begin with ROOT_MEDIA_DIR, video Path: %{private}s", videoInfo.videoPath.c_str());
    // 原始视频
    videoInfo.videoPath = MEDIA_EDIT_DATA_DIR + videoInfo.videoPath.substr(ROOT_MEDIA_DIR.length()) + "/source.mp4";
    CHECK_AND_RETURN_LOG(PathToRealPath(videoInfo.videoPath, videoInfo.absSrcFilePath),
        "file is not real path, file path: %{private}s", videoInfo.videoPath.c_str());

    int32_t lowSrcFd = 0;
    int32_t srcFd = 0;
    int32_t srcFdCopy = 0;
    CHECK_AND_RETURN(Openfd4AddDoubleVideo(effectVideoPath, videoInfo, lowSrcFd, srcFd, srcFdCopy));

    std::string dirPath = videoInfo.filePath.substr(0, videoInfo.filePath.rfind('/'));
    char realDirPath[PATH_MAX] = {0};
    if (realpath(dirPath.c_str(), realDirPath) == nullptr) {
        MEDIA_ERR_LOG("check dirPath fail, dirPath = %{public}s", dirPath.c_str());
        close(lowSrcFd);
        close(srcFd);
        close(srcFdCopy);
        return;
    }

    if (isMovingPhoto) {
        videoInfo.videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(videoInfo.filePath);
    } else {
        videoInfo.videoPath = effectVideoPath;
    }
    std::string tempPath = realDirPath + videoInfo.videoPath.substr(videoInfo.videoPath.rfind('/'),
        videoInfo.videoPath.rfind('.') - videoInfo.videoPath.rfind('/')) + "_tmp" +
        videoInfo.videoPath.substr(videoInfo.videoPath.rfind('.'));
    int32_t dstFd = open(tempPath.c_str(), O_CREAT|O_WRONLY|O_TRUNC, fileMode);
    if (dstFd < 0) {
        MEDIA_ERR_LOG("open file fail, dstFd = %{public}d, errno:%{public}d", dstFd, errno);
        close(lowSrcFd);
        close(srcFd);
        close(srcFdCopy);
        return;
    }

    std::vector<int32_t> fds {lowSrcFd, dstFd, srcFd, srcFdCopy};
    deferredProcSession_->AddVideo(videoId, fds);
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(videoId);
    DfxManager::GetInstance()->HandleCinematicVideoAddStartTime(CinematicWaitType::PROCESS_CINEMATIC, videoId);
    DfxManager::GetInstance()->HandleCinematicVideoAddStartTime(CinematicWaitType::CANCEL_CINEMATIC, videoId);
}

void MultiStagesVideoCaptureManager::AddVideoInternal(const std::string &videoId,
    VideoInfo &videoInfo, bool isTrashed, bool isMovingPhoto)
{
    MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(videoInfo.fileId, videoId, isTrashed);
#ifdef ABILITY_CAMERA_SUPPORT
    videoInfo.videoPath = videoInfo.filePath;
    if (isMovingPhoto) {
        videoInfo.videoPath = MovingPhotoFileUtils::GetSourceMovingPhotoVideoPath(videoInfo.filePath);
        if (!MediaFileUtils::IsFileExists(videoInfo.videoPath)) {
            videoInfo.videoPath = MovingPhotoFileUtils::GetMovingPhotoVideoPath(videoInfo.filePath);
        }
    }

    CHECK_AND_RETURN_LOG(PathToRealPath(videoInfo.videoPath, videoInfo.absSrcFilePath),
        "file is not real path, file path: %{private}s", videoInfo.videoPath.c_str());

    AddVideoInfo(videoId, videoInfo);
    MEDIA_INFO_LOG("AddVideoInternal, file path: %{public}s", DfxUtils::GetSafePath(videoInfo.absSrcFilePath).c_str());
    switch (videoInfo.videoCount) {
        case VideoCount::SINGLE:
            AddSingleVideo(videoId, videoInfo, isMovingPhoto);
            break;
        case VideoCount::DOUBLE:
            AddDoubleVideo(videoId, videoInfo, isMovingPhoto);
            break;
        default:
            MEDIA_ERR_LOG("videoCount is invalid, videoCount.");
            break;
    }
#endif
}

void MultiStagesVideoCaptureManager::AddVideo(const std::string &videoId,
    const std::string &fileId, VideoInfo &videoInfo)
{
    if (videoId.empty()) {
        MEDIA_ERR_LOG("videoId is empty");
        return;
    }

    if (fileId.empty()) {
        MEDIA_ERR_LOG("fileId is empty");
        return;
    }

    if (videoInfo.filePath.empty()) {
        MEDIA_ERR_LOG("filePath is empty");
        return;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    string where = MEDIA_DATA_DB_ID + " = ? ";
    vector<string> whereArgs { fileId };
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    NativeRdb::ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_PHOTO_ID, videoId);
    values.PutInt(MEDIA_DATA_DB_PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::LOW));
    cmd.SetValueBucket(values);

    auto result = DatabaseAdapter::Update(cmd);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("AddVideo update fail fileId: %{public}s", fileId.c_str());
        return;
    }
    MEDIA_INFO_LOG("AddVideo update success fileId: %{public}s", fileId.c_str());

    AddVideoInternal(videoId, videoInfo, false);
}

void MultiStagesVideoCaptureManager::AddVideo(const AddProcessVideoDto &dto)
{
    MEDIA_INFO_LOG("calling addVideo, fileId: %{public}d, photoId: %{public}s.", dto.fileId, dto.photoId.c_str());
    if (dto.photoId.empty() || dto.fileId == -1) {
        MEDIA_ERR_LOG("videoId is empty");
        return;
    }

    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    std::string where = MediaColumn::MEDIA_ID + " = ? ";
    std::vector<std::string> whereArgs { to_string(dto.fileId) };
    updateCmd.GetAbsRdbPredicates()->SetWhereClause(where);
    updateCmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);

    NativeRdb::ValuesBucket updateValues;
    if (dto.photoId != to_string(0)) {
        updateValues.PutString(PhotoColumn::PHOTO_ID, dto.photoId);
    }
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, dto.photoQuality);
    updateCmd.SetValueBucket(updateValues);
    auto result = DatabaseAdapter::Update(updateCmd);

    if (dto.VideoEnhancementType == 0) {
        MEDIA_WARN_LOG("videoEnhancementType not support DeferredProcVideo");
        return;
    }
    vector<std::string> fileAssetColumns = { PhotoColumn::MEDIA_FILE_PATH };
    shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(dto.fileId), OperationObject::FILESYSTEM_PHOTO, fileAssetColumns);
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "fileAsset is nullptr.");
    std::string filePath = fileAsset->GetFilePath();
    VideoInfo videoInfo = {
        .fileId = dto.fileId,
        .videoCount = static_cast<VideoCount>(dto.videoCount),
        .filePath = filePath,
    };
    AddVideoInternal(dto.photoId, videoInfo, false);
}

bool MultiStagesVideoCaptureManager::Init()
{
    SyncWithDeferredVideoProcSessionInternal();
    return true;
}

void MultiStagesVideoCaptureManager::SyncWithDeferredVideoProcSessionInternal()
{
    MEDIA_INFO_LOG("SyncWithDeferredVideoProcSessionInternal enter");

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    string where = MEDIA_DATA_DB_PHOTO_ID + " IS NOT NULL AND " +
        "((" + MEDIA_DATA_DB_PHOTO_QUALITY + " > 0 AND (" + MEDIA_DATA_DB_MEDIA_TYPE + " = " +
        to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) + ") OR (" +
        MEDIA_DATA_DB_PHOTO_QUALITY + " = 0 AND " + MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS + " IN (" +
        to_string(static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_TO_DELIVER)) + ", " +
        to_string(static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_DELIVERED)) + "))))";
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    vector<string> columns { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_FILE_PATH,
                            MEDIA_DATA_DB_DATE_TRASHED, MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS };

    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        MEDIA_ERR_LOG("result set is empty");
        return;
    }

    deferredProcSession_->BeginSynchronize();
    do {
        std::string videoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
        std::string filePath = GetStringVal(MEDIA_DATA_DB_FILE_PATH, resultSet);
        bool isMovingPhoto = GetInt32Val(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, resultSet) > 0;
        bool isCinematicVideo = QuerySubType(videoId) == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO);
        bool isTrashed = GetInt64Val(MEDIA_DATA_DB_DATE_TRASHED, resultSet) > 0;
        int32_t fileId = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        VideoInfo videoInfo = {fileId, isCinematicVideo ? VideoCount::DOUBLE : VideoCount::SINGLE,
            filePath, "", ""};
        GetVideoInfo(videoId, videoInfo);
        MEDIA_INFO_LOG("videoId: %{public}s, filePath: %{public}s, fileId: %{public}d",
            videoId.c_str(), filePath.c_str(), fileId);
        AddVideoInternal(videoId, videoInfo, isTrashed, isMovingPhoto);

        if (isTrashed) {
            RemoveVideo(videoId, true);
        }
    } while (!resultSet->GoToNextRow());

    deferredProcSession_->EndSynchronize();
    MEDIA_INFO_LOG("SyncWithDeferredVideoProcSessionInternal exit");
}

static void SyncWithDeferredVideoProcSessionAsync(AsyncTaskData *data)
{
    MultiStagesVideoCaptureManager::GetInstance().SyncWithDeferredVideoProcSessionInternal();
}

void MultiStagesVideoCaptureManager::SyncWithDeferredVideoProcSession()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    if (asyncWorker == nullptr) {
        MEDIA_INFO_LOG("can not get async worker");
        return;
    }

    shared_ptr<MediaLibraryAsyncTask> asyncTask =
        make_shared<MediaLibraryAsyncTask>(SyncWithDeferredVideoProcSessionAsync, nullptr);
    if (asyncTask == nullptr) {
        MEDIA_ERR_LOG("SyncWithDeferredVideoProcSession create task fail");
        return;
    }
    MEDIA_INFO_LOG("SyncWithDeferredVideoProcSession add task success");
    asyncWorker->AddTask(asyncTask, false);
}

void MultiStagesVideoCaptureManager::RemoveVideo(const std::string &videoId, const bool restorable)
{
    MEDIA_INFO_LOG("RemoveVideo videoId = %{public}s, restorable = %{public}s",
        videoId.c_str(), restorable ? "true" : "false");
    MultiStagesCaptureRequestTaskManager::RemovePhotoInProgress(videoId, restorable);
    deferredProcSession_->RemoveVideo(videoId, restorable);
    if (restorable) {
        return;
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    string where = PhotoColumn::PHOTO_ID + " = ? ";
    vector<string> whereArgs { videoId };
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    vector<string> columns { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_SUBTYPE };
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_INFO_LOG("result set is empty");
        return;
    }

    string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    if (GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet) ==
        static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        data = MovingPhotoFileUtils::GetMovingPhotoVideoPath(data);
    }
    int ret = MediaLibraryPhotoOperations::RemoveTempVideo(data);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete temp video file failed. ret: %{public}d, errno: %{public}d", ret, errno);
    }
}

void MultiStagesVideoCaptureManager::RemoveVideo(const std::string &videoId, const std::string &mediaFilePath,
    const int32_t &photoSubType, const bool restorable)
{
    MEDIA_INFO_LOG("RemoveVideo videoId = %{public}s, restorable = %{public}s",
        videoId.c_str(), restorable ? "true" : "false");

    deferredProcSession_->RemoveVideo(videoId, restorable);
    if (restorable) {
        return;
    }
    string data = mediaFilePath;
    if (photoSubType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) {
        data = MovingPhotoFileUtils::GetMovingPhotoVideoPath(data);
    }
    int ret = MediaLibraryPhotoOperations::RemoveTempVideo(data);
    if (ret != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Delete temp video file failed. ret: %{public}d, errno: %{public}d", ret, errno);
    }
}

void MultiStagesVideoCaptureManager::RestoreVideo(const std::string &videoId)
{
    deferredProcSession_->RestoreVideo(videoId);
}

static int32_t UpdateIsTempAndDirty(int32_t fileId, int32_t subType)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    int32_t updateRows = -1;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SAVE_CAMERA_PHOTO_BUSSINESS_NAME);
    values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));

    updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
    CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR, "update temp flag fail.");
    assetRefresh.RefreshAlbum(static_cast<NotifyAlbumType>(SYS_ALBUM | USER_ALBUM | SOURCE_ALBUM));
    assetRefresh.Notify();
    return updateRows;
}

void MultiStagesVideoCaptureManager::ProcessVideo(const ProcessVideoDto &dto)
{
    MEDIA_INFO_LOG("Begin ProcessVideo");
    std::string videoId = MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(dto.fileId);
    if (videoId.empty() || videoId != dto.photoId) {
        MEDIA_ERR_LOG("ProcessVideo videoId is invalid, fileId: %{public}d, photoId: %{public}s, videoId: %{public}s",
            dto.fileId, dto.photoId.c_str(), videoId.c_str());
        return;
    }

    std::string callerBundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(videoId, RequestType::REQUEST);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "ProcessVideo, pkg name: %{public}s, videoId %{public}s, mode: %{public}d, count: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        callerBundleName.c_str(), videoId.c_str(), dto.deliveryMode, currentRequestCount);
    if (dto.deliveryMode == static_cast<int32_t>(RequestPolicy::HIGH_QUALITY_MODE) && currentRequestCount <= 1) {
        deferredProcSession_->ProcessVideo(callerBundleName, videoId);
        InsertCinematicProgress(dto.photoId, dto.requestId, 0);
    }
}

static int32_t UpdateIsTempAndDirty(const SaveCameraPhotoDto &dto, int32_t subType)
{
    RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, dto.fileId);
    ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    if (dto.supportedWatermarkType != INT32_MIN) {
        values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, dto.supportedWatermarkType);
    }
    if (dto.cameraShotKey != "NotSet") {
        values.Put(PhotoColumn::CAMERA_SHOT_KEY, dto.cameraShotKey);
    }
    int32_t updateRows = 0;
    AccurateRefresh::AssetAccurateRefresh assetRefresh(AccurateRefresh::SAVE_CAMERA_PHOTO_BUSSINESS_NAME);
    if (subType == static_cast<int32_t>(PhotoSubType::CINEMATIC_VIDEO)) {
        updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
        CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR, "update temp flag fail.");
        predicates.EqualTo(PhotoColumn::PHOTO_QUALITY, to_string(static_cast<int32_t>(MultiStagesPhotoQuality::FULL)));
        ValuesBucket valuesBucketDirty;
        valuesBucketDirty.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        int32_t updateDirtyRows = MediaLibraryRdbStore::UpdateWithDateTime(valuesBucketDirty, predicates);
    } else {
        values.Put(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
        updateRows = assetRefresh.UpdateWithDateTime(values, predicates);
        CHECK_AND_RETURN_RET_LOG(updateRows >= 0, E_ERR, "update temp flag fail.");
    }

    assetRefresh.RefreshAlbum(static_cast<NotifyAlbumType>(SYS_ALBUM | USER_ALBUM | SOURCE_ALBUM));
    assetRefresh.Notify();
    return updateRows;
}

int32_t MultiStagesVideoCaptureManager::SaveCameraVideo(const SaveCameraPhotoDto &dto)
{
    MediaLibraryTracer tracer;
    tracer.Start("MediaLibraryPhotoOperations::SaveCameraVideo");
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture, start save fileId: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, dto.fileId);

    std::vector<std::string> fileAssetColumns = { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_SUBTYPE };
    shared_ptr<FileAsset> fileAsset = MediaLibraryAssetOperations::GetFileAssetFromDb(
        PhotoColumn::MEDIA_ID, to_string(dto.fileId), OperationObject::FILESYSTEM_PHOTO, fileAssetColumns);
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, E_ERR, "fileAsset is nullptr.");
    int32_t ret = UpdateIsTempAndDirty(dto, fileAsset->GetPhotoSubType());
    CHECK_AND_RETURN_RET_LOG(!(fileAsset->GetPath().empty()), E_ERR, "path is empty.");
    MediaLibraryAssetOperations::ScanFile(fileAsset->GetPath(), false, true, true, dto.fileId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture Success, fileId: %{public}d, ret: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, dto.fileId, ret);
    return ret;
}

void MultiStagesVideoCaptureManager::CancelProcessRequest(const string &videoId)
{
    CHECK_AND_RETURN_LOG(MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(videoId),
        "videoId is empty or not in process");
    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(videoId, RequestType::CANCEL_REQUEST);
    CHECK_AND_RETURN_LOG(currentRequestCount <= 0,
        "not cancel request because request count(%{public}d) greater than 0", currentRequestCount);
    DfxManager::GetInstance()->HandleCinematicVideoAddEndTime(CinematicWaitType::CANCEL_CINEMATIC, videoId);
    deferredProcSession_->CancelProcessVideo(videoId);
}

int32_t MultiStagesVideoCaptureManager::QuerySubType(const string &photoId)
{
    NativeRdb::AbsRdbPredicates predicatesNew(PhotoColumn::PHOTOS_TABLE);
    string where = PhotoColumn::PHOTO_ID + " = ? ";
    vector<string> whereArgs { photoId };
    predicatesNew.SetWhereClause(where);
    predicatesNew.SetWhereArgs(whereArgs);
    vector<string> columns { PhotoColumn::PHOTO_SUBTYPE };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicatesNew, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("Result set is empty, photoId: %{public}s", photoId.c_str());
        return static_cast<int32_t>(PhotoSubType::CAMERA);
    }

    return GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
}

void MultiStagesVideoCaptureManager::InsertCinematicProgress(const std::string &videoId, const std::string &requestId,
    double progress)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    MEDIA_INFO_LOG("InsertCinematicProgress map size: %{public}zu.", cinematicProgressMap_.size());
    cinematicProgressMap_[videoId] = std::make_pair(requestId, progress);
}

void MultiStagesVideoCaptureManager::InsertCinematicProgress(const std::string &videoId, double progress)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    MEDIA_INFO_LOG("InsertCinematicProgress map size: %{public}zu.", cinematicProgressMap_.size());
    cinematicProgressMap_[videoId].second = progress;
}

int32_t MultiStagesVideoCaptureManager::ClearCinematicProgressMap(const std::string &videoId)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    auto iter = cinematicProgressMap_.find(videoId);
    if (iter == cinematicProgressMap_.end()) {
        MEDIA_ERR_LOG("faid to ClearCinematicProgressMap, cause videoId is empty.");
        return E_ERR;
    }
    cinematicProgressMap_.erase(iter);
    MEDIA_INFO_LOG("ClearCinematicProgressMap map size: %{public}zu.", cinematicProgressMap_.size());
    return E_OK;
}

int32_t MultiStagesVideoCaptureManager::GetProgressCallback(GetProgressCallbackRespBody &respbody)
{
    std::lock_guard<std::mutex> lock(progressMutex_);
    auto it = cinematicProgressMap_.begin();
    auto endIt = cinematicProgressMap_.end();
    while (it != endIt) {
        // respbody.progressMap[requestId] = progress
        MEDIA_INFO_LOG("cinematicProgressMap_ videoId: %{public}s, requestId: %{public}s, progress: %{public}f.",
            it->first.c_str(), it->second.first.c_str(), it->second.second);
        respbody.progressMap[it->second.first] = it->second.second;
        it++;
    }
    return E_OK;
}

int32_t MultiStagesVideoCaptureManager::ToInt32(const std::string &str)
{
    char *end = nullptr;
    long number = std::strtol(str.c_str(), &end, 10);
    if (*end != '\0') {
        MEDIA_ERR_LOG("ToInt32 failed, has invalid char. str: %{public}s", str.c_str());
        return 0;
    } else if (number < INT_MIN || number > INT_MAX) {
        MEDIA_ERR_LOG("ToInt32 failed, number overflow. str: %{public}s", str.c_str());
        return 0;
    }
    return static_cast<int32_t>(number);
}
} // Media
} // OHOS