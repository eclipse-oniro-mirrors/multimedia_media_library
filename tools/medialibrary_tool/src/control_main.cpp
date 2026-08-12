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

#define MLOG_TAG "Mediatool"

#include "control_main.h"

#include <array>
#include <unistd.h>

#include "command/command.h"
#include "command_line.h"
#include "constant.h"
#include "directory_ex.h"
#include "exec_env.h"
#include "get_self_permissions.h"
#include "iservice_registry.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "mimetype_utils.h"
#include "system_ability_definition.h"
#include "userfile_client_ex.h"

namespace OHOS {
namespace Media {
namespace MediaTool {

constexpr int32_t ROOT_UID = 0;

static int32_t Init(ExecEnv &env, const std::vector<std::string> &args)
{
    env.args.assign(args.begin(), args.end());
    std::array<char, PATH_MAX> buffer {0};
    getcwd(buffer.data(), PATH_MAX);
    env.workPath.append(buffer.data());
    env.workPath = IncludeTrailingPathDelimiter(env.workPath);
    env.isRoot = getuid() == ROOT_UID;
    return Media::E_OK;
}

int32_t ControlMain::Main(const std::vector<std::string> &args)
{
    ExecEnv env;
    int32_t res = Init(env, args);
    if (res != Media::E_OK) {
        MEDIA_ERR_LOG("Init failed, res: %{public}d", res);
        return res;
    }
    res = CommandLine::Parser(env);
    if (res != Media::E_OK) {
        MEDIA_ERR_LOG("Parse args failed, res: %{public}d", res);
        return res;
    }
    do {
        res = UserFileClientEx::Init();
        if (res != Media::E_OK) {
            MEDIA_ERR_LOG("UserfileClient init failed, res: %{public}d", res);
            break;
        }
        std::unique_ptr<Command> cmd = Command::Create(env);
        if (cmd == nullptr) {
            res = Media::E_ERR;
            MEDIA_ERR_LOG("Create command failed, res: %{public}d", res);
            break;
        }
        MEDIA_INFO_LOG("Mediatool command prepare done, start to execute env:{%{public}s}",
            env.ToStr().c_str());
        res = cmd->Start(env);
        if (res != Media::E_OK) {
            MEDIA_ERR_LOG("Mediatool main error, res: %{public}d", res);
            break;
        }
    } while (0);
    return res;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
