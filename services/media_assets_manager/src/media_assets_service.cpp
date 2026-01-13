/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "MediaAssetsService"

#include "media_assets_service.h"

#include <unordered_set>

#include "medialibrary_bundle_manager.h"
#include "medialibrary_vision_operations.h"
#include "media_visit_count_manager.h"
#include "result_set_utils.h"
#include "dfx_manager.h"
#include "multistages_capture_request_task_manager.h"
#include "multistages_capture_manager.h"
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
#include "enhancement_manager.h"
#endif
#include "medialibrary_photo_operations.h"
#include "medialibrary_album_fusion_utils.h"
#include "medialibrary_album_operations.h"
#include "rdb_utils.h"
#include "medialibrary_data_manager.h"
#include "media_app_uri_permission_column.h"
#include "media_app_uri_sensitive_column.h"
#include "duplicate_photo_operation.h"
#include "medialibrary_common_utils.h"
#include "photo_map_column.h"
#include "album_operation_uri.h"
#include "medialibrary_business_code.h"
#include "rdb_utils.h"
#include "datashare_result_set.h"
#include "query_result_vo.h"
#include "user_photography_info_column.h"
#include "userfile_manager_types.h"
#include "datashare_predicates.h"
#include "medialibrary_file_operations.h"
#include "close_asset_vo.h"
#include "medialibrary_db_const.h"
#include "medialibrary_object_utils.h"
#include "media_column.h"
#include "media_old_photos_column.h"
#include "cloud_media_asset_manager.h"
#include "heif_transcoding_check_utils.h"
#include "database_adapter.h"

using namespace std;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media {

const int32_t YES = 1;
const int32_t NO = 0;
const int32_t MEDIA_HO_LAKE_CONST = 3;
const std::string SET_LOCATION_KEY = "set_location";
const std::string SET_LOCATION_VALUE = "1";
const std::string COLUMN_FILE_ID = "file_id";
const std::string COLUMN_DATA = "data";
const std::string COLUMN_OLD_FILE_ID = "old_file_id";
const std::string COLUMN_OLD_DATA = "old_data";
const std::string COLUMN_DISPLAY_NAME = "display_name";
const std::string HEIF_MIME_TYPE = "image/heif";
const std::string HEIC_MIME_TYPE = "image/heic";
constexpr int32_t HIGH_QUALITY_IMAGE = 0;
unordered_set<std::string> DFXTaskSet;
std::mutex DFXTaskMutex;

static void UpdateVisionTableForEdit(AsyncTaskData *taskData)
{
    CHECK_AND_RETURN_LOG(taskData != nullptr, "taskData is nullptr");
    UpdateVisionAsyncTaskData* data = static_cast<UpdateVisionAsyncTaskData*>(taskData);
    CHECK_AND_RETURN_LOG(data != nullptr, "UpdateVisionAsyncTaskData is nullptr");
    string fileId = to_string(data->fileId_);
    MediaAssetsRdbOperations::DeleteFromVisionTables(fileId);
}

MediaAssetsService &MediaAssetsService::GetInstance()
{
    static MediaAssetsService service;
    return service;
}

int32_t MediaAssetsService::RemoveFormInfo(const string& formId)
{
    MEDIA_INFO_LOG("MediaAssetsService::RemoveFormInfo, formId:%{public}s", formId.c_str());
    int32_t deleteRows = this->rdbOperation_.RemoveFormInfo(formId);
    CHECK_AND_RETURN_RET_LOG(deleteRows > 0, E_ERR, "Failed to remove form info");
    return deleteRows;
}

int32_t MediaAssetsService::RemoveGalleryFormInfo(const string& formId)
{
#ifdef MEDIALIBRARY_FACARD_SUPPORT
    MEDIA_INFO_LOG("MediaAssetsService::RemoveGalleryFormInfo, formId:%{public}s", formId.c_str());
    return this->rdbOperation_.RemoveGalleryFormInfo(formId);
#else
    MEDIA_ERR_LOG("unsupported operation, formId: %{public}s", formId.c_str());
    return E_FAIL;
#endif
}

int32_t MediaAssetsService::SaveFormInfo(const FormInfoDto& formInfoDto)
{
    string formId = formInfoDto.formIds.front();
    string fileUri = formInfoDto.fileUris.front();
    return this->rdbOperation_.SaveFormInfo(formId, fileUri);
}

int32_t MediaAssetsService::SaveGalleryFormInfo(const FormInfoDto& formInfoDto)
{
#ifdef MEDIALIBRARY_FACARD_SUPPORT
    return this->rdbOperation_.SaveGalleryFormInfo(formInfoDto.formIds, formInfoDto.fileUris);
#else
    MEDIA_ERR_LOG("unsupported operation, formIds size: %{public}zu", formInfoDto.formIds.size());
    return E_FAIL;
#endif
}

int32_t MediaAssetsService::CommitEditedAsset(const CommitEditedAssetDto& commitEditedAssetDto)
{
    int32_t errCode = this->rdbOperation_.CommitEditInsert(commitEditedAssetDto.editData,
        commitEditedAssetDto.fileId);
    CHECK_AND_RETURN_RET(errCode == E_SUCCESS, errCode);
    shared_ptr<MediaLibraryAsyncWorker> asyncWorker = MediaLibraryAsyncWorker::GetInstance();
    CHECK_AND_RETURN_RET_LOG(asyncWorker != nullptr, E_ERR, "Can not get asyncWorker");
    UpdateVisionAsyncTaskData* taskData =
        new (std::nothrow) UpdateVisionAsyncTaskData(commitEditedAssetDto.fileId);
    CHECK_AND_RETURN_RET_LOG(taskData != nullptr, E_ERR, "Failed to new taskData");
    shared_ptr<MediaLibraryAsyncTask> updateAsyncTask =
        make_shared<MediaLibraryAsyncTask>(UpdateVisionTableForEdit, taskData);
    CHECK_AND_PRINT_LOG(updateAsyncTask != nullptr, "UpdateAnalysisDataForEdit fail");
    asyncWorker->AddTask(updateAsyncTask, true);
    return errCode;
}

int32_t MediaAssetsService::TrashPhotos(const std::vector<std::string> &uris)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::TRASH_PHOTO, MediaLibraryApi::API_10);
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, uris);
    cmd.SetDataSharePred(predicates);
    NativeRdb::ValuesBucket values;
    values.Put(MediaColumn::MEDIA_DATE_TRASHED, MediaFileUtils::UTCTimeSeconds());
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::TrashPhotos(cmd);
}

int32_t MediaAssetsService::DeletePhotos(const std::vector<std::string> &uris)
{
    DataShare::DataSharePredicates predicates;
    predicates.In(MediaColumn::MEDIA_ID, uris);
    return MediaLibraryAlbumOperations::DeletePhotoAssets(predicates, false, false);
}

int32_t MediaAssetsService::DeletePhotosCompleted(const std::vector<std::string> &fileIds)
{
    DataShare::DataSharePredicates predicates;
    predicates.In(PhotoColumn::MEDIA_ID, fileIds);
    return MediaLibraryAlbumOperations::DeletePhotoAssetsCompleted(predicates, false);
}

static std::string GetLocalDeviceName()
{
    return "";
}

static std::string GetClientBundleName()
{
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    if (bundleName.empty()) {
        MEDIA_ERR_LOG("GetClientBundleName failed");
    }
    return bundleName;
}

int32_t MediaAssetsService::AssetChangeSetFavorite(const int32_t fileId, const bool favorite)
{
    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_IS_FAV, favorite ? YES : NO);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::AssetChangeSetHidden(const std::string &uri, const bool hidden)
{
    DataShare::DataSharePredicates predicate;
    vector<string> uris{uri};
    predicate.In(PhotoColumn::MEDIA_ID, uris);

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::MEDIA_HIDDEN, hidden ? YES : NO);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::HIDE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::AssetChangeSetUserComment(const int32_t fileId, const std::string &userComment)
{
    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_USER_COMMENT, userComment);

    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::SET_USER_COMMENT, MediaLibraryApi::API_10);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::AssetChangeSetLocation(const SetLocationDto &dto)
{
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_ID, dto.fileId);
    values.Put(PhotoColumn::MEDIA_FILE_PATH, dto.path);
    values.Put(PhotoColumn::PHOTO_LATITUDE, dto.latitude);
    values.Put(PhotoColumn::PHOTO_LONGITUDE, dto.longitude);
    MultiStagesPhotoCaptureManager::UpdateLocation(values, false);
    return E_OK;
}

