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

#define MLOG_TAG "SystemApiCheckHandler"

#include "system_api_check_handler.h"

#include "delete_permanently_operations_uri.h"
#include "medialibrary_operation.h"
#include "permission_utils.h"
#include "userfilemgr_uri.h"
#include "album_operation_uri.h"
#include "data_secondary_directory_uri.h"

namespace OHOS::Media {

static const std::unordered_set<std::string> SYSTEM_API_URIS = {
    CONST_PAH_DELETE_PHOTO_ALBUM,
    CONST_PAH_DELETE_PHOTOS,
    CONST_PAH_RECOVER_PHOTOS,
    CONST_PAH_SYS_CREATE_PHOTO,
    CONST_PAH_SYS_TRASH_PHOTO,
    CONST_UFM_DELETE_PHOTO_ALBUM,
    CONST_PAH_REMOVE_FORM_MAP,
    CONST_PAH_CREATE_APP_URI_PERMISSION,
    CONST_PAH_SET_LOCATION,
    CONST_URI_DELETE_PHOTOS_COMPLETED,
    CONST_PAH_DISMISS_ASSET,
    CONST_PAH_GROUP_ANAALBUM_DISMISS,
    CONST_UFM_QUERY_HIDDEN_ALBUM,
    CONST_PAH_QUERY_HIDDEN_ALBUM,
    CONST_PAH_HIDE_PHOTOS,
    CONST_PAH_BATCH_UPDATE_OWNER_ALBUM_ID,
    CONST_QUERY_TAB_OLD_PHOTO,
};

int32_t SystemApiCheckHandler::ExecuteCheckPermission(MediaLibraryCommand &cmd, PermParam &permParam)
{
    std::string uri = cmd.GetUriStringWithoutSegment();
    OperationObject obj = cmd.GetOprnObject();
    OperationType type = cmd.GetOprnType();
    if (SYSTEM_API_URIS.find(uri) == SYSTEM_API_URIS.end()) {
        return E_SUCCESS;
    }
    if (!PermissionUtils::IsSystemApp()) {
        MEDIA_ERR_LOG("not system app, uri:%{public}s obj:%{public}d, type:%{public}d", uri.c_str(), obj, type);
        return -E_CHECK_SYSTEMAPP_FAIL;
    }
    return E_SUCCESS;
}

} // namespace OHOS::Media