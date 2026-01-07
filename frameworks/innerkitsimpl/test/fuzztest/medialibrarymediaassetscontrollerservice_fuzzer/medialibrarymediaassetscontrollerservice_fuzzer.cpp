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

#include "medialibrarymediaassetscontrollerservice_fuzzer.h"

#include <cstdint>
#include <string>
#include <fuzzer/FuzzedDataProvider.h>
#define private public
#include "media_assets_controller_service.h"
#include "media_analysis_data_controller_service.h"
#undef private
#include "user_define_ipc.h"
#include "form_info_vo.h"
#include "commit_edited_asset_vo.h"
#include "trash_photos_vo.h"
#include "delete_photos_vo.h"
#include "delete_photos_completed_vo.h"
#include "asset_change_vo.h"
#include "submit_cache_vo.h"
#include "asset_change_vo.h"
#include "add_image_vo.h"
#include "save_camera_photo_vo.h"
#include "get_assets_vo.h"
#include "create_asset_vo.h"
#include "modify_assets_vo.h"
#include "get_asset_analysis_data_vo.h"
#include "clone_asset_vo.h"
#include "revert_to_original_vo.h"
#include "convert_format_vo.h"
#include "cloud_enhancement_vo.h"
#include "start_download_cloud_media_vo.h"
#include "retain_cloud_media_asset_vo.h"
#include "get_cloudmedia_asset_status_vo.h"
#include "get_edit_data_vo.h"
#include "request_edit_data_vo.h"
#include "is_edited_vo.h"
#include "start_asset_analysis_vo.h"
#include "grant_photo_uri_permission_vo.h"
#include "grant_photo_uris_permission_vo.h"
#include "cancel_photo_uri_permission_vo.h"
#include "stop_thumbnail_creation_task_vo.h"
#include "start_thumbnail_creation_task_vo.h"
#include "request_content_vo.h"
#include "get_cloud_enhancement_pair_vo.h"
#include "query_cloud_enhancement_task_state_vo.h"
#include "query_photo_vo.h"
#include "add_visit_count_vo.h"
#include "adapted_vo.h"
#include "grant_photo_uri_permission_inner_vo.h"
#include "cancel_photo_uri_permission_inner_vo.h"
#include "get_result_set_from_db_vo.h"
#include "get_result_set_from_photos_extend_vo.h"
#include "get_moving_photo_date_modified_vo.h"
#include "get_uri_from_filepath_vo.h"
#include "get_filepath_from_uri_vo.h"
#include "close_asset_vo.h"
#include "check_photo_uri_permission_inner_vo.h"
#include "get_uris_by_old_uris_inner_vo.h"
#include "restore_vo.h"
#include "stop_restore_vo.h"

#include "media_old_photos_column.h"
#include "media_column.h"
#include "message_parcel.h"
#include "rdb_utils.h"

namespace OHOS {
using namespace std;
using namespace OHOS::Media;

static const int32_t FORM_INFO_NUM_BYTES = 10;
static const int32_t NUM_BYTES = 8;
static const string EDIT_DATA_VALUE = "{\"imageEffect\":{\"filters\":[{\"name\":\"InplaceSticker\",\"values\":"
    "{\"RESOURCE_DIRECTORY\":\"/sys_prod/resource/camera\",\"cameraPosition\":1}}],\"name\":\"imageEdit\"}}";

FuzzedDataProvider* FDP;
shared_ptr<MediaAssetsControllerService> mediaAssetsControllerService = nullptr;
shared_ptr<AnalysisData::MediaAnalysisDataControllerService> analysisDataControllerService = nullptr;

static inline std::vector<std::string> FuzzVector()
{
    return {FDP->ConsumeBytesAsString(FORM_INFO_NUM_BYTES)};
}

static void FormInfoFuzzer()
{
    FormInfoReqBody reqBody;
    reqBody.formIds = FuzzVector();
    string uri = "file://media/Photo/" + to_string(FDP->ConsumeIntegral<int32_t>());
    vector<string> fileUris = { uri };
    reqBody.fileUris = fileUris;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SaveFormInfo(data, reply);
    mediaAssetsControllerService->SaveGalleryFormInfo(data, reply);
    mediaAssetsControllerService->RemoveFormInfo(data, reply);
    mediaAssetsControllerService->RemoveGalleryFormInfo(data, reply);
    mediaAssetsControllerService->UpdateGalleryFormInfo(data, reply);
}

static void CommitEditedAssetFuzzer()
{
    CommitEditedAssetReqBody reqBody;
    reqBody.editData = EDIT_DATA_VALUE;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CommitEditedAsset(data, reply);
}

static void SysTrashPhotosFuzzer()
{
    TrashPhotosReqBody reqBody;
    auto uri = "file://media/Photo/" + to_string(FDP->ConsumeIntegral<int32_t>());
    std::vector<std::string> testUris = {uri};
    reqBody.uris = testUris;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SysTrashPhotos(data, reply);
    mediaAssetsControllerService->TrashPhotos(data, reply);
}

static void DeletePhotosFuzzer()
{
    DeletePhotosReqBody reqBody;
    std::vector<std::string> testUris = {"file://media/Photo/1/IMG_1748423699_000/IMG_20250528_171318.jpg", ""};
    reqBody.uris = testUris;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->DeletePhotos(data, reply);
}

static void DeletePhotosCompletedFuzzer()
{
    DeletePhotosCompletedReqBody reqBody;
    reqBody.fileIds = {to_string(FDP->ConsumeIntegral<int32_t>())};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->DeletePhotosCompleted(data, reply);
}

static void AssetChangeSetFavoriteFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.favorite = FDP->ConsumeBool();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetFavorite(data, reply);
}

static void AssetChangeSetHiddenFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.uri = "file://media/Photo/1/IMG_1748423699_000/IMG_20250528_171318.jpg";
    reqBody.hidden = FDP->ConsumeBool();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetHidden(data, reply);
}

static void AssetChangeSetUserCommentFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.userComment = "user comment";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetUserComment(data, reply);
}

static void AssetChangeSetLocationFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.path = "/data/local/tmp/IMG_1501924305_001.jpg";
    reqBody.latitude = FDP->ConsumeFloatingPoint<double>();
    reqBody.longitude = FDP->ConsumeFloatingPoint<double>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetLocation(data, reply);
}

static void AssetChangeSetTitleFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.title = "title_test";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetTitle(data, reply);
}

static void AssetChangeSetEditDataFuzzer()
{
    AssetChangeReqBody reqBody;
    DataShare::DataShareValuesBucket valuesBucket;
    int fileId = FDP->ConsumeIntegral<int32_t>();
    valuesBucket.Put("file_id", fileId);
    std::string editData = "edit_data";
    valuesBucket.Put("edit_data", editData);
    reqBody.values = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSetEditData(data, reply);
}

static void AssetChangeSubmitCacheFuzzer()
{
    SubmitCacheReqBody reqBody;
    DataShare::DataShareValuesBucket valuesBucket;
    int fileId = FDP->ConsumeIntegral<int32_t>();
    valuesBucket.Put("file_id", fileId);
    std::string editData = "edit_data";
    valuesBucket.Put("edit_data", editData);
    reqBody.values = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    reqBody.isWriteGpsAdvanced = FDP->ConsumeBool();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeSubmitCache(data, reply);
}

static void AssetChangeCreateAssetFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.values.Put("extention", "jpg");
    reqBody.values.Put("media_type", MEDIA_TYPE_IMAGE);
    reqBody.values.Put("title", "20250602162718617");
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeCreateAsset(data, reply);
}

static void AssetChangeAddImageFuzzer()
{
    AddImageReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.photoId = "20250527162718617";
    reqBody.deferredProcType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AssetChangeAddImage(data, reply);
}

static void SetCameraShotKeyFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.cameraShotKey = "shot_key_xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx_test";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetCameraShotKey(data, reply);
}

static void SaveCameraPhotoFuzzer()
{
    SaveCameraPhotoReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.needScan = FDP->ConsumeBool();
    reqBody.path = "file//media/Photo/";
    reqBody.photoSubType = 1;
    reqBody.imageFileType = 1;
    reqBody.supportedWatermarkType = FDP->ConsumeIntegral<int32_t>();
    reqBody.cameraShotKey = "Set";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SaveCameraPhoto(data, reply);
}

static void DiscardCameraPhotoFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->DiscardCameraPhoto(data, reply);
}

static void SetEffectModeFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.effectMode = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetEffectMode(data, reply);
}

static void SetOrientationFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.orientation = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetOrientation(data, reply);
}

static void SetVideoEnhancementAttrFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.photoId = "202410011800";
    reqBody.path = "file//media/Photo/";
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetVideoEnhancementAttr(data, reply);
}

static void SetSupportedWatermarkTypeFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.watermarkType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetSupportedWatermarkType(data, reply);
}

