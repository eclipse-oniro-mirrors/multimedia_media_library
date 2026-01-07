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

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "media_assets_controller_service.h"
#include "media_permission_policy_type.h"
#include "medialibrary_business_code.h"

using namespace std;

namespace OHOS::Media {
/* Map the API code to the required permissions.
 * (1)If the API code is not in the mapping table, the API cannot be executed.
 *    If vector is empty, no permission is required, for example, the close API.
 * (2)The outer nested vector indicates the OR relationship. That is,
 *    if any set of permissions is satisfied, it is considered to have the permission to execute the API.
 * (3)A single inner vector indicates the AND relationship. That is,
 *    an API can be executed only when all permissions are met.
 * For example, the {{SYSTEMAPI_PERM, WRITE_PERM}, {CLOUDFILE_SYNC}},
 *   must have the system and write permissions, or must have the device-cloud permission.
 */
static std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>> mediaAssetsPermissionPolicy = {
    // MEDIA_BUSINESS_CODE_START begin
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_FORM_INFO), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REMOVE_GALLERY_FORM_INFO), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_FORM_INFO), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_GALLERY_FORM_INFO), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CLONE_ASSET), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::REVERT_TO_ORIGINAL), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN), {}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::UPDATE_GALLERY_FORM_INFO), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SUBMIT_CLOUD_ENHANCEMENT_TASKS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PRIORITIZE_CLOUD_ENHANCEMENT_TASK),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_CLOUD_ENHANCEMENT_TASKS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_ALL_CLOUD_ENHANCEMENT_TASKS),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_START_THUMBNAIL_CREATION_TASK),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_STOP_THUMBNAIL_CREATION_TASK), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLOUD_ENHANCEMENT_PAIR), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_CLOUD_ENHANCEMENT_TASK_STATE),
        {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SYNC_CLOUD_ENHANCEMENT_TASK_STATUS),
        {{SYSTEMAPI_PERM, READ_PERM}}},
    // ASSETS_BUSINESS_CODE_START begin
    {static_cast<uint32_t>(MediaLibraryBusinessCode::HEIF_TRANSCODING_CHECK), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::COMMIT_EDITED_ASSET), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_CREATE_ASSET_FOR_APP), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_MODE),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ASSET_FOR_APP_WITH_ALBUM),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CREATE_TMP_DUPLICATE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYS_TRASH_PHOTOS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_TRASH_PHOTOS), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_PHOTOS_COMPLETED), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTOS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_FAVORITE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_HIDDEN), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_USER_COMMENT), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_LOCATION), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_TITLE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_EDIT_DATA), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SUBMIT_CACHE_FOR_PICKER),
        {{SYSTEMAPI_PERM, WRITE_PERM, ACCESS_MEDIALIB_THUMB_DB_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_CREATE_ASSET), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_ADD_IMAGE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_CAMERA_SHOT_KEY), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SAVE_CAMERA_PHOTO), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DISCARD_CAMERA_PHOTO), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_EFFECT_MODE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_ORIENTATION), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_VIDEO_ENHANCEMENT_ATTR), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUPPORTED_WATERMARK_TYPE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HAS_APPLINK), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_APPLINK), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_PUBLIC_SET_TITLE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_PENDING), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_FAVORITE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_SET_USER_COMMENT), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_HIDDEN), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_FAVORITE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_RECENT_SHOW),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_BATCH_SET_USER_COMMENT),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_ADD_ASSET_VISIT_COUNT), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ASSET), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CANCEL_PHOTO_URI_PERMISSION), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GRANT_PHOTO_URI_PERMISSION),
        {{SYSTEMINNERAPI_PERM, WRITE_PERM}, {SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_PHOTO_URI_PERMISSION), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHECK_AUDIO_URI_PERMISSION), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URIS_BY_OLD_URIS), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CHANGE_SCAN_ASSET), {{READ_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URI_PERMISSION), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GRANT_PHOTO_URIS_PERMISSION), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PHOTO_URI_PERMISSION), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::START_DOWNLOAD_CLOUDMEDIA), {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_DOWNLOAD_CLOUDMEDIA), {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_DOWNLOAD_CLOUDMEDIA), {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::RETAIN_CLOUDMEDIA_ASSET), {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_IS_EDITED), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_REQUEST_EDIT_DATA), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_EDIT_DATA), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_ASSET_STATUS), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::START_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::RESUME_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAUSE_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CANCEL_BATCH_DOWNLOAD_CLOUD_RESOURCES),
        {{SYSTEMAPI_PERM, CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_STATUS),
        {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_GET_CLOUDMEDIA_BATCH_RESOURCES_COUNT),
        {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REQUEST_CONTENT), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_PHOTO_STATUS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_QUERY_PHOTO_STATUS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_MOVING_PHOTO), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::LOG_CINEMATIC_VIDEO), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_ALL_DUPLICATE_ASSETS), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::FIND_DUPLICATE_ASSETS_TO_DELETE), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_DB_EXTEND), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_RESULT_SET_FROM_PHOTOS_EXTEND), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_MOVING_PHOTO_DATE_MODIFIED), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_FILEPATH_FROM_URI), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_URI_FROM_FILEPATH), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CLOSE_ASSET), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CUSTOM_RESTORE_CANCEL), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CANCEL_PROCESS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_RETAIN_CLOUDMEDIA_ASSET), {{CLOUDFILE_SYNC}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_SET_COMPOSITE_DISPLAY_MODE),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_IMAGE), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ACQUIRE_DEBUG_DATABASE), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::RELEASE_DEBUG_DATABASE), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_TRASH_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_DELETE_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CONVERT_FORMAT_MIME_TYPE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_FUSION_ASSET_INFO), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_DELETE_LOCAL_ASSETS_WITH_URI),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ASSET_CHANGE_DELETE_CLOUD_ASSETS_WITH_URI),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_OPEN_ASSET_COMPRESS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_NOTIFY_ASSET_SENDED), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSET_COMPRESS_VERSION), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_COMPRESS_ASSET_SIZE), {{READ_PERM}}},
};

static std::unordered_set<uint32_t> mediaAssetsPermissionDbBypass = {
    static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::GET_BURST_ASSETS),
};

int32_t MediaAssetsControllerService::GetPermissionPolicy(
    uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass)
{
    if (mediaAssetsPermissionDbBypass.find(code) != mediaAssetsPermissionDbBypass.end()) {
        isBypass = true;
    }
    auto it = mediaAssetsPermissionPolicy.find(code);
    if (it != mediaAssetsPermissionPolicy.end()) {
        permissionPolicy = it->second;
        return E_SUCCESS;
    }
    return E_FAIL;
}

} // namespace OHOS::Media