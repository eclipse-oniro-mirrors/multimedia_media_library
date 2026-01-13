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
#include <unordered_map>
#include <unordered_set>
#include <vector>
 
#include "media_analysis_data_controller_service.h"
#include "media_permission_policy_type.h"
#include "medialibrary_business_code.h"
 
using namespace std;
 
namespace OHOS::Media::AnalysisData {
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
static std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>> mediaAnalysisDataPermissionPolicy = {
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ASSET_ANALYSIS_DATA), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_INDEX_CONSTRUCT_PROGRESS), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::QUERY_START_ASSET_ANALYSIS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_ORDER_POSITION),
        {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_ORDER_POSITION), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_RELATIONSHIP), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_GET_RELATIONSHIP), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_ANALYSIS_PROCESS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_FACE_ID), { {SYSTEMAPI_PERM, READ_PERM} }},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::GET_HIGHLIGHT_ALBUM_INFO), {{SYSTEMAPI_PERM, READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_HIGH_LIGHT_USER_ACTION_DATA), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::SET_SUBTITLE), {{WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::DELETE_HIGH_LIGHT_ALBUMS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_IS_ME), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_SET_DISPLAY_LEVEL), {{SYSTEMAPI_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS_ASSETS), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_MERGE_ALBUM), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_PLACE_BEFORE), {{SYSTEMAPI_PERM, WRITE_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CHANGE_REQUEST_DISMISS), {{SYSTEMAPI_PERM}}},
};
 
 
int32_t MediaAnalysisDataControllerService::GetPermissionPolicy(
    uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass)
{
    auto it = mediaAnalysisDataPermissionPolicy.find(code);
    if (it != mediaAnalysisDataPermissionPolicy.end()) {
        permissionPolicy = it->second;
        return E_SUCCESS;
    }
    return E_FAIL;
}
 
} // namespace OHOS::Media::AnalysisData