int32_t MediaAssetsService::UpdateExistedTasksTitle(int32_t fileId)
{
    MEDIA_INFO_LOG("UpdateExistedTasksTitle In fileId: %{public}d", fileId);
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_RDB_STORE_NULL, "UpdateExistedTasksTitle Failed to get rdbStore.");
    const std::string UPDATE_DISPLAY_NAME_IN_DOWNLOADTASK = "UPDATE " + DownloadResourcesColumn::TABLE +
        " SET " + DownloadResourcesColumn::MEDIA_NAME + "=" + "(SELECT " + MediaColumn::MEDIA_NAME + " FROM " +
        PhotoColumn::PHOTOS_TABLE + " WHERE " + PhotoColumn::MEDIA_ID + "=" +
        std::to_string(fileId) + ")  WHERE " + DownloadResourcesColumn::MEDIA_ID + "=" + std::to_string(fileId);
    int32_t ret = rdbStore->ExecuteSql(UPDATE_DISPLAY_NAME_IN_DOWNLOADTASK);
    MEDIA_INFO_LOG("UpdateExistedTasksTitle End ret: %{public}d", ret);
    return ret;
}

int32_t MediaAssetsService::AssetChangeSetTitle(const int32_t fileId, const std::string &title)
{
    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(PhotoColumn::MEDIA_ID, fileId);

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_TITLE, title);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    int32_t ret = MediaLibraryPhotoOperations::Update(cmd);
    UpdateExistedTasksTitle(fileId);
    return ret;
}

int32_t MediaAssetsService::AssetChangeSetEditData(const NativeRdb::ValuesBucket &values)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::ADD_FILTERS, values, MediaLibraryApi::API_10);

    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    return MediaLibraryPhotoOperations::AddFilters(cmd);
}

int32_t MediaAssetsService::AssetChangeSubmitCache(SubmitCacheDto &dto)
{
    MEDIA_INFO_LOG("SubmitCache isWriteGpsAdvanced: %{public}d", dto.isWriteGpsAdvanced);

    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::SUBMIT_CACHE, dto.values, MediaLibraryApi::API_10);
    if (dto.isWriteGpsAdvanced) {
        cmd.SetApiParam(SET_LOCATION_KEY, SET_LOCATION_VALUE);
    }

    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    int32_t ret = MediaLibraryPhotoOperations::SubmitCache(cmd);
    CHECK_AND_RETURN_RET_LOG(ret >= 0, ret, "MediaLibraryPhotoOperations::SubmitCache failed");
    dto.fileId = ret;
    dto.outUri = cmd.GetResult();
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeCreateAsset(AssetChangeCreateAssetDto &dto)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, dto.values, MediaLibraryApi::API_10);

    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    CHECK_AND_RETURN_RET_LOG(ret > 0, ret, "MediaLibraryPhotoOperations::Create failed");
    dto.fileId = ret;
    dto.outUri = cmd.GetResult();
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeAddImage(AddImageDto &dto)
{
    DataShare::DataSharePredicates predicates;
    predicates.SetWhereClause(PhotoColumn::MEDIA_ID + " = ? ");
    predicates.SetWhereArgs({to_string(dto.fileId)});

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MEDIA_ID, dto.fileId);
    values.Put(PhotoColumn::PHOTO_ID, dto.photoId);
    values.Put(PhotoColumn::PHOTO_DEFERRED_PROC_TYPE, dto.deferredProcType);

    MediaLibraryCommand cmd(
        OperationObject::PAH_MULTISTAGES_CAPTURE, OperationType::ADD_IMAGE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    MultiStagesPhotoCaptureManager::GetInstance().AddImage(cmd);
    return E_OK;
}

int32_t MediaAssetsService::CameraInnerAddImage(AddImageDto &dto)
{
    MEDIA_INFO_LOG("enter CameraInnerAddImage");
    MultiStagesPhotoCaptureManager::GetInstance().AddImage(dto);
    return E_OK;
}

int32_t MediaAssetsService::GetFusionAssetsInfo(const int32_t albumId, GetFussionAssetsRespBody &respBody)
{
    MEDIA_INFO_LOG("enter GetFusionAssetsInfo");
    NativeRdb::RdbPredicates rdbPredicate(PhotoColumn::PHOTOS_TABLE);
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_OWNER_ALBUM_ID, albumId);
    rdbPredicate.NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::CLOUD));
    rdbPredicate.EqualTo(MediaColumn::MEDIA_DATE_TRASHED, 0);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_HIDDEN, 0);
    rdbPredicate.EqualTo(MediaColumn::MEDIA_TIME_PENDING, 0);
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_IS_TEMP, 0);
    rdbPredicate.NotEqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_DELETED));
    rdbPredicate.EqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE, MEDIA_HO_LAKE_CONST);
    rdbPredicate.NotEqualTo(PhotoColumn::PHOTO_STORAGE_PATH, "");
    rdbPredicate.IsNotNull(PhotoColumn::PHOTO_STORAGE_PATH);

    std::vector<std::string> fetchColumn {
        "count(*) AS count",
        "MAX(storage_path) AS storage_path"
    };
    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicate, fetchColumn);
    if (resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }
    respBody.queryResult.push_back(
        FussionAssetsResult(0, GetInt32Val("count", resultSet), GetStringVal("storage_path", resultSet)));
    return E_OK;
}

int32_t MediaAssetsService::SetCameraShotKey(const int32_t fileId, const std::string &cameraShotKey)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::CAMERA_SHOT_KEY, cameraShotKey);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::UpdateFileAsset(cmd);
}

int32_t MediaAssetsService::SaveCameraPhoto(const SaveCameraPhotoDto &dto)
{
    if (dto.mediaType == static_cast<int32_t>(MediaType::MEDIA_TYPE_VIDEO)) {
        return MultiStagesVideoCaptureManager::GetInstance().SaveCameraVideo(dto);
    }
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::SAVE_CAMERA_PHOTO, MediaLibraryApi::API_10);

    if (dto.discardHighQualityPhoto) {
        string photoId = MultiStagesCaptureRequestTaskManager::GetProcessingPhotoId(dto.fileId);
        MEDIA_INFO_LOG("fileId: %{public}d, photoId: %{public}s", dto.fileId, photoId.c_str());
        MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(photoId);
        MultiStagesPhotoCaptureManager::GetInstance().RemoveImage(photoId, false);

        cmd.SetApiParam(PhotoColumn::PHOTO_DIRTY, to_string(static_cast<int32_t>(DirtyType::TYPE_NEW)));
    }
    cmd.SetApiParam(MEDIA_OPERN_KEYWORD, to_string(dto.needScan));
    cmd.SetApiParam(PhotoColumn::MEDIA_FILE_PATH, dto.path);
    cmd.SetApiParam(PhotoColumn::MEDIA_ID, to_string(dto.fileId));
    cmd.SetApiParam(PhotoColumn::PHOTO_SUBTYPE, to_string(dto.photoSubType));
    cmd.SetApiParam(IMAGE_FILE_TYPE, to_string(dto.imageFileType));
    NativeRdb::ValuesBucket values;
    if (dto.supportedWatermarkType != INT32_MIN) {
        values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, dto.supportedWatermarkType);
    }
    if (dto.cameraShotKey != "NotSet") {
        values.Put(PhotoColumn::CAMERA_SHOT_KEY, dto.cameraShotKey);
    }

    values.Put(PhotoColumn::PHOTO_IS_TEMP, false);
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::SaveCameraPhoto(cmd);
}

int32_t MediaAssetsService::DiscardCameraPhoto(const int32_t fileId)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::DISCARD_CAMERA_PHOTO, MediaLibraryApi::API_10);
    cmd.SetApiParam(PhotoColumn::MEDIA_ID, to_string(fileId));

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
    predicates.EqualTo(PhotoColumn::PHOTO_IS_TEMP, "1");  // only temp camera photo can be discarded
    cmd.SetDataSharePred(predicates);

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_IS_TEMP, true);
    cmd.SetValueBucket(values);
    return MediaLibraryPhotoOperations::DiscardCameraPhoto(cmd);
}

int32_t MediaAssetsService::SetEffectMode(const int32_t fileId, const int32_t effectMode)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::MOVING_PHOTO_EFFECT_MODE, effectMode);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::UpdateFileAsset(cmd);
}

int32_t MediaAssetsService::SetOrientation(const int32_t fileId, const int32_t orientation)
{
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_ORIENTATION, orientation);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::UpdateFileAsset(cmd);
}

int32_t MediaAssetsService::SetVideoEnhancementAttr(
    const int32_t fileId, const std::string &photoId, const std::string &path)
{
    MEDIA_DEBUG_LOG("photoId:%{public}s, fileId:%{public}d, path:%{public}s", photoId.c_str(), fileId, path.c_str());
    VideoInfo videoInfo = {fileId, VideoCount::SINGLE, path, "", ""};
    MultiStagesVideoCaptureManager::GetInstance().AddVideo(photoId, to_string(fileId), videoInfo);
    return E_OK;
}

