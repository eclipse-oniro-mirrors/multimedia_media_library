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
#include "command_line.h"

#include <cstddef>
#include <string>

#include "constant.h"
#include "directory_ex.h"
#include "exec_env.h"
#include "file_ex.h"
#include "media_column.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "option_args.h"
#include "userfile_client_ex.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
constexpr char CHAR_PATH_DELIMITER = '/';
constexpr int MEDIATOOL_ARG_MIN = 3;
constexpr int MEDIATOOL_ARG_CMD = 1;
constexpr int MEDIATOOL_ARG_FIRST = 2;
constexpr int MEDIATOOL_ARG_SECOND = 3;
const std::string OPT_STR_SEND = "send";
const std::string OPT_STR_RECV = "recv";
const std::string OPT_STR_LIST = "list";
const std::string OPT_STR_DELETE = "delete";
const std::string OPT_STR_QUERY = "query";
const std::string OPT_STR_LS = "ls";
const std::string OPT_STR_ALL = "all";

const std::string SEND_CREATE_THUMBNAIL_SYNC = "-ts";
const std::string SEND_CREATE_THUMBNAIL_ASYNC = "-tas";
const std::string SEND_CREATE_REMOVE_ORIGIN_FILE = "-rf";
const std::string SEND_CREATE_UNREMOVE_ORIGIN_FILE = "-urf";
const std::string SEND_RECOGNIZE_MOVING_PHOTO = "-im";

const std::string DELETE_ONLY_DATABASE = "-db";

static void ShowUsage(bool isRoot)
{
    std::string str;
    str.append("usage:\n");
    str.append("  send file from path to medialibrary\n");
    str.append("    command: mediatool send <file-or-dir-path> [-ts] [-tas] [-rf] [-urf] [-im]\n");
    if (isRoot) {
        str.append("  list media assets in medialibrary\n");
        str.append("    command: mediatool list <media-uri>\n");
    }
    str.append("  receive file from medialibrary to path\n");
    str.append("    command: mediatool recv <src-media> <dest-path>\n");
    str.append("  delete media assets in medialibrary\n");
    str.append("    command: mediatool delete <media-uri>\n");
    str.append("  query path or uri by displayname in medialibrary\n");
    str.append("    command: mediatool query <display-name> [-p] [-u]\n");
    str.append("  execute ls -l in system media directory\n");
    str.append("    command: mediatool ls -l <media-directory>\n");

    printf("%s", str.c_str());
}

static inline bool IsDirPath(const std::string path)
{
    if (path.empty()) {
        return false;
    }
    string subName;
    string::size_type delimiterPos = path.rfind(CHAR_PATH_DELIMITER);
    if (delimiterPos == std::string::npos) {
        subName = path;
    } else {
        subName = path.substr(delimiterPos + 1);
    }
    if (subName.find('.') == std::string::npos || subName == "." || subName == "..") {
        return true;
    } else {
        return false;
    }
}

static bool CheckRecvPath(ExecEnv &env)
{
    std::string path;
    if (IsDirPath(env.recvParam.recvPath)) {
        env.recvParam.isRecvPathDir = true;
        path = env.recvParam.recvPath;
    } else {
        env.recvParam.isRecvPathDir = false;
        path = ExtractFilePath(env.recvParam.recvPath);
    }

    if (!MediaFileUtils::IsDirectory(path)) {
        ForceCreateDirectory(path);
    }
    if (!MediaFileUtils::IsDirectory(path)) {
        MEDIA_ERR_LOG("recv path issue. path: %{public}s", path.c_str());
        printf("%s path issue. path:%s\n", STR_FAIL.c_str(), path.c_str());
        return false;
    }
    if (!env.recvParam.isRecvPathDir) {
        if (FileExists(env.recvParam.recvPath)) {
            MEDIA_ERR_LOG("recv file has exist. file: %{public}s", env.recvParam.recvPath.c_str());
            printf("%s file has exist. file:%s\n", STR_FAIL.c_str(), env.recvParam.recvPath.c_str());
            return false;
        }
    } else {
        env.recvParam.recvPath = IncludeTrailingPathDelimiter(env.recvParam.recvPath);
    }
    return true;
}

