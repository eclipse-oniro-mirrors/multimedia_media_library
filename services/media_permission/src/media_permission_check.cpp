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
#define MLOG_TAG "MediaPermissionCheck"
#include <string>
#include "media_permission_check.h"
#include "media_composite_permission_check.h"
#include "media_system_api_permission_check.h"
#include "media_system_innerapi_permission_check.h"
#include "media_private_permission_check.h"
#include "media_read_permission_check.h"
#include "media_write_permission_check.h"
#include "media_cloudfile_sync_permission_check.h"
#include "media_cloud_permission_check.h"
#include "media_file_utils.h"
#include "permission_common.h"
#include "medialibrary_data_manager.h"
#include "file_asset.h"
#include "parameters.h"
#include "media_db_permission_check.h"
#include "media_access_medialib_thumb_db_permission_check.h"

using namespace std;
using namespace OHOS::Media;
using namespace OHOS::DataShare;
using namespace OHOS::Security::AccessToken;
namespace OHOS::Media {
std::unordered_map<PermissionType, std::shared_ptr<PermissionCheck>> PermissionCheck::permissionRegistry = {
    {SYSTEMAPI_PERM, std::make_shared<SystemApiPermissionCheck>()},
    {PRIVATE_PERM, std::make_shared<PrivatePermissionCheck>()},
    {READ_PERM, std::make_shared<ReadCompositePermCheck>()},
    {WRITE_PERM, std::make_shared<WriteCompositePermCheck>()},
    {CLOUDFILE_SYNC, std::make_shared<CloudFileSyncPermissionCheck>()},
    {CLOUD_READ, std::make_shared<CloudReadPermissionCheck>()},
    {CLOUD_WRITE, std::make_shared<CloudWritePermissionCheck>()},
    {SYSTEMINNERAPI_PERM, std::make_shared<SystemInnerApiPermissionCheck>()},
    {ACCESS_MEDIALIB_THUMB_DB_PERM, std::make_shared<AccessMedialibThumbDbPermissionCheck>()},
};

static void CollectPermissionInfo(MediaLibraryCommand &cmd, const string &mode,
    const bool permGranted, PermissionUsedType type)
{
    if ((cmd.GetOprnObject() == OperationObject::FILESYSTEM_PHOTO) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL) ||
        (cmd.GetOprnObject() == OperationObject::THUMBNAIL_ASTC)) {
        CHECK_AND_EXECUTE(mode.find("r") == string::npos,
            PermissionUtils::CollectPermissionInfo(PERM_READ_IMAGEVIDEO, permGranted, type));
        CHECK_AND_EXECUTE(mode.find("w") == string::npos,
            PermissionUtils::CollectPermissionInfo(PERM_WRITE_IMAGEVIDEO, permGranted, type));
    }
}

int32_t PermissionCheck::VerifyOpenFilePermissions(uint32_t businessCode, const PermissionHeaderReq &data)
{
    std::string openUri = data.getOpenUri();
    std::string mode = data.getOpenMode();
    std::vector<PermissionType> openFileToPermissions;
    openFileToPermissions.push_back(SYSTEMAPI_PERM);
    Uri uri(openUri);
#ifdef MEDIALIBRARY_COMPATIBILITY
    string realUriStr = MediaFileUtils::GetRealUriFromVirtualUri(uri.ToString());
    Uri realUri(realUriStr);
    MediaLibraryCommand command(realUri, Media::OperationType::OPEN);
#else
    MediaLibraryCommand command(uri, Media::OperationType::OPEN);
#endif
    transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    if (mode.find('r') != string::npos) {
        openFileToPermissions.push_back(READ_PERM);
    }
    if (mode.find('w') != string::npos) {
        openFileToPermissions.push_back(WRITE_PERM);
    }

    auto permissionCheck = std::make_shared<SinglePermissionCheck>();
    for (const auto& permKey : openFileToPermissions) {
        auto check = PermissionCheck::permissionRegistry[permKey];
        if (check) {
            permissionCheck->AddCheck(check);
        }
    }
    auto ret = permissionCheck->CheckPermission(businessCode, data);
    if (ret != E_SUCCESS && ret != E_DOUBLE_CHECK) {
        MEDIA_ERR_LOG("Permission Denied! err = %{public}d", ret);
        CollectPermissionInfo(command, mode, false,
            PermissionUsedTypeValue::SECURITY_COMPONENT_TYPE);
        return E_PERMISSION_DENIED;
    }
    MEDIA_INFO_LOG("VerifyPermissions API code=%{public}d success", businessCode);
    return ret;
}