int32_t MediaAssetsService::SetHasAppLink(const int32_t fileId, const int32_t hasAppLink)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE_HAS_APPLINK, MediaLibraryApi::API_10);
 
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
 
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_HAS_APPLINK, hasAppLink);
 
    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::UpdateAppLink(cmd);
}
 
int32_t MediaAssetsService::SetAppLink(const int32_t fileId, const string appLink)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE_APPLINK, MediaLibraryApi::API_10);
 
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));
 
    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::PHOTO_APPLINK, appLink);
 
    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::UpdateAppLink(cmd);
}

int32_t MediaAssetsService::SubmitMetadataChanged(const int32_t fileId)
{
    auto rdbStore = MediaLibraryUnistoreManager::GetInstance().GetRdbStore();
    CHECK_AND_RETURN_RET_LOG(rdbStore != nullptr, E_HAS_DB_ERROR, "Failed to get rdbStore.");

    NativeRdb::ValuesBucket values;
    values.PutInt(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_MDIRTY));

    NativeRdb::AbsRdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId))
        ->And()
        ->NotEqualTo(PhotoColumn::PHOTO_POSITION, static_cast<int32_t>(PhotoPositionType::LOCAL))
        ->And()
        ->BeginWrap()
        ->EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SYNCED))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_SDIRTY))
        ->Or()
        ->EqualTo(PhotoColumn::PHOTO_DIRTY, static_cast<int32_t>(DirtyTypes::TYPE_TDIRTY))
        ->EndWrap();
    
    int32_t updateRows = -1;
    int32_t ret = rdbStore->Update(updateRows, values, predicates);
    MEDIA_INFO_LOG("SubmitMetadataChanged executed ret: %{public}d, updateRows: %{public}d", ret, updateRows);
    return ret;
}

int32_t MediaAssetsService::SetSupportedWatermarkType(const int32_t fileId, const int32_t watermarkType)
{
    MediaLibraryCommand cmd(
        OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE_SUPPORTED_WATERMARK_TYPE, MediaLibraryApi::API_10);

    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(PhotoColumn::MEDIA_ID, to_string(fileId));

    NativeRdb::ValuesBucket values;
    values.Put(PhotoColumn::SUPPORTED_WATERMARK_TYPE, watermarkType);

    cmd.SetValueBucket(values);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    int32_t updateRows = MediaLibraryPhotoOperations::UpdateSupportedWatermarkType(cmd);
    CHECK_AND_RETURN_RET_LOG(updateRows > 0, updateRows,
        "UpdateSupportedWatermarkType failed. updateRows: %{public}d", updateRows);
    SubmitMetadataChanged(fileId);
    return updateRows;
}

int32_t MediaAssetsService::SetCompositeDisplayMode(const int32_t fileId, const int32_t compositeDisplayMode)
{
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    MEDIA_INFO_LOG(
        "enter MediaAssetsService::SetCompositeDisplayMode, compositeDisplayMode:%{public}d ", compositeDisplayMode);
    int32_t ret = EnhancementManager::GetInstance().SetCompositeDisplayMode(fileId, compositeDisplayMode);
    MEDIA_INFO_LOG("MediaAssetsService::SetCompositeDisplayMode ret:%{public}d", ret);
    return ret;
#endif
    return E_ERR;
}

int32_t MediaAssetsService::GrantPhotoUriPermissionInner(const GrantUriPermissionInnerDto& grantUrisPermissionInnerDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::GrantPhotoUriPermissionInner");
    auto fileIds_size = grantUrisPermissionInnerDto.fileIds.size();
    auto uriTypes_size = grantUrisPermissionInnerDto.uriTypes.size();
    auto permissionTypes_size = grantUrisPermissionInnerDto.permissionTypes.size();
    bool isValid = ((fileIds_size == uriTypes_size) && (uriTypes_size == permissionTypes_size));
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "GrantPhotoUriPermissionInner Failed");
    std::vector<DataShare::DataShareValuesBucket> values;
    for (size_t i = 0; i < grantUrisPermissionInnerDto.fileIds.size(); i++) {
        DataShare::DataShareValuesBucket value;
        if (!grantUrisPermissionInnerDto.appId.empty()) {
            value.Put(AppUriPermissionColumn::APP_ID, grantUrisPermissionInnerDto.appId);
        }
        if (grantUrisPermissionInnerDto.tokenId != -1) {
            value.Put(AppUriPermissionColumn::TARGET_TOKENID, grantUrisPermissionInnerDto.tokenId);
        }
        if (grantUrisPermissionInnerDto.srcTokenId != -1) {
            value.Put(AppUriPermissionColumn::SOURCE_TOKENID, grantUrisPermissionInnerDto.srcTokenId);
        }
        value.Put(AppUriPermissionColumn::FILE_ID, grantUrisPermissionInnerDto.fileIds[i]);
        value.Put(AppUriPermissionColumn::PERMISSION_TYPE, grantUrisPermissionInnerDto.permissionTypes[i]);
        value.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, grantUrisPermissionInnerDto.hideSensitiveType);
        value.Put(AppUriPermissionColumn::URI_TYPE, grantUrisPermissionInnerDto.uriTypes[i]);
        values.push_back(value);
    }
    string uri = "datashare:///media/phaccess_granturipermission";
    Uri createUri(uri);
    MediaLibraryCommand grantUrisPermCmd(createUri);
    int32_t errCode = this->rdbOperation_.GrantPhotoUrisPermissionInner(grantUrisPermCmd, values);
    MEDIA_INFO_LOG("MediaAssetsService::GrantPhotoUriPermissionInner ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::CheckPhotoUriPermissionInner(CheckUriPermissionInnerDto& checkUriPermissionInnerDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::CheckPhotoUriPermissionInner");
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, checkUriPermissionInnerDto.targetTokenId);
    predicates.And()->EqualTo(AppUriPermissionColumn::URI_TYPE, checkUriPermissionInnerDto.uriType);
    predicates.And()->In(AppUriPermissionColumn::FILE_ID, checkUriPermissionInnerDto.inFileIds);
    std::vector<std::string> columns = checkUriPermissionInnerDto.columns;
    string uri = "datashare:///media/phaccess_checkuripermission";
    Uri checkUri(uri);
    MediaLibraryCommand grantUrisPermCmd(checkUri);
    std::vector<std::string> outFileIds;
    std::vector<int32_t> permissionTypes;
    int32_t errCode = this->rdbOperation_.CheckPhotoUriPermissionInner(grantUrisPermCmd,
        predicates, columns, outFileIds, permissionTypes);
    checkUriPermissionInnerDto.outFileIds = outFileIds;
    checkUriPermissionInnerDto.permissionTypes = permissionTypes;
    MEDIA_INFO_LOG("MediaAssetsService::CheckPhotoUriPermissionInner ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::StartAssetChangeScanInner(
    const StartAssetChangeScanDto& startAssetChangeScanDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::StartAssetChangeScanInner");
    auto resultSet = MediaLibraryDataManager::GetInstance()->ProcessBrokerChangeMsg(startAssetChangeScanDto.operation);
    return NativeRdb::E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPermissionInner(
    const CancelUriPermissionInnerDto& cancelUriPermissionInnerDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::CancelPhotoUriPermissionInner");
    auto fileIds_size = cancelUriPermissionInnerDto.fileIds.size();
    auto uriTypes_size = cancelUriPermissionInnerDto.uriTypes.size();
    auto permissionTypes_size = cancelUriPermissionInnerDto.permissionTypes.size();
    bool isValid = ((fileIds_size == uriTypes_size) && (uriTypes_size == permissionTypes_size));
    CHECK_AND_RETURN_RET_LOG(isValid, E_ERR, "CancelPhotoUriPermissionInner Failed");
    DataShare::DataSharePredicates predicates;
    for (size_t i = 0; i < cancelUriPermissionInnerDto.fileIds.size(); i++) {
        if (i > 0) {
            predicates.Or();
        }
        predicates.BeginWrap();
        predicates.BeginWrap();
        predicates.EqualTo(AppUriPermissionColumn::SOURCE_TOKENID, cancelUriPermissionInnerDto.srcTokenId);
        predicates.Or();
        predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, cancelUriPermissionInnerDto.srcTokenId);
        predicates.EndWrap();
        predicates.EqualTo(AppUriPermissionColumn::FILE_ID, cancelUriPermissionInnerDto.fileIds[i]);
        predicates.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, cancelUriPermissionInnerDto.targetTokenId);
        predicates.EqualTo(AppUriPermissionColumn::URI_TYPE, cancelUriPermissionInnerDto.uriTypes[i]);
        vector<string> permissionTypes = cancelUriPermissionInnerDto.permissionTypes[i];
        predicates.In(AppUriPermissionColumn::PERMISSION_TYPE, permissionTypes);
        predicates.EndWrap();
    }
    string uri = "datashare:///media/phaccess_granturipermission";
    Uri deleteUri(uri);
    MediaLibraryCommand cancelUriPermCmd(deleteUri, Media::OperationType::DELETE);

    int32_t errCode = this->rdbOperation_.CancelPhotoUrisPermissionInner(cancelUriPermCmd, predicates);
    MEDIA_INFO_LOG("MediaAssetsService::CancelPhotoUriPermissionInner ret:%{public}d", errCode);
    return errCode;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetAssets(GetAssetsDto &dto)
{
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(dto.columns);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.SetDataSharePred(dto.predicates);
    // MEDIALIBRARY_TABLE just for RdbPredicates
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(dto.predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());

    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, dto.columns);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to query assets");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetAllDuplicateAssets(GetAssetsDto &dto)
{
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(dto.columns);
    NativeRdb::RdbPredicates predicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, PhotoColumn::PHOTOS_TABLE);
    auto resultSet = DuplicatePhotoOperation::GetAllDuplicateAssets(predicates, dto.columns);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to query duplicate assets");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetDuplicateAssetsToDelete(GetAssetsDto &dto)
{
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(dto.columns);
    NativeRdb::RdbPredicates predicates =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, PhotoColumn::PHOTOS_TABLE);
    auto resultSet = DuplicatePhotoOperation::GetDuplicateAssetsToDelete(predicates, dto.columns);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to query duplicate assets for delete");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

