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

#define MLOG_TAG "MultiStagesCaptureDeferredPhotoProcSessionCallback"

#include "multistages_capture_deferred_photo_proc_session_callback.h"

#include "database_adapter.h"
#include "file_utils.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "medialibrary_asset_operations.h"
#include "multistages_capture_dao.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_tracer.h"
#include "multistages_capture_manager.h"
#include "multistages_capture_notify.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_notify_info.h"
#include "multistages_capture_request_task_manager.h"
#include "multistages_moving_photo_capture_manager.h"
#include "medialibrary_notify_new.h"
#include "result_set_utils.h"
#include "media_change_effect.h"
#include "exif_metadata.h"
#include "picture_adapter.h"
#include "high_quality_scan_file_callback.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif

using namespace std;
using namespace OHOS::CameraStandard;
using namespace OHOS::Media::Notification;

constexpr int32_t ORIENTATION_0 = 1;
constexpr int32_t ORIENTATION_90 = 6;
constexpr int32_t ORIENTATION_180 = 3;
constexpr int32_t ORIENTATION_270 = 8;
constexpr uint32_t MANUAL_ENHANCEMENT = 1;
constexpr uint32_t AUTO_ENHANCEMENT = 1 << 1;
constexpr uint32_t MOVINGPHOTO_VIDEO_ENHANCEMENT = 1 << 2;
static constexpr int32_t BOTH = 2;

static const std::unordered_map<int, int> ORIENTATION_MAP = {
    {0, ORIENTATION_0},
    {90, ORIENTATION_90},
    {180, ORIENTATION_180},
    {270, ORIENTATION_270}
};

const std::string HIGH_TEMPERATURE = "high_temperature";

namespace OHOS {
namespace Media {
MultiStagesCaptureDeferredPhotoProcSessionCallback::MultiStagesCaptureDeferredPhotoProcSessionCallback()
{}

MultiStagesCaptureDeferredPhotoProcSessionCallback::~MultiStagesCaptureDeferredPhotoProcSessionCallback()
{}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::NotifyIfTempFile(
    const std::shared_ptr<FileAsset> &fileAsset, bool isError)
{
    CHECK_AND_RETURN_LOG(fileAsset != nullptr, "resultSet is nullptr");
    string displayName = fileAsset->GetDisplayName();
    string filePath = fileAsset->GetFilePath();
    int32_t mediaType = fileAsset->GetMediaType();
    int32_t fileId = fileAsset->GetId();

    auto watch = MediaLibraryNotify::GetInstance();
    CHECK_AND_RETURN_LOG(watch != nullptr, "get instance notify failed NotifyIfTempFile abortion");

    string extrUri = MediaFileUtils::GetExtraUri(displayName, filePath);
    auto notifyUri = MediaFileUtils::GetUriByExtrConditions(ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(
        static_cast<MediaType>(mediaType), MEDIA_API_VERSION_V10) + "/", to_string(fileId), extrUri);
    notifyUri = MediaFileUtils::GetUriWithoutDisplayname(notifyUri);
    if (isError) {
        notifyUri += HIGH_TEMPERATURE;
    }
    MEDIA_DEBUG_LOG("MultistagesCapture notify: %{public}s", notifyUri.c_str());
    watch->Notify(notifyUri, NOTIFY_UPDATE);
}

int32_t MultiStagesCaptureDeferredPhotoProcSessionCallback::UpdatePhotoQuality(const int32_t &fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePhotoQuality " + std::to_string(fileId));
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t updatePhotoQualityResult = DatabaseAdapter::Update(updateCmd);
    return updatePhotoQualityResult;
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::UpdatePhotoQuality(const int32_t &fileId,
    NativeRdb::ValuesBucket &updateValues)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePhotoQuality " + std::to_string(fileId));
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
}

static void CheckMovingPhotoFlag(uint32_t cloudImageEnhanceFlag, NativeRdb::ValuesBucket &updateValues)
{
    MEDIA_INFO_LOG("start CheckMovingPhotoFlag cloudImageEnhanceFlag: %{public}d", cloudImageEnhanceFlag);
    if (cloudImageEnhanceFlag & MOVINGPHOTO_VIDEO_ENHANCEMENT) {
        updateValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
        updateValues.PutInt(PhotoColumn::PHOTO_MOVINGPHOTO_ENHANCEMENT_TYPE, BOTH);
    }
    return;
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::UpdateCEAvailable(const int32_t& fileId,
    uint32_t cloudImageEnhanceFlag, NativeRdb::ValuesBucket &updateValues, int32_t modifyType)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d, modify type is %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, fileId, modifyType);