static bool CheckList(ExecEnv &env)
{
    if (env.optArgs.uri == OPT_STR_ALL) {
        env.listParam.isListAll = true;
        env.listParam.listUri.clear();
    } else if (!env.optArgs.uri.empty()) {
        std::string tableName = UserFileClientEx::GetTableNameByUri(env.optArgs.uri);
        if (tableName.empty()) {
            printf("%s uri invalid. uri:%s\n", STR_FAIL.c_str(), env.optArgs.uri.c_str());
            return false;
        }
        env.listParam.listUri = env.optArgs.uri;
    } else {
        printf("%s input uri incorrect.\n", STR_FAIL.c_str());
        return false;
    }
    return true;
}

static bool CheckRecv(ExecEnv &env)
{
    if (env.optArgs.uri == OPT_STR_ALL) {
        env.recvParam.recvTarget.clear();
        env.recvParam.isRecvAll = true;
    } else if (!env.optArgs.uri.empty()) {
        env.recvParam.recvTarget = env.optArgs.uri;
        env.recvParam.isRecvAll = false;
    } else {
        MEDIA_ERR_LOG("recv target is empty");
        printf("%s recv target is empty.\n", STR_FAIL.c_str());
        return false;
    }
    if (env.optArgs.recvPath.empty()) {
        MEDIA_ERR_LOG("recv path empty");
        printf("%s recv path empty.\n", STR_FAIL.c_str());
        return false;
    }
    if (env.optArgs.recvPath.find(CHAR_PATH_DELIMITER) == 0) {
        env.recvParam.recvPath = env.optArgs.recvPath;
    } else {
        env.recvParam.recvPath = env.workPath + env.optArgs.recvPath;
    }
    if (!CheckRecvPath(env)) {
        return false;
    }
    return true;
}

static void CheckExtraArgsInSend(ExecEnv &env)
{
    for (size_t i = 0; i < env.optArgs.extraArgs.size(); i++) {
        string param = env.optArgs.extraArgs[i];
        if (param == SEND_CREATE_THUMBNAIL_SYNC) {
            env.sendParam.isCreateThumbSyncInSend = true;
        }
        if (param == SEND_CREATE_THUMBNAIL_ASYNC) {
            env.sendParam.isCreateThumbSyncInSend = false;
        }
        if (param == SEND_CREATE_REMOVE_ORIGIN_FILE) {
            env.sendParam.isRemoveOriginFileInSend = true;
        }
        if (param == SEND_CREATE_UNREMOVE_ORIGIN_FILE) {
            env.sendParam.isRemoveOriginFileInSend = false;
        }
        if (param == SEND_RECOGNIZE_MOVING_PHOTO) {
            env.sendParam.isRecognizeMovingPhotoInSend = true;
        }
    }
}

static bool CheckSend(ExecEnv &env)
{
    if (env.optArgs.path.empty()) {
        printf("%s path empty.\n", STR_FAIL.c_str());
        return false;
    }
    if (!PathToRealPath(env.optArgs.path, env.sendParam.sendPath)) {
        printf("%s path issue. errno:%d, path:%s.\n", STR_FAIL.c_str(), errno, env.optArgs.path.c_str());
        return false;
    }
    if (!MediaFileUtils::IsDirectory(env.sendParam.sendPath)) {
        env.sendParam.isFile = true;
    } else if (MediaFileUtils::IsDirectory(env.sendParam.sendPath)) {
        env.sendParam.sendPath = IncludeTrailingPathDelimiter(env.sendParam.sendPath);
        env.sendParam.isFile = false;
    } else {
        printf("%s path issue. not file and not directory. path:%s.\n", STR_FAIL.c_str(),
            env.sendParam.sendPath.c_str());
        return false;
    }
    CheckExtraArgsInSend(env);
    if (env.sendParam.isRecognizeMovingPhotoInSend && env.sendParam.isFile) {
        printf("%s moving photo recognition requires a directory. path:%s\n", STR_FAIL.c_str(),
            env.sendParam.sendPath.c_str());
        return false;
    }
    return true;
}

static bool CheckDelete(ExecEnv &env)
{
    if (env.optArgs.uri == OPT_STR_ALL) {
        env.deleteParam.deleteUri.clear();
        env.deleteParam.isDeleteAll = true;
        for (size_t i = 0; i < env.optArgs.extraArgs.size(); i++) {
            string param = env.optArgs.extraArgs[i];
            if (param == DELETE_ONLY_DATABASE) {
                env.deleteParam.isOnlyDeleteDb = true;
            }
        }
    } else if (!env.optArgs.uri.empty()) {
        std::string tableName = UserFileClientEx::GetTableNameByUri(env.optArgs.uri);
        if (tableName.empty()) {
            printf("%s uri invalid. uri:%s\n", STR_FAIL.c_str(), env.optArgs.uri.c_str());
            return false;
        }
        env.deleteParam.isDeleteAll = false;
        env.deleteParam.deleteUri = env.optArgs.uri;
    } else {
        printf("%s input uri incorrect.\n", STR_FAIL.c_str());
        return false;
    }

    return true;
}

