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
#include "media_log.h"
#include <string>
#include <sstream>
#include <unistd.h>
#include <vector>
#include <cstdlib>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::MediaTool;

constexpr int32_t SHELL_UID = 2000;
constexpr int32_t ROOT_UID = 0;

static string BuildCommandLine(std::vector<std::string> args)
{
    std::ostringstream commandLine;
    for (size_t i = 0; i < args.size(); ++i) {
        commandLine << args[i];
        if (i != args.size() - 1) {
            commandLine << " ";
        }
    }
    return commandLine.str();
}

int main(int argc, char *argv[])
{
    int32_t id = getuid();
    if (id != ROOT_UID && id != SHELL_UID) {
        MEDIA_ERR_LOG("Invalid uid");
        //手动刷新
        fflush(stdout);
        fflush(stderr);
        _exit(0);
    }
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }

    MEDIA_INFO_LOG("mediatool main start: %{public}s", BuildCommandLine(args).c_str());
    int32_t ret = ControlMain::Main(args);
    // 确保所有输出都被刷新
    fflush(stdout);
    fflush(stderr);

    _exit(ret);
}