static void SetCompositeDisplayModeFuzzer()
{
    AssetChangeReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.compositeDisplayMode = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetCompositeDisplayMode(data, reply);
}

static void DuplicateAssetsFuzzer()
{
    GetAssetsReqBody reqBody;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetAllDuplicateAssets(data, reply);
    mediaAssetsControllerService->GetDuplicateAssetsToDelete(data, reply);
}

static void CreateAssetFuzzer()
{
    CreateAssetReqBody reqBody;
    reqBody.mediaType = MEDIA_TYPE_IMAGE;
    reqBody.title = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.extension = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->PublicCreateAsset(data, reply);
    CreateAssetReqBody sysReqBody;
    sysReqBody.mediaType = MEDIA_TYPE_IMAGE;
    sysReqBody.photoSubtype = FDP->ConsumeIntegral<int32_t>();
    sysReqBody.displayName = FDP->ConsumeBytesAsString(NUM_BYTES);
    sysReqBody.cameraShotKey = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel sysData;
    MessageParcel sysReply;
    sysReqBody.Marshalling(sysData);
    mediaAssetsControllerService->SystemCreateAsset(sysData, sysReply);
}

static void CreateAssetForAppFuzzer()
{
    CreateAssetForAppReqBody reqBody;
    reqBody.tokenId = FDP->ConsumeIntegral<int32_t>();
    reqBody.mediaType = MEDIA_TYPE_IMAGE;
    reqBody.photoSubtype = static_cast<int32_t>(PhotoSubType::DEFAULT);
    reqBody.title = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.extension = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.appId = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.packageName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.bundleName = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->PublicCreateAssetForApp(data, reply);
    mediaAssetsControllerService->SystemCreateAssetForApp(data, reply);
    reqBody.ownerAlbumId = to_string(FDP->ConsumeIntegral<int32_t>());
    MessageParcel album_data;
    MessageParcel album_reply;
    reqBody.Marshalling(album_data);
    mediaAssetsControllerService->CreateAssetForAppWithAlbum(album_data, album_reply);
}

static void SetAssetTitleFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.title = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetTitle(data, reply);
}

static void SetAssetPendingFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.pending = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetPending(data, reply);
}

static void SetAssetsFavoriteFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.favorite = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetsFavorite(data, reply);
}

static void SetAssetsHiddenStatusFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.hiddenStatus = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetsHiddenStatus(data, reply);
}

static void SetAssetsRecentShowStatusFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.recentShowStatus = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetsRecentShowStatus(data, reply);
}

static void SetAssetsUserCommentFuzzer()
{
    ModifyAssetsReqBody reqBody;
    reqBody.fileIds.push_back(FDP->ConsumeIntegral<int32_t>());
    reqBody.userComment = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SetAssetsUserComment(data, reply);
}

static void GetAssetAnalysisDataFuzzer()
{
    GetAssetAnalysisDataReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.language = "zh-Hans";
    reqBody.analysisType = FDP->ConsumeIntegral<int32_t>();
    reqBody.analysisTotal = FDP->ConsumeBool();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->GetAssetAnalysisData(data, reply);
}

static void CloneAssetFuzzer()
{
    CloneAssetReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.displayName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.title = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CloneAsset(data, reply);
}

static void RevertToOriginalFuzzer()
{
    RevertToOriginalReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.fileUri = "file://media/Photo/" + to_string(reqBody.fileId);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->RevertToOriginal(data, reply);
}

static void ConvertFormatFuzzer()
{
    ConvertFormatReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.title = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.extension =FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->ConvertFormat(data, reply);
}

static void SubmitCloudEnhancementTasksFuzzer()
{
    CloudEnhancementReqBody reqBody;
    reqBody.hasCloudWatermark = FDP->ConsumeBool();
    reqBody.triggerMode = 1;
    reqBody.fileUris = { "file://media/Photo/" + to_string(FDP->ConsumeIntegral<int32_t>()) };
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->SubmitCloudEnhancementTasks(data, reply);
    mediaAssetsControllerService->PrioritizeCloudEnhancementTask(data, reply);
    mediaAssetsControllerService->CancelCloudEnhancementTasks(data, reply);
    mediaAssetsControllerService->CancelAllCloudEnhancementTasks(data, reply);
    mediaAssetsControllerService->SyncCloudEnhancementTaskStatus(data, reply);
}

