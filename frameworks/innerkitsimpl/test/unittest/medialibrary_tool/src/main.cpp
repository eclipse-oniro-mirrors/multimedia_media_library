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
#include <unistd.h>
#include <vector>

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::MediaTool;

constexpr int32_t SHELL_UID = 2000;
int main(int argc, char *argv[])
{
    int32_t id = getuid();
    if (id != 0 && id != SHELL_UID) {
        MEDIA_ERR_LOG("Invalid uid");
        return 0;
    }
    std::vector<std::string> args;
    for (int i = 0; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }
    return ControlMain::Main(args);
}

