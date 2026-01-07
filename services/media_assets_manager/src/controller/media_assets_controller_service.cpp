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

#define MLOG_TAG "MediaAssetsControllerService"

#include "media_assets_controller_service.h"
#include "media_assets_service.h"
#include "media_log.h"
#include "create_asset_vo.h"
#include "create_asset_dto.h"
#include "form_info_vo.h"
#include "form_info_dto.h"
#include "modify_assets_vo.h"
#include "parameter_utils.h"
#include "commit_edited_asset_vo.h"
#include "delete_photos_completed_vo.h"
#include "asset_change_vo.h"
#include "delete_photos_vo.h"
#include "trash_photos_vo.h"
#include "clone_asset_vo.h"
#include "clone_asset_dto.h"
#include "revert_to_original_vo.h"
#include "revert_to_original_dto.h"
#include "media_file_utils.h"
#include "cloud_enhancement_vo.h"
#include "cloud_enhancement_dto.h"
#include "start_download_cloud_media_vo.h"
#include "retain_cloud_media_asset_vo.h"
#include "grant_photo_uri_permission_vo.h"
#include "grant_photo_uris_permission_vo.h"
#include "grant_photo_uri_permission_inner_vo.h"
#include "cancel_photo_uri_permission_vo.h"
#include "cancel_photo_uri_permission_inner_vo.h"
#include "check_photo_uri_permission_inner_vo.h"
#include "start_asset_change_scan_vo.h"
#include "start_asset_change_scan_dto.h"
#include "start_thumbnail_creation_task_vo.h"
#include "stop_thumbnail_creation_task_vo.h"
#include "is_edited_vo.h"
#include "request_edit_data_vo.h"
#include "get_edit_data_vo.h"
#include "get_cloudmedia_asset_status_vo.h"
#include "request_content_vo.h"
#include "get_cloud_enhancement_pair_vo.h"
#include "query_cloud_enhancement_task_state_vo.h"
#include "query_cloud_enhancement_task_state_dto.h"
#include "adapted_vo.h"
#include "log_cinematic_access_vo.h"
#include "get_photo_index_vo.h"
#include "query_photo_vo.h"
#include "get_highlight_album_info_vo.h"
#include "get_analysis_process_vo.h"
#include "medialibrary_rdb_utils.h"
#include "permission_common.h"
#include "convert_format_vo.h"
#include "convert_format_dto.h"
#include "create_tmp_compatible_dup_vo.h"
#include "create_tmp_compatible_dup_dto.h"
#include "add_visit_count_vo.h"
#include "get_result_set_from_db_vo.h"
#include "get_result_set_from_photos_extend_vo.h"
#include "get_moving_photo_date_modified_vo.h"
#include "get_uri_from_filepath_vo.h"
#include "get_filepath_from_uri_vo.h"
#include "get_uris_by_old_uris_inner_vo.h"
#include "close_asset_vo.h"
#include "stop_restore_vo.h"
#include "heif_transcoding_check_vo.h"
#include "medialibrary_file_operations.h"
#include "dfx_timer.h"
#include "dfx_const.h"
#include "cloud_media_asset_types.h"
#include "is_edited_dto.h"
#include "request_edit_data_dto.h"
#include "get_edit_data_dto.h"
#include "get_cloud_enhancement_pair_dto.h"
#include "permission_utils.h"
#include "media_app_uri_permission_column.h"
#include "cancel_request_vo.h"
#include "start_batch_download_cloud_resources_vo.h"
#include "resume_batch_download_cloud_resources_vo.h"
#include "pause_batch_download_cloud_resources_vo.h"
#include "cancel_batch_download_cloud_resources_vo.h"
#include "get_batch_download_cloud_resources_status_vo.h"
#include "get_batch_download_cloud_resources_count_vo.h"
#include "acquire_debug_database_vo.h"
#include "release_debug_database_vo.h"
#include "get_fussion_assets_vo.h"
#include "photo_album.h"
#include "get_asset_compress_version_vo.h"
#include "notify_asset_sended_vo.h"
#include "open_asset_compress_vo.h"
#include "open_asset_compress_dto.h"
#include "get_compress_asset_size_vo.h"

namespace OHOS::Media {
using namespace std;
static const size_t FACARD_MAX_REGISTER_OBSERVER = 500;

using SpecialRequestHandle = int32_t (MediaAssetsControllerService::*)(
    MessageParcel &, MessageParcel &, OHOS::Media::IPC::IPCContext &);

using RequestHandle = int32_t (MediaAssetsControllerService::*)(MessageParcel &, MessageParcel &);

const std::map<uint32_t, SpecialRequestHandle> SPECIAL_HANDLERS = {
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS),
        &MediaAssetsControllerService::GetAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS),
        &MediaAssetsControllerService::GetBurstAssets
    },
};

