/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#define MLOG_TAG "PermissionHandlerCommon"

#include "permission_common.h"

#include <cstdlib>

#include "medialibrary_bundle_manager.h"
#include "medialibrary_operation.h"
#include "media_column.h"
#include "parameters.h"
#include "permission_utils.h"
#include "mediatool_uri.h"

using namespace std;

namespace OHOS::Media {

string GetClientAppId()
{
    string bundleName = MediaLibraryBundleManager::GetInstance()->GetClientBundleName();
    return PermissionUtils::GetAppIdByBundleName(bundleName);
}

bool IsMediatoolOperation(MediaLibraryCommand &cmd)
{
    return cmd.GetOprnObject() == OperationObject::TOOL_PHOTO || cmd.GetOprnObject() == OperationObject::TOOL_AUDIO ||
        cmd.GetOprnObject() == OperationObject::TOOL_ALBUM || cmd.GetOprnType() == Media::OperationType::DELETE_TOOL ||
        (cmd.GetOprnObject() == OperationObject::MISCELLANEOUS &&
        cmd.GetOprnType() == Media::OperationType::QUERY_ACTIVE_USER_ID);
}

static bool IsHdcShellMediatoolCommand(MediaLibraryCommand &cmd, const std::string &openFileMode)
{
    return cmd.GetOprnType() == Media::OperationType::TOOL_QUERY_BY_DISPLAY_NAME ||
        cmd.GetOprnType() == Media::OperationType::DELETE_TOOL ||
        cmd.GetOprnType() == Media::OperationType::UPDATE ||
        cmd.GetOprnType() == Media::OperationType::ALBUM_DELETE_ASSETS ||
        (cmd.GetOprnType() == Media::OperationType::DELETE &&
        cmd.GetOprnObject() == OperationObject::FILESYSTEM_AUDIO) ||
        (cmd.GetOprnType() == Media::OperationType::OPEN && openFileMode.find('w') == string::npos) ||
        cmd.GetOprnType() == Media::OperationType::LS_MEDIA_FILES ||
        cmd.GetOprnType() == Media::OperationType::QUERY_ACTIVE_USER_ID;
}

static bool IsMediatoolSendOperation(MediaLibraryCommand &cmd, const std::string &openFileMode)
{
    const auto object = cmd.GetOprnObject();
    const bool isPhotoOrAudio = object == OperationObject::TOOL_PHOTO || object == OperationObject::TOOL_AUDIO ||
        object == OperationObject::FILESYSTEM_PHOTO || object == OperationObject::FILESYSTEM_AUDIO;
    std::string operationUri = cmd.GetUri().ToString();
    operationUri = operationUri.substr(0, operationUri.find('?'));
    if (cmd.GetOprnType() == OperationType::OPEN) {
        const bool isMovingPhoto = object == OperationObject::PAH_MOVING_PHOTO &&
            operationUri.rfind(PhotoColumn::PHOTO_URI_PREFIX, 0) == 0;
        return (isPhotoOrAudio || isMovingPhoto) &&
            cmd.GetQuerySetParam(CONST_IS_TOOL_OPEN) == CONST_TOOL_OPEN_TRUE &&
            openFileMode.find('w') != std::string::npos;
    }
    if (!isPhotoOrAudio) {
        return false;
    }
    if (cmd.GetOprnType() == OperationType::CREATE) {
        return operationUri == CONST_TOOL_CREATE_PHOTO || operationUri == CONST_TOOL_CREATE_AUDIO;
    }
    if (cmd.GetOprnType() == OperationType::CLOSE) {
        return operationUri == CONST_TOOL_CLOSE_PHOTO || operationUri == CONST_TOOL_CLOSE_AUDIO;
    }
    return false;
}

bool IsDeveloperMediaTool(MediaLibraryCommand &cmd, const std::string &openFileMode)
{
    if (IsMediatoolSendOperation(cmd, openFileMode)) {
        if (PermissionUtils::IsRootShell()) {
            return true;
        }
        const bool allowed = PermissionUtils::IsHdcShell() &&
            OHOS::system::GetBoolParameter("const.security.developermode.state", true);
        if (!allowed) {
            MEDIA_ERR_LOG("Mediatool send denied: non-shell caller or developer mode disabled, "
                "object:%{public}d, operation:%{public}d",
                static_cast<int32_t>(cmd.GetOprnObject()), static_cast<int32_t>(cmd.GetOprnType()));
        }
        return allowed;
    }
    if (!PermissionUtils::IsRootShell() &&
        !(PermissionUtils::IsHdcShell() && IsHdcShellMediatoolCommand(cmd, openFileMode))) {
        MEDIA_ERR_LOG("Mediatool permission check failed: target is not root");
        return false;
    }
    if (!OHOS::system::GetBoolParameter("const.security.developermode.state", true)) {
        MEDIA_ERR_LOG("Mediatool permission check failed: target is not in developer mode");
        return false;
    }
    return true;
}

int32_t ConvertPermResult(bool isPermSuccess)
{
    return isPermSuccess ? E_SUCCESS : E_PERMISSION_DENIED;
}

} // namespace name

