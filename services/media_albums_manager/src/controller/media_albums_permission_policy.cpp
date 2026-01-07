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
#include "media_albums_controller_service.h"
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
static std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>> mediaAlbumsPermissionPolicy = {
    // ALBUMS_BUSINESS_CODE_START begin
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_HIGH_LIGHT_ALBUMS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SYSTEM_CREATE_ALBUM), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_DELETE_PHOTO_ALBUMS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_COVER_URI), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ALBUM_NAME), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HIGH_LIGHT_USER_ACTION_DATA), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUBTITLE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_ADD_ASSETS), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_REMOVE_ASSETS), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MOVE_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RECOVER_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DELETE_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_COMMIT_MODIFY), { {WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_ADD_ASSETS), { {WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_REMOVE_ASSETS), { {WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_RECOVER_ASSETS), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_COVER_URI), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_PHOTO_ALBUMS), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_HIDDEN_ALBUMS),
        { {SYSTEMAPI_PERM, READ_PERM, PRIVATE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_QUERY_GET_ALBUMS_BY_IDS), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_PHOTO_INDEX), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_HIGHLIGHT_ALBUM_INFO), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_SELECTED_ASSETS), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_RESET_COVER_URI), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUMS), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_PHOTO_ALBUM_ORDER), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_SET_PHOTO_ALBUM_ORDER), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_GET_ALBUMS_BY_IDS), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_PAH_QUERY_PHOTO_ALBUMS), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_HIGHLIGHT_ATTRIBUTE),
        { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_CLONED_ALBUM_URIS), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ALBUMS), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_DELETE_ALBUMS), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_CREATE_ALBUM), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_MOVE_ASSETS), { {SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SMART_MOVE_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSETS), { {READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_UPLOAD_STATUS),
        {{SYSTEMAPI_PERM, WRITE_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_LPATH), { }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ALBUM_BY_BUNDLENAME), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_CREATE_ANALYSIS_ALBUM), { {SYSTEMAPI_PERM, WRITE_PERM} }},
};

static std::unordered_set<uint32_t> mediaAlbumsPermissionDbBypass = {
    static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_SYS_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::ALBUM_GET_ASSETS),
    static_cast<uint32_t>(MediaLibraryBusinessCode::INNER_GET_ASSETS),
};

int32_t MediaAlbumsControllerService::GetPermissionPolicy(
    uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass)
{
    if (mediaAlbumsPermissionDbBypass.find(code) != mediaAlbumsPermissionDbBypass.end()) {
        isBypass = true;
    }
    auto it = mediaAlbumsPermissionPolicy.find(code);
    if (it != mediaAlbumsPermissionPolicy.end()) {
        permissionPolicy = it->second;
        return E_SUCCESS;
    }
    return E_FAIL;
}

} // namespace OHOS::Media