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
#include "command/command.h"

#include "command/list_command_v10.h"
#include "command/ls_command.h"
#include "command/recv_command_v10.h"
#include "command/send_command_v10.h"
#include "command/delete_command_v10.h"
#include "command/query_command_v10.h"
#include "option_args.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
std::unique_ptr<Command> Command::Create(const ExecEnv &env)
{
    if (env.optArgs.cmdType == OptCmdType::TYPE_LIST) {
        return std::make_unique<ListCommandV10>();
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_RECV) {
        return std::make_unique<RecvCommandV10>();
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_SEND) {
        return std::make_unique<SendCommandV10>();
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_DELETE) {
        return std::make_unique<DeleteCommandV10>();
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_QUERY) {
        return std::make_unique<QueryCommandV10>();
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_LS) {
        return std::make_unique<LSCommand>();
    }
    return nullptr;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
