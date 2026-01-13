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

#ifndef OHOS_MEDIA_ASSETS_SERVICE_H
#define OHOS_MEDIA_ASSETS_SERVICE_H

#include <stdint.h>
#include <string>

#include "media_assets_rdb_operations.h"
#include "form_info_dto.h"
#include "commit_edited_asset_dto.h"
#include "create_asset_dto.h"
#include "get_asset_analysis_data_dto.h"
#include "clone_asset_dto.h"
#include "revert_to_original_dto.h"
#include "cloud_enhancement_dto.h"
#include "grant_photo_uri_permission_dto.h"
#include "grant_photo_uris_permission_dto.h"
#include "grant_photo_uri_permission_inner_dto.h"
#include "cancel_photo_uri_permission_dto.h"
#include "cancel_photo_uri_permission_inner_dto.h"
#include "start_thumbnail_creation_task_dto.h"
#include "stop_thumbnail_creation_task_dto.h"
#include "set_location_dto.h"
#include "submit_cache_dto.h"
#include "asset_change_create_asset_dto.h"
#include "add_image_dto.h"
#include "save_camera_photo_dto.h"
#include "get_assets_dto.h"
#include "query_cloud_enhancement_task_state_dto.h"
#include "query_photo_vo.h"
#include "adapted_vo.h"
#include "log_cinematic_access_vo.h"
#include "convert_format_dto.h"
#include "create_tmp_compatible_dup_dto.h"
#include "check_photo_uri_permission_inner_dto.h"
#include "get_result_set_from_db_vo.h"
#include "get_result_set_from_db_dto.h"
#include "get_result_set_from_photos_extend_vo.h"
#include "get_moving_photo_date_modified_vo.h"
#include "close_asset_vo.h"
#include "get_uris_by_old_uris_inner_dto.h"
#include "restore_dto.h"
#include "cloud_media_asset_types.h"
#include "is_edited_dto.h"
#include "is_edited_vo.h"
#include "request_edit_data_dto.h"
#include "request_edit_data_vo.h"
#include "get_edit_data_dto.h"
#include "get_edit_data_vo.h"
#include "start_asset_change_scan_dto.h"
#include "get_cloud_enhancement_pair_dto.h"
#include "get_cloud_enhancement_pair_vo.h"
#include "get_cloud_enhancement_pair_dto.h"
#include "get_filepath_from_uri_vo.h"
#include "get_uri_from_filepath_vo.h"
#include "heif_transcoding_check_vo.h"
#include "start_batch_download_cloud_resources_vo.h"
#include "resume_batch_download_cloud_resources_vo.h"
#include "pause_batch_download_cloud_resources_vo.h"
#include "cancel_batch_download_cloud_resources_vo.h"
#include "get_batch_download_cloud_resources_status_vo.h"
#include "get_batch_download_cloud_resources_count_vo.h"
#include "acquire_debug_database_vo.h"
#include "get_fussion_assets_vo.h"
#include "get_compress_asset_size_vo.h"
#include "open_asset_compress_vo.h"
#include "open_asset_compress_dto.h"

namespace OHOS::Media {
class MediaAssetsService {
public:
    static MediaAssetsService &GetInstance();

