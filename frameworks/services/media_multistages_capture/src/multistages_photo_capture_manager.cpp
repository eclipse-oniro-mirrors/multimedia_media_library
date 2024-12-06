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

#include "multistages_photo_capture_manager.h"

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
    SyncWithDeferredProcSession();
    return true;
}

void MultiStagesPhotoCaptureManager::CancelRequestAndRemoveImage(const vector<string> &columns)
{
    if (columns.size() < 1) {
        MEDIA_ERR_LOG("invalid param");
        return;
    }
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
            UpdateLocation(cmd.GetValueBucket());
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
    if (pictureManagerThread == nullptr) {
        return;
    }
    if (pictureManagerThread->IsExsitPictureByImageId(imageId)) {
        return;
    }
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
    MEDIA_INFO_LOG("photoid: %{public}s", imageId.c_str());
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
    if (pictureManagerThread == nullptr) {
        return;
    }
    pictureManagerThread->SaveLowQualityPicture(imageId);
}

// 高质量编辑图片存20S
void MultiStagesPhotoCaptureManager::DealHighQualityPicture(const std::string &imageId,
    std::shared_ptr<Media::Picture> picture, bool isEdited)
{
    MEDIA_INFO_LOG("photoid: %{public}s", imageId.c_str());
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread == nullptr) {
        return;
    }
    // 将低质量图存入缓存
    time_t currentTime;
    if ((currentTime = time(NULL)) == -1) {
        MEDIA_ERR_LOG("Get time is error");
        currentTime = time(NULL);
    }
    time_t expireTime = currentTime + SAVE_PICTURE_TIMEOUT_SEC;
    std::string imageIdInPair = imageId;
    sptr<PicturePair> picturePair= new PicturePair(std::move(picture), imageIdInPair, expireTime, true, isEdited);
    pictureManagerThread->InsertPictureData(imageId, picturePair, HIGH_QUALITY_PICTURE);
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

void MultiStagesPhotoCaptureManager::UpdateLocation(const NativeRdb::ValuesBucket &values)
{
    MediaLibraryTracer tracer;
    tracer.Start("UpdateLocation");
    double longitude = 0;
    ValueObject valueObject;
    if (values.GetObject(PhotoColumn::PHOTO_LONGITUDE, valueObject)) {
        valueObject.GetDouble(longitude);
    }
    double latitude = 0;
    if (values.GetObject(PhotoColumn::PHOTO_LATITUDE, valueObject)) {
        valueObject.GetDouble(latitude);
    }

    string path = "";
    if (values.GetObject(MediaColumn::MEDIA_FILE_PATH, valueObject)) {
        valueObject.GetString(path);
    }

    // update exif info
    auto ret = ExifUtils::WriteGpsExifInfo(path, longitude, latitude);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("write gps info fail");
        return;
    }

    string originPath = MediaLibraryAssetOperations::GetEditDataSourcePath(path);
    if (MediaFileUtils::IsFileExists(originPath)) {
        // write gps info if this photo was edited.
        auto ret = ExifUtils::WriteGpsExifInfo(path, longitude, latitude);
        if (ret != E_OK) {
            MEDIA_ERR_LOG("write origin file gps info fail");
            return;
        }
    }

    int32_t fileId = 0;
    if (values.GetObject(MediaColumn::MEDIA_ID, valueObject)) {
        valueObject.GetInt(fileId);
    }
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, fileId);
    ValuesBucket updateValues;
    updateValues.PutDouble(MEDIA_DATA_DB_LATITUDE, latitude);
    updateValues.PutDouble(MEDIA_DATA_DB_LONGITUDE, longitude);
    cmd.SetValueBucket(updateValues);

    auto result = DatabaseAdapter::Update(cmd);
    if (result != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("update fail fileId: %{public}d", fileId);
    }
}

void MultiStagesPhotoCaptureManager::AddImageInternal(int32_t fileId, const string &photoId, int32_t deferredProcType,
    bool discardable)
{
    MultiStagesCaptureRequestTaskManager::AddPhotoInProgress(fileId, photoId, discardable);

#ifdef ABILITY_CAMERA_SUPPORT
    DpsMetadata metadata;
    metadata.Set(CameraStandard::DEFERRED_PROCESSING_TYPE_KEY, deferredProcType);
    deferredProcSession_->AddImage(photoId, metadata, discardable);
#endif
}

void MultiStagesPhotoCaptureManager::AddImage(int32_t fileId, const string &photoId, int32_t deferredProcType)
{
    if (photoId.empty()) {
        MEDIA_ERR_LOG("photo is empty");
        return;
    }
    MEDIA_INFO_LOG("enter fileId: %{public}d, photoId: %{public}s, deferredProcType: %{public}d", fileId,
        photoId.c_str(), deferredProcType);

    // called when camera low quality photo saved, isTrashed must be false.
    AddImageInternal(fileId, photoId, deferredProcType, false);
}

