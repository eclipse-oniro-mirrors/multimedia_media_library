/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FRAMEWORKS_ANI_SRC_INCLUDE_ANI_CLASSNAME_H
#define FRAMEWORKS_ANI_SRC_INCLUDE_ANI_CLASSNAME_H

#include <string>

namespace OHOS {
namespace Media {

static const std::string PAH_CLASS_SPACE = "@ohos.file.photoAccessHelper.photoAccessHelper.";
static const std::string UFM_CLASS_SPACE = "@ohos.filemanagement.userFileManager.userFileManager.";

// ETS Class in photoAccessHelper
static const std::string PAH_ANI_CLASS_FILE_ASSET_INFO = PAH_CLASS_SPACE + "FileAssetInfoHandle";
static const std::string PAH_ANI_CLASS_CLOUD_ENHANCEMENT = PAH_CLASS_SPACE + "CloudEnhancement";
static const std::string PAH_ANI_CLASS_CLOUD_ENHANCEMENT_TASK_STATE_HANDLE =
    PAH_CLASS_SPACE + "CloudEnhancementTaskStateHandle";
static const std::string PAH_ANI_CLASS_CLOUD_MEDIA_ASSET_MANAGER =
    PAH_CLASS_SPACE + "CloudMediaAssetManager";
static const std::string PAH_ANI_CLASS_CLOUD_MEDIA_ASSET_STATUS_HANDLE =
    PAH_CLASS_SPACE + "CloudMediaAssetStatusHandle";
static const std::string PAH_ANI_CLASS_FETCH_RESULT_HANDLE = PAH_CLASS_SPACE + "FetchResultHandle";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET_CUSTOM_RECORD_MANAGER =
    PAH_CLASS_SPACE + "PhotoAssetCustomRecordManager";
static const std::string PAH_ANI_CLASS_PHOTO_ALBUM_HANDLE = PAH_CLASS_SPACE + "AlbumHandle";
static const std::string PAH_ANI_CLASS_HIGHLIGHT_ALBUM = PAH_CLASS_SPACE + "HighlightAlbum";
static const std::string PAH_ANI_CLASS_ANALYSIS_ALBUM = PAH_CLASS_SPACE + "AnalysisAlbum";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET = PAH_CLASS_SPACE + "PhotoAsset";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET_HANDLE = PAH_CLASS_SPACE + "PhotoAssetHandle";
static const std::string PAH_ANI_CLASS_PHOTO_ACCESS_HELPER_HANDLE = PAH_CLASS_SPACE + "PhotoAccessHelperHandle";
static const std::string PAH_ANI_CLASS_PHOTO_PROXY_HANDLE = PAH_CLASS_SPACE + "PhotoProxyHandle";
static const std::string PAH_ANI_CLASS_SIZE = PAH_CLASS_SPACE + "SizeImpl";
static const std::string PAH_ANI_CLASS_MOVING_PHOTO_HANDLE = PAH_CLASS_SPACE + "MovingPhotoHandle";
static const std::string PAH_ANI_CLASS_CHANGE_DATA_HANDLE = PAH_CLASS_SPACE + "ChangeDataHandle";
static const std::string PAH_ANI_CLASS_MEDIA_ALBUM_CHANGE_REQUEST = PAH_CLASS_SPACE + "MediaAlbumChangeRequest";
static const std::string PAH_ANI_CLASS_MEDIA_ANALYSIS_ALBUM_CHANGE_REQUEST =
    PAH_CLASS_SPACE + "MediaAnalysisAlbumChangeRequest";
static const std::string PAH_ANI_CLASS_MEDIA_ASSET_CHANGE_REQUEST = PAH_CLASS_SPACE + "MediaAssetChangeRequest";
static const std::string PAH_ANI_CLASS_MEDIA_ASSETS_CHANGE_REQUEST = PAH_CLASS_SPACE + "MediaAssetsChangeRequest";
static const std::string PAH_ANI_CLASS_MEDIA_ASSETS_EDIT_DATA = PAH_CLASS_SPACE + "MediaAssetEditData";
static const std::string PAH_ANI_CLASS_MEDIA_MANAGER = PAH_CLASS_SPACE + "MediaAssetManager";
static const std::string PAH_ANI_CLASS_MEDIA_DATA_HANDLER = PAH_CLASS_SPACE + "MediaAssetDataHandler";
static const std::string PAH_ANI_CLASS_MEDIA_PROGRESS_HANDLER = PAH_CLASS_SPACE + "MediaAssetProgressHandler";
static const std::string PAH_ANI_CLASS_SHARED_PHOTO_ASSET_HANDLE = PAH_CLASS_SPACE + "SharedPhotoAssetHandle";
static const std::string PAH_ANI_CLASS_SHARED_ALBUM_ASSET_HANDLE = PAH_CLASS_SPACE + "SharedAlbumAssetHandle";

// ETS Enum in photoAccessHelper
static const std::string PAH_ANI_CLASS_ENUM_PHOTO_TYPE = PAH_CLASS_SPACE + "PhotoType";
static const std::string PAH_ANI_CLASS_ENUM_PHOTO_SUBTYPE = PAH_CLASS_SPACE + "PhotoSubtype";
static const std::string PAH_ANI_CLASS_ENUM_DYNAMIC_RANGE_TYPE = PAH_CLASS_SPACE + "DynamicRangeType";
static const std::string PAH_ANI_CLASS_ENUM_POSITION_TYPE = PAH_CLASS_SPACE + "PositionType";
static const std::string PAH_ANI_CLASS_ENUM_ALBUM_TYPE = PAH_CLASS_SPACE + "AlbumType";
static const std::string PAH_ANI_CLASS_ENUM_ALBUM_SUBTYPE = PAH_CLASS_SPACE + "AlbumSubtype";
static const std::string PAH_ANI_CLASS_ENUM_NOTIFY_TYPE = PAH_CLASS_SPACE + "NotifyType";
static const std::string PAH_ANI_CLASS_ENUM_MOVING_PHOTO_EFFECT_MODE = PAH_CLASS_SPACE + "MovingPhotoEffectMode";
static const std::string PAH_ANI_CLASS_ENUM_CLOUD_ENHANCEMENT_TASK_STAGE =
    PAH_CLASS_SPACE + "CloudEnhancementTaskStage";
static const std::string PAH_ANI_CLASS_ENUM_RESOURCE_TYPE = PAH_CLASS_SPACE + "ResourceType";
static const std::string PAH_ANI_CLASS_ENUM_IMAGEFILE_TYPE = PAH_CLASS_SPACE + "ImageFileType";
static const std::string PAH_ANI_CLASS_ENUM_COVERURI_SOURCE = PAH_CLASS_SPACE + "CoverUriSource";
static const std::string PAH_ANI_CLASS_ENUM_NOTIFYCHANGE_TYPE = PAH_CLASS_SPACE + "NotifyChangeType";
static const std::string PAH_ANI_CLASS_ENUM_STRONG_ASSOCIATION_TYPE = PAH_CLASS_SPACE + "StrongAssociationType";
static const std::string PAH_ANI_CLASS_ENUM_THUMBNAIL_VISIBILITY_TYPE = PAH_CLASS_SPACE + "ThumbnailVisibility";

// ETS Class in userFileManager
static const std::string UFM_ANI_CLASS_USER_FILE_MANAGER_HANDLE = UFM_CLASS_SPACE + "UserFileManagerHandle";
static const std::string UFM_ANI_CLASS_FETCH_RESULT_HANDLE = UFM_CLASS_SPACE + "FetchResultHandle";
static const std::string UFM_ANI_CLASS_FILE_ASSET = UFM_CLASS_SPACE + "FileAsset";
static const std::string UFM_ANI_CLASS_FILE_ASSET_HANDLE = UFM_CLASS_SPACE + "FileAssetHandle";
static const std::string UFM_ANI_CLASS_PHOTO_ALBUM_HANDLE = UFM_CLASS_SPACE + "AlbumHandle";

static const std::string PAH_ANI_CLASS_ALBUM_CHANGE_INFOS_HANDLE = PAH_CLASS_SPACE + "AlbumChangeInfosHandle";
static const std::string PAH_ANI_CLASS_ALBUM_CHANGE_DATA_HANDLE = PAH_CLASS_SPACE + "AlbumChangeDataHandle";
static const std::string PAH_ANI_CLASS_ALBUM_CHANGE_INFO_HANDLE = PAH_CLASS_SPACE + "AlbumChangeInfoHandle";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFOS_HANDLE =
    PAH_CLASS_SPACE + "PhotoAssetChangeInfosHandle";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_DATA_HANDLE = PAH_CLASS_SPACE + "PhotoAssetChangeDataHandle";
static const std::string PAH_ANI_CLASS_PHOTO_ASSET_CHANGE_INFO_HANDLE = PAH_CLASS_SPACE + "PhotoAssetChangeInfoHandle";
} // namespace Media
} // namespace OHOS
#endif  // FRAMEWORKS_ANI_SRC_INCLUDE_ANI_CLASSNAME_H
