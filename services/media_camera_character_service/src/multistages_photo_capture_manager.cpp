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

#define MLOG_TAG "MultiStagesPhotoCaptureManager"

#include "multistages_capture_manager.h"

#include "database_adapter.h"
#include "image_packer.h"
#include "exif_utils.h"
#include "medialibrary_asset_operations.h"
#include "medialibrary_bundle_manager.h"
#include "medialibrary_command.h"
#include "medialibrary_errno.h"
#include "medialibrary_rdbstore.h"
#include "medialibrary_type_const.h"
#include "medialibrary_tracer.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_formmap_operations.h"
#include "picture_manager_thread.h"
#include "multistages_capture_dfx_first_visit.h"
#include "multistages_capture_dfx_request_policy.h"
#include "multistages_capture_dfx_total_time.h"
#include "multistages_capture_dfx_trigger_ratio.h"
#include "multistages_capture_request_task_manager.h"
#include "request_policy.h"
#include "result_set_utils.h"
#include "userfilemgr_uri.h"
#include "medialibrary_transcode_data_aging_operation.h"

using namespace std;
#ifdef ABILITY_CAMERA_SUPPORT
using namespace OHOS::CameraStandard;
#endif

namespace OHOS {
namespace Media {
const int32_t SAVE_PICTURE_TIMEOUT_SEC = 20;

MultiStagesPhotoCaptureManager::MultiStagesPhotoCaptureManager()
{
    deferredProcSession_ = make_shared<DeferredPhotoProcessingAdapter>();
}

MultiStagesPhotoCaptureManager::~MultiStagesPhotoCaptureManager() {}

MultiStagesPhotoCaptureManager& MultiStagesPhotoCaptureManager::GetInstance()
{
    static MultiStagesPhotoCaptureManager instance;
    return instance;
}

bool MultiStagesPhotoCaptureManager::Init()
{
    SyncWithDeferredProcSessionInternal();
    return true;
}

void MultiStagesPhotoCaptureManager::CancelRequestAndRemoveImage(const vector<string> &columns)
{
    CHECK_AND_RETURN_LOG(columns.size() >= 1, "invalid param");
    int32_t fileId = stoi(columns[0]);
    string photoId = MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(fileId);
    MEDIA_INFO_LOG("fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
    CancelProcessRequest(photoId);
    RemoveImage(photoId, false);
}

shared_ptr<OHOS::NativeRdb::ResultSet> MultiStagesPhotoCaptureManager::HandleMultiStagesOperation(
    MediaLibraryCommand &cmd, const vector<string> &columns)
{
    switch (cmd.GetOprnType()) {
        case OperationType::PROCESS_IMAGE: {
            int fileId = std::stoi(columns[0]); // 0 indicates file id
            int deliveryMode = std::stoi(columns[1]); // 1 indicates delivery mode
            ProcessImage(fileId, deliveryMode);
            MultiStagesCaptureDfxTriggerRatio::GetInstance().SetTrigger(MultiStagesCaptureTriggerType::THIRD_PART);
            break;
        }
        case OperationType::ADD_IMAGE: {
            AddImage(cmd);
            break;
        }
        case OperationType::SET_LOCATION: {
            UpdateLocation(cmd.GetValueBucket(), false);
            break;
        }
        case OperationType::CANCEL_PROCESS_IMAGE: {
            string photoId = columns[0]; // 0 indicates photo id
            MEDIA_INFO_LOG("cancel request photoId: %{public}s", photoId.c_str());
            CancelProcessRequest(photoId);
            break;
        }
        case OperationType::REMOVE_MSC_TASK: {
            CancelRequestAndRemoveImage(columns);
            break;
        }
        case OperationType::ADD_LOWQUALITY_IMAGE: {
            MEDIA_DEBUG_LOG("save low quality Image");
            SaveLowQualityImageInfo(cmd);
            break;
        }
        default:
            break;
    }
    return nullptr;
}

void MultiStagesPhotoCaptureManager::SaveLowQualityImageInfo(MediaLibraryCommand &cmd)
{
    auto values = cmd.GetValueBucket();
    string photoId = "";
    ValueObject valueObject;
    if (values.GetObject(PhotoColumn::PHOTO_ID, valueObject)) {
        valueObject.GetString(photoId);
    }
    int32_t deferredProcType = -1;
    if (values.GetObject(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, valueObject)) {
        valueObject.GetInt(deferredProcType);
    }
    int32_t fileId = 0;
    if (values.GetObject(MediaColumn::MEDIA_ID, valueObject)) {
        valueObject.GetInt(fileId);
    }
}

// 低质量入缓存
void MultiStagesPhotoCaptureManager::DealLowQualityPicture(const std::string &imageId,
    std::shared_ptr<Media::Picture> picture, bool isEdited)
{
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_LOG(pictureManagerThread != nullptr, "pictureManagerThread not init yet");
    CHECK_AND_RETURN_LOG(!pictureManagerThread->IsExsitPictureByImageId(imageId),
        "current photo already in the cache");

    // 将低质量图存入缓存
    time_t currentTime;
    if ((currentTime = time(NULL)) == -1) {
        MEDIA_ERR_LOG("Get time is error");
        currentTime = time(NULL);
    }
    time_t expireTime = currentTime + SAVE_PICTURE_TIMEOUT_SEC;
    std::string imageIdInPair = imageId;
    sptr<PicturePair> picturePair = new PicturePair(std::move(picture), imageIdInPair, expireTime, true, false);
    // 存低质量裸picture
    pictureManagerThread->InsertPictureData(imageId, picturePair, LOW_QUALITY_PICTURE);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} MultistagesCapture photoid: %{public}s",
        MLOG_TAG, __FUNCTION__, __LINE__, imageId.c_str());
}

bool MultiStagesPhotoCaptureManager::IsHighQualityPhotoExist(const std::string &uri)
{
    string filePath = MediaFileUri::GetPathFromUri(uri, true);
    string filePathTemp = filePath + ".high";
    return MediaFileUtils::IsFileExists(filePathTemp) || MediaFileUtils::IsFileExists(filePath);
}

void MultiStagesPhotoCaptureManager::SaveLowQualityPicture(const std::string &imageId)
{
    MEDIA_INFO_LOG("photoid: %{public}s", imageId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_LOG(pictureManagerThread != nullptr, "pictureManagerThread not init yet");
    pictureManagerThread->SaveLowQualityPicture(imageId);
}

// 高质量编辑图片存20S
void MultiStagesPhotoCaptureManager::DealHighQualityPicture(const std::string &imageId,
    std::shared_ptr<Media::Picture> picture, bool isEdited, bool isTakeEffect)
{
    MEDIA_INFO_LOG("photoid: %{public}s", imageId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    CHECK_AND_RETURN_LOG(pictureManagerThread != nullptr, "pictureManagerThread not init yet");
    // 将低质量图存入缓存
    time_t currentTime;
    if ((currentTime = time(NULL)) == -1) {
        MEDIA_ERR_LOG("Get time is error");
        currentTime = time(NULL);
    }
    time_t expireTime = currentTime + SAVE_PICTURE_TIMEOUT_SEC;
    std::string imageIdInPair = imageId;
    sptr<PicturePair> picturePair= new PicturePair(std::move(picture), imageIdInPair, expireTime, true, isEdited);
    picturePair->SetTakeEffect(isTakeEffect);
    pictureManagerThread->InsertPictureData(imageId, picturePair, HIGH_QUALITY_PICTURE);
    // delete raw file
    MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(imageId, false);
}

int32_t MultiStagesPhotoCaptureManager::UpdateDbInfo(MediaLibraryCommand &cmd)
{
    MediaLibraryCommand cmdLocal (OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    auto values = cmd.GetValueBucket();
    ValueObject valueObject;
    int32_t photoQuality = static_cast<int32_t>(MultiStagesPhotoQuality::LOW);
    if (values.GetObject(PhotoColumn::PHOTO_QUALITY, valueObject)) {
        valueObject.GetInt(photoQuality);
    }
    if (photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::LOW)) {
        values.PutInt(MEDIA_DATA_DB_DIRTY, -1); // prevent uploading low-quality photo
    }
    cmdLocal.SetValueBucket(values);
    cmdLocal.GetAbsRdbPredicates()->SetWhereClause(cmd.GetAbsRdbPredicates()->GetWhereClause());
    cmdLocal.GetAbsRdbPredicates()->SetWhereArgs(cmd.GetAbsRdbPredicates()->GetWhereArgs());
    auto result = DatabaseAdapter::Update(cmdLocal);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("update failed");
    }
    return result;
}

static std::string QueryPathByFileId(const int32_t &fileId)
{
    MediaLibraryCommand queryCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    std::string where = MediaColumn::MEDIA_ID + " = " + to_string(fileId);
    queryCmd.GetAbsRdbPredicates()->SetWhereClause(where);
    
    std::vector<std::string> columns { MediaColumn::MEDIA_FILE_PATH };
    auto resultSet = DatabaseAdapter::Query(queryCmd, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != 0);
    CHECK_AND_RETURN_RET_LOG(!cond, "", "result set is empty");

    std::string path = GetStringVal(MediaColumn::MEDIA_FILE_PATH, resultSet);
    return path;
}

static int32_t WriteGpsInfoAndUpdateDb(const std::string &path, const int32_t &fileId, const double longitude,
    const double latitude)
{
    MediaLibraryTracer tracer;
    tracer.Start("WriteGpsInfoAndUpdateDb, path");
    auto ret = ExifUtils::WriteGpsExifInfo(path, longitude, latitude);
    tracer.Finish();
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "write gps info fail");

    std::string originPath = MediaLibraryAssetOperations::GetEditDataSourcePath(path);
    if (MediaFileUtils::IsFileExists(originPath)) {
        // write gps info if this photo was edited.
        tracer.Start("WriteGpsInfoAndUpdateDb, originPath");
        auto ret = ExifUtils::WriteGpsExifInfo(originPath, longitude, latitude);
        tracer.Finish();
        CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "write origin file gps info fail");
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, fileId);

