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

#ifndef OHOS_MEDIA_ASSETS_CONTROLLER_SERVICE_H
#define OHOS_MEDIA_ASSETS_CONTROLLER_SERVICE_H

#include "message_parcel.h"
#include "medialibrary_business_code.h"
#include "i_media_controller_service.h"
#include "user_define_ipc.h"
#include "medialibrary_errno.h"
#include "media_assets_delete_service.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))
class EXPORT MediaAssetsControllerService : public IPC::IMediaControllerService {
public:
    EXPORT int32_t RemoveFormInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RemoveGalleryFormInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SaveFormInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SaveGalleryFormInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CommitEditedAssetOpen(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CommitEditedAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SysTrashPhotos(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t TrashPhotos(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeletePhotos(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeletePhotosCompleted(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetFavorite(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetHidden(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetUserComment(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetLocation(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetTitle(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSetEditData(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeSubmitCache(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeCreateAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AssetChangeAddImage(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CameraInnerAddImage(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetCameraShotKey(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SaveCameraPhoto(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DiscardCameraPhoto(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetEffectMode(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetOrientation(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetVideoEnhancementAttr(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetSupportedWatermarkType(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetHasAppLink(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAppLink(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAssets(MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context);
    EXPORT int32_t GetBurstAssets(MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context);
    EXPORT int32_t GetAllDuplicateAssets(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetDuplicateAssetsToDelete(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PublicCreateAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SystemCreateAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PublicCreateAssetForApp(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SystemCreateAssetForApp(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CreateAssetForAppWithAlbum(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetTitle(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetPending(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetsFavorite(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetsHiddenStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetsRecentShowStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetAssetsUserComment(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AddAssetVisitCount(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CloneAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RevertToOriginal(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t UpdateGalleryFormInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SubmitCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PrioritizeCloudEnhancementTask(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelAllCloudEnhancementTasks(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StartDownloadCloudMedia(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PauseDownloadCloudMedia(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelDownloadCloudMedia(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RetainCloudMediaAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetCloudMediaAssetStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StartBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t ResumeBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t PauseBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelBatchDownloadCloudResources(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetCloudMediaBatchDownloadResourcesStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetCloudMediaBatchDownloadResourcesCount(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetEditData(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RequestEditData(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t IsEdited(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAlbumsByAlbumIds(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GrantPhotoUriPermission(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GrantPhotoUrisPermission(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GrantPhotoUriPermissionInner(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelPhotoUriPermission(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelPhotoUriPermissionInner(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StartThumbnailCreationTask(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StopThumbnailCreationTask(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t RequestContent(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetCloudEnhancementPair(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t QueryCloudEnhancementTaskState(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SyncCloudEnhancementTaskStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t QueryPhotoStatus(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t LogMovingPhoto(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t LogCinematicvideo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t ConvertFormat(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CheckMimeType(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CreateTmpCompatibleDup(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetResultSetFromDb(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetResultSetFromPhotosExtend(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetMovingPhotoDateModified(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetFilePathFromUri(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetUriFromFilePath(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CloseAsset(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CheckUriPermissionInner(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetUrisByOldUrisInner(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t Restore(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StopRestore(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t CancelRequest(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t HeifTranscodingCheck(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t SetCompositeDisplayMode(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t AcquireDebugDatabase(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t ReleaseDebugDatabase(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t StartChangeScanAssetInner(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetFusionAssetsInfo(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeleteLocalAssetsWithUri(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t DeleteCloudAssetsWithUri(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t OpenAssetCompress(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t NotifyAssetSended(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetAssetCompressVersion(MessageParcel &data, MessageParcel &reply);
    EXPORT int32_t GetCompressAssetSize(MessageParcel &data, MessageParcel &reply);

public:
    virtual ~MediaAssetsControllerService() = default;
    bool Accept(uint32_t code) override;
    int32_t OnRemoteRequest(
        uint32_t code, MessageParcel &data, MessageParcel &reply, OHOS::Media::IPC::IPCContext &context) override;
    int32_t GetPermissionPolicy(
        uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass) override;

private:
    Common::MediaAssetsDeleteService mediaAssetsDeleteService_;
};
} // namespace OHOS::Media
#endif  // OHOS_MEDIA_ASSETS_CONTROLLER_SERVICE_H