const std::map<uint32_t, RequestHandle> HANDLERS = {
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO),
        &MediaAssetsControllerService::RemoveFormInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO),
        &MediaAssetsControllerService::RemoveGalleryFormInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_FORM_INFO),
        &MediaAssetsControllerService::SaveFormInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO),
        &MediaAssetsControllerService::SaveGalleryFormInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::COMMIT_EDITED_ASSET),
        &MediaAssetsControllerService::CommitEditedAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS),
        &MediaAssetsControllerService::SysTrashPhotos
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_TRASH_PHOTOS),
        &MediaAssetsControllerService::TrashPhotos
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTOS),
        &MediaAssetsControllerService::DeletePhotos
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_PHOTOS_COMPLETED),
        &MediaAssetsControllerService::DeletePhotosCompleted
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_FAVORITE),
        &MediaAssetsControllerService::AssetChangeSetFavorite
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN),
        &MediaAssetsControllerService::AssetChangeSetHidden
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_USER_COMMENT),
        &MediaAssetsControllerService::AssetChangeSetUserComment
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION),
        &MediaAssetsControllerService::AssetChangeSetLocation
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE),
        &MediaAssetsControllerService::AssetChangeSetTitle
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA),
        &MediaAssetsControllerService::AssetChangeSetEditData
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE),
        &MediaAssetsControllerService::AssetChangeSubmitCache
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET),
        &MediaAssetsControllerService::AssetChangeCreateAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE),
        &MediaAssetsControllerService::AssetChangeAddImage
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_CAMERA_SHOT_KEY),
        &MediaAssetsControllerService::SetCameraShotKey
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_CAMERA_PHOTO),
        &MediaAssetsControllerService::SaveCameraPhoto
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::DISCARD_CAMERA_PHOTO),
        &MediaAssetsControllerService::DiscardCameraPhoto
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_EFFECT_MODE),
        &MediaAssetsControllerService::SetEffectMode
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_ORIENTATION),
        &MediaAssetsControllerService::SetOrientation
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR),
        &MediaAssetsControllerService::SetVideoEnhancementAttr
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE),
        &MediaAssetsControllerService::SetSupportedWatermarkType
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HAS_APPLINK),
        &MediaAssetsControllerService::SetHasAppLink
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SET_APPLINK),
        &MediaAssetsControllerService::SetAppLink
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS),
        &MediaAssetsControllerService::GetAllDuplicateAssets
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE),
        &MediaAssetsControllerService::GetDuplicateAssetsToDelete
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET),
        &MediaAssetsControllerService::PublicCreateAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET),
        &MediaAssetsControllerService::SystemCreateAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP),
        &MediaAssetsControllerService::PublicCreateAssetForApp
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP),
        &MediaAssetsControllerService::SystemCreateAssetForApp
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE),
        &MediaAssetsControllerService::SystemCreateAssetForApp
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM),
        &MediaAssetsControllerService::CreateAssetForAppWithAlbum
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE),
        &MediaAssetsControllerService::SetAssetTitle
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING),
        &MediaAssetsControllerService::SetAssetPending
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE),
        &MediaAssetsControllerService::SetAssetsFavorite
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT),
        &MediaAssetsControllerService::SetAssetsUserComment
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN),
        &MediaAssetsControllerService::SetAssetsHiddenStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE),
        &MediaAssetsControllerService::SetAssetsFavorite
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW),
        &MediaAssetsControllerService::SetAssetsRecentShowStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT),
        &MediaAssetsControllerService::SetAssetsUserComment
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_ADD_ASSET_VISIT_COUNT),
        &MediaAssetsControllerService::AddAssetVisitCount
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ASSET),
        &MediaAssetsControllerService::SystemCreateAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CANCEL_PHOTO_URI_PERMISSION),
        &MediaAssetsControllerService::CancelPhotoUriPermissionInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHANGE_SCAN_ASSET),
        &MediaAssetsControllerService::StartChangeScanAssetInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION),
        &MediaAssetsControllerService::GrantPhotoUriPermissionInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION),
        &MediaAssetsControllerService::CheckUriPermissionInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_AUDIO_URI_PERMISSION),
        &MediaAssetsControllerService::CheckUriPermissionInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URIS_BY_OLD_URIS),
        &MediaAssetsControllerService::GetUrisByOldUrisInner
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_ASSET),
        &MediaAssetsControllerService::CloneAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::REVERT_TO_ORIGINAL),
        &MediaAssetsControllerService::RevertToOriginal
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO),
        &MediaAssetsControllerService::UpdateGalleryFormInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS),
        &MediaAssetsControllerService::SubmitCloudEnhancementTasks
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK),
        &MediaAssetsControllerService::PrioritizeCloudEnhancementTask
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS),
        &MediaAssetsControllerService::CancelCloudEnhancementTasks
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS),
        &MediaAssetsControllerService::CancelAllCloudEnhancementTasks
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA),
        &MediaAssetsControllerService::StartDownloadCloudMedia
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA),
        &MediaAssetsControllerService::PauseDownloadCloudMedia
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA),
        &MediaAssetsControllerService::CancelDownloadCloudMedia
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET),
        &MediaAssetsControllerService::RetainCloudMediaAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_RETAIN_CLOUDMEDIA_ASSET),
        &MediaAssetsControllerService::RetainCloudMediaAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION),
        &MediaAssetsControllerService::GrantPhotoUriPermission
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION),
        &MediaAssetsControllerService::GrantPhotoUrisPermission
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION),
        &MediaAssetsControllerService::CancelPhotoUriPermission
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK),
        &MediaAssetsControllerService::StartThumbnailCreationTask
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK),
        &MediaAssetsControllerService::StopThumbnailCreationTask
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED),
        &MediaAssetsControllerService::IsEdited
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA),
        &MediaAssetsControllerService::RequestEditData
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA),
        &MediaAssetsControllerService::GetEditData
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS),
        &MediaAssetsControllerService::GetCloudMediaAssetStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::START_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        &MediaAssetsControllerService::StartBatchDownloadCloudResources
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::RESUME_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        &MediaAssetsControllerService::ResumeBatchDownloadCloudResources
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        &MediaAssetsControllerService::PauseBatchDownloadCloudResources
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        &MediaAssetsControllerService::CancelBatchDownloadCloudResources
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_STATUS),
        &MediaAssetsControllerService::GetCloudMediaBatchDownloadResourcesStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_COUNT),
        &MediaAssetsControllerService::GetCloudMediaBatchDownloadResourcesCount
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT),
        &MediaAssetsControllerService::RequestContent
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR),
        &MediaAssetsControllerService::GetCloudEnhancementPair
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE),
        &MediaAssetsControllerService::QueryCloudEnhancementTaskState
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS),
        &MediaAssetsControllerService::SyncCloudEnhancementTaskStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_PHOTO_STATUS),
        &MediaAssetsControllerService::QueryPhotoStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_QUERY_PHOTO_STATUS),
        &MediaAssetsControllerService::QueryPhotoStatus
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_MOVING_PHOTO),
        &MediaAssetsControllerService::LogMovingPhoto
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_CINEMATIC_VIDEO),
        &MediaAssetsControllerService::LogCinematicvideo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT),
        &MediaAssetsControllerService::ConvertFormat
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT_MIME_TYPE),
        &MediaAssetsControllerService::CheckMimeType
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CREATE_TMP_DUPLICATE),
        &MediaAssetsControllerService::CreateTmpCompatibleDup
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB),
        &MediaAssetsControllerService::GetResultSetFromDb
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB_EXTEND),
        &MediaAssetsControllerService::GetResultSetFromDb
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS),
        &MediaAssetsControllerService::GetResultSetFromPhotosExtend
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS_EXTEND),
        &MediaAssetsControllerService::GetResultSetFromPhotosExtend
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_MOVING_PHOTO_DATE_MODIFIED),
        &MediaAssetsControllerService::GetMovingPhotoDateModified
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_FILEPATH_FROM_URI),
        &MediaAssetsControllerService::GetFilePathFromUri
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URI_FROM_FILEPATH),
        &MediaAssetsControllerService::GetUriFromFilePath
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CLOSE_ASSET),
        &MediaAssetsControllerService::CloseAsset
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE),
        &MediaAssetsControllerService::Restore
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL),
        &MediaAssetsControllerService::StopRestore
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PROCESS),
        &MediaAssetsControllerService::CancelRequest
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::HEIF_TRANSCODING_CHECK),
        &MediaAssetsControllerService::HeifTranscodingCheck
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_COMPOSITE_DISPLAY_MODE),
        &MediaAssetsControllerService::SetCompositeDisplayMode
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_IMAGE),
        &MediaAssetsControllerService::CameraInnerAddImage
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ACQUIRE_DEBUG_DATABASE),
        &MediaAssetsControllerService::AcquireDebugDatabase
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::RELEASE_DEBUG_DATABASE),
        &MediaAssetsControllerService::ReleaseDebugDatabase
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE_FOR_PICKER),
        &MediaAssetsControllerService::AssetChangeSubmitCache
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_TRASH_ASSETS),
        &MediaAssetsControllerService::SysTrashPhotos
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_DELETE_ASSETS),
        &MediaAssetsControllerService::DeletePhotos
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_FUSION_ASSET_INFO),
        &MediaAssetsControllerService::GetFusionAssetsInfo
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_DELETE_LOCAL_ASSETS_WITH_URI),
        &MediaAssetsControllerService::DeleteLocalAssetsWithUri
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_DELETE_CLOUD_ASSETS_WITH_URI),
        &MediaAssetsControllerService::DeleteCloudAssetsWithUri
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_OPEN_ASSET_COMPRESS),
        &MediaAssetsControllerService::OpenAssetCompress
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_NOTIFY_ASSET_SENDED),
        &MediaAssetsControllerService::NotifyAssetSended
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSET_COMPRESS_VERSION),
        &MediaAssetsControllerService::GetAssetCompressVersion
    },
    {
        static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_COMPRESS_ASSET_SIZE),
        &MediaAssetsControllerService::GetCompressAssetSize
    },
};

