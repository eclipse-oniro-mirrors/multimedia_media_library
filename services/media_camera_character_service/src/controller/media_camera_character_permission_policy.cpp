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

#define MLOG_TAG "MediaCameraCharacterService"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "media_camera_character_controller_service.h"
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
static std::unordered_map<uint32_t, std::vector<std::vector<PermissionType>>> mediaCameraCharacterPermissionPolicy = {
    // CAMERA_CHARACTER_BUSINESS_CODE_START begin
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_INNER_ADD_PROCESS_VIDEO), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_MAM_CANCEL_PROCESS), {{READ_PERM}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_PROCESS_VIDEO), {{}}},
    {static_cast<uint32_t>(MediaLibraryBusinessCode::CAMERA_DEFINE_GET_PROGRESS_CALLBACK), {{}}},
};

int32_t MediaCameraCharacterControllerService::GetPermissionPolicy(
    uint32_t code, std::vector<std::vector<PermissionType>> &permissionPolicy, bool &isBypass)
{
    auto it = mediaCameraCharacterPermissionPolicy.find(code);
    if (it != mediaCameraCharacterPermissionPolicy.end()) {
        permissionPolicy = it->second;
        return E_SUCCESS;
    }
    return E_FAIL;
}
} // namespace OHOS::Media