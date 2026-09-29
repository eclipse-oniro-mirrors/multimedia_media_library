/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "media_manage_share_permission_check.h"
#include "permission_utils.h"

namespace OHOS::Media {
int32_t ManageSharePermissionCheck::CheckPermission(uint32_t businessCode, const PermissionHeaderReq &data)
{
    MEDIA_INFO_LOG("ManageSharePermissionCheck enter, API code=%{public}d", businessCode);
    CHECK_AND_RETURN_RET_LOG(PermissionUtils::CheckCallerPermission(PERM_MANAGE_SHARE_PHOTO), E_PERMISSION_DENIED,
        "ManageSharePermissionCheck failed: lack of %{public}s permission", PERM_MANAGE_SHARE_PHOTO.c_str());
    return E_SUCCESS;
}

bool ManageSharePermissionCheck::CheckOpenPermission(const std::shared_ptr<FileAsset> fileAsset,
    const std::string &openMode)
{
    CHECK_AND_RETURN_RET_LOG(fileAsset != nullptr, false, "fileAsset is nullptr");
    // 非共享资产无需校验
    CHECK_AND_RETURN_RET(fileAsset->GetIsShared() == static_cast<int32_t>(PhotoSharedType::SHARED), true);
    // 共享相册资产支持读
    CHECK_AND_RETURN_RET(openMode != MEDIA_FILEMODE_READONLY, true);
    // 不支持写
    return false;
}
} // namespace OHOS::Media
