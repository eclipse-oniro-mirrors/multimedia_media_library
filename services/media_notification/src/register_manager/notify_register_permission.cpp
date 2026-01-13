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

#include "notify_register_permission.h"
#include "db_permission_handler.h"
#include "permission_common.h"
#include "permission_utils.h"
#include "medialibrary_errno.h"
#include "media_log.h"
#include "media_change_info.h"

namespace OHOS::Media {
using namespace Notification;
const std::string SINGLE_PATH = "file://media/Photo/";
const std::string SINGLE_NAME = "/IMG_0000000000_001/single.jpg";

int32_t NotifyRegisterPermission::BasicPermissionCheck()
{
    std::string perm = PERM_READ_IMAGEVIDEO;
    int32_t err = PermissionUtils::CheckCallerPermission(perm) ? E_SUCCESS : E_PERMISSION_DENIED;
    if (err < 0) {
        return err;
    }
    return E_OK;
}

int32_t NotifyRegisterPermission::TranshPermissionCheck()
{
    if (!PermissionUtils::IsSystemApp()) {
        MEDIA_ERR_LOG("the caller is not system app");
        return -E_CHECK_SYSTEMAPP_FAIL;
    }
    std::string perm = PERM_READ_IMAGEVIDEO;
    int32_t err = PermissionUtils::CheckCallerPermission(perm) ? E_SUCCESS : E_PERMISSION_DENIED;
    if (err < 0) {
        MEDIA_ERR_LOG("the caller does not have read permission");
        return err;
    }
    return E_OK;
}

int32_t NotifyRegisterPermission::HiddenPermissionCheck()
{
    if (!PermissionUtils::IsSystemApp()) {
        MEDIA_ERR_LOG("the caller is not system app");
        return -E_CHECK_SYSTEMAPP_FAIL;
    }
    std::vector<std::string> perms;
    perms.push_back(PERM_READ_IMAGEVIDEO);
    perms.push_back(PERM_MANAGE_PRIVATE_PHOTOS);
    int32_t err = PermissionUtils::CheckCallerPermission(perms) ? E_SUCCESS : E_PERMISSION_DENIED;
    if (err < 0) {
        MEDIA_ERR_LOG("the caller does not have read permission or private permission");
        return err;
    }
    return E_OK;
}

int32_t NotifyRegisterPermission::SinglePermissionCheck(const NotifyUriType &registerUriType,
    const std::string& singleId)
{
    int32_t err = BasicPermissionCheck();
    if (err < 0 && registerUriType == NotifyUriType::SINGLE_PHOTO_URI) {
        PermParam permParam = {
            .isWrite = false,
            .isOpenFile = true,
            .openFileNode = "r"
        };
        Uri realUri(SINGLE_PATH + singleId + SINGLE_NAME);
        MediaLibraryCommand command(realUri, Media::OperationType::OPEN);
        DbPermissionHandler permissionHandler;
        err = permissionHandler.CheckPermission(command, permParam);
    }
    if (err < 0) {
        MEDIA_ERR_LOG("the caller does not have read permission");
        return err;
    }
    return E_OK;
}

int32_t NotifyRegisterPermission::ExecuteCheckPermission(const NotifyUriType &registerUriType)
{
    int32_t ret = -1;
    if (registerUriType == NotifyUriType::PHOTO_URI || registerUriType == NotifyUriType::PHOTO_ALBUM_URI
        || registerUriType == NotifyUriType::BATCH_DOWNLOAD_PROGRESS_URI) {
        ret = BasicPermissionCheck();
    } else if (registerUriType == NotifyUriType::HIDDEN_PHOTO_URI ||
               registerUriType == NotifyUriType::HIDDEN_ALBUM_URI) {
        ret = HiddenPermissionCheck();
    } else if (registerUriType == NotifyUriType::TRASH_PHOTO_URI || registerUriType == NotifyUriType::TRASH_ALBUM_URI) {
        ret = TranshPermissionCheck();
    } else if (registerUriType == NotifyUriType::USER_DEFINE_NOTIFY_URI) {
        MEDIA_INFO_LOG("Permission validation is not applicable to USER_DEFINE_NOTIFY_URI.");
        return E_OK;
    } else if (registerUriType == NotifyUriType::SINGLE_PHOTO_URI || registerUriType == SINGLE_PHOTO_ALBUM_URI) {
        MEDIA_INFO_LOG("Single permission check in full check.");
        return E_OK;
    }
    CHECK_AND_RETURN_RET_LOG(ret == E_OK, ret, "Permission validation failed");
    MEDIA_INFO_LOG("Permission validation succeeded");
    return E_OK;
}

bool NotifyRegisterPermission::isSystemApp()
{
    return PermissionUtils::IsSystemApp();
}

} // namespace OHOS::Media