static void StartDownloadCloudMediaFuzzer()
{
    StartDownloadCloudMediaReqBody reqBody;
    reqBody.cloudMediaType = FDP->ConsumeBool() ? 1 : 0;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->StartDownloadCloudMedia(data, reply);
}

static void RetainCloudMediaAssetFuzzer()
{
    RetainCloudMediaAssetReqBody reqBody;
    reqBody.cloudMediaRetainType = FDP->ConsumeBool() ? 1 : 0;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->RetainCloudMediaAsset(data, reply);
}

static void GetCloudMediaAssetStatusFuzzer()
{
    GetCloudMediaAssetStatusReqBody reqBody;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetCloudMediaAssetStatus(data, reply);
}

static void GetEditDataFuzzer()
{
    GetEditDataReqBody reqBody;
    reqBody.predicates.EqualTo("file_id", "1111111");
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetEditData(data, reply);
}

static void RequestEditDataFuzzer()
{
    RequestEditDataReqBody reqBody;
    reqBody.predicates.EqualTo("file_id", "1111111");
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->RequestEditData(data, reply);
}

static void IsEditedFuzzer()
{
    IsEditedReqBody reqBody;
    reqBody.fileId = 1;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->IsEdited(data, reply);
}

static void StartAssetAnalysisFuzzer()
{
    StartAssetAnalysisReqBody reqBody;
    std::vector<std::string> fileIds{"111111", "222222", to_string(FDP->ConsumeIntegral<int32_t>())};
    reqBody.predicates.In("Photos.file_id", fileIds);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    analysisDataControllerService->StartAssetAnalysis(data, reply);
}

static void GrantPhotoUriPermissionFuzzer()
{
    GrantUriPermissionReqBody reqBody;
    reqBody.tokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.srcTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.permissionType = FDP->ConsumeIntegral<int32_t>();
    reqBody.hideSensitiveType = FDP->ConsumeIntegral<int32_t>();
    reqBody.uriType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GrantPhotoUriPermission(data, reply);
}

static void GrantPhotoUrisPermissionFuzzer()
{
    GrantUrisPermissionReqBody reqBody;
    reqBody.tokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.srcTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.fileIds = {FDP->ConsumeIntegral<int32_t>()};
    reqBody.permissionType = FDP->ConsumeIntegral<int32_t>();
    reqBody.hideSensitiveType = FDP->ConsumeIntegral<int32_t>();
    reqBody.uriType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GrantPhotoUrisPermission(data, reply);
}

static void CancelPhotoUriPermissionFuzzer()
{
    CancelUriPermissionReqBody reqBody;
    reqBody.tokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.srcTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.permissionType = FDP->ConsumeIntegral<int32_t>();
    reqBody.uriType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CancelPhotoUriPermission(data, reply);
}

static void StartThumbnailCreationTaskFuzzer()
{
    StartThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->StartThumbnailCreationTask(data, reply);
}

static void StopThumbnailCreationTaskFuzzer()
{
    StopThumbnailCreationTaskReqBody reqBody;
    reqBody.requestId = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->StopThumbnailCreationTask(data, reply);
}

static void RequestContentFuzzer()
{
    RequestContentReqBody reqBody;
    reqBody.mediaId = to_string(FDP->ConsumeIntegral<int32_t>());
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->RequestContent(data, reply);
}

static void GetCloudEnhancementPairFuzzer()
{
    GetCloudEnhancementPairReqBody reqBody;
    reqBody.photoUri = FDP->ConsumeBytesAsString(NUM_BYTES);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetCloudEnhancementPair(data, reply);
}

static void QueryCloudEnhancementTaskStateFuzzer()
{
    QueryCloudEnhancementTaskStateReqBody reqBody;
    reqBody.photoUri = FDP->ConsumeBytesAsString(NUM_BYTES);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->QueryCloudEnhancementTaskState(data, reply);
}

static void QueryPhotoStatusFuzzer()
{
    QueryPhotoReqBody reqBody;
    reqBody.fileId = std::to_string(FDP->ConsumeIntegral<int32_t>());

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->QueryPhotoStatus(data, reply);
}

static void LogMovingPhotoFuzzer()
{
    AdaptedReqBody reqBody;
    reqBody.adapted = FDP->ConsumeBool();

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->LogMovingPhoto(data, reply);
}

static void AddAssetVisitCountFuzzer()
{
    AddAssetVisitCountReqBody reqBody;
    reqBody.fileId = FDP->ConsumeIntegral<int32_t>();
    reqBody.visitType = FDP->ConsumeIntegral<int32_t>();
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->AddAssetVisitCount(data, reply);
}