    ValuesBucket updateValues;
    updateValues.PutDouble(PhotoColumn::PHOTO_LATITUDE, latitude);
    updateValues.PutDouble(PhotoColumn::PHOTO_LONGITUDE, longitude);
    cmd.SetValueBucket(updateValues);

    auto result = DatabaseAdapter::Update(cmd);
    CHECK_AND_RETURN_RET_LOG(result == NativeRdb::E_OK, E_ERR, "update fail fileId: %{public}d", fileId);
    TransCodeExifInfo transCodeExifInfo;
    transCodeExifInfo.latitude = latitude;
    transCodeExifInfo.longitude = longitude;
    MediaLibraryTranscodeDataAgingOperation::ModifyTransCodeFileExif(ExifType::EXIF_GPS,
        path, transCodeExifInfo, __func__);
    return E_OK;
}

void MultiStagesPhotoCaptureManager::UpdateLocation(const NativeRdb::ValuesBucket &values, bool isWriteGpsAdvanced,
    const std::string &path, const int32_t &id)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLocation");
    MEDIA_INFO_LOG("UpdateLocation begin, isWriteGpsAdvanced: %{public}d, path: %{public}s, fileId: %{public}d",
        isWriteGpsAdvanced, MediaFileUtils::DesensitizePath(path).c_str(), id);
    ValueObject valueObject;
    double longitude = 0;
    if (values.GetObject(PhotoColumn::PHOTO_LONGITUDE, valueObject)) {
        valueObject.GetDouble(longitude);
    }
    double latitude = 0;
    if (values.GetObject(PhotoColumn::PHOTO_LATITUDE, valueObject)) {
        valueObject.GetDouble(latitude);
    }

    std::string filePath;
    int32_t fileId = 0;
    if (isWriteGpsAdvanced) {
        filePath = path;
        fileId = id;
    } else {
        if (values.GetObject(MediaColumn::MEDIA_ID, valueObject)) {
            valueObject.GetInt(fileId);
        }
        filePath = QueryPathByFileId(fileId);
    }
    CHECK_AND_RETURN_LOG((!filePath.empty() && fileId > 0), "UpdateLocation invalid value!");
    WriteGpsInfoAndUpdateDb(filePath, fileId, longitude, latitude);
    MEDIA_INFO_LOG("UpdateLocation success");
}

