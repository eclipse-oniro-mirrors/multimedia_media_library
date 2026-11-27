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
#include "exec_env.h"
#include "option_args.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
static inline void AppendStr(std::string &str, const std::string paramName, const int param)
{
    if (str.empty()) {
        str.append(paramName + ":[");
        str.append(to_string(param));
        str.append("]");
    } else {
        str.append(", " + paramName + ":[");
        str.append(to_string(param));
        str.append("]");
    }
}

static inline void AppendStr(std::string &str, const std::string paramName, const std::string param)
{
    if (str.empty()) {
        str.append(paramName + ":[");
        str.append(param);
        str.append("]");
    } else {
        str.append(", " + paramName + ":[");
        str.append(param);
        str.append("]");
    }
}

std::string ExecEnv::ToStr() const
{
    std::string str;
    if (optArgs.cmdType == OptCmdType::TYPE_LIST) {
        AppendStr(str, "listUri", listParam.listUri);
        AppendStr(str, "isListAll", listParam.isListAll);
    }
    if (optArgs.cmdType == OptCmdType::TYPE_RECV) {
        AppendStr(str, "recvTarget", recvParam.recvTarget);
        AppendStr(str, "recvPath", recvParam.recvPath);
        AppendStr(str, "isRecvAll", recvParam.isRecvAll);
        AppendStr(str, "isRecvPathDir", recvParam.isRecvPathDir);
    }
    if (optArgs.cmdType == OptCmdType::TYPE_SEND) {
        AppendStr(str, "sendPath", sendParam.sendPath);
        AppendStr(str, "isFile", sendParam.isFile);
        AppendStr(str, "isRemoveOriginFileInSend", sendParam.isRemoveOriginFileInSend);
        AppendStr(str, "isRemoveOriginFileInSend", sendParam.isRemoveOriginFileInSend);
        AppendStr(str, "isRecognizeMovingPhotoInSend", sendParam.isRecognizeMovingPhotoInSend);
    }
    if (optArgs.cmdType == OptCmdType::TYPE_DELETE) {
        AppendStr(str, "isOnlyDeleteDb", deleteParam.isOnlyDeleteDb);
    }
    AppendStr(str, "workPath", workPath);
    AppendStr(str, "isRoot", isRoot);
    return str;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