    int32_t ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::SUPPORT);
    if (modifyType == static_cast<int32_t>(FirstStageModifyType::EDITED)) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d edited",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::EDIT);
    } else if (modifyType == static_cast<int32_t>(FirstStageModifyType::TRASHED)) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d trashed",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        ceAvailable = static_cast<int32_t>(CloudEnhancementAvailableType::TRASH);
    }

    if (cloudImageEnhanceFlag & AUTO_ENHANCEMENT) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d is AUTO_ENHANCEMENT",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        updateValues.PutInt(PhotoColumn::PHOTO_IS_AUTO, static_cast<int32_t>(CloudEnhancementIsAutoType::AUTO));
        updateValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
        CheckMovingPhotoFlag(cloudImageEnhanceFlag, updateValues);
    } else if (cloudImageEnhanceFlag & MANUAL_ENHANCEMENT) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d is MANUAL_ENHANCEMENT",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        updateValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE, ceAvailable);
        CheckMovingPhotoFlag(cloudImageEnhanceFlag, updateValues);
    } else {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} fileId: %{public}d doesn't support enhancement",
            MLOG_TAG, __FUNCTION__, __LINE__, fileId);
        updateValues.PutInt(PhotoColumn::PHOTO_CE_AVAILABLE,
            static_cast<int32_t>(CloudEnhancementAvailableType::NOT_SUPPORT));
    }
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleOnError(
    const string &imageId, const DpsErrorCode error)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} error %{public}d, photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, static_cast<int32_t>(error), imageId.c_str());
    switch (error) {
        case ERROR_SESSION_SYNC_NEEDED:
            MultiStagesPhotoCaptureManager::GetInstance().SyncWithDeferredProcSession();
            break;
        case ERROR_IMAGE_PROC_INVALID_PHOTO_ID:
        case ERROR_IMAGE_PROC_FAILED: {
            const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
                PhotoColumn::MEDIA_TYPE, MediaColumn::MEDIA_NAME };
            auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(imageId, columns);
            MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(imageId, false);
            if (fileAsset == nullptr) {
                MEDIA_ERR_LOG("fileAsset set is empty.");
                return;
            }
            UpdatePhotoQuality(fileAsset->GetId());
            MultiStagesCaptureDao().UpdatePhotoDirtyNew(fileAsset->GetId());
            MEDIA_ERR_LOG("error %{public}d, photoid: %{public}s", static_cast<int32_t>(error), imageId.c_str());
            break;
        }
        case ERROR_IMAGE_PROC_ABNORMAL: {
            const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
                PhotoColumn::MEDIA_TYPE, MediaColumn::MEDIA_NAME };
            auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(imageId, columns);
            if (fileAsset == nullptr) {
                MEDIA_ERR_LOG("fileAsset set is empty.");
                return;
            }
            MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_ERROR_IMAGE);
            NotifyIfTempFile(fileAsset, true);
            MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(imageId);
            break;
        }
        default:
            MultiStagesCaptureRequestTaskManager::ClearPhotoInProcessRequestCount(imageId);
            break;
    }

    if (error != ERROR_SESSION_SYNC_NEEDED) {
        int32_t mediaType = (MultiStagesVideoCaptureManager::QuerySubType(imageId) ==
            static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)) ?
            static_cast<int32_t>(MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE) :
            static_cast<int32_t>(MultiStagesCaptureMediaType::IMAGE);
        MultiStagesCaptureDfxResult::Report(imageId, static_cast<int32_t>(error), mediaType);
    }
    CallProcessImageDone(false, imageId);
}
void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnError(const string &imageId, const DpsErrorCode error)
{
    HandleOnError(imageId, error);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage();
}

