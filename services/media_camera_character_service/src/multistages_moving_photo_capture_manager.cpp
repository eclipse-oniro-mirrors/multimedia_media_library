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

#define MLOG_TAG "MultiStagesMovingPhotoCaptureManager"

#include "multistages_moving_photo_capture_manager.h"
#include "multistages_video_capture_manager.h"

#include "media_log.h"
#include "moving_photo_file_utils.h"
#include "database_adapter.h"
#include "medialibrary_rdbstore.h"
#include "result_set_utils.h"
#include "userfilemgr_uri.h"

using namespace std;
#ifdef ABILITY_CAMERA_SUPPORT
using namespace OHOS::CameraStandard;
#endif

namespace OHOS {
namespace Media {
static constexpr int32_t BOTH = 2;

MultiStagesMovingPhotoCaptureManager::MultiStagesMovingPhotoCaptureManager() {}

MultiStagesMovingPhotoCaptureManager::~MultiStagesMovingPhotoCaptureManager() {}

static void UpdateMultStagesMovingPhotoVideoCloudEnhancementType(int32_t enhancementType, int32_t fileId)
{
    NativeRdb::ValuesBucket updateValues;
    int32_t ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT);
    CHECK_AND_RETURN_LOG(enhancementType == BOTH, "enhancementType is not both");
    updateValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);

    CHECK_AND_RETURN_LOG(!updateValues.IsEmpty(), "UpdateHighQualityPictureInfo failed, updateValues is null");

    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t updatePhotoResult = DatabaseAdapter::Update(updateCmd);
    CHECK_AND_RETURN_LOG(updatePhotoResult >= 0, "UpdateHighQualityPictureInfo fail, photoId: %{public}d", fileId);
}

static int32_t UpdateMultStagesMovingPhotoVideoTaskStatus(const int32_t &fileId, StageVideoTaskStatus taskStatus)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    string where = MediaColumn::MEDIA_ID + " = ? ";
    vector<string> whereArgs { to_string(fileId) };
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    NativeRdb::ValuesBucket values;
    values.PutString(MEDIA_DATA_DB_STAGE_VIDEO_TASK_STATUS, to_string(static_cast<int32_t>(taskStatus)));
    cmd.SetValueBucket(values);

    auto result = DatabaseAdapter::Update(cmd);
    return result;
}

void MultiStagesMovingPhotoCaptureManager::SaveMovingPhotoVideoFinished(const int32_t &fileId)
{
    int32_t ret = UpdateMultStagesMovingPhotoVideoTaskStatus(fileId, StageVideoTaskStatus::STAGE_TASK_TO_DELIVER);
    CHECK_AND_RETURN_LOG(ret == NativeRdb::E_OK,
        "Stage video task status update fail fileId: %{public}d", fileId);
    AddVideoFromMovingPhoto(fileId);
}

void MultiStagesMovingPhotoCaptureManager::AddVideoFromMovingPhoto(const int32_t &fileId)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Enter AddVideoFromMovingPhoto, fileId: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, fileId);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    string where = MediaColumn::MEDIA_ID + " = ? ";
    vector<string> whereArgs { to_string(fileId) };
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
    vector<string> columns { MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_QUALITY,
        PhotoColumn::STAGE_VIDEO_TASK_STATUS, PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_ID,
            PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, PhotoColumn::PHOTO_CE_AVAILABLE };
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != NativeRdb::E_OK);
    CHECK_AND_RETURN_LOG(!cond, "result set is empty");

    int32_t subType = GetInt32Val(PhotoColumn::PHOTO_SUBTYPE, resultSet);
    CHECK_AND_RETURN_LOG(subType == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO),
        "task must be moving photo.");

    int32_t photoQuality = GetInt32Val(PhotoColumn::PHOTO_QUALITY, resultSet);
    CHECK_AND_RETURN_INFO_LOG(photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL),
        "photo multi stage task not yet.");

    int32_t stageVideoTaskStatus = GetInt32Val(PhotoColumn::STAGE_VIDEO_TASK_STATUS, resultSet);
    CHECK_AND_RETURN_INFO_LOG(stageVideoTaskStatus ==
        static_cast<int32_t>(StageVideoTaskStatus::STAGE_TASK_TO_DELIVER), "moving photo video saving not yet.");

    string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    int32_t enhancementType = GetInt32Val(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, resultSet);
    UpdateMultStagesMovingPhotoVideoCloudEnhancementType(enhancementType, fileId);
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, data, "", ""};
    MultiStagesVideoCaptureManager::GetInstance().AddVideoInternal(photoId, videoInfo, false, true);
    UpdateMultStagesMovingPhotoVideoTaskStatus(fileId, StageVideoTaskStatus::STAGE_TASK_DELIVERED);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} Moving photo mulit stage video task has been delivered",
        MLOG_TAG, __FUNCTION__, __LINE__);
}
} // Media
} // OHOS