int32_t MediaAssetsService::CreateAsset(CreateAssetDto& dto)
{
    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutString(ASSET_EXTENTION, dto.extension);
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, dto.mediaType);
    assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, dto.photoSubtype);
    if (!dto.title.empty()) {
        assetInfo.PutString(MediaColumn::MEDIA_TITLE, dto.title);
    }
    if (!dto.displayName.empty()) {
        assetInfo.PutString(MediaColumn::MEDIA_NAME, dto.displayName);
    }
    if (!dto.cameraShotKey.empty()) {
        assetInfo.PutString(PhotoColumn::CAMERA_SHOT_KEY, dto.cameraShotKey);
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    CHECK_AND_RETURN_RET_LOG(ret > 0, ret, "MediaLibraryPhotoOperations::Create failed");
    dto.fileId = ret;
    dto.outUri = cmd.GetResult();
    return E_OK;
}

int32_t MediaAssetsService::CreateAssetForApp(CreateAssetDto& dto)
{
    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutString(ASSET_EXTENTION, dto.extension);
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, dto.mediaType);
    assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, dto.photoSubtype);
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_APPID, dto.appId);
    assetInfo.PutString(MEDIA_DATA_DB_PACKAGE_NAME, dto.packageName);
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_PACKAGE, dto.bundleName);
    if (!dto.title.empty()) {
        assetInfo.PutString(MediaColumn::MEDIA_TITLE, dto.title);
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetApiParam("tokenId", to_string(dto.tokenId));
    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    CHECK_AND_RETURN_RET_LOG(ret > 0, ret, "MediaLibraryPhotoOperations::Create failed");
    dto.fileId = ret;
    dto.outUri = cmd.GetResult();
    return E_OK;
}

int32_t MediaAssetsService::CreateAssetForAppWithAlbum(CreateAssetDto& dto)
{
    if (!this->rdbOperation_.QueryAlbumIdIfExists(dto.ownerAlbumId)) {
        MEDIA_ERR_LOG("Invalid ownerAlbumId:%{public}s", dto.ownerAlbumId.c_str());
        return -EINVAL;
    }

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutString(ASSET_EXTENTION, dto.extension);
    assetInfo.PutInt(MediaColumn::MEDIA_TYPE, dto.mediaType);
    assetInfo.PutInt(PhotoColumn::PHOTO_SUBTYPE, dto.photoSubtype);
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_APPID, dto.appId);
    assetInfo.PutString(MEDIA_DATA_DB_PACKAGE_NAME, dto.packageName);
    assetInfo.PutString(MEDIA_DATA_DB_OWNER_PACKAGE, dto.bundleName);
    assetInfo.PutString(PhotoColumn::PHOTO_OWNER_ALBUM_ID, dto.ownerAlbumId);
    if (!dto.title.empty()) {
        assetInfo.PutString(MediaColumn::MEDIA_TITLE, dto.title);
    }

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::CREATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetApiParam("tokenId", to_string(dto.tokenId));
    cmd.SetDeviceName(GetLocalDeviceName());
    cmd.SetBundleName(GetClientBundleName());
    int32_t ret = MediaLibraryPhotoOperations::Create(cmd);
    CHECK_AND_RETURN_RET_LOG(ret > 0, ret, "MediaLibraryPhotoOperations::Create failed");
    dto.fileId = ret;
    dto.outUri = cmd.GetResult();
    return E_OK;
}

static void ConvertToString(const vector<int32_t> &fileIds, std::vector<std::string> &strIds)
{
    for (int32_t fileId : fileIds) {
        strIds.push_back(to_string(fileId));
    }
}

int32_t MediaAssetsService::SetAssetTitle(int32_t fileId, const std::string &title)
{
    MEDIA_INFO_LOG("SetAssetTitle fileId: %{public}d, title: %{public}s.", fileId, title.c_str());
    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutString(PhotoColumn::MEDIA_TITLE, title);

    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    int32_t ret = MediaLibraryPhotoOperations::Update(cmd);
    UpdateExistedTasksTitle(fileId);
    return ret;
}

int32_t MediaAssetsService::SetAssetPending(int32_t fileId, int32_t pending)
{
    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(PhotoColumn::MEDIA_ID, std::to_string(fileId));

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutInt(PhotoColumn::MEDIA_TIME_PENDING, pending);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::UPDATE_PENDING, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::SetAssetsFavorite(const std::vector<int32_t> &fileIds, int32_t favorite)
{
    std::vector<std::string> strIds;
    ConvertToString(fileIds, strIds);
    std::vector<std::string> uris;
    this->rdbOperation_.QueryAssetsUri(strIds, uris);

    DataShare::DataSharePredicates predicate;
    predicate.In(PhotoColumn::MEDIA_ID, uris);

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutInt(PhotoColumn::MEDIA_IS_FAV, favorite);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::BATCH_UPDATE_FAV,
        MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::SetAssetsHiddenStatus(const std::vector<int32_t> &fileIds, int32_t hiddenStatus)
{
    std::vector<std::string> strIds;
    ConvertToString(fileIds, strIds);
    std::vector<std::string> uris;
    this->rdbOperation_.QueryAssetsUri(strIds, uris);

    DataShare::DataSharePredicates predicate;
    predicate.In(PhotoColumn::MEDIA_ID, uris);

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutInt(PhotoColumn::MEDIA_HIDDEN, hiddenStatus);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::HIDE, MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::SetAssetsRecentShowStatus(const std::vector<int32_t> &fileIds, int32_t recentShowStatus)
{
    std::vector<std::string> strIds;
    ConvertToString(fileIds, strIds);
    std::vector<std::string> uris;
    this->rdbOperation_.QueryAssetsUri(strIds, uris);

    DataShare::DataSharePredicates predicate;
    predicate.In(PhotoColumn::MEDIA_ID, uris);

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.PutInt(PhotoColumn::PHOTO_IS_RECENT_SHOW, recentShowStatus);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::BATCH_UPDATE_RECENT_SHOW,
        MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::SetAssetsUserComment(const std::vector<int32_t> &fileIds, const std::string &userComment)
{
    std::vector<std::string> strIds;
    ConvertToString(fileIds, strIds);
    std::vector<std::string> uris;
    this->rdbOperation_.QueryAssetsUri(strIds, uris);

    DataShare::DataSharePredicates predicate;
    predicate.In(PhotoColumn::MEDIA_ID, uris);

    NativeRdb::ValuesBucket assetInfo;
    assetInfo.Put(PhotoColumn::PHOTO_USER_COMMENT, userComment);

    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::BATCH_UPDATE_USER_COMMENT,
        MediaLibraryApi::API_10);

    cmd.SetValueBucket(assetInfo);
    cmd.SetDataSharePred(predicate);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicate, cmd.GetTableName());
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    return MediaLibraryPhotoOperations::Update(cmd);
}

int32_t MediaAssetsService::AddAssetVisitCount(int32_t fileId, int32_t visitType)
{
    MEDIA_INFO_LOG("AddAssetVisitCount fileId:%{public}d, type:%{public}d", fileId, visitType);
    auto type = static_cast<MediaVisitCountManager::VisitCountType>(visitType);
    MediaVisitCountManager::AddVisitCount(type, to_string(fileId));
    return E_OK;
}

int32_t MediaAssetsService::CloneAsset(const CloneAssetDto& cloneAssetDto)
{
    MEDIA_INFO_LOG("MediaAssetsService::CloneAsset, fileId:%{public}d, title:%{public}s",
        cloneAssetDto.fileId, cloneAssetDto.title.c_str());

    int32_t fileId = cloneAssetDto.fileId;
    string title = cloneAssetDto.title;
    return MediaLibraryAlbumFusionUtils::CloneSingleAsset(fileId, title);
}

shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::ConvertFormat(const ConvertFormatDto& convertFormatDto)
{
    MEDIA_INFO_LOG("ConvertFormat: %{public}s", convertFormatDto.ToString().c_str());
    int32_t fileId = convertFormatDto.fileId;
    std::string title = convertFormatDto.title;
    std::string extension = convertFormatDto.extension;
    
    auto resultSet = MediaLibraryAlbumFusionUtils::ConvertFormatAsset(fileId, title, extension);
    CHECK_AND_RETURN_RET_LOG(resultSet, nullptr, "Failed to ConvertFormatAsset");
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    return make_shared<DataShare::DataShareResultSet>(resultSetBridge);
}

bool MediaAssetsService::CheckMimeType(const int32_t fileId)
{
    CHECK_AND_RETURN_RET_LOG(fileId > 0, false,
        "Invalid parameters for CheckMimeType, fileId: %{public}d", fileId);
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY);
    cmd.GetAbsRdbPredicates()->EqualTo(PhotoColumn::MEDIA_ID, fileId);
    vector<string> columns {MediaColumn::MEDIA_ID, PhotoColumn::MEDIA_NAME, MediaColumn::MEDIA_MIME_TYPE};
    auto resultSet = DatabaseAdapter::Query(cmd, columns);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("result set is empty");
        return false;
    }
    string mimeType = GetStringVal(MediaColumn::MEDIA_MIME_TYPE, resultSet);
    if (mimeType != HEIF_MIME_TYPE && mimeType != HEIC_MIME_TYPE) {
        MEDIA_ERR_LOG("mimeType : %{public}s, The requested asset must be heif|heic", mimeType.c_str());
        return false;
    }
    return true;
}