static void HandleOrientation(const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> picture)
{
    if (fileAsset == nullptr || picture == nullptr) {
        MEDIA_ERR_LOG("fileAsset or picture is nullptr.");
        return;
    }
    int32_t orientation = fileAsset->GetOrientation();
    if (orientation != 0) {
        auto metadata = picture->GetExifMetadata();
        CHECK_AND_RETURN_LOG(metadata != nullptr, "metadata is null");
        auto imageSourceOrientation = ORIENTATION_MAP.find(orientation);
        CHECK_AND_RETURN_LOG(imageSourceOrientation != ORIENTATION_MAP.end(),
            "imageSourceOrientation value is invalid");
        metadata->SetValue(PHOTO_DATA_IMAGE_ORIENTATION, std::to_string(imageSourceOrientation->second));
    }
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::ProcessAndSaveHighQualityImage(
    const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> picture,
    uint32_t cloudImageEnhanceFlag)
{
    if (fileAsset == nullptr) {
        MEDIA_ERR_LOG("fileAsset is nullptr");
        return;
    }
    HandleOrientation(fileAsset, picture);

    string imageId = fileAsset->GetPhotoId();
    string data = fileAsset->GetPath();
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    int32_t fileId = fileAsset->GetId();
    string mimeType = fileAsset->GetMimeType();
    bool isMovingPhoto = (fileAsset->GetPhotoSubType() ==
        static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    int32_t mediaType = isMovingPhoto ? static_cast<int32_t>(MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE) :
        static_cast<int32_t>(MultiStagesCaptureMediaType::IMAGE);
    std::shared_ptr<Media::Picture> resultPicture = nullptr;
    bool isTakeEffect = false;
    int ret = MediaLibraryPhotoOperations::ProcessMultistagesPhotoForPicture(
        fileAsset, picture, resultPicture, isTakeEffect);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Save high quality image failed. ret: %{public}d, errno: %{public}d", ret, errno);
        MultiStagesCaptureDfxResult::Report(imageId,
            static_cast<int32_t>(MultiStagesCaptureResultErrCode::SAVE_IMAGE_FAIL), mediaType);
        return;
    }
    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(
        imageId, std::move(picture), isEdited, isTakeEffect);
    bool isTrashed = fileAsset->GetIsTrash() > 0;
    int32_t modifyType = isEdited ? static_cast<int32_t>(FirstStageModifyType::EDITED) :
        (isTrashed ? static_cast<int32_t>(FirstStageModifyType::TRASHED) :
            static_cast<int32_t>(FirstStageModifyType::NOT_MODIFIED));
    UpdateHighQualityPictureInfo(fileId, cloudImageEnhanceFlag, modifyType);
    MediaLibraryObjectUtils::ScanFileAsync(
        data, to_string(fileId), MediaLibraryApi::API_10, isMovingPhoto, resultPicture,
        HighQualityScanFileCallback::Create(fileId));
    MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    NotifyIfTempFile(fileAsset);

    MultiStagesCaptureDfxTotalTime::GetInstance().Report(imageId, mediaType);
    MultiStagesCaptureDfxResult::Report(imageId,
        static_cast<int32_t>(MultiStagesCaptureResultErrCode::SUCCESS), mediaType);
    if (isMovingPhoto) {
        MultiStagesMovingPhotoCaptureManager::AddVideoFromMovingPhoto(fileId);
    }
}

std::shared_ptr<Media::Picture> GetPictureFromPictureIntf(std::shared_ptr<CameraStandard::PictureIntf> pictureIntf)
{
    if (pictureIntf == nullptr) {
        return nullptr;
    }
    auto pictureAdapter = reinterpret_cast<CameraStandard::PictureAdapter*>(pictureIntf.get());
    if (pictureAdapter == nullptr) {
        return nullptr;
    }
    return pictureAdapter->GetPicture();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleForNullData(const std::string &imageId,
    std::shared_ptr<Media::Picture> picture)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} result set is empty",
        MLOG_TAG, __FUNCTION__, __LINE__);
    if (picture != nullptr) {
        MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(imageId, std::move(picture));
    }
    MultiStagesCaptureDfxTotalTime::GetInstance().RemoveStartTime(imageId);
    // When subType query failed, default mediaType is Image
    MultiStagesCaptureDfxResult::Report(imageId, static_cast<int32_t>(MultiStagesCaptureResultErrCode::SQL_ERR),
        static_cast<int32_t>(MultiStagesCaptureMediaType::IMAGE));
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleForIsTemp(
    const std::shared_ptr<FileAsset> &fileAsset, std::shared_ptr<Media::Picture> &picture,
    uint32_t cloudImageEnhanceFlag)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture, this picture is temp.",
        MLOG_TAG, __FUNCTION__, __LINE__);
    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(fileAsset->GetPhotoId(),
        std::move(picture));
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    bool isTrashed = fileAsset->GetIsTrash() > 0;
    int32_t modifyType = isEdited ? static_cast<int32_t>(FirstStageModifyType::EDITED) :
        (isTrashed ? static_cast<int32_t>(FirstStageModifyType::TRASHED) :
            static_cast<int32_t>(FirstStageModifyType::NOT_MODIFIED));
    UpdateHighQualityPictureInfo(fileAsset->GetId(), cloudImageEnhanceFlag, modifyType);
    MultiStagesCaptureDao().UpdatePhotoDirtyNew(fileAsset->GetId());
}
void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleOnProcessImageDone(const std::string &imageId,
    std::shared_ptr<CameraStandard::PictureIntf> pictureIntf, uint32_t cloudImageEnhanceFlag)
{
MediaLibraryTracer tracer;
    tracer.Start("OnProcessImageDone with PictureIntf " + imageId);
    std::shared_ptr<Media::Picture> picture = GetPictureFromPictureIntf(pictureIntf);
    if (picture == nullptr || picture->GetMainPixel() == nullptr) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture picture is null",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return;
    }
    // 1. 分段式拍照已经处理完成，保存全质量图
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture yuv photoid: %{public}s, cloudImageEnhanceFlag: %{public}u enter",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), cloudImageEnhanceFlag);
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_IS_TEMP, PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_ORIENTATION, MediaColumn::MEDIA_DATE_TRASHED };
    tracer.Start("Query");
    auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(imageId, columns);
    tracer.Finish();
    if (fileAsset == nullptr) {
        HandleForNullData(imageId, picture);
        return;
    }
    if (fileAsset->GetPhotoIsTemp()) {
        HandleForIsTemp(fileAsset, picture, cloudImageEnhanceFlag);
        return;
    }
    tracer.Start("ProcessAndSaveHighQualityImage");
    ProcessAndSaveHighQualityImage(fileAsset, picture, cloudImageEnhanceFlag);
    tracer.Finish();

    CallProcessImageDone(true, imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "MultistagesCapture yuv success photoid: %{public}s, fileid: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), fileAsset->GetId());
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessImageDone(const std::string &imageId,
    std::shared_ptr<CameraStandard::PictureIntf> pictureIntf, uint32_t cloudImageEnhanceFlag)
{
    HandleOnProcessImageDone(imageId, pictureIntf, cloudImageEnhanceFlag);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::GetCommandByImageId(const std::string &imageId,
    MediaLibraryCommand &cmd)
{
    size_t slashIndex = imageId.rfind("/");
    string where = "";
    vector<string> whereArgs;
    if (slashIndex != string::npos) {
        string fileId = MediaFileUtils::GetIdFromUri(imageId);
        where = PhotoColumn::MEDIA_ID + " = ? ";
        whereArgs = { fileId };
    }
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    cmd.GetAbsRdbPredicates()->SetWhereArgs(whereArgs);
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::UpdateHighQualityPictureInfo(const int32_t &fileId,
    uint32_t cloudImageEnhanceFlag, int32_t modifyType)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "UpdateHighQualityPictureInfo enter, fileId: %{public}d, "
        "cloudImageEnhanceFlag: %{public}u, modifyType: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        fileId, cloudImageEnhanceFlag, modifyType);
    NativeRdb::ValuesBucket updateValues;
    // 2. 更新数据库 photoQuality 到高质量
    UpdatePhotoQuality(fileId, updateValues);
    // 3. update cloud enhancement avaiable
    if (cloudImageEnhanceFlag) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} UpdateHighQualityPictureInfo UpdateCEAvailable enter",
            MLOG_TAG, __FUNCTION__, __LINE__);
        UpdateCEAvailable(fileId, cloudImageEnhanceFlag, updateValues, modifyType);
    }

    CHECK_AND_RETURN_LOG(!updateValues.IsEmpty(), "UpdateHighQualityPictureInfo failed, updateValues is null");
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t updatePhotoResult = DatabaseAdapter::Update(updateCmd);
    CHECK_AND_RETURN_LOG(updatePhotoResult >= 0, "UpdateHighQualityPictureInfo fail, fileId: %{public}d", fileId);
}


