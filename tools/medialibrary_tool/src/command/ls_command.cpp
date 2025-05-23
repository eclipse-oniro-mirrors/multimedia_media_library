
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

#include "command/ls_command.h"

#include <sstream>
#include <string>
#include <vector>

#include "constant.h"
#include "medialibrary_errno.h"
#include "utils/mediatool_command_utils.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "mediatool_uri.h"
#include "userfile_client.h"

namespace OHOS {
namespace Media {
namespace MediaTool {

using namespace std;

const int LSCommand::LS_POS_ARG_MIN = 1;
const int LSCommand::LS_POS_ARG_MAX = 1;

bool LSCommand::ParsePositionalArgs(const std::vector<std::string>& positionalArgs)
{
    if (positionalArgs.size() < LS_POS_ARG_MIN || positionalArgs.size() > LS_POS_ARG_MAX) {
        MEDIA_ERR_LOG("ls command positional args size is invalid. size:%{public}zu", positionalArgs.size());
        printf("%s invalid arguments.\n", STR_FAIL.c_str());
        return false;
    }

    inputPath_ = positionalArgs[0];
    inputPath_ = positionalArgs[0];
    return true;
}

bool LSCommand::ParseFlagsArgs(const std::vector<std::string>& flags)
{
    for (const auto& flag : flags) {
        if (flag == "-l") {
            longFormat_ = true;
        } else {
            MEDIA_ERR_LOG("ls command flags args is invalid. flag:%{public}s", flag.c_str());
            printf("%s invalid option: %s.\n", STR_FAIL.c_str(), flag.c_str());
            return false;
        }
    }

    return true;
}

bool LSCommand::ParseArgs(const std::vector<std::string>& commandArgs)
{
    vector<string> positionalArgs;
    vector<string> flags;
    for (size_t i = 0; i < commandArgs.size(); ++i) {
        if (MediaFileUtils::StartsWith(commandArgs[i], "-")) {
            flags.push_back(commandArgs[i]);
        } else {
            positionalArgs.push_back(commandArgs[i]);
        }
    }

    CHECK_AND_RETURN_RET_LOG(ParsePositionalArgs(positionalArgs), false, "Parse positional args failed");
    CHECK_AND_RETURN_RET_LOG(ParseFlagsArgs(flags), false, "Parse flags args failed");

    return true;
}

bool LSCommand::CheckArgs()
{
    if (!longFormat_) {
        MEDIA_ERR_LOG("ls failed: ls command long format is not enabled.");
        printf("%s -l option is required for ls command.\n", STR_FAIL.c_str());
        return false;
    }

    string reformattedPath;
    if (!MediatoolCommandUtils::CheckAndReformatPathParam(inputPath_, reformattedPath)) {
        MEDIA_ERR_LOG("ls failed: path is invalid. path: %{public}s", inputPath_.c_str());
        printf("%s path invalid: %s.\n", STR_FAIL.c_str(), inputPath_.c_str());
        return false;
    }

    lsPath_ = reformattedPath;

    return true;
}

string LSCommand::ToString()
{
    string result = "ls command, long format: ";
    result += longFormat_ ? "true" : "false";
    result += ", path: ";
    result += lsPath_;
    return result;
}

string LSCommand::ErrorCodeToMsg(int32_t errorCode)
{
    switch (errorCode) {
        case E_INVALID_PATH:
        {
            std::ostringstream errMsgOss;
            errMsgOss << "Path error: " << inputPath_;
            return errMsgOss.str();
        }
        default:
            return "Unknown error";
    }
}

void LSCommand::PrintFileInfo(const string& fileInfoJSONString)
{
    nlohmann::json jsonObj = nlohmann::json::parse(fileInfoJSONString);

    for (const auto& file : jsonObj["files"]) {
        std::cout << file["permissions"].get<std::string>() << " ";
        std::cout << file["links"].get<int>() << " ";
        std::cout << file["owner"].get<std::string>() << " ";
        std::cout << file["group"].get<std::string>() << " ";
        std::cout << file["size"].get<long>() << " ";
        std::cout << file["modTime"].get<std::string>() << " ";
        std::cout << file["fileName"].get<std::string>() << "\n";
    }
}

int32_t LSCommand::Execute()
{
    std::string lsUriStr = TOOL_LS_PHOTO;
    Uri lsUri(lsUriStr);
    DataShare::DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_FILE_PATH, lsPath_);
    MEDIA_INFO_LOG("mediatool ls execute, %{public}s", this->ToString().c_str());
    string outString;
    auto ret = UserFileClient::InsertExt(lsUri, values, outString);
    if (ret != E_OK) {
        MEDIA_ERR_LOG("mediatool ls execute failed. ret:%{public}d", ret);
        return ret;
    }
    PrintFileInfo(outString);
    return E_OK;
}

int32_t LSCommand::Start(const ExecEnv &env)
{
    CHECK_AND_RETURN_RET_LOG(ParseArgs(env.commandArgs), Media::E_ERR, "Parse args failed");
    CHECK_AND_RETURN_RET_LOG(CheckArgs(), Media::E_ERR, "Check args failed");
    int32_t ret = Execute();
    if (ret != E_OK) {
        printf("%s %s\n", STR_FAIL.c_str(), ErrorCodeToMsg(ret).c_str());
    }
    return ret;
}

}
}
}