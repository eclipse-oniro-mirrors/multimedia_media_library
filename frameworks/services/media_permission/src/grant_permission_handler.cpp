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

#define MLOG_TAG "GrantPermissionHandler"

#include "grant_permission_handler.h"
#include "ipc_skeleton.h"

namespace OHOS::Media {

/**
 * 是否鉴权操作
 */
static bool IsGrantOperation(MediaLibraryCommand &cmd)
{
    return cmd.GetOprnObject() == OperationObject::APP_URI_PERMISSION_INNER;
}

int32_t GrantPermissionHandler::ExecuteCheckPermission(MediaLibraryCommand &cmd, PermParam &permParam)
{
    MEDIA_DEBUG_LOG("GrantPermissionHandler enter ");
    return ConvertPermResult(IsGrantOperation(cmd) && (IPCSkeleton::GetCallingUid() == GRANT_PERMISSION_CALLING_UID ||
        IPCSkeleton::GetCallingUid() == ROOT_UID || IPCSkeleton::GetCallingUid() == SANDBOX_UID));
}

} // namespace name