bool MediaAssetsControllerService::Accept(uint32_t code)
{
    return HANDLERS.find(code) != HANDLERS.end() || SPECIAL_HANDLERS.find(code) != SPECIAL_HANDLERS.end();
}

int32_t MediaAssetsControllerService::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    auto handlersIt = HANDLERS.find(code);
    if (handlersIt != HANDLERS.end()) {
        return (this->*(handlersIt->second))(data, reply);
    }
    auto specialHandlersIt = SPECIAL_HANDLERS.find(code);
    if (specialHandlersIt != SPECIAL_HANDLERS.end()) {
        return (this->*(specialHandlersIt->second))(data, reply, context);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, E_IPC_SEVICE_NOT_FOUND);
}

int32_t MediaAssetsControllerService::RemoveFormInfo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RemoveFormInfo");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    FormInfoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RemoveFormInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckFormIds(reqBody.formIds);
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().RemoveFormInfo(reqBody.formIds.front());
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::RemoveGalleryFormInfo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RemoveGalleryFormInfo");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    FormInfoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RemoveFormInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckFormIds(reqBody.formIds);
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().RemoveGalleryFormInfo(reqBody.formIds.front());
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SaveFormInfo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SaveFormInfo");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_FORM_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    FormInfoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SaveFormInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckFormIds(reqBody.formIds);
    FormInfoDto formInfoDto;
    formInfoDto.formIds = reqBody.formIds;
    formInfoDto.fileUris = reqBody.fileUris;
    if (formInfoDto.formIds.empty() || formInfoDto.fileUris.empty()) {
        MEDIA_ERR_LOG("formIds or fileUris is empty");
        ret = E_GET_PRAMS_FAIL;
    }
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().SaveFormInfo(formInfoDto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SaveGalleryFormInfo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SaveGalleryFormInfo");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    FormInfoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SaveGalleryFormInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckFormIds(reqBody.formIds);
    FormInfoDto formInfoDto;
    formInfoDto.formIds = reqBody.formIds;
    formInfoDto.fileUris = reqBody.fileUris;
    bool cond = (formInfoDto.formIds.size() != formInfoDto.fileUris.size()) || formInfoDto.formIds.empty()
        || formInfoDto.fileUris.empty();
    ret = cond ? E_GET_PRAMS_FAIL : ret;
    CHECK_AND_PRINT_LOG(!cond, "formIds or fileUris is empty or count not equal");
    if (formInfoDto.fileUris.size() > FACARD_MAX_REGISTER_OBSERVER) {
        MEDIA_INFO_LOG("registered uri exceeds the maximum limit, uri size:%{public}d",
            static_cast<int32_t>(formInfoDto.formIds.size()));
        formInfoDto.formIds = std::vector<std::string>(formInfoDto.formIds.begin(),
            formInfoDto.formIds.begin() + FACARD_MAX_REGISTER_OBSERVER);
        formInfoDto.fileUris = std::vector<std::string>(formInfoDto.fileUris.begin(),
            formInfoDto.fileUris.begin() + FACARD_MAX_REGISTER_OBSERVER);
    }
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().SaveGalleryFormInfo(formInfoDto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CommitEditedAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CommitEditedAsset");
    CommitEditedAssetReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CommitEditedAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    bool cond = ParameterUtils::CheckEditDataLength(reqBody.editData) && reqBody.fileId > 0;
    if (!cond) {
        MEDIA_ERR_LOG("params not valid");
        ret = E_INVALID_VALUES;
    }

    CommitEditedAssetDto commitEditedAssetDto;
    commitEditedAssetDto.editData = reqBody.editData;
    commitEditedAssetDto.fileId = reqBody.fileId;

    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().CommitEditedAsset(commitEditedAssetDto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SysTrashPhotos(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SysTrashPhotos");
    TrashPhotosReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SysTrashPhotos Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (reqBody.uris.empty()) {
        MEDIA_ERR_LOG("uris is empty");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    ret = MediaAssetsService::GetInstance().TrashPhotos(reqBody.uris);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::TrashPhotos(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter TrashPhotos");
    TrashPhotosReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("TrashPhotos Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckTrashPhotos(reqBody.uris);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("uris is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().TrashPhotos(reqBody.uris);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::DeletePhotos(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter DeletePhotos");
    DeletePhotosReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeletePhotos Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (reqBody.uris.empty()) {
        MEDIA_ERR_LOG("uris is empty");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    ret = MediaAssetsService::GetInstance().DeletePhotos(reqBody.uris);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::DeletePhotosCompleted(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter DeletePhotosCompleted");
    DeletePhotosCompletedReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeletePhotosCompleted Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckDeletePhotosCompleted(reqBody.fileIds);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("fileIds is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().DeletePhotosCompleted(reqBody.fileIds);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetFavorite(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetFavorite");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetFavorite Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("fileId is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    ret = MediaAssetsService::GetInstance().AssetChangeSetFavorite(reqBody.fileId, reqBody.favorite);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetHidden(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetHidden");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetHidden Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (!ParameterUtils::IsPhotoUri(reqBody.uri)) {
        MEDIA_ERR_LOG("uri is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    ret = MediaAssetsService::GetInstance().AssetChangeSetHidden(reqBody.uri, reqBody.hidden);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetUserComment(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetUserComment");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetUserComment Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckUserComment(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    ret = MediaAssetsService::GetInstance().AssetChangeSetUserComment(reqBody.fileId, reqBody.userComment);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetLocation(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetLocation");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetLocation Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    auto dto = SetLocationDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().AssetChangeSetLocation(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetTitle(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetTitle");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetTitle Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    ret = MediaAssetsService::GetInstance().AssetChangeSetTitle(reqBody.fileId, reqBody.title);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSetEditData(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSetEditData");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSetEditData Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.values.IsEmpty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }

    ret = MediaAssetsService::GetInstance().AssetChangeSetEditData(reqBody.values);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AssetChangeSubmitCache(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeSubmitCache");
    SubmitCacheReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSubmitCache Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.values.IsEmpty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    auto dto = SubmitCacheDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().AssetChangeSubmitCache(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeSubmitCache Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    auto respBody = dto.CreateRespBody();
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::AssetChangeCreateAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeCreateAsset");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeCreateAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.values.IsEmpty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    auto dto = AssetChangeCreateAssetDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().AssetChangeCreateAsset(dto);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeCreateAsset Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    auto respBody = dto.CreateRespBody();
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::AssetChangeAddImage(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter AssetChangeAddImage");
    AddImageReqBody reqBody;
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AssetChangeAddImage Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    auto dto = AddImageDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().AssetChangeAddImage(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CameraInnerAddImage(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CameraInnerAddImage");
    AddImageReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CameraInnerAddImage Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    auto dto = AddImageDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().CameraInnerAddImage(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GetFusionAssetsInfo(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetFusionAssetsInfo");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_FUSION_ASSET_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetFussionAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFusionAssetsInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (!PhotoAlbum::CheckPhotoAlbumType(static_cast<PhotoAlbumType>(reqBody.albumType)) ||
        static_cast<PhotoAlbumType>(reqBody.albumType) == PhotoAlbumType::SMART) {
        MEDIA_WARN_LOG("albumType: %{public}d, not support getFusionAssetsInfo", reqBody.albumType);
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    GetFussionAssetsRespBody respBody;
    ret = MediaAssetsService::GetInstance().GetFusionAssetsInfo(reqBody.albumId, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::SetCameraShotKey(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetCameraShotKey");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetCameraShotKey Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckCameraShotKey(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().SetCameraShotKey(reqBody.fileId, reqBody.cameraShotKey);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SaveCameraPhoto(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SaveCameraPhoto");
    SaveCameraPhotoReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SaveCameraPhoto Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    auto dto = SaveCameraPhotoDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().SaveCameraPhoto(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::DiscardCameraPhoto(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter DiscardCameraPhoto");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DiscardCameraPhoto Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().DiscardCameraPhoto(reqBody.fileId);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetEffectMode(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetEffectMode");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetEffectMode Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.fileId <= 0|| !MediaFileUtils::CheckMovingPhotoEffectMode(reqBody.effectMode)) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, -EINVAL);
    }
    ret = MediaAssetsService::GetInstance().SetEffectMode(reqBody.fileId, reqBody.effectMode);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetOrientation(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetOrientation");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetOrientation Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckOrientation(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().SetOrientation(reqBody.fileId, reqBody.orientation);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetVideoEnhancementAttr(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetVideoEnhancementAttr");
    AssetChangeReqBody reqBody;
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetVideoEnhancementAttr Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckVideoEnhancementAttr(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().SetVideoEnhancementAttr(reqBody.fileId, reqBody.photoId, reqBody.path);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetSupportedWatermarkType(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetSupportedWatermarkType");
    AssetChangeReqBody reqBody;
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetSupportedWatermarkType Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWatermarkType(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().SetSupportedWatermarkType(reqBody.fileId, reqBody.watermarkType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetHasAppLink(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetHasAppLink");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetHasAppLink Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetHasAppLink(reqBody.fileId, reqBody.hasAppLink);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAppLink(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetAppLink");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAppLink Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAppLink(reqBody.fileId, reqBody.appLink);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetCompositeDisplayMode(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SetCompositeDisplayMode");
    AssetChangeReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetCompositeDisplayMode Read Request Error, errCode:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckCompositeDisplayMode(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().SetCompositeDisplayMode(reqBody.fileId, reqBody.compositeDisplayMode);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

static void BuildDoubleCheckPredicates(DataShare::DataSharePredicates &predicates,
    std::vector<std::string> &columns, int64_t tokenId, bool ownerCheck)
{
    string clause = AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "." + AppUriPermissionColumn::FILE_ID +
            " = " + PhotoColumn::PHOTOS_TABLE + "." + AppUriPermissionColumn::FILE_ID;
    predicates.InnerJoin(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE)->On({clause});
    if (ownerCheck) {
        predicates.EqualTo(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "." +
            AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_PERSIST_READ_WRITE);
    } else {
        predicates.In(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "." +
            AppUriPermissionColumn::PERMISSION_TYPE, AppUriPermissionColumn::PERMISSION_TYPES_READ_STR);
    }
    predicates.EqualTo(AppUriPermissionColumn::APP_URI_PERMISSION_TABLE + "." +
        AppUriPermissionColumn::TARGET_TOKENID, tokenId);
    MediaLibraryRdbUtils::CleanAmbiguousColumn(columns, predicates, PhotoColumn::PHOTOS_TABLE);
}

int32_t MediaAssetsControllerService::GetAssets(
    MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    GetAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetAssetsDto dto = GetAssetsDto::Create(reqBody);
    if (context.GetByPassCode() == E_PERMISSION_DB_BYPASS) {
        int64_t tokenId = static_cast<int64_t>(PermissionUtils::GetTokenId());
        if (tokenId == 0) {
            MEDIA_ERR_LOG("Get tokenId fail");
            return IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
        }
        BuildDoubleCheckPredicates(dto.predicates, dto.columns, tokenId, true);
    } else if (context.GetByPassCode() == E_DOUBLE_CHECK) {
        int64_t tokenId = static_cast<int64_t>(PermissionUtils::GetTokenId());
        if (tokenId == 0) {
            MEDIA_ERR_LOG("Get tokenId fail");
            return IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
        }
        BuildDoubleCheckPredicates(dto.predicates, dto.columns, tokenId, false);
    }

    auto resultSet = MediaAssetsService::GetInstance().GetAssets(dto);
    MEDIA_DEBUG_LOG("GetAssets finish");
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    GetAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::GetBurstAssets(
    MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context)
{
    MEDIA_INFO_LOG("enter");
    GetAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK || reqBody.burstKey.empty()) {
        MEDIA_ERR_LOG("CheckWhereClause fialed or burstKey is empty");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetAssetsDto dto = GetAssetsDto::Create(reqBody);
    if (context.GetByPassCode() == E_PERMISSION_DB_BYPASS) {
        int64_t tokenId = static_cast<int64_t>(PermissionUtils::GetTokenId());
        if (tokenId == 0) {
            MEDIA_ERR_LOG("Get tokenId fail");
            return IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
        }
        BuildDoubleCheckPredicates(dto.predicates, dto.columns, tokenId, true);
    } else if (context.GetByPassCode() == E_DOUBLE_CHECK) {
        int64_t tokenId = static_cast<int64_t>(PermissionUtils::GetTokenId());
        if (tokenId == 0) {
            MEDIA_ERR_LOG("Get tokenId fail");
            return IPC::UserDefineIPC().WriteResponseBody(reply, Media::E_PERMISSION_DENIED);
        }
        BuildDoubleCheckPredicates(dto.predicates, dto.columns, tokenId, false);
    }
    dto.predicates.OrderByAsc(MediaColumn::MEDIA_NAME);
    auto resultSet = MediaAssetsService::GetInstance().GetAssets(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    GetAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::GetAllDuplicateAssets(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetAssetsDto dto = GetAssetsDto::Create(reqBody);
    auto resultSet = MediaAssetsService::GetInstance().GetAllDuplicateAssets(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    GetAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::GetDuplicateAssetsToDelete(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckWhereClause(reqBody.predicates.GetWhereClause());
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckWhereClause fialed");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GetAssetsDto dto = GetAssetsDto::Create(reqBody);
    auto resultSet = MediaAssetsService::GetInstance().GetDuplicateAssetsToDelete(dto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("resultSet is null");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_FAIL);
    }
    GetAssetsRespBody respBody;
    respBody.resultSet = resultSet;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::PublicCreateAsset(MessageParcel &data, MessageParcel &reply)
{
    CreateAssetReqBody reqBody;
    CreateAssetRespBody respBody;
    MEDIA_INFO_LOG("enter PublicCreateAsset");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PublicCreateAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    ret = ParameterUtils::CheckPublicCreateAsset(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckPublicCreateAsset ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CreateAssetDto dto(reqBody);
    ret = MediaAssetsService::GetInstance().CreateAsset(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, dto.GetRespBody(), ret);
}

int32_t MediaAssetsControllerService::SystemCreateAsset(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CreateAssetReqBody reqBody;
    CreateAssetRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SystemCreateAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    ret = ParameterUtils::CheckSystemCreateAsset(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSystemCreateAsset ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CreateAssetDto dto(reqBody);
    (int32_t)ParameterUtils::GetTitleAndExtension(dto.displayName, dto.title, dto.extension);
    ret = MediaAssetsService::GetInstance().CreateAsset(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, dto.GetRespBody(), ret);
}

int32_t MediaAssetsControllerService::PublicCreateAssetForApp(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CreateAssetForAppReqBody reqBody;
    CreateAssetForAppRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAssetForApp Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    ret = ParameterUtils::CheckPublicCreateAssetForApp(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckPublicCreateAssetForApp ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CreateAssetDto dto(reqBody);
    ret = MediaAssetsService::GetInstance().CreateAssetForApp(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, dto.GetRespBody(), ret);
}

int32_t MediaAssetsControllerService::SystemCreateAssetForApp(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CreateAssetForAppReqBody reqBody;
    CreateAssetForAppRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAssetForApp Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    ret = ParameterUtils::CheckSystemCreateAssetForApp(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSystemCreateAssetForApp ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CreateAssetDto dto(reqBody);
    ret = MediaAssetsService::GetInstance().CreateAssetForApp(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, dto.GetRespBody(), ret);
}

int32_t MediaAssetsControllerService::CreateAssetForAppWithAlbum(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(
        MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CreateAssetForAppReqBody reqBody;
    CreateAssetForAppRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateAssetForAppWithAlbum Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    ret = ParameterUtils::CheckCreateAssetForAppWithAlbum(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckCreateAssetForAppWithAlbum ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }

    CreateAssetDto dto(reqBody);
    ret = MediaAssetsService::GetInstance().CreateAssetForAppWithAlbum(dto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, dto.GetRespBody(), ret);
}

int32_t MediaAssetsControllerService::SetAssetTitle(MessageParcel &data, MessageParcel &reply)
{
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetTitle Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetTitle(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetTitle ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetTitle(reqBody.fileIds.front(), reqBody.title);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAssetPending(MessageParcel &data, MessageParcel &reply)
{
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetPending Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetPending(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetPending ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetPending(reqBody.fileIds.front(), reqBody.pending);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAssetsFavorite(MessageParcel &data, MessageParcel &reply)
{
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetsFavorite Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetsFavorite(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetsFavorite ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetsFavorite(reqBody.fileIds, reqBody.favorite);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAssetsHiddenStatus(MessageParcel &data, MessageParcel &reply)
{
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetsHiddenStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetsHiddenStatus(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetsHiddenStatus ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetsHiddenStatus(reqBody.fileIds, reqBody.hiddenStatus);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAssetsRecentShowStatus(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetsRecentShowStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetsRecentShowStatus(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetsRecentShowStatus ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetsRecentShowStatus(reqBody.fileIds, reqBody.recentShowStatus);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SetAssetsUserComment(MessageParcel &data, MessageParcel &reply)
{
    ModifyAssetsReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetsUserComment Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckSetAssetsUserComment(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckSetAssetsUserComment ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().SetAssetsUserComment(reqBody.fileIds, reqBody.userComment);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::AddAssetVisitCount(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_ADD_ASSET_VISIT_COUNT);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AddAssetVisitCountReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SetAssetsUserComment Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckAddAssetVisitCount(reqBody.fileId, reqBody.visitType);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckAddAssetVisitCount ret:%{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().AddAssetVisitCount(reqBody.fileId, reqBody.visitType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CloneAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloneAsset");
    CloneAssetReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloneAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    string extension = MediaFileUtils::SplitByChar(reqBody.displayName, '.');
    string displayName = reqBody.title + "." + extension;
    if (MediaFileUtils::CheckDisplayName(displayName, true) != E_OK) {
        MEDIA_ERR_LOG("CloneAsset invalid params");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    CloneAssetDto cloneAssetDto;
    cloneAssetDto.fileId = reqBody.fileId;
    cloneAssetDto.title = reqBody.title;
    int32_t newAssetId = MediaAssetsService::GetInstance().CloneAsset(cloneAssetDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, newAssetId);
}

int32_t MediaAssetsControllerService::ConvertFormat(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter ConvertFormat");
    ConvertFormatReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ConvertFormat Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ConvertFormatDto convertFormatDto = ConvertFormatDto::Create(reqBody);
    auto resultSet = MediaAssetsService::GetInstance().ConvertFormat(convertFormatDto);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("Failed to convertFormat.");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_PARAM_CONVERT_FORMAT);
    }
    ConvertFormatRespBody respBody;
    respBody.resultSet = move(resultSet);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::CheckMimeType(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CheckMimeType");
    MimeTypeReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckMimeType Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    auto result = MediaAssetsService::GetInstance().CheckMimeType(reqBody.fileId);
    MimeTypeRespBody respBody;
    respBody.result = result;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::CreateTmpCompatibleDup(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CreateTmpCompatibleDup");
    CreateTmpCompatibleDupReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CreateTmpCompatibleDup Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    CreateTmpCompatibleDupDto createTmpCompatibleDupDto;
    createTmpCompatibleDupDto.fileId = reqBody.fileId;
    createTmpCompatibleDupDto.path = reqBody.path;

    ret = MediaAssetsService::GetInstance().CreateTmpCompatibleDup(createTmpCompatibleDupDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::RevertToOriginal(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RevertToOriginal");
    RevertToOriginalReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RevertToOriginal Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    RevertToOriginalDto revertToOriginalDto;
    revertToOriginalDto.fileId = reqBody.fileId;
    revertToOriginalDto.fileUri = reqBody.fileUri;

    ret = MediaAssetsService::GetInstance().RevertToOriginal(revertToOriginalDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::UpdateGalleryFormInfo(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    MEDIA_INFO_LOG("enter UpdateGalleryFormInfo");
    FormInfoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("UpdateGalleryFormInfo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckFormIds(reqBody.formIds);
    FormInfoDto formInfoDto;
    formInfoDto.formIds = reqBody.formIds;
    formInfoDto.fileUris = reqBody.fileUris;
    bool cond = (formInfoDto.formIds.size() != formInfoDto.fileUris.size()) || formInfoDto.formIds.empty()
        || formInfoDto.fileUris.empty();
    if (cond) {
        MEDIA_ERR_LOG("formIds or fileUris is empty or count not equal");
        ret = E_GET_PRAMS_FAIL;
    }
    if (formInfoDto.formIds.size() > FACARD_MAX_REGISTER_OBSERVER &&
        formInfoDto.fileUris.size() > FACARD_MAX_REGISTER_OBSERVER) {
        MEDIA_INFO_LOG("registered uri exceeds the maximum limit, uri size:%{public}d",
            static_cast<int32_t>(formInfoDto.formIds.size()));
        formInfoDto.formIds = std::vector<std::string>(formInfoDto.formIds.begin(),
            formInfoDto.formIds.begin() + FACARD_MAX_REGISTER_OBSERVER);
        formInfoDto.fileUris = std::vector<std::string>(formInfoDto.fileUris.begin(),
            formInfoDto.fileUris.begin() + FACARD_MAX_REGISTER_OBSERVER);
    }
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().RemoveGalleryFormInfo(formInfoDto.formIds.front());
        ret = MediaAssetsService::GetInstance().SaveGalleryFormInfo(formInfoDto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::SubmitCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SubmitCloudEnhancementTasks");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CloudEnhancementReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SubmitCloudEnhancementTasks Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    bool isValid = false;
    for (auto uri : reqBody.fileUris) {
        isValid = ParameterUtils::IsPhotoUri(uri);
    }
    if (!isValid) {
        MEDIA_ERR_LOG("fileUris in params is invalid");
        ret = E_GET_PRAMS_FAIL;
    }
    CloudEnhancementDto dto;
    dto.hasCloudWatermark = reqBody.hasCloudWatermark;
    dto.triggerMode = reqBody.triggerMode;
    dto.fileUris = reqBody.fileUris;
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().SubmitCloudEnhancementTasks(dto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::PrioritizeCloudEnhancementTask(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter PrioritizeCloudEnhancementTask");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CloudEnhancementReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PrioritizeCloudEnhancementTask Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    bool isValid = false;
    for (auto uri : reqBody.fileUris) {
        isValid = ParameterUtils::IsPhotoUri(uri);
    }
    if (!isValid) {
        MEDIA_ERR_LOG("fileUris in params is invalid");
        ret = E_GET_PRAMS_FAIL;
    }
    CloudEnhancementDto dto;
    dto.fileUris = reqBody.fileUris;
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().PrioritizeCloudEnhancementTask(dto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GrantPhotoUriPermissionInner(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GrantPhotoUriPermissionInner");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GrantUrisPermissionInnerReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GrantPhotoUriPermissionInner Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GrantUriPermissionInnerDto grantUriPermInnerDto;
    grantUriPermInnerDto.tokenId = reqBody.tokenId;
    grantUriPermInnerDto.srcTokenId = reqBody.srcTokenId;
    grantUriPermInnerDto.appId = reqBody.appId;
    grantUriPermInnerDto.fileIds = reqBody.fileIds;
    grantUriPermInnerDto.uriTypes = reqBody.uriTypes;
    grantUriPermInnerDto.permissionTypes = reqBody.permissionTypes;
    grantUriPermInnerDto.hideSensitiveType = reqBody.hideSensitiveType;

    ret = MediaAssetsService::GetInstance().GrantPhotoUriPermissionInner(grantUriPermInnerDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CancelCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CancelCloudEnhancementTasks");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CloudEnhancementReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelCloudEnhancementTasks Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    bool isValid = false;
    for (auto uri : reqBody.fileUris) {
        isValid = ParameterUtils::IsPhotoUri(uri);
    }
    if (!isValid) {
        MEDIA_ERR_LOG("fileUris in params is invalid");
        ret = E_GET_PRAMS_FAIL;
    }
    CloudEnhancementDto dto;
    dto.fileUris = reqBody.fileUris;
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().CancelCloudEnhancementTasks(dto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CancelAllCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CancelAllCloudEnhancementTasks");
    CloudEnhancementReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelAllCloudEnhancementTasks Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().CancelAllCloudEnhancementTasks();
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::StartDownloadCloudMedia(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter StartDownloadCloudMedia");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StartDownloadCloudMediaReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartDownloadCloudMedia Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    CloudMediaDownloadType downloadType = static_cast<CloudMediaDownloadType>(reqBody.cloudMediaType);

    if (downloadType != CloudMediaDownloadType::DOWNLOAD_FORCE &&
            downloadType != CloudMediaDownloadType::DOWNLOAD_GENTLE) {
        ret = E_INVALID_VALUES;
        MEDIA_ERR_LOG("StartDownloadCloudMedia error, err param");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().StartDownloadCloudMedia(downloadType);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::PauseDownloadCloudMedia(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter PauseDownloadCloudMedia");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = MediaAssetsService::GetInstance().PauseDownloadCloudMedia();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CancelDownloadCloudMedia(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CancelDownloadCloudMedia");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    int32_t ret = MediaAssetsService::GetInstance().CancelDownloadCloudMedia();
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::RetainCloudMediaAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RetainCloudMediaAsset");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    RetainCloudMediaAssetReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RetainCloudMediaAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    auto cloudMediaRetainType = static_cast<CloudMediaRetainType>(reqBody.cloudMediaRetainType);
    if (cloudMediaRetainType == CloudMediaRetainType::RETAIN_FORCE ||
        cloudMediaRetainType == CloudMediaRetainType::HDC_RETAIN_FORCE) {
        ret = MediaAssetsService::GetInstance().RetainCloudMediaAsset(cloudMediaRetainType);
    } else {
        ret = E_INVALID_VALUES;
        MEDIA_ERR_LOG("RetainCloudMediaAsset error, err type: %{public}d", reqBody.cloudMediaRetainType);
    }
    MEDIA_INFO_LOG("RetainCloudMediaAsset end, ret:%{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GrantPhotoUriPermission(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GrantPhotoUriPermission");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GrantUriPermissionReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GrantPhotoUriPermission Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GrantUriPermissionDto grantUriPermDto;
    grantUriPermDto.tokenId = reqBody.tokenId;
    grantUriPermDto.srcTokenId = reqBody.srcTokenId;
    grantUriPermDto.fileId = reqBody.fileId;
    grantUriPermDto.permissionType = reqBody.permissionType;
    grantUriPermDto.hideSensitiveType = reqBody.hideSensitiveType;
    grantUriPermDto.uriType = reqBody.uriType;

    ret = MediaAssetsService::GetInstance().GrantPhotoUriPermission(grantUriPermDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GrantPhotoUrisPermission(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GrantPhotoUrisPermission");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GrantUrisPermissionReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GrantPhotoUrisPermission Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    GrantUrisPermissionDto grantUrisPermDto;
    grantUrisPermDto.tokenId = reqBody.tokenId;
    grantUrisPermDto.srcTokenId = reqBody.srcTokenId;
    grantUrisPermDto.fileIds.assign(reqBody.fileIds.begin(), reqBody.fileIds.end());
    grantUrisPermDto.permissionType = reqBody.permissionType;
    grantUrisPermDto.hideSensitiveType = reqBody.hideSensitiveType;
    grantUrisPermDto.uriType = reqBody.uriType;

    ret = MediaAssetsService::GetInstance().GrantPhotoUrisPermission(grantUrisPermDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CheckUriPermissionInner(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CheckUriPermissionInnerReqBody reqBody;
    CheckUriPermissionInnerRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CheckUriPermissionInner Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }
    CheckUriPermissionInnerDto dto;
    reqBody.Convert2Dto(dto);
    ret = MediaAssetsService::GetInstance().CheckPhotoUriPermissionInner(dto);
    if (ret == E_OK) {
        respBody.InitByDto(dto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::CancelPhotoUriPermission(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GrantPhotoUriPermission");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CancelUriPermissionReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelPhotoUriPermission Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    CancelUriPermissionDto cancelUriPermDto;
    cancelUriPermDto.tokenId = reqBody.tokenId;
    cancelUriPermDto.srcTokenId = reqBody.srcTokenId;
    cancelUriPermDto.fileId = reqBody.fileId;
    cancelUriPermDto.permissionType = reqBody.permissionType;

    cancelUriPermDto.uriType = reqBody.uriType;

    ret = MediaAssetsService::GetInstance().CancelPhotoUriPermission(cancelUriPermDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CancelPhotoUriPermissionInner(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CancelPhotoUriPermissionInner");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CancelUriPermissionInnerReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelPhotoUriPermissionInner Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    CancelUriPermissionInnerDto cancelUriPermInnerDto;
    cancelUriPermInnerDto.targetTokenId = reqBody.targetTokenId;
    cancelUriPermInnerDto.srcTokenId = reqBody.srcTokenId;
    cancelUriPermInnerDto.fileIds = reqBody.fileIds;
    cancelUriPermInnerDto.uriTypes = reqBody.uriTypes;
    cancelUriPermInnerDto.permissionTypes = reqBody.permissionTypes;
    ret = MediaAssetsService::GetInstance().CancelPhotoUriPermissionInner(cancelUriPermInnerDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::StartChangeScanAssetInner(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter StartChangeScanAssetInner");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHANGE_SCAN_ASSET);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StartAssetChangeScanReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartChangeScanAssetInner Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    StartAssetChangeScanDto startAssetChangeScanDto;
    startAssetChangeScanDto.operation = reqBody.operation;
    ret = MediaAssetsService::GetInstance().StartAssetChangeScanInner(startAssetChangeScanDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::StartThumbnailCreationTask(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter StartThumbnailCreationTask");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StartThumbnailCreationTaskReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartThumbnailCreationTask Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    StartThumbnailCreationTaskDto startCreationTaskDto;
    startCreationTaskDto.predicates = reqBody.predicates;
    startCreationTaskDto.requestId = reqBody.requestId;
    startCreationTaskDto.pid = reqBody.pid;
    ret = MediaAssetsService::GetInstance().StartThumbnailCreationTask(startCreationTaskDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::StopThumbnailCreationTask(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter StopThumbnailCreationTask");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StopThumbnailCreationTaskReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StopThumbnailCreationTask Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    StopThumbnailCreationTaskDto stopCreationTaskDto;
    stopCreationTaskDto.requestId = reqBody.requestId;
    stopCreationTaskDto.pid = reqBody.pid;
    ret = MediaAssetsService::GetInstance().StopThumbnailCreationTask(stopCreationTaskDto);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::IsEdited(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter IsEdited");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    IsEditedReqBody reqBody;
    IsEditedRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("IsEdited Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    IsEditedDto dto;
    dto.fileId = reqBody.fileId;
    ret = MediaAssetsService::GetInstance().IsEdited(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::RequestEditData(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RequestEditData");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    RequestEditDataReqBody reqBody;
    RequestEditDataRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RequestEditData Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    RequestEditDataDto dto;
    dto.predicates = reqBody.predicates;
    ret = MediaAssetsService::GetInstance().RequestEditData(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetEditData(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetEditData");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetEditDataReqBody reqBody;
    GetEditDataRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetEditData Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetEditDataDto dto;
    dto.predicates = reqBody.predicates;
    ret = MediaAssetsService::GetInstance().GetEditData(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetCloudMediaAssetStatus(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetCloudMediaAssetStatus");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetCloudMediaAssetStatusReqBody reqBody;
    GetCloudMediaAssetStatusReqBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCloudMediaAssetStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().GetCloudMediaAssetStatus(respBody.status);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::StartBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService StartBatchDownloadCloudResources");
    StartBatchDownloadCloudResourcesReqBody reqBody;
    StartBatchDownloadCloudResourcesRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StartBatchDownloadCloudResources Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().StartBatchDownloadCloudResources(reqBody, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::ResumeBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService ResumeBatchDownloadCloudResources");
    ResumeBatchDownloadCloudResourcesReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ResumeBatchDownloadCloudResources Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().ResumeBatchDownloadCloudResources(reqBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::PauseBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService PauseBatchDownloadCloudResources");
    PauseBatchDownloadCloudResourcesReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("PauseBatchDownloadCloudResources Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().PauseBatchDownloadCloudResources(reqBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::CancelBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService CancelBatchDownloadCloudResources");
    CancelBatchDownloadCloudResourcesReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelBatchDownloadCloudResources Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().CancelBatchDownloadCloudResources(reqBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::GetCloudMediaBatchDownloadResourcesStatus(MessageParcel &data,
    MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService GetCloudMediaBatchDownloadResourcesStatus");
    GetBatchDownloadCloudResourcesStatusReqBody reqBody;
    GetBatchDownloadCloudResourcesStatusRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCloudMediaBatchDownloadResourcesStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().GetCloudMediaBatchDownloadResourcesStatus(reqBody, respBody);
    MEDIA_INFO_LOG("MediaAssetsControllerService GetCloudMediaBatchDownloadResourcesStatus"
        "size: %{public}zu", respBody.downloadResourcesStatus.size());
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::GetCloudMediaBatchDownloadResourcesCount(MessageParcel &data,
    MessageParcel &reply)
{
#ifdef MEDIALIBRARY_FEATURE_CLOUD_DOWNLOAD
    MEDIA_INFO_LOG("enter MediaAssetsControllerService GetCloudMediaBatchDownloadResourcesCount");
    GetBatchDownloadCloudResourcesCountReqBody reqBody;
    GetBatchDownloadCloudResourcesCountRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCloudMediaBatchDownloadResourcesCount Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().GetCloudMediaBatchDownloadResourcesCount(reqBody, respBody);
    MEDIA_INFO_LOG("MediaAssetsControllerService GetCloudMediaBatchDownloadResourcesCount count: %{public}d",
        respBody.count);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
#else
    return 0;
#endif
}

int32_t MediaAssetsControllerService::RequestContent(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter RequestContent");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    RequestContentReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("RequestContent Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    int32_t position = 0;
    RequestContentRespBody respBody;

    ret = MediaAssetsService::GetInstance().RequestContent(reqBody.mediaId, position);
    respBody.position = position;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetCloudEnhancementPair(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetCloudEnhancementPair");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetCloudEnhancementPairReqBody reqBody;
    GetCloudEnhancementPairRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCloudEnhancementPair Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetCloudEnhancementPairDto dto;
    dto.photoUri = reqBody.photoUri;
    ret = MediaAssetsService::GetInstance().GetCloudEnhancementPair(dto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::QueryCloudEnhancementTaskState(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter QueryCloudEnhancementTaskState");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    QueryCloudEnhancementTaskStateReqBody reqBody;
    QueryCloudEnhancementTaskStateRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryCloudEnhancementTaskState Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    QueryCloudEnhancementTaskStateDto dto;
    ret = MediaAssetsService::GetInstance().QueryCloudEnhancementTaskState(reqBody.photoUri, dto);
    respBody.fileId = dto.fileId;
    respBody.photoId = dto.photoId;
    respBody.ceAvailable = dto.ceAvailable;
    respBody.ceErrorCode = dto.ceErrorCode;
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::SyncCloudEnhancementTaskStatus(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter SyncCloudEnhancementTaskStatus");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CloudEnhancementReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("SyncCloudEnhancementTaskStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    if (ret == E_OK) {
        ret = MediaAssetsService::GetInstance().SyncCloudEnhancementTaskStatus();
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::QueryPhotoStatus(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_PHOTO_STATUS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    QueryPhotoReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("QueryPhotoStatus Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    QueryPhotoRespBody respBody;
    ret = MediaAssetsService::GetInstance().QueryPhotoStatus(reqBody, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::LogMovingPhoto(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_MOVING_PHOTO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AdaptedReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("LogMovingPhoto Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().LogMovingPhoto(reqBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::LogCinematicvideo(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_CINEMATIC_VIDEO);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    CinematicVideoAccessReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("LogCinematicVideo Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().LogCinematicVideo(reqBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GetResultSetFromDb(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetResultSetFromDb");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetResultSetFromDbReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetResultSetFromDb Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetResultSetFromDbDto getResultSetFromDbDto;
    getResultSetFromDbDto.columnName = reqBody.columnName;
    getResultSetFromDbDto.value = reqBody.value;
    getResultSetFromDbDto.columns = reqBody.columns;
    GetResultSetFromDbRespBody respBody;
    ret = MediaAssetsService::GetInstance().GetResultSetFromDb(getResultSetFromDbDto, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GetResultSetFromPhotosExtend(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetResultSetFromPhotosExtend");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetResultSetFromPhotosExtendReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetResultSetFromPhotosExtend Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (!ParameterUtils::CheckPhotoUri(reqBody.value)) {
        MEDIA_ERR_LOG("Failed to check invalid uri: %{public}s", reqBody.value.c_str());
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }

    GetResultSetFromPhotosExtendRespBody respBody;
    ret = MediaAssetsService::GetInstance().GetResultSetFromPhotosExtend(reqBody.value, reqBody.columns, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetMovingPhotoDateModified(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetMovingPhotoDateModified");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_MOVING_PHOTO_DATE_MODIFIED);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetMovingPhotoDateModifiedReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetMovingPhotoDateModified Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    GetMovingPhotoDateModifiedRespBody respBody;
    ret = MediaAssetsService::GetInstance().GetMovingPhotoDateModified(reqBody.fileId, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
int32_t MediaAssetsControllerService::GetFilePathFromUri(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetFilePathFromUri");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_FILEPATH_FROM_URI);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetFilePathFromUriReqBody reqBody;
    GetFilePathFromUriRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetFilePathFromUri Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().GetFilePathFromUri(reqBody.virtualId, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetUriFromFilePath(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter GetUriFromFilePath");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URI_FROM_FILEPATH);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetUriFromFilePathReqBody reqBody;
    GetUriFromFilePathRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetUriFromFilePath Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().GetUriFromFilePath(reqBody.tempPath, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::CloseAsset(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter CloseAsset");
    CloseAssetReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CloseAsset Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().CloseAsset(reqBody);
    MEDIA_INFO_LOG("CloseAsset ret=%{public}d", ret);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GetUrisByOldUrisInner(MessageParcel &data, MessageParcel &reply)
{
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URIS_BY_OLD_URIS);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    GetUrisByOldUrisInnerReqBody reqBody;
    GetUrisByOldUrisInnerRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetUrisByOldUrisInner Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
    }
    GetUrisByOldUrisInnerDto dto;
    reqBody.Convert2Dto(dto);
    ret = MediaAssetsService::GetInstance().GetUrisByOldUrisInner(dto);
    if (ret == E_OK) {
        respBody.InitByDto(dto);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::Restore(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter Restore");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    RestoreReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("Restore Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = ParameterUtils::CheckRestore(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    auto dto = RestoreDto::Create(reqBody);
    ret = MediaAssetsService::GetInstance().Restore(dto);
    CHECK_AND_PRINT_LOG(ret == E_OK, "Restore failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::StopRestore(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter StopRestore");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    StopRestoreReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("StopRestore Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    if (reqBody.keyPath.empty()) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, E_INVALID_VALUES);
    }
    ret = MediaAssetsService::GetInstance().StopRestore(reqBody.keyPath);
    CHECK_AND_PRINT_LOG(ret == E_OK, "StopRestore failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::CancelRequest(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("Enter CancelRequest");
    CancelRequestReqBody reqBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("CancelRequest Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckCancelRequest(reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("params is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().CancelRequest(reqBody.photoId, reqBody.mediaType);
    CHECK_AND_PRINT_LOG(ret == E_OK, "CancelRequest failed");
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::HeifTranscodingCheck(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("enter HeifTranscodingCheck");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::HEIF_TRANSCODING_CHECK);
    HeifTranscodingCheckReqBody reqBody;
    HeifTranscodingCheckRespBody respBody;

    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("HeifTranscodingCheck Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().CanSupportedCompatibleDuplicate(reqBody.bundleName, respBody);
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::AcquireDebugDatabase(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::AcquireDebugDatabase start");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::ACQUIRE_DEBUG_DATABASE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    AcquireDebugDatabaseReqBody reqBody;
    AcquireDebugDatabaseRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("AcquireDebugDatabase ReadRequestBody Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().AcquireDebugDatabase(reqBody.betaIssueId, reqBody.betaScenario, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::AcquireDebugDatabase fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody);
}

int32_t MediaAssetsControllerService::ReleaseDebugDatabase(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::ReleaseDebugDatabase start");
    uint32_t operationCode = static_cast<uint32_t>(MediaLibraryBusinessCode::RELEASE_DEBUG_DATABASE);
    int64_t timeout = DfxTimer::GetOperationCodeTimeout(operationCode);
    DfxTimer dfxTimer(operationCode, timeout, true);
    ReleaseDebugDatabaseReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("ReleaseDebugDatabase ReadRequestBody Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().ReleaseDebugDatabase(reqBody.betaIssueId);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::ReleaseDebugDatabase fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::DeleteLocalAssetsWithUri(MessageParcel &data, MessageParcel &reply)
{
    DeletePhotosCompletedReqBody reqBody;
 
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeleteLocalAssetsWithUri Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckDeletePhotosCompleted(reqBody.fileIds);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("fileIds is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->mediaAssetsDeleteService_.DeleteLocalAssets(reqBody.fileIds);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}
 
int32_t MediaAssetsControllerService::DeleteCloudAssetsWithUri(MessageParcel &data, MessageParcel &reply)
{
    DeletePhotosCompletedReqBody reqBody;
 
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("DeleteCloudAssetsWithUri Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = ParameterUtils::CheckDeletePhotosCompleted(reqBody.fileIds);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("fileIds is invalid");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = this->mediaAssetsDeleteService_.DeleteCloudAssets(reqBody.fileIds);
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::OpenAssetCompress(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::OpenAssetCompress start");
    OpenAssetCompressReqBody reqBody;
    OpenAssetCompressRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("OpenAssetCompress Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    OpenAssetCompressDto dto;
    reqBody.Convert2Dto(dto);
    ret = MediaAssetsService::GetInstance().OpenAssetCompress(dto, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::OpenAssetCompress fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::NotifyAssetSended(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::NotifyAssetSended start");
    NotifyAssetSendedReqBody reqBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("NotifyAssetSended Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    ret = MediaAssetsService::GetInstance().NotifyAssetSended(reqBody.uri);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::NotifyAssetSended fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
}

int32_t MediaAssetsControllerService::GetAssetCompressVersion(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::GetAssetCompressVersion start");
    GetAssetCompressVersionRespBody respBody;
    int32_t ret = MediaAssetsService::GetInstance().GetAssetCompressVersion(respBody.version);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::GetAssetCompressVersion fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}

int32_t MediaAssetsControllerService::GetCompressAssetSize(MessageParcel &data, MessageParcel &reply)
{
    MEDIA_INFO_LOG("MediaAssetsControllerService::GetCompressAssetSize start");
    GetCompressAssetSizeReqBody reqBody;
    GetCompressAssetSizeRespBody respBody;
    int32_t ret = IPC::UserDefineIPC().ReadRequestBody(data, reqBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("GetCompressAssetSize Read Request Error");
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }

    ret = MediaAssetsService::GetInstance().GetCompressAssetSize(reqBody.uris, respBody);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("MediaAssetsControllerService::GetCompressAssetSize fail, ret: %{public}d", ret);
        return IPC::UserDefineIPC().WriteResponseBody(reply, ret);
    }
    return IPC::UserDefineIPC().WriteResponseBody(reply, respBody, ret);
}
} // namespace OHOS::Media
