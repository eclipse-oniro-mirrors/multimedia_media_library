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
#include <string>
#include "media_permission_check.h"
#include "media_system_api_permission_check.h"
#include "media_private_permission_check.h"
#include "media_read_permission_check.h"
#include "media_write_permission_check.h"

using namespace std;
namespace OHOS::Media {
// API whitelist for deprecated read or write permission
// Allow access to these interfaces using the deprecated permissions
std::unordered_set<uint32_t> PermissionCheck::deprecatedReadPermissionSet = {
    // assets start
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED),
    static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA),
    static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_FILEPATH_FROM_URI),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URI_FROM_FILEPATH),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_MOVING_PHOTO_DATE_MODIFIED),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS_EXTEND),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB_EXTEND),
    // albums start
    static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS),
};

std::unordered_set<uint32_t> PermissionCheck::deprecatedWritePermissionSet = {
    // business start
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK),
    // assets start
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_TRASH_PHOTOS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTOS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_PHOTOS_COMPLETED),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_FAVORITE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_USER_COMMENT),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_CAMERA_SHOT_KEY),
    static_cast<uint32_t>(MediaLibraryBusinessCode::DISCARD_CAMERA_PHOTO),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_EFFECT_MODE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_ORIENTATION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HAS_APPLINK),
    static_cast<uint32_t>(MediaLibraryBusinessCode::SET_APPLINK),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ASSET),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CLOSE_ASSET),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL),
    // albums start
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SMART_MOVE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM),
    static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REMOVE_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_RECOVER_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI),
};
// API whitelist for check grant operation permission
std::unordered_set<uint32_t> PermissionCheck::grantOperationPermissionSet = {
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CANCEL_PHOTO_URI_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_AUDIO_URI_PERMISSION),
};
// API whitelist for check media tool operation permission
std::unordered_set<uint32_t> PermissionCheck::mediaToolOperationPermissionSet = {
    static_cast<uint32_t>(MediaLibraryBusinessCode::MEDIA_BUSINESS_CODE_START)};
}  // namespace OHOS::Media
