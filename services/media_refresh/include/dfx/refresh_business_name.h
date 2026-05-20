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

#ifndef OHOS_MEDIA_REFRESH_BUSINESS_NAME_H
#define OHOS_MEDIA_REFRESH_BUSINESS_NAME_H

#include <string>
#include "parcel.h"

namespace OHOS {
namespace Media::AccurateRefresh {

static const std::string CLONE_SINGLE_ASSET_BUSSINESS_NAME = "CloneSingleAsset";

static const std::string CONVERT_FORMAT_ASSET_BUSSINESS_NAME = "ConvertFormatAsset";

static const std::string CREATE_PHOTO_TABLE_BUSSINESS_NAME = "CreatePhotoAlbum";

static const std::string DELETE_PHOTO_ALBUMS_BUSSINESS_NAME = "DeletePhotoAlbums";

static const std::string RENAME_USER_ALBUM_BUSSINESS_NAME = "RenameUserAlbum";

static const std::string UPDATE_PHOTO_ALBUM_BUSSINESS_NAME = "UpdatePhotoAlbum";

static const std::string RECOVER_ASSETS_BUSSINESS_NAME = "RecoverAssets";

static const std::string DELETE_PHOTOS_BUSSINESS_NAME = "DeletePhotos";

static const std::string DELETE_PERMANENTLY_BUSSINESS_NAME = "DeletePermanently";

static const std::string TRASH_PHOTOS_BUSSINESS_NAME = "TrashPhotos";

static const std::string SAVE_CAMERA_PHOTO_BUSSINESS_NAME = "SaveCameraPhoto";

static const std::string HIDE_PHOTOS_BUSSINESS_NAME = "HidePhotos";

static const std::string SET_ASSETS_FAVORITE_BUSSINESS_NAME = "SetAssetsFavorite";

static const std::string SET_ASSETS_USER_COMMENT_BUSSINESS_NAME = "SetAssetsUserComment";

static const std::string UPDATE_SYSTEM_ASSET_BUSSINESS_NAME = "UpdateSystemAsset";

static const std::string MOVE_ASSETS_BUSSINESS_NAME = "MoveAssets";

static const std::string UPDATE_FILE_ASSTE_BUSSINESS_NAME = "UpdateFileAsset";

static const std::string UPDATE_OWNER_ALBUMID_BUSSINESS_NAME = "UpdateOwnerAlbumId";

static const std::string DELETE_PTP_ALBUM_BUSSINESS_NAME = "DeletePtpAlbum";

static const std::string COMMIT_EDITE_ASSET_BUSSINESS_NAME = "commitEditedAsset";

static const std::string UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME = "UpdateTrashedAssetOnAlbum";

static const std::string CUSTOM_RESTORE_BUSSINESS_NAME = "CustomRestore";

static const std::string REMOTE_ASSETS_BUSSINESS_NAME = "RemoveAssets";

static const std::string SUBMIT_CLOUD_ENHANCEMENT_TASKS_BUSSINESS_NAME = "SubmitCloudEnhancementTasks";

static const std::string CANCELALL_CLOUDE_ENHANCEMENT_BUSSINESS_NAME = "CancelAllCloudEnhancementTasks";

static const std::string DEAL_WITH_SUCCESSED_BUSSINESS_NAME = "DealWithSuccessedTask";

static const std::string DEAL_WITH_FAILED_BUSSINESS_NAME = "DealWithFailedTask";

static const std::string SCAN_FILE_BUSSINESS_NAME = "ScanFile";

static const std::string THUMBNAIL_GENERATION_BUSSINESS_NAME = "ThumbnailGeneration";

static const std::string UPDATE_POSITION_BUSSINESS_NAME = "UpdatePosition";

static const std::string ORDER_SINGLE_ALBUM_BUSSINESS_NAME = "OrderSingleAlbum";

static const std::string GET_ASSETS_BUSSINESS_NAME = "getAssets";

static const std::string GET_SELECTED_ASSETS_BUSSINESS_NAME = "getSelectedAssets";

static const std::string DEAL_ALBUMS_BUSSINESS_NAME = "getAlbums";

static const std::string DELETE_PHOTOS_COMPLETED_BUSSINESS_NAME = "DeletePhotosCompleted";

static const std::string YUV_READY_BUSSINESS_NAME = "YuvReady";

static const std::string CREATE_CAMERA_FILE_FD = "CreateCameraFileId";

} // namespace Media
} // namespace OHOS

#endif