int32_t MediaAssetsService::CreateTmpCompatibleDup(const CreateTmpCompatibleDupDto &createTmpCompatibleDupDto)
{
    MEDIA_DEBUG_LOG("CreateTmpCompatibleDup: %{public}s", createTmpCompatibleDupDto.ToString().c_str());
    int32_t fileId = createTmpCompatibleDupDto.fileId;
    std::string path = std::move(createTmpCompatibleDupDto.path);
    CHECK_AND_RETURN_RET_LOG(fileId > 0 && !path.empty(), E_INNER_FAIL,
        "Invalid parameters for CreateTmpCompatibleDup, fileId: %{public}d, path: %{public}s",
        fileId, path.c_str());

    auto startTime = std::chrono::high_resolution_clock::now();
    size_t size = 0;
    int32_t dupExist = 0;
    auto dfxManager = DfxManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dfxManager != nullptr, E_INVALID_VALUES, "DfxManager::GetInstance() returned nullptr");
    auto err = MediaLibraryAlbumFusionUtils::CreateTmpCompatibleDup(fileId, path, size, dupExist);
    if (err == E_OK && dupExist == 0) {
        err = this->rdbOperation_.UpdateTmpCompatibleDup(fileId, size);
        if (err == E_OK) {
            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<uint16_t, std::milli> duration =
                std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            MEDIA_INFO_LOG("CreateTmpCompatibleDup duration:%{public}d ms", duration.count());
            dfxManager->HandleTranscodeCostTime(duration.count());
        } else {
            MEDIA_ERR_LOG("CreateTmpCompatibleDup dfx updata database failed");
            dfxManager->HandleTranscodeFailed(INNER_FAILED);
        }
    }
    return err;
}

int32_t MediaAssetsService::RevertToOriginal(const RevertToOriginalDto& revertToOriginalDto)
{
    int32_t fileId = revertToOriginalDto.fileId;
    MEDIA_INFO_LOG("MediaAssetsService::RevertToOriginal, fileId:%{public}d", fileId);

    int32_t errCode = this->rdbOperation_.RevertToOrigin(fileId);
    if (errCode == E_SUCCESS) {
        string fileUri = revertToOriginalDto.fileUri;
        Uri uri(fileUri);
        MediaLibraryCommand cmdEditCommit(uri);
        cmdEditCommit.SetOprnObject(OperationObject::FILESYSTEM_PHOTO);
        NativeRdb::ValuesBucket values;
        values.Put(PhotoColumn::MEDIA_ID, fileId);
        cmdEditCommit.SetValueBucket(values);
        MediaLibraryVisionOperations::EditCommitOperation(cmdEditCommit);
    }
    return errCode;
}

int32_t MediaAssetsService::SubmitCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    MediaLibraryCommand cmd(PhotoColumn::PHOTOS_TABLE);
    DataShare::DataSharePredicates predicate;
    predicate.In(MediaColumn::MEDIA_ID, cloudEnhancementDto.fileUris);
    cmd.SetDataSharePred(predicate);
#ifdef ABILITY_CLOUD_ENHANCEMENT_SUPPORT
    return EnhancementManager::GetInstance().HandleAddOperation(cmd, cloudEnhancementDto.hasCloudWatermark,
        cloudEnhancementDto.triggerMode);
#else
    MEDIA_ERR_LOG("not supply cloud enhancement service");
    return E_ERR;
#endif
}

int32_t MediaAssetsService::PrioritizeCloudEnhancementTask(const CloudEnhancementDto& cloudEnhancementDto)
{
    MediaLibraryCommand cmd(PhotoColumn::PHOTOS_TABLE);
    DataShare::DataSharePredicates predicate;
    predicate.EqualTo(MediaColumn::MEDIA_ID, cloudEnhancementDto.fileUris.front());
    cmd.SetDataSharePred(predicate);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    return EnhancementManager::GetInstance().HandlePrioritizeOperation(cmd);
#else
    return E_ERR;
#endif
}

int32_t MediaAssetsService::CancelCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    MediaLibraryCommand cmd(PhotoColumn::PHOTOS_TABLE);
    DataShare::DataSharePredicates predicate;
    predicate.In(MediaColumn::MEDIA_ID, cloudEnhancementDto.fileUris);
    cmd.SetDataSharePred(predicate);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    return EnhancementManager::GetInstance().HandleCancelOperation(cmd);
#else
    return E_ERR;
#endif
}

int32_t MediaAssetsService::CancelAllCloudEnhancementTasks()
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    return EnhancementManager::GetInstance().HandleCancelAllOperation();
#else
    return E_ERR;
#endif
}