bool PermissionCheck::GetNeedPermissionCheck() const
{
    return needPermissionCheck_;
}
void PermissionCheck::SetNeedPermissionCheck(bool needPermissionCheck)
{
    needPermissionCheck_ = needPermissionCheck;
}

std::shared_ptr<PermissionCheck> PermissionCheck::BuildPermissionCheckChain(uint32_t businessCode,
    const PermissionHeaderReq &data)
{
    std::vector<std::vector<PermissionType>> ruleLists = data.getPermissionPolicy();

    bool needPermissionCheck = true;
    if (ruleLists.empty()) {
        needPermissionCheck = false;
    } else {
        needPermissionCheck = false;
        for (const auto& ruleList : ruleLists) {
            if (!ruleList.empty()) {
                needPermissionCheck = true;
                break;
            }
        }
    }

    auto compositePermChain = std::make_shared<CompositePermissionCheck>();
    compositePermChain->SetNeedPermissionCheck(needPermissionCheck);
    if (!needPermissionCheck) {
        return compositePermChain;
    }

    for (const auto& ruleList : ruleLists) {
        if (ruleList.empty()) {
            continue;
        }
        auto singlePermCheck = std::make_shared<SinglePermissionCheck>();
        for (const auto& permKey : ruleList) {
            auto check = PermissionCheck::permissionRegistry[permKey];
            if (check) {
                singlePermCheck->AddCheck(check);
            }
        }
        compositePermChain->AddCheck(singlePermCheck);
    }
    MEDIA_DEBUG_LOG("BuildPermissionCheckChain: %{public}d end", businessCode);
    return compositePermChain;
}

int32_t PermissionCheck::VerifyPermissions(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_DEBUG_LOG("VerifyPermissions API code=%{public}d", businessCode);
    if (businessCode == static_cast<uint32_t>(MediaLibraryBusinessCode::PAH_OPEN)) {
        auto ret = VerifyOpenFilePermissions(businessCode, data);
        MEDIA_INFO_LOG("Verify OpenFile Permissions ret=%{public}d, API code=%{public}d", ret, businessCode);
        CHECK_AND_RETURN_RET((ret == E_SUCCESS), E_PERMISSION_DENIED);
        return E_SUCCESS;
    }
    
    auto permissionCheck = BuildPermissionCheckChain(businessCode, data);
    if (permissionCheck == nullptr) {
        MEDIA_DEBUG_LOG("%{public}s permissionCheck is nullptr", __func__);
        return E_PERMISSION_DENIED;
    }
    if (!permissionCheck->GetNeedPermissionCheck()) {
        MEDIA_INFO_LOG("No Need Permission Check, API code=%{public}d", businessCode);
        return E_SUCCESS;
    }
    auto ret = permissionCheck->CheckPermission(businessCode, data);
    if (ret != E_SUCCESS && ret != E_DOUBLE_CHECK) {
        MEDIA_DEBUG_LOG("checkPermission API code=%{public}d fail, err=%{public}d", businessCode, ret);
        // if bypass success, return E_PERMISSION_DB_BYPASS
        CHECK_AND_RETURN_RET(!data.getIsDBBypass(), DbPermissionCheck::CheckDBPermissionBypass(businessCode, data));
    } else {
        MEDIA_INFO_LOG("VerifyPermissions API code=%{public}d success", businessCode);
    }
    return ret;
}

} // namespace OHOS::Media
