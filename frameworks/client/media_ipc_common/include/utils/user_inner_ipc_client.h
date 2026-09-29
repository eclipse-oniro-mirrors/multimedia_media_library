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
 
#ifndef OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H
#define OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H
 
#include "unified_ipc_client.h"
 
namespace OHOS::Media::IPC {
// UserInnerIPCClient is now a typedef of UnifiedIPCClient.
// Supports injection mode (SetDataShareHelper) and global mode (SA token by userId).
// If a helper is injected, it takes priority over global creation.
using UserInnerIPCClient = UnifiedIPCClient;
}  // namespace OHOS::Media::IPC
#endif  // OHOS_MEDIA_IPC_USER_INNER_IPC_CLIENT_H