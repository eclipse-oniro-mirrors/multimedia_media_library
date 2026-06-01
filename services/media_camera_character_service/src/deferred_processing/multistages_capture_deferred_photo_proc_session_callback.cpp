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

#include <sstream>
#include <sys/mman.h>

#include "camera_asset_pipeline.h"
#include "camera_character_types.h"
#include "database_adapter.h"
#include "file_utils.h"
#include "media_exif.h"
#include "media_file_utils.h"
#include "media_file_uri.h"
#include "media_log.h"
#include "media_uri_utils.h"
#include "medialibrary_asset_operations.h"
#include "multistages_capture_dao.h"
#include "medialibrary_errno.h"
#include "medialibrary_notify.h"
#include "medialibrary_object_utils.h"
#include "medialibrary_photo_operations.h"
#include "medialibrary_tracer.h"
#include "multistages_camera_capture_manager.h"
#include "multistages_capture_manager.h"
#include "multistages_capture_notify.h"
#include "multistages_capture_dfx_result.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_dfx_save_camera_photo.h"
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
#include "refresh_business_name.h"

using namespace std;
using namespace OHOS::CameraStandard;
using namespace OHOS::Media::Notification;

constexpr int32_t ORIENTATION_0 = 1;
constexpr int32_t ORIENTATION_90 = 6;
constexpr int32_t ORIENTATION_180 = 3;
constexpr int32_t ORIENTATION_270 = 8;
constexpr uint32_t MANUAL_ENHANCEMENT = 1;
constexpr const char* CLOUD_FLAG = "cloudImageEnhanceFlag";
constexpr const char* CPATURE_FLAG = "captureEnhancementFlag";
constexpr const char* EDIT_DATA_FLAG = "editData";
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
static const std::unordered_map<DpsErrorCode, MediaDpsErrorCode> ERROR_CODE_MAP = {
    { ERROR_IMAGE_PROC_INVALID_PHOTO_ID,    MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INVALID_PHOTO_ID },
    { ERROR_IMAGE_PROC_FAILED,              MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_FAILED },
    { ERROR_IMAGE_PROC_TIMEOUT,             MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_TIMEOUT },
    { ERROR_IMAGE_PROC_ABNORMAL,            MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_ABNORMAL },
    { ERROR_IMAGE_PROC_INTERRUPTED,         MediaDpsErrorCode::MEDIA_ERROR_IMAGE_PROC_INTERRUPTED },
};

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
    auto notifyUri = MediaFileUtils::GetUriByExtrConditions(CONST_ML_FILE_URI_PREFIX + MediaFileUri::GetMediaTypeUri(
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

static MediaDpsErrorCode ConvertDpsErrorCode(const DpsErrorCode errorCode)
{
    if (ERROR_CODE_MAP.find(errorCode) == ERROR_CODE_MAP.end()) {
        MEDIA_ERR_LOG("errorCode is not in map, %{public}d.", static_cast<int32_t>(errorCode));
        return MediaDpsErrorCode::UNDEFINED;
    }
    return ERROR_CODE_MAP.at(errorCode);
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleOnError(
    const string &imageId, const DpsErrorCode error)
{
    HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} error %{public}d, photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, static_cast<int32_t>(error), imageId.c_str());

    if (error == ERROR_SESSION_SYNC_NEEDED) {
        MultiStagesPhotoCaptureManager::GetInstance().SyncWithDeferredProcSession();
        return;
    }

    auto errorCode = ConvertDpsErrorCode(error);
    CameraPipelineType type = CameraPipelineType::UNDEFINED;
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoId(imageId, type);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("pipeline is nullptr.");
        return;
    }
    bool isMovingPhoto = false;
    bool ret = pipeline->OnErrorImage(errorCode, isMovingPhoto);
    if (error != ERROR_SESSION_SYNC_NEEDED && ret) {
        int32_t mediaType = isMovingPhoto ? static_cast<int32_t>(MultiStagesCaptureMediaType::MOVING_PHOTO_IMAGE)
                                          : static_cast<int32_t>(MultiStagesCaptureMediaType::IMAGE);
        MultiStagesCaptureDfxResult::Report(imageId, static_cast<int32_t>(error), mediaType);
    }
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnError(const string &imageId, const DpsErrorCode error)
{
    HandleOnError(imageId, error);
    if (error == ERROR_IMAGE_PROC_INVALID_PHOTO_ID || error == ERROR_IMAGE_PROC_FAILED) {
        size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithPhotoId(imageId, false);
        MEDIA_INFO_LOG("DeletePipelineWithPhotoId count: %{public}zu.", count);
    }
    CallProcessImageDone(false, imageId);
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
    }
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

static MediaDpsMetadata ConvertDpsMetadata(const DpsMetadata &metadata)
{
    MediaDpsMetadata mediaMetadata;
    mediaMetadata.isReady = true;
    metadata.Get(CLOUD_FLAG, mediaMetadata.cloudImageEnhanceFlag);
    metadata.Get(CPATURE_FLAG, mediaMetadata.captureEnhancementFlag);
    metadata.Get(EDIT_DATA_FLAG, mediaMetadata.editData);
    return mediaMetadata;
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::HandleForNullData(const std::string &imageId,
    std::shared_ptr<Media::Picture> picture)
{
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} result set is empty",
        MLOG_TAG, __FUNCTION__, __LINE__);
    if (picture != nullptr) {
        MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(imageId, 0, std::move(picture));
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
    MultiStagesPhotoCaptureManager::GetInstance().DealHighQualityPicture(fileAsset->GetPhotoId(), fileAsset->GetId(),
        std::move(picture));
    bool isEdited = fileAsset->GetPhotoEditTime() > 0;
    bool isTrashed = fileAsset->GetIsTrash() > 0;
    int32_t modifyType = isEdited ? static_cast<int32_t>(FirstStageModifyType::EDITED) :
        (isTrashed ? static_cast<int32_t>(FirstStageModifyType::TRASHED) :
            static_cast<int32_t>(FirstStageModifyType::NOT_MODIFIED));
    UpdateHighQualityPictureInfo(fileAsset->GetId(), cloudImageEnhanceFlag, modifyType);
    MultiStagesCaptureDao().UpdatePhotoDirtyNew(fileAsset->GetId());
    NotifyIfTempFile(fileAsset);
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

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessImageDone(
    const std::string &imageId, std::shared_ptr<CameraStandard::PictureIntf> pictureIntf, const DpsMetadata &metadata)
{
    MediaLibraryTracer tracer;
    tracer.Start("OnProcessImageDone with yuv " + imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture yuv photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());
 
    OnProcessInternal(imageId, pictureIntf, metadata);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage(); // 无论执行结果, 都需要通知
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessInternal(const std::string &imageId,
    std::shared_ptr<CameraStandard::PictureIntf> pictureIntf, const DpsMetadata &metadata)
{
    // 1.参数转化&校验
    OnProcessImageWrapper param;
    CHECK_AND_RETURN_LOG(ConvertOnProcessParam(pictureIntf, metadata, param), "invalid input.");

    // 2.执行
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoIdWithExpected(
        imageId, CameraPipelineType::YUV);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, photoId: %{public}s.", imageId.c_str());
        return;
    }
    int32_t ret = pipeline->OnProcessImageDone(param);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed execute pipeline->OnProcessImageDone.");
        return;
    }

    // 3.清理pipeline
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithPhotoId(imageId, false);
    CallProcessImageDone(true, imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} success yuv photoid: %{public}s, count: %{public}zu",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), count);
}

bool MultiStagesCaptureDeferredPhotoProcSessionCallback::ConvertOnProcessParam(
    std::shared_ptr<CameraStandard::PictureIntf> pictureIntf, const DpsMetadata &metadata,
    OnProcessImageWrapper& wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertOnProcessParam");

    // 获取yuv
    OnProcessParamForYuv yuv;
    std::shared_ptr<Media::Picture> picture = GetPictureFromPictureIntf(pictureIntf);
    CHECK_AND_RETURN_RET_LOG(picture != nullptr && picture->GetMainPixel() != nullptr, false, "picture is nullptr.");
    yuv.picture = std::move(picture);

    // metadata
    auto mediaMetadata = ConvertDpsMetadata(metadata);

    wrapper.yuv = yuv;
    wrapper.metadata = mediaMetadata;
    MEDIA_INFO_LOG("ConvertOnProcessParam success, metadata: %{public}s", wrapper.metadata.ToString().c_str());
    return true;
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
    std::shared_ptr<CameraStandard::PictureIntf> pictureIntf)
{
    std::shared_ptr<Media::Picture> picture = GetPictureFromPictureIntf(pictureIntf);
    if (picture == nullptr || picture->GetMainPixel() == nullptr) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} MultistagesCapture picture is null",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return;
    }

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture uri: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());
    MediaLibraryTracer tracer;
    tracer.Start("OnDeliveryLowQualityImage " + imageId);

    // 1.从缓存中获取对象
    int32_t fileId = MediaUriUtils::GetFileId(imageId);
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        fileId, CameraPipelineType::YUV);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, fileId: %{public}d.", fileId);
        return;
    }

    // 2.存入缓存
    pipeline->OnDelivery(picture);
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
    MediaLibraryTracer tracer;
    tracer.Start("OnProcessImageDone with image " + imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture Image photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());

    OnProcessInternal(imageId, addr, bytes, cloudImageEnhanceFlag);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage(); // 无论执行结果, 都需要通知
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessInternal(const string &imageId, const uint8_t *addr,
    const long bytes, uint32_t cloudImageEnhanceFlag)
{
    CHECK_AND_RETURN_LOG(MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(imageId),
        "this photo was delete or err photoId: %{public}s", imageId.c_str());

    // 1.参数转化&校验
    OnProcessImageWrapper param;
    CHECK_AND_RETURN_LOG(ConvertOnProcessParam(addr, bytes, cloudImageEnhanceFlag, param), "invalid input.");

    // 2.执行
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoIdWithExpected(
        imageId, CameraPipelineType::IMAGE);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, photoId: %{public}s.", imageId.c_str());
        return;
    }
    int32_t ret = pipeline->OnProcessImageDone(param);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed execute pipeline->OnProcessImageDone.");
        return;
    }

    // 3.清理数据
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithPhotoId(imageId, false);
    CallProcessImageDone(true, imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} success image photoid: %{public}s, count: %{public}zu",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), count);
}