int32_t MediaAssetsService::GrantPhotoUriPermission(const GrantUriPermissionDto &grantUriPermissionDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::GrantPhotoUriPermission");
    NativeRdb::ValuesBucket values;
    values.PutLong(AppUriPermissionColumn::TARGET_TOKENID, grantUriPermissionDto.tokenId);
    values.PutLong(AppUriPermissionColumn::SOURCE_TOKENID, grantUriPermissionDto.srcTokenId);
    values.PutInt(AppUriPermissionColumn::FILE_ID, grantUriPermissionDto.fileId);
    values.PutInt(AppUriPermissionColumn::PERMISSION_TYPE, grantUriPermissionDto.permissionType);
    values.PutInt(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, grantUriPermissionDto.hideSensitiveType);
    values.PutInt(AppUriPermissionColumn::URI_TYPE, grantUriPermissionDto.uriType);
    MediaLibraryCommand grantUriPermCmd(values);
    int32_t errCode = this->rdbOperation_.GrantPhotoUriPermission(grantUriPermCmd);
    MEDIA_INFO_LOG("MediaAssetsService::GrantPhotoUriPermission ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::GrantPhotoUrisPermission(const GrantUrisPermissionDto &grantUrisPermissionDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::GrantPhotoUrisPermission");
    std::vector<DataShare::DataShareValuesBucket> values;
    for (int32_t fileId : grantUrisPermissionDto.fileIds) {
        DataShare::DataShareValuesBucket value;
        value.Put(AppUriPermissionColumn::TARGET_TOKENID, grantUrisPermissionDto.tokenId);
        value.Put(AppUriPermissionColumn::SOURCE_TOKENID, grantUrisPermissionDto.srcTokenId);
        value.Put(AppUriPermissionColumn::FILE_ID, fileId);
        value.Put(AppUriPermissionColumn::PERMISSION_TYPE, grantUrisPermissionDto.permissionType);
        value.Put(AppUriSensitiveColumn::HIDE_SENSITIVE_TYPE, grantUrisPermissionDto.hideSensitiveType);
        value.Put(AppUriPermissionColumn::URI_TYPE, grantUrisPermissionDto.uriType);
        values.push_back(value);
    }
    string uri = "datashare:///media/app_uri_permission_operation/create?api_version=10";
    Uri createUri(uri);
    MediaLibraryCommand grantUrisPermCmd(createUri);
    // Use values only
    int32_t errCode = this->rdbOperation_.GrantPhotoUrisPermission(grantUrisPermCmd, values);
    MEDIA_INFO_LOG("MediaAssetsService::GrantPhotoUrisPermission ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::CancelPhotoUriPermission(const CancelUriPermissionDto &cancelUriPermissionDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::CancelPhotoUriPermission");
    std::string uriPermissionTabel = "UriPermission";
    NativeRdb::RdbPredicates rdbPredicate(uriPermissionTabel);
    rdbPredicate.EqualTo(AppUriPermissionColumn::TARGET_TOKENID, cancelUriPermissionDto.tokenId);
    rdbPredicate.EqualTo(AppUriPermissionColumn::SOURCE_TOKENID, cancelUriPermissionDto.srcTokenId);
    rdbPredicate.EqualTo(AppUriPermissionColumn::FILE_ID, cancelUriPermissionDto.fileId);
    rdbPredicate.EqualTo(AppUriPermissionColumn::PERMISSION_TYPE, cancelUriPermissionDto.permissionType);
    rdbPredicate.EqualTo(AppUriPermissionColumn::URI_TYPE, cancelUriPermissionDto.uriType);
    int32_t errCode = this->rdbOperation_.CancelPhotoUriPermission(rdbPredicate);
    MEDIA_INFO_LOG("MediaAssetsService::CancelPhotoUriPermission ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::StartThumbnailCreationTask(
    const StartThumbnailCreationTaskDto &startThumbnailCreationTaskDto)
{
    MEDIA_INFO_LOG(
        "enter MediaAssetsService::StartThumbnailCreationTask, %{public}d", startThumbnailCreationTaskDto.requestId);
    NativeRdb::RdbPredicates rdbPredicate = OHOS::RdbDataShareAdapter::RdbUtils::ToPredicates(
        startThumbnailCreationTaskDto.predicates, PhotoColumn::PHOTOS_TABLE);
    int32_t errCode =
        this->rdbOperation_.StartThumbnailCreationTask(rdbPredicate, startThumbnailCreationTaskDto.requestId,
            startThumbnailCreationTaskDto.pid);
    MEDIA_INFO_LOG("MediaAssetsService::startThumbnailCreationTaskDto ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::StopThumbnailCreationTask(const StopThumbnailCreationTaskDto &stopThumbnailCreationTaskDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::StopThumbnailCreationTask");
    int32_t errCode = this->rdbOperation_.StopThumbnailCreationTask(stopThumbnailCreationTaskDto.requestId,
        stopThumbnailCreationTaskDto.pid);
    MEDIA_INFO_LOG("MediaAssetsService::StopThumbnailCreationTask ret:%{public}d", errCode);
    return errCode;
}

int32_t MediaAssetsService::RequestContent(const string& mediaId, int32_t& position)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, mediaId);
    std::vector<std::string> fetchColumn { PhotoColumn::PHOTO_POSITION };

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, fetchColumn);
    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    int index;
    int err = resultSet->GetColumnIndex(PhotoColumn::PHOTO_POSITION, index);
    if (err != E_OK) {
        MEDIA_ERR_LOG("Failed to GetColumnIndex");
        return E_ERR;
    }
    resultSet->GetInt(index, position);
    return E_OK;
}

shared_ptr<NativeRdb::ResultSet> MediaAssetsService::GetCloudEnhancementPair(const string& photoUri)
{
    MediaLibraryCommand cmd(PhotoColumn::PHOTOS_TABLE);
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, photoUri);
    cmd.SetDataSharePred(predicates);
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    return EnhancementManager::GetInstance().HandleGetPairOperation(cmd);
#else
    return nullptr;
#endif
}

int32_t MediaAssetsService::QueryCloudEnhancementTaskState(const string& photoUri,
    QueryCloudEnhancementTaskStateDto& dto)
{
    return this->rdbOperation_.QueryEnhancementTaskState(photoUri, dto);
}

int32_t MediaAssetsService::SyncCloudEnhancementTaskStatus()
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_ENHANCEMENT
    return EnhancementManager::GetInstance().HandleSyncOperation();
#else
    return E_ERR;
#endif
}

int32_t MediaAssetsService::QueryPhotoStatus(const QueryPhotoReqBody &req, QueryPhotoRespBody &resp)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MediaColumn::MEDIA_ID, req.fileId);
    using namespace RdbDataShareAdapter;
    NativeRdb::RdbPredicates rdbPredicates = RdbUtils::ToPredicates(predicates, PhotoColumn::PHOTOS_TABLE);
    std::vector<std::string> columns { PhotoColumn::PHOTO_QUALITY, PhotoColumn::PHOTO_ID };

    shared_ptr<NativeRdb::ResultSet> resSet = MediaLibraryRdbStore::QueryWithFilter(rdbPredicates, columns);

    auto resultSetBridge = RdbUtils::ToResultSetBridge(resSet);
    using namespace DataShare;
    auto resultSet = make_shared<DataShareResultSet>(resultSetBridge);

    if (resultSet == nullptr || resultSet->GoToFirstRow() != E_OK) {
        MEDIA_INFO_LOG("query resultSet is nullptr");
        return E_ERR;
    }
    int indexOfPhotoId = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_ID, indexOfPhotoId);
    std::string photoId;
    resultSet->GetString(indexOfPhotoId, photoId);
    resp.photoId = photoId;

    int columnIndexQuality = -1;
    resultSet->GetColumnIndex(PhotoColumn::PHOTO_QUALITY, columnIndexQuality);
    int currentPhotoQuality = HIGH_QUALITY_IMAGE;
    resultSet->GetInt(columnIndexQuality, currentPhotoQuality);
    resp.photoQuality = currentPhotoQuality;
    return E_SUCCESS;
}

int32_t MediaAssetsService::LogMovingPhoto(const AdaptedReqBody &req)
{
    string packageName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    CHECK_AND_WARN_LOG(!packageName.empty(), "Package name is empty, adapted: %{public}d",
        static_cast<int>(req.adapted));
    DfxManager::GetInstance()->HandleAdaptationToMovingPhoto(packageName, req.adapted);
    return E_SUCCESS;
}