void MultiStagesPhotoCaptureManager::AddImageInternal(int32_t fileId, const string &photoId, int32_t deferredProcType,
    bool discardable, const std::string &packageName)
{
    MEDIA_INFO_LOG("AddImageInternal, file_id: %{public}d, photoId: %{public}s, deferredProcType: %{public}d, "
        "discardable: %{public}d, packageName: %{public}s",
        fileId, photoId.c_str(), deferredProcType, discardable, packageName.c_str());
    MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(fileId, photoId, discardable);

#ifdef ABILITY_CAMERA_SUPPORT
    DpsMetadata metadata;
    metadata.Set(CameraStandard::DEFERRED_PROCESSING_TYPE_KEY, deferredProcType);
    deferredProcSession_->AddImage(photoId, metadata, discardable, packageName);
#endif
}

void MultiStagesPhotoCaptureManager::AddImage(int32_t fileId, const string &photoId, int32_t deferredProcType,
    const std::string &packageName)
{
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photo is empty");
        return;
    }

    // called when camera low quality photo saved, isTrashed must be false.
    AddImageInternal(fileId, photoId, deferredProcType, false, packageName);
}

void MultiStagesPhotoCaptureManager::AddImage(AddImageDto &dto)
{
    MEDIA_INFO_LOG("Calling AddImage");
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread == nullptr) {
        return;
    }
    if (dto.photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
        pictureManagerThread->SavePictureWithImageId(dto.photoId);
        UpdatePictureQualityByFileId(dto.fileId);
        return;
    }
    AddImage(dto.fileId, dto.photoId, dto.deferredProcType, dto.packageName);
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(dto.photoId);
    MultiStagesCaptureDfxTriggerRatio::GetInstance().SetTrigger(MultiStagesCaptureTriggerType::AUTO);
}