bool MultiStagesCaptureDeferredPhotoProcSessionCallback::ConvertOnProcessParam(const uint8_t *addr, const long bytes,
    uint32_t cloudImageEnhanceFlag, OnProcessImageWrapper& wrapper)
{
    CHECK_AND_RETURN_RET_LOG((addr != nullptr) && (bytes > 0), false, "addr is nullptr or bytes is zero.");
    MediaLibraryTracer tracer;
    tracer.Start("ConvertOnProcessParam");

    // image
    ImageFileMapper fileMapper = {
        .addr = (void*)addr,
        .bytes = static_cast<int64_t>(bytes),
    };
    OnProcessParamForImage image;
    image.file = fileMapper;

    // metadata
    MediaDpsMetadata mediaMetadata;
    mediaMetadata.cloudImageEnhanceFlag = cloudImageEnhanceFlag;

    wrapper.image = image;
    wrapper.metadata = mediaMetadata;
    MEDIA_INFO_LOG("ConvertOnProcessParam success, metadata: %{public}s", wrapper.metadata.ToString().c_str());
    return true;
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


void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnDeliveryLowQualityLcd(const std::string &imageId,
    std::shared_ptr<CameraStandard::PictureIntf> picture)
{
    std::shared_ptr<Media::Picture> lcdPicture = GetPictureFromPictureIntf(picture);
    if (lcdPicture == nullptr || lcdPicture->GetMainPixel() == nullptr) {
        HILOG_COMM_ERROR("%{public}s:{%{public}s:%{public}d} MultistagesCapture picture is null",
            MLOG_TAG, __FUNCTION__, __LINE__);
        return;
    }

    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture uri: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());
    MediaLibraryTracer tracer;
    tracer.Start("OnDeliveryLowQualityLCD " + imageId);

    // 1.从缓存中获取对象
    int32_t fileId = MediaUriUtils::GetFileId(imageId);
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByFileIdWithExpected(
        fileId, CameraPipelineType::NEW_IMAGE);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, fileId: %{public}d.", fileId);
        return;
    }

    // 2.存入缓存
    pipeline->OnDelivery(lcdPicture);
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessImageDone(const std::string &imageId,
    const std::vector<CameraStandard::ImageFd> &imageFds, std::shared_ptr<CameraStandard::PictureIntf> lcdImage,
    const DpsMetadata& metadata)
{
    MediaLibraryTracer tracer;
    tracer.Start("OnProcessImageDone with newImage " + imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture newImage photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());

    OnProcessInternal(imageId, imageFds, lcdImage, metadata);
    MultiStagesPhotoCaptureManager::GetInstance().NotifyProcessImage(); // 无论执行结果, 都需要通知
}