void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnDeliveryLowQualityImage(const std::string &imageId,
    std::shared_ptr<PictureIntf> pictureIntf)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());
    std::shared_ptr<Media::Picture> picture = GetPictureFromPictureIntf(pictureIntf);
    if (picture != nullptr && picture->GetMainPixel() != nullptr) {
        MEDIA_ERR_LOG("MultistagesCapture picture is not null");
    } else {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} MultistagesCapture picture is null",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return;
    }
    MediaLibraryTracer tracer;
    tracer.Start("OnDeliveryLowQualityImage " + imageId);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    GetCommandByImageId(imageId, cmd);
    vector<string> columns { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH, PhotoColumn::PHOTO_EDIT_TIME,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_ID};
    tracer.Start("Query");
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        tracer.Finish();
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture result set is empty",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return;
    }
    tracer.Finish();
    string photoId = GetStringVal(PhotoColumn::PHOTO_ID, resultSet);
    string data = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    bool isEdited = (GetInt64Val(PhotoColumn::PHOTO_EDIT_TIME, resultSet) > 0);
    resultSet->Close();
    MultiStagesPhotoCaptureManager::GetInstance().DealLowQualityPicture(photoId, std::move(picture), isEdited);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture save low quality image end",
        MLOG_TAG, __FUNCTION__, __LINE__);
}