static void GrantPhotoUriPermissionInnerFuzzer()
{
    GrantUrisPermissionInnerReqBody reqBody;
    reqBody.tokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.srcTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.fileIds = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    reqBody.permissionTypes = {FDP->ConsumeIntegral<int32_t>()};
    reqBody.hideSensitiveType = FDP->ConsumeIntegral<int32_t>();
    reqBody.uriTypes = {FDP->ConsumeIntegral<int32_t>()};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GrantPhotoUriPermissionInner(data, reply);
}

static void CancelPhotoUriPermissionInnerFuzzer()
{
    CancelUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.srcTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.fileIds = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    reqBody.uriTypes = {FDP->ConsumeIntegral<int32_t>()};
    reqBody.permissionTypes = {{FDP->ConsumeBytesAsString(NUM_BYTES)}};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CancelPhotoUriPermissionInner(data, reply);
}

static void GetResultSetFromDbFuzzer()
{
    GetResultSetFromDbReqBody reqBody;
    reqBody.columnName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.value = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.columns = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetResultSetFromDb(data, reply);
}

static void GetResultSetFromPhotosExtendFuzzer()
{
    GetResultSetFromPhotosExtendReqBody reqBody;
    reqBody.value = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.columns = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetResultSetFromPhotosExtend(data, reply);
}

static void GetMovingPhotoDateModifiedFuzzer()
{
    GetMovingPhotoDateModifiedReqBody reqBody;
    reqBody.fileId = to_string(FDP->ConsumeIntegral<int32_t>());

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetMovingPhotoDateModified(data, reply);
}

static void GetFilePathFromUriFuzzer()
{
    GetFilePathFromUriReqBody reqBody;
    reqBody.virtualId = to_string(FDP->ConsumeIntegral<int32_t>());

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetFilePathFromUri(data, reply);
}

static void GetUriFromFilePathFuzzer()
{
    GetUriFromFilePathReqBody reqBody;
    reqBody.tempPath = to_string(FDP->ConsumeIntegral<int32_t>());

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetUriFromFilePath(data, reply);
}

static void CloseAssetFuzzer()
{
    CloseAssetReqBody reqBody;
    reqBody.uri = FDP->ConsumeBytesAsString(NUM_BYTES);

    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CloseAsset(data, reply);
}

static void CheckUriPermissionInnerFuzzer()
{
    CheckUriPermissionInnerReqBody reqBody;
    reqBody.targetTokenId = FDP->ConsumeIntegral<int64_t>();
    reqBody.uriType = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.fileIds = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    reqBody.columns = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->CheckUriPermissionInner(data, reply);
}

static void GetUrisByOldUrisInnerFuzzer()
{
    GetUrisByOldUrisInnerReqBody reqBody;
    reqBody.uris = {FDP->ConsumeBytesAsString(NUM_BYTES)};
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "file_id");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "data");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_file_id");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "old_data");
    reqBody.columns.push_back(TabOldPhotosColumn::OLD_PHOTOS_TABLE + "." + "clone_sequence");
    reqBody.columns.push_back(PhotoColumn::PHOTOS_TABLE + "." + "display_name");
   
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->GetUrisByOldUrisInner(data, reply);
}

static void RestoreFuzzer()
{
    RestoreReqBody reqBody;
    reqBody.albumLpath = "/" + FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.keyPath = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.bundleName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.appName = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.appId = FDP->ConsumeBytesAsString(NUM_BYTES);
    reqBody.isDeduplication = FDP->ConsumeBool();
   
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->Restore(data, reply);
}

static void StopRestoreFuzzer()
{
    StopRestoreReqBody reqBody;
    reqBody.keyPath = FDP->ConsumeBytesAsString(NUM_BYTES);
   
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    mediaAssetsControllerService->StopRestore(data, reply);
}

static void GetAssetsFuzzer()
{
    GetAssetsReqBody reqBody;
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    OHOS::Media::IPC::IPCContext context(MessageOption(), 0);
    mediaAssetsControllerService->GetAssets(data, reply, context);
}

static void GetBurstAssetsFuzzer()
{
    GetAssetsReqBody reqBody;
    reqBody.burstKey = FDP->ConsumeBytesAsString(NUM_BYTES);
    MessageParcel data;
    MessageParcel reply;
    reqBody.Marshalling(data);
    OHOS::Media::IPC::IPCContext context(MessageOption(), 0);
    mediaAssetsControllerService->GetBurstAssets(data, reply, context);
}