void MultiStagesPhotoCaptureManager::AddImage(MediaLibraryCommand &cmd)
{
    MEDIA_DEBUG_LOG("calling addImage");
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread == nullptr) {
        return;
    }
    auto values = cmd.GetValueBucket();
    ValueObject valueObject;
    int32_t photoQuality = static_cast<int32_t>(MultiStagesPhotoQuality::LOW);
    if (values.GetObject(PhotoColumn::PHOTO_QUALITY, valueObject)) {
        valueObject.GetInt(photoQuality);
    }

    string photoId = "";
    if (values.GetObject(PhotoColumn::PHOTO_ID, valueObject)) {
        valueObject.GetString(photoId);
    }
    if (photoQuality == static_cast<int32_t>(MultiStagesPhotoQuality::FULL)) {
        pictureManagerThread->SavePictureWithImageId(photoId);
        UpdatePictureQuality(photoId);
        return;
    }
    int32_t deferredProcType = -1;
    if (values.GetObject(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, valueObject)) {
        valueObject.GetInt(deferredProcType);
    }
    int32_t fileId = 0;
    if (values.GetObject(MediaColumn::MEDIA_ID, valueObject)) {
        valueObject.GetInt(fileId);
    }

    AddImage(fileId, photoId, deferredProcType);
    MultiStagesCaptureDfxTotalTime::GetInstance().AddStartTime(photoId);
    MultiStagesCaptureDfxTriggerRatio::GetInstance().SetTrigger(MultiStagesCaptureTriggerType::AUTO);
    if (OPRN_ADD_LOWQUALITY_IMAGE == cmd.GetQuerySetParam("save_picture")) {
        MEDIA_DEBUG_LOG("save last low quality Image");
        SaveLowQualityImageInfo(cmd);
    }
}