int32_t GetModifyType(std::shared_ptr<FileAsset> fileAsset)
{
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    bool isTrashed = fileAsset->GetIsTrash() > 0;
    int32_t modifyType = isEdited ? static_cast<int32_t>(FirstStageModifyType::EDITED) :
        (isTrashed ? static_cast<int32_t>(FirstStageModifyType::TRASHED) :
            static_cast<int32_t>(FirstStageModifyType::NOT_MODIFIED));
    return modifyType;
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleOnProcessImageDone(
    const string &imageId, const uint8_t *addr, const long bytes, uint32_t cloudImageEnhanceFlag)
{
    CHECK_AND_RETURN_LOG((addr != nullptr) && (bytes != 0), "addr is nullptr or bytes(%{public}ld) is 0", bytes);
    CHECK_AND_RETURN_LOG(MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(imageId),
        "this photo was delete or err photoId: %{public}s", imageId.c_str());
    MediaLibraryTracer tracer;
    tracer.Start("OnProcessImageDone with addr " + imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "photoid: %{public}s, bytes: %{public}ld, cloudImageEnhanceFlag: %{public}u enter",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), bytes, cloudImageEnhanceFlag);
    const std::vector<std::string> columns = { MediaColumn::MEDIA_ID, MediaColumn::MEDIA_FILE_PATH,
        PhotoColumn::MEDIA_TYPE, MediaColumn::MEDIA_NAME, MediaColumn::MEDIA_MIME_TYPE,
        PhotoColumn::PHOTO_SUBTYPE, PhotoColumn::PHOTO_IS_TEMP, PhotoColumn::PHOTO_ID,
        PhotoColumn::PHOTO_EDIT_TIME, PhotoColumn::PHOTO_ORIENTATION, MediaColumn::MEDIA_DATE_TRASHED };
    tracer.Start("Query");
    auto fileAsset = MultiStagesCaptureDao().QueryDataByPhotoId(imageId, columns);
    tracer.Finish();
    if (fileAsset == nullptr) {
        HandleForNullData(imageId, nullptr);
        return;
    }
    int32_t modifyType = GetModifyType(fileAsset);
    bool isMovingPhoto = (fileAsset->GetPhotoSubType() == static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    int32_t mediaType = isMovingPhoto ? static_cast<int32_t>(MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE) :
        static_cast<int32_t>(MultiStagesCaptureMediaType::IMAGE);
    int ret = MediaLibraryPhotoOperations::ProcessMultistagesPhoto(fileAsset, addr, bytes);
    if (ret != E_OK) {
        HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
            "Save high quality image failed. ret: %{public}d, errno: %{public}d",
            MLOG_TAG, __FUNCTION__, __LINE__, ret, errno);
        MultiStagesCaptureDfxResult::Report(imageId,
            static_cast<int32_t>(MultiStagesCaptureResultErrCode::SAVE_IMAGE_FAIL), mediaType);
        return;
    }
    UpdateHighQualityPictureInfo(fileAsset->GetId(), cloudImageEnhanceFlag, modifyType);
    MediaLibraryObjectUtils::ScanFileAsync(fileAsset->GetPath(), to_string(fileAsset->GetId()),
        MediaLibraryApi::API_10, isMovingPhoto, nullptr, HighQualityScanFileCallback::Create(fileAsset->GetId()));
    MultistagesCaptureNotify::NotifyOnProcess(fileAsset, MultistagesCaptureNotifyType::ON_PROCESS_IMAGE_DONE);
    NotifyIfTempFile(fileAsset);
    MultiStagesCaptureDfxTotalTime::GetInstance().Report(imageId, mediaType);
    MultiStagesCaptureDfxResult::Report(imageId,
        static_cast<int32_t>(MultiStagesCaptureResultErrCode::SUCCESS), mediaType);
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(imageId, false);
    if (isMovingPhoto) {
        MultiStagesMovingPhotoCaptureManager::AddVideoFromMovingPhoto(fileAsset->GetId());
        MEDIA_INFO_LOG("AddVideoFromMovingPhoto finish");
    }
    CallProcessImageDone(true, imageId);
}
void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessImageDone(const string &imageId, const uint8_t *addr,
    const long bytes, uint32_t cloudImageEnhanceFlag)
{
    HandleOnProcessImageDone(imageId, addr, bytes, cloudImageEnhanceFlag);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage();
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnStateChanged(const DpsStatusCode state)
{
    MEDIA_INFO_LOG("OnStateChanged, status: %{public}d", state);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    EnhancementManager::GetInstance().HandleStateChangedOperation(state == DpsStatusCode::SESSION_STATE_IDLE);
#endif
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::SetProcessImageDoneCallback(const ProcessDoneHandler &func)
{
    processDoneCallback_ = func;
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::CallProcessImageDone(bool success, const std::string &photoId)
{
    if (processDoneCallback_ != nullptr) {
        processDoneCallback_(success, photoId);
    }
}
} // namespace Media
} // namespace OHOS