void MultiStagesPhotoCaptureManager::AddImage(MediaLibraryCommand &cmd)
{
    MEDIA_DEBUG_LOG("calling addImage");
    UpdateDbInfo(cmd);
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
    auto pictureManagerThread = PictureManagerThread::GetInstance();
    if (pictureManagerThread == nullptr) {
        return;
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
    tracer.Start("UpdatePhotoQuality " + photoId);
    MediaLibraryCommand updateCmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE);
    NativeRdb::ValuesBucket updateValues;
    updateValues.PutInt(PhotoColumn::PHOTO_QUALITY, static_cast<int32_t>(MultiStagesPhotoQuality::FULL));
    updateCmd.SetValueBucket(updateValues);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_ID, photoId);
    int32_t updatePhotoIdResult = DatabaseAdapter::Update(updateCmd);
    updateCmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::PHOTO_IS_TEMP, false);
    updateCmd.GetAbsRdbPredicates()->NotEqualTo(PhotoColumn::PHOTO_SUBTYPE,
        to_string(static_cast<int32_t>(PhotoSubType::MOVING_PHOTO)));
    NativeRdb::ValuesBucket updateValuesDirty;
    updateValuesDirty.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyType::TYPE_NEW));
    updateCmd.SetValueBucket(updateValuesDirty);
    auto isTempResult = DatabaseAdapter::Update(updateCmd);
    if (isTempResult < 0) {
        MEDIA_WARN_LOG("update temp flag fail, photoId: %{public}s", photoId.c_str());
    }
    return updatePhotoIdResult;
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
        MEDIA_DATA_DB_DEFERRED_PROC_TYPE };
    
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != 0) {
        MEDIA_ERR_LOG("result set is empty");
        return;
    }

    MediaLibraryTracer tracer;
    tracer.Start("MultiStagesPhotoCaptureManager::SyncWithDeferredProcSession");

    deferredProcSession_->BeginSynchronize();
    do {
        int32_t fileId = GetInt32Val(MEDIA_DATA_DB_ID, resultSet);
        string photoId = GetStringVal(MEDIA_DATA_DB_PHOTO_ID, resultSet);
        bool isTrashed = GetInt32Val(MEDIA_DATA_DB_DATE_TRASHED, resultSet) > 0;
        if (setOfDeleted_.find(fileId) != setOfDeleted_.end()) {
            MEDIA_INFO_LOG("remove image, fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
            deferredProcSession_->RemoveImage(photoId);
            continue;
        }
        MEDIA_INFO_LOG("AddImage fileId: %{public}d, photoId: %{public}s", fileId, photoId.c_str());
        int32_t deferredProcType = GetInt32Val(MEDIA_DATA_DB_DEFERRED_PROC_TYPE, resultSet);
        AddImageInternal(fileId, photoId, deferredProcType, isTrashed);
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
    if (asyncWorker == nullptr) {
        MEDIA_INFO_LOG("can not get async worker");
        return;
    }

    shared_ptr<MediaLibraryAsyncTask> asyncTask =
        make_shared<MediaLibraryAsyncTask>(SyncWithDeferredPhotoProcSessionAsync, nullptr);
    if (asyncTask == nullptr) {
        MEDIA_ERR_LOG("SyncWithDeferredProcSession create task fail");
        return;
    }
    MEDIA_INFO_LOG("SyncWithDeferredProcSession add task success");
    asyncWorker->AddTask(asyncTask, false);
}

bool MultiStagesPhotoCaptureManager::CancelProcessRequest(const string &photoId)
{
    if (!MultiStagesCaptureRequestTaskManager::IsPhotoInProcess(photoId)) {
        MEDIA_ERR_LOG("photoId is empty or not in process");
        return false;
    }

    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(photoId, RequestType::CANCEL_REQUEST);
    if (currentRequestCount > 0) {
        MEDIA_ERR_LOG("not cancel request because request count(%{public}d) greater than 0", currentRequestCount);

        return false;
    }
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
    if (photoId.empty()) {
        MEDIA_DEBUG_LOG("photoId is empty");
    }

    MultiStagesCaptureRequestTaskManager::UpdatePhotoInProgress(photoId);
    deferredProcSession_->RestoreImage(photoId);
}

void MultiStagesPhotoCaptureManager::ProcessImage(int fileId, int deliveryMode)
{
    string photoId = MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(fileId) ;
    if (photoId.size() == 0) {
        MEDIA_ERR_LOG("processimage image id is invalid, fileId: %{public}d", fileId);
        return;
    }

    string callerBundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();

    MultiStagesCaptureDfxTriggerRatio::GetInstance().SetTrigger(MultiStagesCaptureTriggerType::THIRD_PART);
    MultiStagesCaptureDfxRequestPolicy::GetInstance().SetPolicy(callerBundleName,
        static_cast<RequestPolicy>(deliveryMode));
    MultiStagesCaptureDfxFirstVisit::GetInstance().Report(photoId);
    int32_t currentRequestCount =
        MultiStagesCaptureRequestTaskManager::UpdatePhotoInProcessRequestCount(photoId, RequestType::REQUEST);
    MEDIA_INFO_LOG("processimage, pkg name: %{public}s, photoid %{public}s, mode: %{public}d, count: %{public}d",
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
} // namespace Media
} // namespace OHOS