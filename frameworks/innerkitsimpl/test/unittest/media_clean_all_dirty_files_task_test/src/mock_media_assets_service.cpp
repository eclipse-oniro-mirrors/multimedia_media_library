/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "media_visit_count_manager.h"
#include "result_set_utils.h"
#include "medialibrary_photo_operations.h"
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
#include "datashare_predicates.h"
#include "close_asset_vo.h"
#include "medialibrary_db_const.h"
#include "medialibrary_object_utils.h"
#include "media_column.h"
#include "media_old_photos_column.h"
#include "medialibrary_tab_old_photos_operations.h"
#include "database_adapter.h"
#include "commit_edited_asset_dto.h"

using namespace std;
using namespace OHOS::RdbDataShareAdapter;

namespace OHOS::Media {

const std::string SET_LOCATION_KEY = "set_location";
const std::string SET_LOCATION_VALUE = "1";
const std::string COLUMN_FILE_ID = "file_id";
const std::string COLUMN_DATA = "data";
const std::string COLUMN_OLD_FILE_ID = "old_file_id";
const std::string COLUMN_OLD_DATA = "old_data";
const std::string COLUMN_DISPLAY_NAME = "display_name";
const std::string HEIF_MIME_TYPE = "image/heif";
const std::string HEIC_MIME_TYPE = "image/heic";
static const string IS_ORIGINAL_IMAGE_RESOURCE = "is_original_image_resource";
static const string ORIGINAL_IMAGE_RESOURCE = "1";
unordered_set<std::string> uploadDatabaseTaskSet;
std::mutex uploadDatabaseMutex;

MediaAssetsService &MediaAssetsService::GetInstance()
{
    static MediaAssetsService service;
    return service;
}

int32_t MediaAssetsService::RemoveFormInfo(const string& formId)
{
    return E_OK;
}

int32_t MediaAssetsService::RemoveGalleryFormInfo(const string& formId)
{
    return E_OK;
}

int32_t MediaAssetsService::SaveFormInfo(const FormInfoDto& formInfoDto)
{
    return E_OK;
}

int32_t MediaAssetsService::SaveGalleryFormInfo(const FormInfoDto& formInfoDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CommitEditedAsset(const CommitEditedAssetDto& commitEditedAssetDto)
{
    return E_OK;
}

int32_t MediaAssetsService::TrashPhotos(const std::vector<std::string> &uris)
{
    return E_OK;
}

int32_t MediaAssetsService::DeletePhotos(const std::vector<std::string> &uris)
{
    return E_OK;
}

int32_t MediaAssetsService::DeletePhotosCompleted(const std::vector<std::string> &fileIds)
{
    return E_OK;
}

int32_t MediaAssetsService::DeleteAssetsPermanentlyWithUri(const std::vector<std::string> &fileIds)
{
    return E_OK;
}

static std::string GetLocalDeviceName()
{
    return "";
}

static std::string GetClientBundleName()
{
    return "";
}

int32_t MediaAssetsService::AssetChangeSetFavorite(const int32_t fileId, const bool favorite)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSetHidden(const std::string &uri, const bool hidden)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSetUserComment(const int32_t fileId, const std::string &userComment)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSetLocation(const SetLocationDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::UpdateExistedTasksTitle(int32_t fileId)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSetTitle(const int32_t fileId, const std::string &title)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSetEditData(const NativeRdb::ValuesBucket &values)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeSubmitCache(SubmitCacheDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeCreateAsset(AssetChangeCreateAssetDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::AssetChangeAddImage(AddImageDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::CameraInnerAddImage(AddImageDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::GetFusionAssetsInfo(const int32_t albumId, GetFussionAssetsRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::SetCameraShotKey(const int32_t fileId, const std::string &cameraShotKey)
{
    return E_OK;
}

int32_t MediaAssetsService::SaveCameraPhoto(const SaveCameraPhotoDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::DiscardCameraPhoto(const int32_t fileId)
{
    return E_OK;
}

int32_t MediaAssetsService::SetEffectMode(const int32_t fileId, const int32_t effectMode)
{
    return E_OK;
}

int32_t MediaAssetsService::SetOrientation(const int32_t fileId, const int32_t orientation)
{
    return E_OK;
}

int32_t MediaAssetsService::SetVideoEnhancementAttr(
    const int32_t fileId, const std::string &photoId, const std::string &path)
{
    return E_OK;
}

int32_t MediaAssetsService::SetHasAppLink(const int32_t fileId, const int32_t hasAppLink)
{
    return E_OK;
}
 
int32_t MediaAssetsService::SetAppLinkState(const int32_t fileId, const int32_t appLinkState)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAppLink(const int32_t fileId, const string appLink)
{
    return E_OK;
}

int32_t MediaAssetsService::SubmitMetadataChanged(const int32_t fileId)
{
    return E_OK;
}

int32_t MediaAssetsService::SetSupportedWatermarkType(const int32_t fileId, const int32_t watermarkType)
{
    return E_OK;
}

int32_t MediaAssetsService::SetCompositeDisplayMode(const int32_t fileId, const int32_t compositeDisplayMode)
{
    return E_OK;
}

int32_t MediaAssetsService::GrantPhotoUriPermissionInner(const GrantUriPermissionInnerDto& grantUrisPermissionInnerDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CheckPhotoUriPermissionInner(CheckUriPermissionInnerDto& checkUriPermissionInnerDto)
{
    return E_OK;
}

int32_t MediaAssetsService::StartAssetChangeScanInner(
    const StartAssetChangeScanDto& startAssetChangeScanDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPermissionInner(
    const CancelUriPermissionInnerDto& cancelUriPermissionInnerDto)
{
    return E_OK;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetAssets(GetAssetsDto &dto, int32_t passCode)
{
    return nullptr;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetAllDuplicateAssets(GetAssetsDto &dto)
{
    return nullptr;
}

std::shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::GetDuplicateAssetsToDelete(GetAssetsDto &dto)
{
    return nullptr;
}

int32_t MediaAssetsService::CreateAsset(CreateAssetDto& dto)
{
    return E_OK;
}

int32_t MediaAssetsService::CreateAssetForApp(CreateAssetDto& dto)
{
    return E_OK;
}

int32_t MediaAssetsService::CreateAssetForAppWithAlbum(CreateAssetDto& dto)
{
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
    return E_OK;
}

int32_t MediaAssetsService::SetAssetPending(int32_t fileId, int32_t pending)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAssetsFavorite(const std::vector<int32_t> &fileIds, int32_t favorite)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAssetsHiddenStatus(const std::vector<int32_t> &fileIds, int32_t hiddenStatus)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAssetsRecentShowStatus(const std::vector<int32_t> &fileIds, int32_t recentShowStatus)
{
    return E_OK;
}

int32_t MediaAssetsService::SetAssetsUserComment(const std::vector<int32_t> &fileIds, const std::string &userComment)
{
    return E_OK;
}

int32_t MediaAssetsService::AddAssetVisitCount(int32_t fileId, int32_t visitType)
{
    return E_OK;
}

int32_t MediaAssetsService::CloneAsset(const CloneAssetDto& cloneAssetDto)
{
    return E_OK;
}

shared_ptr<DataShare::DataShareResultSet> MediaAssetsService::ConvertFormat(const ConvertFormatDto& convertFormatDto)
{
    return nullptr;
}

bool MediaAssetsService::CheckMimeType(const int32_t fileId)
{
    return true;
}

int32_t MediaAssetsService::CreateTmpCompatibleDup(const CreateTmpCompatibleDupDto &createTmpCompatibleDupDto)
{
    return E_OK;
}

int32_t MediaAssetsService::RevertToOriginal(const RevertToOriginalDto& revertToOriginalDto)
{
    return E_OK;
}

int32_t MediaAssetsService::SubmitCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    return E_OK;
}

int32_t MediaAssetsService::PrioritizeCloudEnhancementTask(const CloudEnhancementDto& cloudEnhancementDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelCloudEnhancementTasks(const CloudEnhancementDto& cloudEnhancementDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelAllCloudEnhancementTasks()
{
    return E_OK;
}

int32_t MediaAssetsService::GrantPhotoUriPermission(const GrantUriPermissionDto &grantUriPermissionDto)
{
    return E_OK;
}

int32_t MediaAssetsService::GrantPhotoUrisPermission(const GrantUrisPermissionDto &grantUrisPermissionDto)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelPhotoUriPermission(const CancelUriPermissionDto &cancelUriPermissionDto)
{
    return E_OK;
}

int32_t MediaAssetsService::StartThumbnailCreationTask(
    const StartThumbnailCreationTaskDto &startThumbnailCreationTaskDto)
{
    return E_OK;
}

int32_t MediaAssetsService::StopThumbnailCreationTask(const StopThumbnailCreationTaskDto &stopThumbnailCreationTaskDto)
{
    return E_OK;
}

int32_t MediaAssetsService::RequestContent(const string& mediaId, int32_t& position)
{
    return E_OK;
}

shared_ptr<NativeRdb::ResultSet> MediaAssetsService::GetCloudEnhancementPair(const string& photoUri)
{
    return nullptr;
}

int32_t MediaAssetsService::QueryCloudEnhancementTaskState(const string& photoUri,
    QueryCloudEnhancementTaskStateDto& dto)
{
    return E_OK;
}

int32_t MediaAssetsService::SyncCloudEnhancementTaskStatus()
{
    return E_OK;
}

int32_t MediaAssetsService::QueryPhotoStatus(const QueryPhotoReqBody &req, QueryPhotoRespBody &resp)
{
    return E_OK;
}

int32_t MediaAssetsService::LogMovingPhoto(const AdaptedReqBody &req)
{
    return E_OK;
}

int32_t MediaAssetsService::LogCinematicVideo(const CinematicVideoAccessReqBody &req)
{
    return E_OK;
}

static int32_t IsDateAddedDateUpgradeTaskFinished(bool &result)
{
    return E_OK;
}

int32_t MediaAssetsService::QueryMediaDataStatus(const string &dataKey, bool &result)
{
    return E_OK;
}

int32_t MediaAssetsService::GetResultSetFromDb(const GetResultSetFromDbDto& getResultSetFromDbDto,
    GetResultSetFromDbRespBody& resp)
{
    return E_OK;
}

int32_t MediaAssetsService::GetResultSetFromPhotosExtend(const string &value, vector<string> &columns,
    GetResultSetFromPhotosExtendRespBody& resp)
{
    return E_OK;
}

int32_t MediaAssetsService::GetMovingPhotoDateModified(const string &fileId, GetMovingPhotoDateModifiedRespBody& resp)
{
    return E_OK;
}

int32_t MediaAssetsService::CloseAsset(const CloseAssetReqBody &req)
{
    return E_OK;
}

static int BuildPredicates(const std::vector<std::string> &queryTabOldPhotosUris,
    DataShare::DataSharePredicates &predicates)
{
    return E_OK;
}

int32_t MediaAssetsService::GetUrisByOldUrisInner(GetUrisByOldUrisInnerDto& getUrisByOldUrisInnerDto)
{
    return E_OK;
}

int32_t MediaAssetsService::Restore(const RestoreDto &dto)
{
    return 0;
}

int32_t MediaAssetsService::StopRestore(const std::string &keyPath)
{
    return 0;
}

int32_t MediaAssetsService::StartDownloadCloudMedia(CloudMediaDownloadType downloadType)
{
    return 0;
}

int32_t MediaAssetsService::PauseDownloadCloudMedia()
{
    return 0;
}

int32_t MediaAssetsService::CancelDownloadCloudMedia()
{
    return 0;
}

int32_t MediaAssetsService::RetainCloudMediaAsset(CloudMediaRetainType retainType)
{
    return 0;
}

int32_t MediaAssetsService::IsEdited(const IsEditedDto &dto, IsEditedRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::RequestEditData(const RequestEditDataDto &dto, RequestEditDataRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::GetEditData(const GetEditDataDto &dto, GetEditDataRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::GetCloudMediaAssetStatus(string &status)
{
    return E_OK;
}

int32_t MediaAssetsService::StartBatchDownloadCloudResources(StartBatchDownloadCloudResourcesReqBody &reqBody,
    StartBatchDownloadCloudResourcesRespBody &respBody)
{
    return 0;
}

int32_t MediaAssetsService::ResumeBatchDownloadCloudResources(ResumeBatchDownloadCloudResourcesReqBody &reqBody)
{
    return 0;
}

int32_t MediaAssetsService::PauseBatchDownloadCloudResources(PauseBatchDownloadCloudResourcesReqBody &reqBody)
{
    return 0;
}

int32_t MediaAssetsService::CancelBatchDownloadCloudResources(CancelBatchDownloadCloudResourcesReqBody &reqBody)
{
    return 0;
}

int32_t MediaAssetsService::GetCloudMediaBatchDownloadResourcesStatus(
    GetBatchDownloadCloudResourcesStatusReqBody &reqBody, GetBatchDownloadCloudResourcesStatusRespBody &respBody)
{
    return 0;
}

int32_t MediaAssetsService::GetCloudMediaBatchDownloadResourcesCount(
    GetBatchDownloadCloudResourcesCountReqBody &reqBody, GetBatchDownloadCloudResourcesCountRespBody &respBody)
{
    return 0;
}

int32_t MediaAssetsService::GetCloudEnhancementPair(
    const GetCloudEnhancementPairDto &dto, GetCloudEnhancementPairRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::GetFilePathFromUri(const std::string &virtualId, GetFilePathFromUriRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::GetUriFromFilePath(const std::string &tempPath, GetUriFromFilePathRespBody &respBody)
{
    return E_OK;
}

int32_t MediaAssetsService::CancelRequest(const std::string &photoId,
    const int32_t mediaType)
{
    return E_OK;
}

int32_t MediaAssetsService::CanSupportedCompatibleDuplicate(const std::string &bundleName,
    HeifTranscodingCheckRespBody &respBody)
{
    return E_OK;
}

static int32_t WriteBetaDebugTaskSet(const string& betaIssueId)
{
    return E_SUCCESS;
}

static int32_t EraseBetaDebugTaskSet(const string& betaIssueId)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::AcquireDebugDatabase(const string &betaIssueId, const std::string &betaScenario,
    AcquireDebugDatabaseRespBody &respBody)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::ReleaseDebugDatabase(const string &betaIssueId)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::OpenAssetCompress(const OpenAssetCompressDto &dto, OpenAssetCompressRespBody &respBody)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::NotifyAssetSended(const std::string &uri, int32_t shareType)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetAssetCompressVersion(int32_t &version)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::GetCompressAssetSize(const std::vector<std::string> &uris,
    GetCompressAssetSizeRespBody &respBody)
{
    return E_SUCCESS;
}

int32_t MediaAssetsService::SubmitExistFileDBRecord(SubmitCacheDto &dto)
{
    return E_OK;
}

int32_t MediaAssetsService::ApplyEditEffectToFile(int32_t curBucketNum, const std::string &fileName)
{
    return E_OK;
}

int32_t MediaAssetsService::ScanExistFileRecord(int32_t fileId, const std::string &path)
{
    return E_OK;
}

int32_t MediaAssetsService::CheckSinglePhotoPermission(const std::string &fileId, int32_t registerType)
{
    return E_OK;
}
} // namespace OHOS::Media