void MultiStagesCaptureDeferredPhotoProcSessionCallback::OnProcessInternal(const std::string &imageId,
    const std::vector<CameraStandard::ImageFd> &imageFds, std::shared_ptr<CameraStandard::PictureIntf> lcdImage,
    const DpsMetadata& metadata)
{
    CHECK_AND_RETURN_LOG(MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(imageId),
        "this photo was delete or err photoId: %{public}s", imageId.c_str());

    // 1.参数转换&校验
    OnProcessImageWrapper param;
    CHECK_AND_RETURN_LOG(ConvertOnProcessParam(imageFds, lcdImage, metadata, param), "invalid input.");

    // 2.执行
    auto pipeline = MultistagesCameraCaptureManager::GetInstance().GetPipelineByPhotoIdWithExpected(
        imageId, CameraPipelineType::NEW_IMAGE);
    if (pipeline == nullptr) {
        MEDIA_ERR_LOG("failed, photoId: %{public}s.", imageId.c_str());
        return;
    }
    int32_t ret = pipeline->OnProcessImageDone(param);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("failed execute pipeline->OnProcessImageDone.");
        return;
    }

    // 3.清理pipeline
    size_t count = MultistagesCameraCaptureManager::GetInstance().DeletePipelineWithPhotoId(imageId, false);
    CallProcessImageDone(true, imageId);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} success newImage photoid: %{public}s, count: %{public}zu",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str(), count);
}