static void MediaAssetsControllerServiceFirstFuzzer()
{
    FormInfoFuzzer();
    CommitEditedAssetFuzzer();
    SysTrashPhotosFuzzer();
    DeletePhotosFuzzer();
    DeletePhotosCompletedFuzzer();
    AssetChangeSetFavoriteFuzzer();
    AssetChangeSetHiddenFuzzer();
    AssetChangeSetUserCommentFuzzer();
    AssetChangeSetLocationFuzzer();
    AssetChangeSetTitleFuzzer();
    AssetChangeSetEditDataFuzzer();
    AssetChangeSubmitCacheFuzzer();
    AssetChangeCreateAssetFuzzer();
    AssetChangeAddImageFuzzer();
    SetCameraShotKeyFuzzer();
    SaveCameraPhotoFuzzer();
    DiscardCameraPhotoFuzzer();
    SetEffectModeFuzzer();
    SetOrientationFuzzer();
    SetVideoEnhancementAttrFuzzer();
    SetSupportedWatermarkTypeFuzzer();
    SetCompositeDisplayModeFuzzer();
    DuplicateAssetsFuzzer();
    CreateAssetFuzzer();
    CreateAssetForAppFuzzer();
    SetAssetTitleFuzzer();
    SetAssetPendingFuzzer();
    SetAssetsFavoriteFuzzer();
    SetAssetsHiddenStatusFuzzer();
    SetAssetsRecentShowStatusFuzzer();
    SetAssetsUserCommentFuzzer();
    GetAssetAnalysisDataFuzzer();
    CloneAssetFuzzer();
    RevertToOriginalFuzzer();
    ConvertFormatFuzzer();
    SubmitCloudEnhancementTasksFuzzer();
    StartDownloadCloudMediaFuzzer();
    RetainCloudMediaAssetFuzzer();
    GetCloudMediaAssetStatusFuzzer();
    GetEditDataFuzzer();
}

static void MediaAssetsControllerServiceSecondFuzzer()
{
    RequestEditDataFuzzer();
    IsEditedFuzzer();
    StartAssetAnalysisFuzzer();
    GrantPhotoUriPermissionFuzzer();
    GrantPhotoUrisPermissionFuzzer();
    CancelPhotoUriPermissionFuzzer();
    StartThumbnailCreationTaskFuzzer();
    StopThumbnailCreationTaskFuzzer();
    RequestContentFuzzer();
    GetCloudEnhancementPairFuzzer();
    QueryCloudEnhancementTaskStateFuzzer();
    QueryPhotoStatusFuzzer();
    LogMovingPhotoFuzzer();
    AddAssetVisitCountFuzzer();
    GrantPhotoUriPermissionInnerFuzzer();
    CancelPhotoUriPermissionInnerFuzzer();
    GetResultSetFromDbFuzzer();
    GetResultSetFromPhotosExtendFuzzer();
    GetMovingPhotoDateModifiedFuzzer();
    GetFilePathFromUriFuzzer();
    GetUriFromFilePathFuzzer();
    CloseAssetFuzzer();
    CheckUriPermissionInnerFuzzer();
    GetUrisByOldUrisInnerFuzzer();
    RestoreFuzzer();
    StopRestoreFuzzer();
    GetAssetsFuzzer();
    GetBurstAssetsFuzzer();
    
    MessageParcel dataParcel;
    MessageParcel reply;
    analysisDataControllerService->GetIndexConstructProgress(dataParcel, reply);
    mediaAssetsControllerService->PauseDownloadCloudMedia(dataParcel, reply);
    mediaAssetsControllerService->CancelDownloadCloudMedia(dataParcel, reply);
}

static void Init()
{
    shared_ptr<MediaAssetsControllerService> mediaAssetsControllerService =
        make_shared<MediaAssetsControllerService>();
    OHOS::analysisDataControllerService =
        make_shared<Media::AnalysisData::MediaAnalysisDataControllerService>();
}
} // namespace OHOS

extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv)
{
    OHOS::Init();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    FuzzedDataProvider fdp(data, size);
    if (data == nullptr) {
        return 0;
    }
    OHOS::FDP = &fdp;
    OHOS::MediaAssetsControllerServiceFirstFuzzer();
    OHOS::MediaAssetsControllerServiceSecondFuzzer();
    return 0;
}