int32_t MediaAssetsService::LogCinematicVideo(const CinematicVideoAccessReqBody &req)
{
    DfxManager::GetInstance()->HandleCinematicVideoAccessTimes(false, req.isHighQaulity);
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetResultSetFromDb(const GetResultSetFromDbDto& getResultSetFromDbDto,
    GetResultSetFromDbRespBody& resp)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(getResultSetFromDbDto.columnName, getResultSetFromDbDto.value);
    predicates.And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));

    Uri uri(MEDIALIBRARY_MEDIA_PREFIX);
    MediaLibraryCommand cmd(uri);
    cmd.SetDataSharePred(predicates);

    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());
    std::vector<std::string> columns = getResultSetFromDbDto.columns;
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(const_cast<vector<string> &>(columns));

    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryFileOperations::QueryFileOperation(cmd, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    auto resultSetBridge = RdbUtils::ToResultSetBridge(resultSet);
    resp.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetResultSetFromPhotosExtend(const string &value, vector<string> &columns,
    GetResultSetFromPhotosExtendRespBody& resp)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    string fileId = MediaFileUtils::GetIdFromUri(value);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    auto resultSetBridge = RdbUtils::ToResultSetBridge(resultSet);
    resp.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetMovingPhotoDateModified(const string &fileId, GetMovingPhotoDateModifiedRespBody& resp)
{
    NativeRdb::RdbPredicates predicates(PhotoColumn::PHOTOS_TABLE);
    predicates.EqualTo(MediaColumn::MEDIA_ID, fileId);

    vector<string> columns = {
        MediaColumn::MEDIA_DATE_MODIFIED,
    };

    auto resultSet = MediaLibraryRdbStore::QueryWithFilter(predicates, columns);
    if (resultSet == nullptr || resultSet->GoToNextRow() != NativeRdb::E_OK) {
        MEDIA_ERR_LOG("query resultSet is nullptr");
        return E_ERR;
    }

    resp.dateModified = GetInt64Val(MediaColumn::MEDIA_DATE_MODIFIED, resultSet);
    return E_OK;
}

int32_t MediaAssetsService::CloseAsset(const CloseAssetReqBody &req)
{
    MEDIA_INFO_LOG("enter CloseAsset, req.uri=%{public}s", req.uri.c_str());
    NativeRdb::ValuesBucket valuesBucket;
    valuesBucket.PutString(MEDIA_DATA_DB_URI, req.uri);
    MediaLibraryCommand cmd(valuesBucket);
    return MediaLibraryObjectUtils::CloseFile(cmd);
}

static int BuildPredicates(const std::vector<std::string> &queryTabOldPhotosUris,
    DataShare::DataSharePredicates &predicates)
{
    const std::string GALLERY_URI_PREFIX = "//media";
    const std::string GALLERY_PATH = "/storage/emulated";

    vector<string> clauses;
        clauses.push_back(PhotoColumn::PHOTOS_TABLE + "." + MediaColumn::MEDIA_ID + " = " +
        TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + TabOldPhotosColumn::MEDIA_ID);
    predicates.InnerJoin(PhotoColumn::PHOTOS_TABLE)->On(clauses);

    int conditionCount = 0;
    for (const auto &uri : queryTabOldPhotosUris) {
        if (uri.find(GALLERY_URI_PREFIX) != std::string::npos) {
            size_t lastSlashPos = uri.rfind('/');
            if (lastSlashPos != std::string::npos && lastSlashPos + 1 < uri.length()) {
                std::string idStr = uri.substr(lastSlashPos + 1);
                predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_ID, idStr);
                conditionCount += 1;
            }
        } else if (uri.find(GALLERY_PATH) != std::string::npos) {
            predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_FILE_PATH, uri);
            conditionCount += 1;
        } else if (!uri.empty() && std::all_of(uri.begin(), uri.end(), ::isdigit)) {
            predicates.Or()->EqualTo(TabOldPhotosColumn::MEDIA_OLD_ID, uri);
            conditionCount += 1;
        }
    }
    CHECK_AND_RETURN_RET_LOG(conditionCount != 0, E_FAIL, "Zero uri condition");
    return E_OK;
}

int32_t MediaAssetsService::GetUrisByOldUrisInner(GetUrisByOldUrisInnerDto& getUrisByOldUrisInnerDto)
{
    MEDIA_INFO_LOG("enter MediaAssetsService::GetUrisByOldUrisInner");
    DataShare::DataSharePredicates predicates;
    int ret = BuildPredicates(getUrisByOldUrisInnerDto.uris, predicates);
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, E_ERR, "MediaAssetsService::GetUrisByOldUrisInner build predicates failed");

    std::vector<std::string> columns = getUrisByOldUrisInnerDto.columns;
    
    string uri = "datashare:///media/tab_old_photos_operation/query";
    Uri getUri(uri);
    MediaLibraryCommand getUrisByOldCmd(getUri);
    auto resultSet = this->rdbOperation_.GetUrisByOldUrisInner(getUrisByOldCmd,
        predicates, columns);
    
    CHECK_AND_RETURN_RET_LOG(resultSet != nullptr, E_ERR,
        "MediaAssetsService::GetUrisByOldUrisInner build predicates failed");
    while (resultSet->GoToNextRow() == NativeRdb::E_OK) {
        getUrisByOldUrisInnerDto.fileIds.emplace_back(GetInt32Val(COLUMN_FILE_ID, resultSet));
        getUrisByOldUrisInnerDto.datas.emplace_back(GetStringVal(COLUMN_DATA, resultSet));
        getUrisByOldUrisInnerDto.displayNames.emplace_back(GetStringVal(COLUMN_DISPLAY_NAME, resultSet));
        getUrisByOldUrisInnerDto.oldFileIds.emplace_back(GetInt32Val(COLUMN_OLD_FILE_ID, resultSet));
        getUrisByOldUrisInnerDto.oldDatas.emplace_back(GetStringVal(COLUMN_OLD_DATA, resultSet));
    }
    return E_OK;
}

int32_t MediaAssetsService::Restore(const RestoreDto &dto)
{
    NativeRdb::ValuesBucket values;
    values.PutString("dbPath", dto.dbPath);
    values.PutString("albumLpath", dto.albumLpath);
    values.PutString("keyPath", dto.keyPath);
    values.PutString("isDeduplication", dto.isDeduplication ? "true" : "false");
    values.PutString("bundleName", dto.bundleName);
    values.PutString("appName", dto.appName);
    values.PutString("appId", dto.appId);
    MediaLibraryCommand cmd(values);
    return MediaLibraryPhotoOperations::ProcessCustomRestore(cmd);
}

int32_t MediaAssetsService::StopRestore(const std::string &keyPath)
{
    NativeRdb::ValuesBucket values;
    values.PutString("keyPath", keyPath);
    MediaLibraryCommand cmd(values);
    return MediaLibraryPhotoOperations::CancelCustomRestore(cmd);
}

int32_t MediaAssetsService::StartDownloadCloudMedia(CloudMediaDownloadType downloadType)
{
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    return instance.StartDownloadCloudAsset(downloadType);
}

int32_t MediaAssetsService::PauseDownloadCloudMedia()
{
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    return instance.PauseDownloadCloudAsset(CloudMediaTaskPauseCause::USER_PAUSED);
}

int32_t MediaAssetsService::CancelDownloadCloudMedia()
{
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    return instance.CancelDownloadCloudAsset();
}

int32_t MediaAssetsService::RetainCloudMediaAsset(CloudMediaRetainType retainType)
{
    CloudMediaAssetManager &instance = CloudMediaAssetManager::GetInstance();
    return instance.ForceRetainDownloadCloudMedia(retainType);
}