    int32_t SaveFormInfo(const FormInfoDto& formInfoDto);
    int32_t SaveGalleryFormInfo(const FormInfoDto& formInfoDto);
    int32_t RemoveFormInfo(const std::string& formId);
    int32_t RemoveGalleryFormInfo(const std::string& formId);
    int32_t CommitEditedAsset(const CommitEditedAssetDto& commitEditedAssetDto);
    int32_t TrashPhotos(const std::vector<std::string> &uris);
    int32_t DeletePhotos(const std::vector<std::string> &uris);
    int32_t DeletePhotosCompleted(const std::vector<std::string> &fileIds);
    int32_t AssetChangeSetFavorite(const int32_t fileId, const bool favorite);
    int32_t AssetChangeSetHidden(const std::string &uri, const bool hidden);
    int32_t AssetChangeSetUserComment(const int32_t fileId, const std::string &userComment);
    int32_t AssetChangeSetLocation(const SetLocationDto &dto);
    int32_t AssetChangeSetTitle(const int32_t fileId, const std::string &title);
    int32_t AssetChangeSetEditData(const NativeRdb::ValuesBucket &values);
    int32_t AssetChangeSubmitCache(SubmitCacheDto &dto);
    int32_t AssetChangeCreateAsset(AssetChangeCreateAssetDto &dto);
    int32_t AssetChangeAddImage(AddImageDto &dto);
    int32_t CameraInnerAddImage(AddImageDto &dto);
    int32_t SetCameraShotKey(const int32_t fileId, const std::string &cameraShotKey);
    int32_t SaveCameraPhoto(const SaveCameraPhotoDto &dto);
    int32_t DiscardCameraPhoto(const int32_t fileId);
    int32_t SetEffectMode(const int32_t fileId, const int32_t effectMode);
    int32_t SetOrientation(const int32_t fileId, const int32_t orientation);
    int32_t SetVideoEnhancementAttr(const int32_t fileId, const std::string &photoId, const std::string &path);
    int32_t SetHasAppLink(const int32_t fileId, const int32_t hasAppLink);
    int32_t SetAppLink(const int32_t fileId, const std::string appLink);
    int32_t SetSupportedWatermarkType(const int32_t fileId, const int32_t watermarkType);
    std::shared_ptr<DataShare::DataShareResultSet> GetAssets(GetAssetsDto &dto);
    std::shared_ptr<DataShare::DataShareResultSet> GetAllDuplicateAssets(GetAssetsDto &dto);
    std::shared_ptr<DataShare::DataShareResultSet> GetDuplicateAssetsToDelete(GetAssetsDto &dto);
    int32_t CreateAsset(CreateAssetDto &dto);
    int32_t CreateAssetForApp(CreateAssetDto &dto);
    int32_t CreateAssetForAppWithAlbum(CreateAssetDto &dto);
    int32_t UpdateExistedTasksTitle(int32_t fileId);
    int32_t SetAssetTitle(int32_t fileId, const std::string &title);
    int32_t SetAssetPending(int32_t fileId, int32_t pending);
    int32_t SetAssetsFavorite(const std::vector<int32_t> &fileIds, int32_t favorite);
    int32_t SetAssetsHiddenStatus(const std::vector<int32_t> &fileIds, int32_t hiddenStatus);
    int32_t SetAssetsRecentShowStatus(const std::vector<int32_t> &fileIds, int32_t recentShowStatus);
    int32_t SetAssetsUserComment(const std::vector<int32_t> &fileIds, const std::string &userComment);
    int32_t AddAssetVisitCount(int32_t fileId, int32_t visitType);
    int32_t CloneAsset(const CloneAssetDto& cloneAssetDto);
    int32_t RevertToOriginal(const RevertToOriginalDto& revertToOriginalDto);
    int32_t SubmitCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto);
    int32_t PrioritizeCloudEnhancementTask(const CloudEnhancementDto& cloudEnhancementDto);
    int32_t CancelCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto);
    int32_t CancelAllCloudEnhancementTasks();
    int32_t GrantPhotoUriPermission(const GrantUriPermissionDto& grantUriPermissionDto);
    int32_t GrantPhotoUrisPermission(const GrantUrisPermissionDto& grantUrisPermissionDto);
    int32_t GrantPhotoUriPermissionInner(const GrantUriPermissionInnerDto& grantUriPermissionInnerDto);
    int32_t CancelPhotoUriPermission(const CancelUriPermissionDto& cancelUriPermissionDto);
    int32_t StartThumbnailCreationTask(const StartThumbnailCreationTaskDto& startThumbnailCreationTaskDto);
    int32_t StopThumbnailCreationTask(const StopThumbnailCreationTaskDto& stopThumbnailCreationTaskDto);
    int32_t RequestContent(const std::string& mediaId, int32_t& position);
    int32_t QueryCloudEnhancementTaskState(const std::string& photoUri, QueryCloudEnhancementTaskStateDto& dto);
    std::shared_ptr<NativeRdb::ResultSet> GetCloudEnhancementPair(const std::string& photoUri);
    int32_t SyncCloudEnhancementTaskStatus();
    int32_t QueryPhotoStatus(const QueryPhotoReqBody &req, QueryPhotoRespBody &resp);
    int32_t LogMovingPhoto(const AdaptedReqBody &req);
    int32_t LogCinematicVideo(const CinematicVideoAccessReqBody &req);
    std::shared_ptr<DataShare::DataShareResultSet> ConvertFormat(const ConvertFormatDto &convertFormatDto);
    bool CheckMimeType(const int32_t fileId);
    int32_t CreateTmpCompatibleDup(const CreateTmpCompatibleDupDto &createTmpCompatibleDupDto);
    int32_t GetResultSetFromDb(const GetResultSetFromDbDto &getResultSetFromDbDto, GetResultSetFromDbRespBody &resp);
    int32_t GetResultSetFromPhotosExtend(const std::string &value, std::vector<std::string> &columns,
        GetResultSetFromPhotosExtendRespBody &resp);
    int32_t GetMovingPhotoDateModified(const std::string &fileId, GetMovingPhotoDateModifiedRespBody &resp);
    int32_t CancelPhotoUriPermissionInner(const CancelUriPermissionInnerDto& cancelUriPermissionInnerDto);
    int32_t CheckPhotoUriPermissionInner(CheckUriPermissionInnerDto& checkUriPermissionInnerDto);
    int32_t CloseAsset(const CloseAssetReqBody &req);
    int32_t GetUrisByOldUrisInner(GetUrisByOldUrisInnerDto& getUrisByOldUrisInnerDto);
    int32_t Restore(const RestoreDto &dto);
    int32_t StopRestore(const std::string &keyPath);
    int32_t StartDownloadCloudMedia(CloudMediaDownloadType downloadType);
    int32_t PauseDownloadCloudMedia();
    int32_t CancelDownloadCloudMedia();
    int32_t RetainCloudMediaAsset(CloudMediaRetainType retainType = CloudMediaRetainType::RETAIN_FORCE);
    int32_t IsEdited(const IsEditedDto &dto, IsEditedRespBody &respBody);
    int32_t RequestEditData(const RequestEditDataDto &dto, RequestEditDataRespBody &respBody);
    int32_t GetEditData(const GetEditDataDto &dto, GetEditDataRespBody &respBody);
    int32_t GetCloudMediaAssetStatus(std::string &status);
    int32_t StartBatchDownloadCloudResources(StartBatchDownloadCloudResourcesReqBody &reqBody,
        StartBatchDownloadCloudResourcesRespBody &respBody);
    int32_t ResumeBatchDownloadCloudResources(ResumeBatchDownloadCloudResourcesReqBody &reqBody);
    int32_t PauseBatchDownloadCloudResources(PauseBatchDownloadCloudResourcesReqBody &reqBody);
    int32_t CancelBatchDownloadCloudResources(CancelBatchDownloadCloudResourcesReqBody &reqBody);
    int32_t GetCloudMediaBatchDownloadResourcesStatus(GetBatchDownloadCloudResourcesStatusReqBody &reqBody,
        GetBatchDownloadCloudResourcesStatusRespBody &respBody);
    int32_t GetCloudMediaBatchDownloadResourcesCount(
        GetBatchDownloadCloudResourcesCountReqBody &reqBody, GetBatchDownloadCloudResourcesCountRespBody &respBody);
    int32_t GetCloudEnhancementPair(const GetCloudEnhancementPairDto &dto, GetCloudEnhancementPairRespBody &respBody);
    int32_t GetFilePathFromUri(const std::string &virtualId, GetFilePathFromUriRespBody &respBody);
    int32_t GetUriFromFilePath(const std::string &tempPath, GetUriFromFilePathRespBody &respBody);
    int32_t CancelRequest(const std::string &photoId, const int32_t mediaType);
    int32_t CanSupportedCompatibleDuplicate(const std::string &bundleName, HeifTranscodingCheckRespBody &respBody);
    int32_t SetCompositeDisplayMode(const int32_t fileId, const int32_t compositeDisplayMode);
    int32_t AcquireDebugDatabase(const std::string &betaIssueId, const std::string &betaScenario,
        AcquireDebugDatabaseRespBody &respBody);
    int32_t ReleaseDebugDatabase(const std::string &betaIssueId);
    int32_t StartAssetChangeScanInner(const StartAssetChangeScanDto& startAssetChangeScanDto);
    int32_t GetFusionAssetsInfo(const int32_t albumId, GetFussionAssetsRespBody &respBody);
    int32_t OpenAssetCompress(const OpenAssetCompressDto &dto, OpenAssetCompressRespBody &respBody);
    int32_t NotifyAssetSended(const std::string &uri);
    int32_t GetAssetCompressVersion(int32_t &version);
    int32_t GetCompressAssetSize(const std::vector<std::string> &uris, GetCompressAssetSizeRespBody &respBody);

private:
    int32_t SubmitMetadataChanged(const int32_t fileId);
    MediaAssetsRdbOperations rdbOperation_;
};
} // namespace OHOS::Media
#endif // OHOS_MEDIA_ASSETS_SERVICE_H
