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

inline const std::string CLONE_SINGLE_ASSET_BUSSINESS_NAME = "CloneSingleAsset";

inline const std::string CONVERT_FORMAT_ASSET_BUSSINESS_NAME = "ConvertFormatAsset";

inline const std::string CREATE_PHOTO_TABLE_BUSSINESS_NAME = "CreatePhotoAlbum";

inline const std::string DELETE_PHOTO_ALBUMS_BUSSINESS_NAME = "DeletePhotoAlbums";

inline const std::string RENAME_USER_ALBUM_BUSSINESS_NAME = "RenameUserAlbum";

inline const std::string UPDATE_PHOTO_ALBUM_BUSSINESS_NAME = "UpdatePhotoAlbum";

inline const std::string RECOVER_ASSETS_BUSSINESS_NAME = "RecoverAssets";

inline const std::string DELETE_PHOTOS_BUSSINESS_NAME = "DeletePhotos";

inline const std::string DELETE_PERMANENTLY_BUSSINESS_NAME = "DeletePermanently";

inline const std::string TRASH_PHOTOS_BUSSINESS_NAME = "TrashPhotos";

inline const std::string SAVE_CAMERA_PHOTO_BUSSINESS_NAME = "SaveCameraPhoto";

inline const std::string HIDE_PHOTOS_BUSSINESS_NAME = "HidePhotos";

inline const std::string SET_ASSETS_FAVORITE_BUSSINESS_NAME = "SetAssetsFavorite";

inline const std::string SET_ASSETS_USER_COMMENT_BUSSINESS_NAME = "SetAssetsUserComment";

inline const std::string UPDATE_SYSTEM_ASSET_BUSSINESS_NAME = "UpdateSystemAsset";

inline const std::string MOVE_ASSETS_BUSSINESS_NAME = "MoveAssets";

inline const std::string UPDATE_FILE_ASSTE_BUSSINESS_NAME = "UpdateFileAsset";

inline const std::string UPDATE_OWNER_ALBUMID_BUSSINESS_NAME = "UpdateOwnerAlbumId";

inline const std::string DELETE_PTP_ALBUM_BUSSINESS_NAME = "DeletePtpAlbum";

inline const std::string COMMIT_EDITE_ASSET_BUSSINESS_NAME = "commitEditedAsset";

inline const std::string UPDATE_TRASHED_ASSETONALBUM_BUSSINESS_NAME = "UpdateTrashedAssetOnAlbum";

inline const std::string CUSTOM_RESTORE_BUSSINESS_NAME = "CustomRestore";

inline const std::string REMOTE_ASSETS_BUSSINESS_NAME = "RemoveAssets";

inline const std::string SUBMIT_CLOUD_ENHANCEMENT_TASKS_BUSSINESS_NAME = "SubmitCloudEnhancementTasks";

inline const std::string CANCELALL_CLOUDE_ENHANCEMENT_BUSSINESS_NAME = "CancelAllCloudEnhancementTasks";

inline const std::string DEAL_WITH_SUCCESSED_BUSSINESS_NAME = "DealWithSuccessedTask";

inline const std::string DEAL_WITH_FAILED_BUSSINESS_NAME = "DealWithFailedTask";

inline const std::string SCAN_FILE_BUSSINESS_NAME = "ScanFile";

inline const std::string THUMBNAIL_GENERATION_BUSSINESS_NAME = "ThumbnailGeneration";

inline const std::string UPDATE_POSITION_BUSSINESS_NAME = "UpdatePosition";

inline const std::string ORDER_SINGLE_ALBUM_BUSSINESS_NAME = "OrderSingleAlbum";

inline const std::string GET_ASSETS_BUSSINESS_NAME = "getAssets";

inline const std::string GET_SELECTED_ASSETS_BUSSINESS_NAME = "getSelectedAssets";

inline const std::string DEAL_ALBUMS_BUSSINESS_NAME = "getAlbums";

inline const std::string DELETE_PHOTOS_COMPLETED_BUSSINESS_NAME = "DeletePhotosCompleted";

inline const std::string YUV_READY_BUSSINESS_NAME = "YuvReady";

inline const std::string CREATE_CAMERA_FILE_FD = "CreateCameraFileId";

inline const std::string SET_SHARE_ALBUM_NAME_BUSSINESS_NAME = "SetShareAlbumName";

inline const std::string DELETE_SHARE_PHOTO_ALBUMS_BUSSINESS_NAME = "DeleteSharePhotoAlbums";

inline const std::string DELETE_MEMBER_SHARE_ALBUM_BUSSINESS_NAME = "DeleteMemberShareAlbum";

inline const std::string SHARE_MEMBER_CHANGE_BUSSINESS_NAME = "ShareMemberChange";

} // namespace Media
} // namespace OHOS

#endif