static bool CheckQuery(ExecEnv &env)
{
    for (size_t i = 0; i < env.optArgs.extraArgs.size(); i++) {
        string param = env.optArgs.extraArgs[i];
        if (param == "-p") {
            env.queryParam.pathFlag = true;
        } else if (param == "-u") {
            env.queryParam.uriFlag = true;
        } else if (env.queryParam.displayName.empty()) {
            env.queryParam.displayName = param;
        }
    }

    if ((env.queryParam.pathFlag && env.queryParam.uriFlag) || env.queryParam.displayName.empty()) {
        printf("The command is not a valid query command. See 'mediatool -h'\n");
        ShowUsage(env.isRoot);
        return false;
    }
    if (!env.queryParam.pathFlag && !env.queryParam.uriFlag) {
        env.queryParam.pathFlag = true;
    }

    return true;
}

static bool Check(ExecEnv &env)
{
    if (env.optArgs.cmdType == OptCmdType::TYPE_SEND) {
        return CheckSend(env);
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_RECV) {
        return CheckRecv(env);
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_LIST) {
        return CheckList(env);
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_DELETE) {
        return CheckDelete(env);
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_QUERY) {
        return CheckQuery(env);
    }
    if (env.optArgs.cmdType == OptCmdType::TYPE_LS) {
        // ls command performs check later
        return true;
    }
    return false;
}

static void PutExtraString(ExecEnv &env, size_t start, size_t end = 0)
{
    if (end == 0) {
        for (size_t i = start; i < env.args.size(); i++) {
            env.optArgs.extraArgs.push_back(env.args[i]);
        }
    } else {
        for (size_t i = start; i <= std::min(env.args.size(), end); i++) {
            env.optArgs.extraArgs.push_back(env.args[i]);
        }
    }
}

int32_t CommandLine::Parser(ExecEnv &env)
{
    if (env.args.size() < MEDIATOOL_ARG_MIN) {
        ShowUsage(env.isRoot);
        return Media::E_ERR;
    }
    env.commandArgs = std::vector<std::string>(env.args.begin() + MEDIATOOL_ARG_MIN - 1, env.args.end());
    std::string cmd = env.args[MEDIATOOL_ARG_CMD];
    std::string optFirst = (env.args.size() > MEDIATOOL_ARG_FIRST) ? env.args[MEDIATOOL_ARG_FIRST] : "";
    std::string optSecond = (env.args.size() > MEDIATOOL_ARG_SECOND) ? env.args[MEDIATOOL_ARG_SECOND] : "";
    if (cmd == OPT_STR_SEND) {
        env.optArgs.cmdType = OptCmdType::TYPE_SEND;
        env.optArgs.path = optFirst;
        if (env.args.size() > MEDIATOOL_ARG_SECOND) {
            PutExtraString(env, MEDIATOOL_ARG_SECOND);
        }
    } else if (cmd == OPT_STR_RECV) {
        env.optArgs.cmdType = OptCmdType::TYPE_RECV;
        env.optArgs.uri = optFirst;
        env.optArgs.recvPath = optSecond;
    } else if (cmd == OPT_STR_LIST) {
        env.optArgs.cmdType = OptCmdType::TYPE_LIST;
        env.optArgs.uri = optFirst;
    } else if (cmd == OPT_STR_DELETE) {
        env.optArgs.uri = optFirst;
        env.optArgs.cmdType = OptCmdType::TYPE_DELETE;
        PutExtraString(env, MEDIATOOL_ARG_SECOND);
    } else if (cmd == OPT_STR_QUERY) {
        env.optArgs.cmdType = OptCmdType::TYPE_QUERY;
        PutExtraString(env, MEDIATOOL_ARG_FIRST);
    } else if (cmd == OPT_STR_LS) {
        env.optArgs.cmdType = OptCmdType::TYPE_LS;
    } else {
        ShowUsage(env.isRoot);
        return Media::E_ERR;
    }
    if (!Check(env)) {
        return Media::E_ERR;
    }
    return Media::E_OK;
}

} // namespace MediaTool
} // namespace Media
} // namespace OHOS