int32_t MediaAssetsService::IsEdited(const IsEditedDto &dto, IsEditedRespBody &respBody)
{
    DataShare::DataSharePredicates predicates;
    vector<string> columns = { PhotoColumn::PHOTO_EDIT_TIME };
    predicates.EqualTo(MediaColumn::MEDIA_ID, to_string(dto.fileId));
    MediaLibraryCommand cmd(OperationObject::FILESYSTEM_PHOTO, OperationType::QUERY, MediaLibraryApi::API_10);
    cmd.SetDataSharePred(predicates);
    auto resultSet = MediaLibraryPhotoOperations::Query(cmd, columns);
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::RequestEditData(const RequestEditDataDto &dto, RequestEditDataRespBody &respBody)
{
    NativeRdb::RdbPredicates rdbPredicate =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, PhotoColumn::PHOTOS_TABLE);

    auto resultSet = MediaLibraryRdbStore::QueryEditDataExists(rdbPredicate);
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetEditData(const GetEditDataDto &dto, GetEditDataRespBody &respBody)
{
    NativeRdb::RdbPredicates rdbPredicate =
        RdbDataShareAdapter::RdbUtils::ToPredicates(dto.predicates, PhotoColumn::PHOTOS_TABLE);

    auto resultSet = MediaLibraryRdbStore::QueryEditDataExists(rdbPredicate);
    auto resultSetBridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetCloudMediaAssetStatus(string &status)
{
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    status = instance.GetCloudMediaAssetTaskStatus();
    return E_OK;
}

int32_t MediaAssetsService::StartBatchDownloadCloudResources(StartBatchDownloadCloudResourcesReqBody &reqBody,
    StartBatchDownloadCloudResourcesRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.StartBatchDownloadCloudResources(reqBody, respBody);
    MEDIA_INFO_LOG("MediaAssetsService StartBatchDownloadCloudResources END ret: %{public}d", ret);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::ResumeBatchDownloadCloudResources(ResumeBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.ResumeBatchDownloadCloudResources(reqBody);
    MEDIA_INFO_LOG("MediaAssetsService ResumeBatchDownloadCloudResources END ret: %{public}d", ret);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::PauseBatchDownloadCloudResources(PauseBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.PauseBatchDownloadCloudResources(reqBody);
    MEDIA_INFO_LOG("MediaAssetsService PauseBatchDownloadCloudResources END ret: %{public}d", ret);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::CancelBatchDownloadCloudResources(CancelBatchDownloadCloudResourcesReqBody &reqBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.CancelBatchDownloadCloudResources(reqBody);
    MEDIA_INFO_LOG("MediaAssetsService CancelBatchDownloadCloudResources END ret: %{public}d", ret);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::GetCloudMediaBatchDownloadResourcesStatus(
    GetBatchDownloadCloudResourcesStatusReqBody &reqBody, GetBatchDownloadCloudResourcesStatusRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.GetCloudMediaBatchDownloadResourcesStatus(reqBody, respBody);
    MEDIA_INFO_LOG("MediaAssetsService GetCloudMediaBatchDownloadResourcesStatus ret: %{public}d", ret);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::GetCloudMediaBatchDownloadResourcesCount(
    GetBatchDownloadCloudResourcesCountReqBody &reqBody, GetBatchDownloadCloudResourcesCountRespBody &respBody)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    CloudMediaAssetManager &instance =  CloudMediaAssetManager::GetInstance();
    int32_t ret = instance.GetCloudMediaBatchDownloadResourcesCount(reqBody, respBody);
    MEDIA_INFO_LOG("MediaAssetsService GetCloudMediaBatchDownloadResourcesCount ret: %{public}d resp size: %{public}d",
        ret, respBody.count);
    return ret;
#else
    return 0;
#endif
}

int32_t MediaAssetsService::GetCloudEnhancementPair(
    const GetCloudEnhancementPairDto &dto, GetCloudEnhancementPairRespBody &respBody)
{
    shared_ptr<NativeRdb::ResultSet> result = MediaAssetsService::GetInstance().GetCloudEnhancementPair(dto.photoUri);
    auto resultSetBridge = RdbUtils::ToResultSetBridge(result);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetFilePathFromUri(const std::string &virtualId, GetFilePathFromUriRespBody &respBody)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_ID, virtualId);
    predicates.And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));

    MediaLibraryCommand cmd(MEDIALIBRARY_TABLE);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());
    vector<string> columns = { MEDIA_DATA_DB_FILE_PATH };
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryFileOperations::QueryFileOperation(cmd, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetFilePathFromUri query resultSet is nullptr");
        return E_ERR;
    }

    auto resultSetBridge = RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::GetUriFromFilePath(const std::string &tempPath, GetUriFromFilePathRespBody &respBody)
{
    DataShare::DataSharePredicates predicates;
    predicates.EqualTo(MEDIA_DATA_DB_FILE_PATH, tempPath);
    predicates.And()->EqualTo(MEDIA_DATA_DB_IS_TRASH, to_string(NOT_TRASHED));

    MediaLibraryCommand cmd(MEDIALIBRARY_TABLE);
    cmd.SetDataSharePred(predicates);
    NativeRdb::RdbPredicates rdbPredicate = RdbUtils::ToPredicates(predicates, MEDIALIBRARY_TABLE);
    cmd.GetAbsRdbPredicates()->SetWhereClause(rdbPredicate.GetWhereClause());
    cmd.GetAbsRdbPredicates()->SetWhereArgs(rdbPredicate.GetWhereArgs());
    cmd.GetAbsRdbPredicates()->SetOrder(rdbPredicate.GetOrder());
    vector<string> columns = { MEDIA_DATA_DB_ID };
    MediaLibraryRdbUtils::AddVirtualColumnsOfDateType(columns);
    shared_ptr<NativeRdb::ResultSet> resultSet = MediaLibraryFileOperations::QueryFileOperation(cmd, columns);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("GetUriFromFilePath query resultSet is nullptr");
        return E_ERR;
    }

    auto resultSetBridge = RdbUtils::ToResultSetBridge(resultSet);
    respBody.resultSet = make_shared<DataShare::DataShareResultSet>(resultSetBridge);
    return E_OK;
}

int32_t MediaAssetsService::CancelRequest(const std::string &photoId,
    const int32_t mediaType)
{
    MEDIA_INFO_LOG("Cancel Request, photoId: %{public}s, mediaType: %{public}d",
        photoId.c_str(), mediaType);
    if (static_cast<MediaType>(mediaType) == MEDIA_TYPE_VIDEO) {
        MultiStagesVideoCaptureManager::GetInstance().CancelProcessRequest(photoId);
        return E_OK;
    }
    MultiStagesPhotoCaptureManager::GetInstance().CancelProcessRequest(photoId);
 
    return E_OK;
}

int32_t MediaAssetsService::CanSupportedCompatibleDuplicate(const std::string &bundleName,
    HeifTranscodingCheckRespBody &respBody)
{
    respBody.canSupportedCompatibleDuplicate = HeifTranscodingCheckUtils::CanSupportedCompatibleDuplicate(bundleName);
    return E_OK;
}

static int32_t WriteBetaDebugTaskSet(const string& betaIssueId)
{
    std::lock_guard<std::mutex> lock(DFXTaskMutex);
    CHECK_AND_RETURN_RET(DFXTaskSet.count(betaIssueId) == 0, E_ACQ_BETA_TASK_FAIL);
    DFXTaskSet.insert(betaIssueId);
    return E_SUCCESS;
}

static int32_t EraseBetaDebugTaskSet(const string& betaIssueId)
{
    std::lock_guard<std::mutex> lock(DFXTaskMutex);
    CHECK_AND_RETURN_RET(DFXTaskSet.count(betaIssueId) != 0, E_ACQ_BETA_TASK_FAIL);
    DFXTaskSet.erase(betaIssueId);
    return E_SUCCESS;
}

int32_t MediaAssetsService::AcquireDebugDatabase(const string &betaIssueId, const std::string &betaScenario,
    AcquireDebugDatabaseRespBody &respBody)
{
    MEDIA_INFO_LOG("MediaAssetsService::AcquireDebugDatabase start");
    int32_t status = WriteBetaDebugTaskSet(betaIssueId);
    CHECK_AND_RETURN_RET_LOG(status == E_SUCCESS, status, "Acquire atabase task is exist, betaIssueId = %{public}s",
        betaIssueId.c_str());
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dataManager != nullptr, E_INNER_FAIL, "dataManager is nullptr");

    std::string fileName;
    std::string fileSize;
    int32_t ret = dataManager->AcquireDebugDatabase(betaIssueId, betaScenario, fileName, fileSize);
    CHECK_AND_PRINT_LOG(EraseBetaDebugTaskSet(betaIssueId) == E_SUCCESS, "Failed to erase beta task");
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "Failed to acquire debug database, errCode = %{public}d", ret);
    respBody.fileName = fileName;
    respBody.fileSize = fileSize;
    return E_SUCCESS;
}

int32_t MediaAssetsService::ReleaseDebugDatabase(const string &betaIssueId)
{
    MEDIA_INFO_LOG("MediaAssetsService::ReleaseDebugDatabase start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dataManager != nullptr, E_INNER_FAIL, "dataManager is nullptr");
    int32_t ret = dataManager->ReleaseDebugDatabase(betaIssueId);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "Failed to release debug database");
    return E_SUCCESS;
}

int32_t MediaAssetsService::OpenAssetCompress(const OpenAssetCompressDto &dto, OpenAssetCompressRespBody &respBody)
{
    MEDIA_INFO_LOG("MediaAssetsService::OpenAssetCompress start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dataManager != nullptr, E_INNER_FAIL, "dataManager is nullptr");
    int32_t ret = dataManager->OpenAssetCompress(dto.uri, dto.type, dto.version, respBody.fileDescriptor);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "Failed to open asset compress, errCode = %{public}d", ret);
    return E_SUCCESS;
}

int32_t MediaAssetsService::NotifyAssetSended(const std::string &uri)
{
    MEDIA_INFO_LOG("MediaAssetsService::NotifyAssetSended start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dataManager != nullptr, E_INNER_FAIL, "dataManager is nullptr");
    int32_t ret = dataManager->NotifyAssetSended(uri);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "Failed to notify asset sended, errCode = %{public}d", ret);
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetAssetCompressVersion(int32_t &version)
{
    MEDIA_INFO_LOG("MediaAssetsService::GetAssetCompressVersion start");
    auto dataManager = MediaLibraryDataManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(dataManager != nullptr, E_INNER_FAIL, "dataManager is nullptr");
    version = dataManager->GetAssetCompressVersion();
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetCompressAssetSize(const std::vector<std::string> &uris,
    GetCompressAssetSizeRespBody &respBody)
{
    MEDIA_INFO_LOG("MediaAssetsService::GetCompressAssetSize start");
    int32_t ret = MediaLibraryPhotoOperations::GetCompressAssetSize(uris, respBody.totalSize);
    CHECK_AND_RETURN_RET_LOG(ret == E_SUCCESS, ret, "Failed to get compress asset size, errCode = %{public}d", ret);
    return E_SUCCESS;
}
} // namespace OHOS::Media