int32_t MultiStagesPhotoCaptureManager::UpdatePictureQuality(const std::string &photoId)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePictureQuality " + photoId);
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_ID, photoId);
    int32_t updatePhotoQualityResult = DatabaseAdapter::Update(updateCmd);
 
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, false);
    updateCmd.GetAbsRdbPredicates()->NotEqualTo(PhotoColumn::PHOTO_SUBTYPE,
        to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    NativeRdb::ValuesBucket updateValuesDirty;
    updateValuesDirty.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    updateCmd.SetValueBucket(updateValuesDirty);
    auto isDirtyResult = DatabaseAdapter::Update(updateCmd);
    CHECK_AND_PRINT_LOG(isDirtyResult >= 0, "update dirty flag fail, photoId: %{public}s", photoId.c_str());
    return updatePhotoQualityResult;
}

int32_t MultiStagesPhotoCaptureManager::UpdatePictureQualityByFileId(int32_t fileId)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdatePictureQuality " + std::to_string(fileId));
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(MediaColumn::MEDIA_ID, fileId);
    int32_t updatePhotoQualityResult = DatabaseAdapter::Update(updateCmd);
 
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, false);
    updateCmd.GetAbsRdbPredicates()->NotEqualTo(PhotoColumn::PHOTO_SUBTYPE,
        to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    NativeRdb::ValuesBucket updateValuesDirty;
    updateValuesDirty.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    updateCmd.SetValueBucket(updateValuesDirty);
    auto isDirtyResult = DatabaseAdapter::Update(updateCmd);
    CHECK_AND_PRINT_LOG(isDirtyResult >= 0, "update dirty flag fail, photoId: %{public}d", fileId);
    return updatePhotoQualityResult;
}

