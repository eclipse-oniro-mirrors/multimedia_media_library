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

#include "dfx_deprecated_perm_usage.h"
#include "media_read_permission_check.h"
#include "media_file_utils.h"
#include "ipc_skeleton.h"
#ifdef MEDIALIBRARY_SECURITY_OPEN
#include "sec_comp_kit.h"
#endif
#include "parameters.h"
#include "media_app_uri_permission_column.h"

namespace OHOS::Media {
static const int32_t GRANT_PERMISSION_CALLING_UID = 5523; // foundation调用方
static const int32_t ROOT_UID = 0;
static const int32_t HDC_SHELL_UID = 2000;
static const int32_t SANDBOX_UID = 3076;
static const int32_t CLOUDSERVICE_UID = 1009;
static int32_t AcrossLocalAccountsPermCheck(const PermissionHeaderReq &data);

ReadCompositePermCheck::ReadCompositePermCheck()
{
    auto readPrivilegePermCheck = std::make_shared<ReadPrivilegePermCheck>();
    AddCheck(readPrivilegePermCheck);

    auto dbReadPermCheck = std::make_shared<DbReadPermCheck>();
    AddCheck(dbReadPermCheck);

    auto grantReadPermCheck = std::make_shared<GrantReadPermCheck>();
    AddCheck(grantReadPermCheck);

    auto mediaToolReadPermCheck = std::make_shared<MediaToolReadPermCheck>();
    AddCheck(mediaToolReadPermCheck);

    auto deprecatedReadPermCheck = std::make_shared<DeprecatedReadPermCheck>();
    AddCheck(deprecatedReadPermCheck);
}

void ReadCompositePermCheck::AddCheck(std::shared_ptr<PermissionCheck> check)
{
    std::lock_guard<std::mutex> lock(mutex_);
    readChecks_.push_back(check);
}

int32_t ReadCompositePermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("ReadCompositePermCheck enter, API code=%{public}d", businessCode);
    int32_t err = AcrossLocalAccountsPermCheck(data);
    if (err != E_SUCCESS) {
        return E_PERMISSION_DENIED;
    }
    if (isCalledBySelfPtr() == E_OK) {
        MEDIA_INFO_LOG("ReadCompositePermCheck isCalledBySelfPtr check success");
        return E_SUCCESS;
    }

    for (const auto& check : readChecks_) {
        int32_t ret = check->CheckPermission(businessCode, data);
        if (ret == E_SUCCESS || ret == E_DOUBLE_CHECK) {
            return ret;
        }
    }
    return E_PERMISSION_DENIED;
}

int32_t ReadPrivilegePermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("ReadPrivilegePermCheck enter, API code=%{public}d", businessCode);
    return PermissionUtils::CheckCallerPermission(PERM_READ_IMAGEVIDEO) ? E_SUCCESS : E_PERMISSION_DENIED;
}

int32_t DbReadPermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("DbReadPermCheck enter, API code=%{public}d", businessCode);
    int32_t permissionType = 0;
    auto ret = GetPermissionType(businessCode, data, permissionType);
    if (ret != E_SUCCESS) {
        return ret;
    }
    return (AppUriPermissionColumn::PERMISSION_TYPES_ALL.count(permissionType)) ? E_DOUBLE_CHECK : E_PERMISSION_DENIED;
}

int32_t GrantReadPermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("GrantReadPermCheck enter, API code=%{public}d", businessCode);
    if (PermissionCheck::grantOperationPermissionSet.find(businessCode) ==
        PermissionCheck::grantOperationPermissionSet.end()) {
        MEDIA_INFO_LOG("Not grant operation");
        return E_PERMISSION_DENIED;
    }
    int32_t uid = getCallingUidPtr();
    if (uid == GRANT_PERMISSION_CALLING_UID || uid == ROOT_UID || uid == SANDBOX_UID || uid == CLOUDSERVICE_UID) {
        MEDIA_INFO_LOG("GrantReadPermCheck callingUid check success");
        return E_SUCCESS;
    }
    MEDIA_INFO_LOG("GrantReadPermCheck callingUid check fail");
    return E_PERMISSION_DENIED;
}

int32_t MediaToolReadPermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("MediaToolReadPermCheck enter, API code=%{public}d", businessCode);
    if (PermissionCheck::mediaToolOperationPermissionSet.find(businessCode) ==
        PermissionCheck::mediaToolOperationPermissionSet.end()) {
        MEDIA_INFO_LOG("Not media tool operation");
        return E_PERMISSION_DENIED;
    }
    if (getCallingUidPtr() != ROOT_UID && getCallingUidPtr() != HDC_SHELL_UID) {
        MEDIA_ERR_LOG("Mediatool permission check failed: target is not root");
        return E_PERMISSION_DENIED;
    }
    if (!OHOS::system::GetBoolParameter("const.security.developermode.state", true)) {
        MEDIA_ERR_LOG("Mediatool permission check failed: target is not in developer mode");
        return E_PERMISSION_DENIED;
    }
    return E_SUCCESS;
}

int32_t DeprecatedReadPermCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("DeprecatedReadPermCheck enter, API code=%{public}d", businessCode);
    if (PermissionCheck::deprecatedReadPermissionSet.find(businessCode) ==
        PermissionCheck::deprecatedReadPermissionSet.end()) {
        MEDIA_INFO_LOG("Unable to use deprecated read permission");
        return E_PERMISSION_DENIED;
    }
    bool ret = PermissionUtils::CheckCallerPermission(PERMISSION_NAME_READ_MEDIA);
    CHECK_AND_EXECUTE(!ret, DfxDeprecatedPermUsage::Record(businessCode, 0));
    return ret ? E_SUCCESS : E_PERMISSION_DENIED;
}

static int32_t AcrossLocalAccountsPermCheck(const PermissionHeaderReq &data)
{
    int32_t userId = data.getUserId();
    if (userId == -1) {
        return E_SUCCESS;
    }
    std::vector<std::string> perms;
    perms.push_back(PERM_INTERACT_ACROSS_LOCAL_ACCOUNTS);
    return PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
}

} // namespace OHOS::Media
