/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#ifndef FRAMEWORKS_MEDIATOOLS_EXEC_ENV_H_
#define FRAMEWORKS_MEDIATOOLS_EXEC_ENV_H_
#include <string>
#include <vector>

#include "option_args.h"
#include "userfile_manager_types.h"
#include "utils/constant_utils.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
struct SendParam {
    bool isCreateThumbSyncInSend = false;
    bool isRemoveOriginFileInSend = true;
    bool isFile = true;
    bool isRecognizeMovingPhotoInSend = false;
    std::string sendPath; // real path for sending
};

struct RecvParam {
    bool isRecvAll = false;
    bool isRecvPathDir = false;
    std::string recvTarget;
    std::string recvPath; // real path for the recvPath in optArgs
};

struct ListParam {
    bool isListAll = false;
    std::string listUri;
};

struct DeleteParam {
    bool isDeleteAll = false;
    bool isOnlyDeleteDb = false;
    std::string deleteUri;
};
struct QueryParam {
    std::string displayName;
    bool pathFlag = false;
    bool uriFlag = false;
};

struct ExecEnv {
    OptionArgs optArgs;
    std::vector<std::string> args;
    std::vector<std::string> commandArgs;
    SendParam sendParam;
    RecvParam recvParam;
    ListParam listParam;
    DeleteParam deleteParam;
    QueryParam queryParam;
    std::string workPath; // current work path
    bool isRoot = false;
    [[nodiscard]] std::string ToStr() const;
};
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
#endif // FRAMEWORKS_MEDIATOOLS_EXEC_ENV_H_