void MultiStagesPhotoCaptureManager::SyncWithDeferredProcSessionInternal()
{
    MEDIA_INFO_LOG("enter");
    // 进程重启场景，媒体库需要和延时子服务同步
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    string where = MEDIA_DATA_DB_PHOTO_ID + " is not null and " +
        MEDIA_DATA_DB_PHOTO_QUALITY + " > 0 and " + MEDIA_DATA_DB_MEDIA_TYPE + " = " +
        to_string(static_cast<int32_t>(MediaType::MEDIA_TYPE_IMAGE));
    cmd.GetAbsRdbPredicates()->SetWhereClause(where);
    vector<string> columns { MEDIA_DATA_DB_ID, MEDIA_DATA_DB_PHOTO_ID, MEDIA_DATA_DB_DATE_TRASHED,
        PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, MediaColumn::MEDIA_OWNER_PACKAGE };
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    bool cond = (resultSet == nullptr || resultSet->GoToFirstRow() != 0);
    CHECK_AND_RETURN_LOG(!cond, "result set is empty");
    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesPhotoCaptureManager::SyncWithDeferredProcSession");
    deferredProcSession_->BeginSynchronize();

    do {
        int32_t fileId = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
        bool isTrashed = GetInt32Val(MEDIA_DATA_DB_DATE_TRASHED, resultSet) > 0;
        std::string packageName = GetStringVal(MediaColumn::MEDIA_OWNER_PACKAGE, resultSet);
        if (setOfDeleted_.find(fileId) != setOfDeleted_.end()) {
            MEDIA_INFO_LOG("remove image, fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
            deferredProcSession_->RemoveImage(photoId);
            continue;
        }
        MEDIA_INFO_LOG("AddImage fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
        int32_t deferredProcType = GetInt32Val(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, resultSet);
        AddImageInternal(fileId, photoId, deferredProcType, isTrashed, packageName);
    } while (!resultSet->GoToNextRow());
    resultSet->Close();
    deferredProcSession_->EndSynchronize();
    MEDIA_INFO_LOG("exit");
}

static void SyncWithDeferredPhotoProcSessionAsync(AsyncTaskData *data)
{
    MultiStagesPhotoCaptureManager::GetInstance().SyncWithDeferredProcSessionInternal();
}

void MultiStagesPhotoCaptureManager::SyncWithDeferredProcSession()
{
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_LOG(asyncWorker != nullptr, "can not get async worker");
    shared_ptr<MediaLibraryAsyncTask> asyncTask =
        make_shared<MediaLibraryAsyncTask>(SyncWithDeferredPhotoProcSessionAsync, nullptr);
    CHECK_AND_RETURN_LOG(asyncTask != nullptr, "SyncWithDeferredProcSession create task fail");
    MEDIA_INFO_LOG("SyncWithDeferredProcSession add task success");
    asyncWorker->AddTask(asyncTask, false);
}

#ifdef ABILITY_CAMERA_SUPPORT
void MultiStagesPhotoCaptureManager::SetProcessImageDoneCallback(const ProcessDoneHandler &func)
{
    if (deferredProcSession_ != nullptr) {
        deferredProcSession_->SetProcessImageDoneCallback(func);
    }
}
#endif

bool MultiStagesPhotoCaptureManager::CancelProcessRequest(const string &photoId)
{
    CHECK_AND_RETURN_RET_LOG(MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(photoId), false,
        "photoId is empty or not in process");
    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(photoId, RequestType::CANCEL_REQUEST);
    CHECK_AND_RETURN_RET_LOG(currentRequestCount <= 0, false,
        "not cancel request because request count(%{public}d) greater than 0", currentRequestCount);
    auto isCancelSucc = deferredProcSession_->CancelProcessImage(photoId);
    MEDIA_INFO_LOG("cancel request isCancelSucc: %{public}d", isCancelSucc);
    return true;
}

void MultiStagesPhotoCaptureManager::RemoveImage(const string &photoId, bool isRestorable)
{
    if (!MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(photoId)) {
        // In order to ensure image can be completely deleted, do not return here, only record the log.
        MEDIA_ERR_LOG("photoId is empty or not in process ");
    }

    MultiStagesCaptureRequestTaskManager::RemovePhotoInProgress(photoId, isRestorable);
    deferredProcSession_->RemoveImage(photoId, isRestorable);
}

void MultiStagesPhotoCaptureManager::RestoreImage(const string &photoId)
{
    CHECK_AND_PRINT_LOG(!photoId.empty(), "photoId is empty");
    MultiStagesCaptureRequestTaskManager::UpdatePhotoInProgress(photoId);
    deferredProcSession_->RestoreImage(photoId);
}

void MultiStagesPhotoCaptureManager::ProcessImage(int fileId, int deliveryMode)
{
    string photoId = MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(fileId) ;
    CHECK_AND_RETURN_LOG(photoId.size() != 0, "processimage image id is invalid, fileId:"
        " %{public}d", fileId);

    string callerBundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    MultiStagesCaptureDfxTriggerRatio::GetInstance().SetTrigger(MultiStagesCaptureTriggerType::THIRD_PART);
    MultiStagesCaptureDfxRequestPolicy::GetInstance().SetPolicy(callerBundleName,
        static_cast<RequestPolicy>(deliveryMode));
    MultiStagesCaptureDfxFirstVisit::GetInstance().Report(photoId, fileId);
    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(photoId, RequestType::REQUEST);
    HILOG_COMM_INFO("%{public}s:{%{public}s:%{public}d} "
        "processimage, pkg name: %{public}s, photoid %{public}s, mode: %{public}d, count: %{public}d",
        MLOG_TAG, __FUNCTION__, __LINE__,
        callerBundleName.c_str(), photoId.c_str(), deliveryMode, currentRequestCount);
    if ((deliveryMode == static_cast<int32_t>(RequestPolicy::HIGH_QUALITY_MODE) ||
        deliveryMode == static_cast<int32_t>(RequestPolicy::BALANCE_MODE)) &&
        currentRequestCount <= 1) {
        deferredProcSession_->ProcessImage(callerBundleName, photoId);
    }
}

bool MultiStagesPhotoCaptureManager::IsPhotoDeleted(const std::string &photoId)
{
    if (!MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(photoId)) {
        return false;
    }

    return true;
}

void MultiStagesPhotoCaptureManager::NotifyProcessImage()
{
    CHECK_AND_RETURN_LOG(deferredProcSession_ != nullptr, "deferredProcSession is nullptr.");
    deferredProcSession_->NotifyProcessImage();
}
} // namespace Media
} // namespace OHOS