static bool convertImageFileMapper(const CameraStandard::ImageFd& imageFd,
    std::string& imageType, ImageFileMapper& imageFileMapper)
{
    CHECK_AND_RETURN_RET_LOG(imageFd.addr != nullptr && imageFd.bytes != 0, false, "bytes is zero.");
    imageFileMapper.addr = (void*)imageFd.addr;
    imageFileMapper.bytes = imageFd.bytes;

    // 解析类型
    if (imageFd.imageType == ImageType::EFFECTIVE_IMAGE) {
        imageType = IMAGE_FILE_EDITED_TYPE;
    } else if (imageFd.imageType == ImageType::ORIGINAL_IMAGE) {
        imageType = IMAGE_FILE_SOURCE_TYPE;
    }
    MEDIA_INFO_LOG("convertImageFileMapper imageType: %{public}s, bytes: %{public}d.",
        imageType.c_str(), imageFd.bytes);
    return true;
}

bool MultiStagesCaptureDeferredPhotoProcSessionCallback::ConvertOnProcessParam(
    const std::vector<CameraStandard::ImageFd>& imageFds, std::shared_ptr<CameraStandard::PictureIntf> lcdImage,
    const DpsMetadata& metadata, OnProcessImageWrapper& wrapper)
{
    MediaLibraryTracer tracer;
    tracer.Start("ConvertOnProcessParam");

    OnProcessParamForNewImage newImage;
    // 获取fd
    for (const auto& imageFd : imageFds) {
        std::string imageType;
        ImageFileMapper imageFileMapper;
        CHECK_AND_RETURN_RET_LOG(convertImageFileMapper(imageFd, imageType, imageFileMapper), false,
            "failed to get imageFileMapper.");
        newImage.files.insert(std::make_pair(imageType, imageFileMapper));
    }
    // lcd 的yuv对象
    std::shared_ptr<Media::Picture> lcdPicture = GetPictureFromPictureIntf(lcdImage);
    newImage.lcdImage = std::move(lcdPicture);

    // matedata
    auto mediaMetadata = ConvertDpsMetadata(metadata);

    wrapper.newImage = newImage;
    wrapper.metadata = mediaMetadata;
    MEDIA_INFO_LOG("ConvertOnProcessParam success, newImage: %{public}s, metadata: %{public}s",
        wrapper.newImage.ToString().c_str(), wrapper.metadata.ToString().c_str());
    return true;
}
} // namespace Media
} // namespace OHOS