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
#include "command/send_command_v10.h"

#include <chrono>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "constant.h"
#include "directory_ex.h"
#include "exec_env.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "medialibrary_errno.h"
#include "medialibrary_napi_utils.h"
#include "userfile_client_ex.h"

namespace OHOS {
namespace Media {
namespace MediaTool {
static constexpr int32_t SLEEP_NOTIFY_TIME = 2000;

struct FileInfo {
    Media::MediaType mediaType {Media::MediaType::MEDIA_TYPE_DEFAULT};
    std::string displayName;
    std::string path;
    std::string uri;
    int32_t id {-1};
    bool toBeRemove {false};
    int32_t result {-1};
    [[nodiscard]] std::string ToStr() const
    {
        std::string str;
        str.append("mediaType:");
        str.append(std::to_string(static_cast<uint32_t>(mediaType)));
        str.append(", displayName:");
        str.append(displayName);
        str.append(", path:");
        str.append(path);
        str.append(", uri:");
        str.append(uri);
        str.append(", id:");
        str.append(std::to_string(id));
        return str;
    }
};

static inline int32_t GetFileInfo(const ExecEnv &env, const std::string &path, std::vector<FileInfo> &fileInfos)
{
    FileInfo fileInfo;
    fileInfo.displayName = ExtractFileName(path);
    fileInfo.mediaType = MediaFileUtils::GetMediaType(fileInfo.displayName);
    fileInfo.path = path;
    fileInfo.result = Media::E_ERR;
    auto mediaTypes = UserFileClientEx::GetSupportTypes();
    for (auto mediaType : mediaTypes) {
        if (mediaType != fileInfo.mediaType) {
            continue;
        }
        fileInfos.push_back(fileInfo);
        return Media::E_OK;
    }
    return Media::E_ERR;
}

static int32_t GetDirInfo(const ExecEnv &env, const std::string &path, std::vector<FileInfo> &fileInfos)
{
    std::vector<std::string> files;
    GetDirFiles(path, files);
    if (!files.size() && errno) {
        printf("%s can not get dir files. errno:%d\n", STR_FAIL.c_str(), errno);
    }
    for (auto &file : files) {
        auto ret = GetFileInfo(env, file, fileInfos);
        if (ret != Media::E_OK) {
            printf("%s get dir information failed. ret:%d, file:%s\n", STR_FAIL.c_str(), ret, file.c_str());
            return ret;
        }
    }
    return Media::E_OK;
}

static void RemoveFiles(const ExecEnv &env, std::vector<FileInfo> &fileInfos)
{
    for (auto &fileInfo : fileInfos) {
        if (fileInfo.result != Media::E_OK) {
            continue;
        }
        if (!fileInfo.toBeRemove) {
            continue;
        }
        RemoveFile(fileInfo.path);
    }
    fileInfos.clear();
}

static std::string EncodeDisplayName(const std::string &displayName)
{
    std::set<char> CHAR_FILTERS = {
        '\\', '/', ':',
        '*', '?', '"',
        '<', '>', '|',
    };
    std::string encodedStr = "";
    for (char c : displayName) {
        if (CHAR_FILTERS.find(c) != CHAR_FILTERS.end()) {
            encodedStr += MediaFileUtils::Encode(string(1, c));
        } else {
            encodedStr += c;
        }
    }
    return encodedStr;
}

static int32_t CreateRecord(const ExecEnv &env, FileInfo &fileInfo, bool isRestart)
{
    const MediaType mediaType = fileInfo.mediaType;
    std::string tableName = UserFileClientEx::GetTableNameByMediaType(mediaType);
    const std::string displayName = EncodeDisplayName(fileInfo.displayName);
    string uri;
    auto fileId = UserFileClientEx::InsertExt(tableName, displayName, uri, isRestart);
    if (fileId <= 0) {
        printf("%s create record failed. fileId:%d, fileInfo:%s\n",
            STR_FAIL.c_str(), fileId, fileInfo.ToStr().c_str());
        return Media::E_ERR;
    }
    if (!uri.empty()) {
        fileInfo.uri = uri;
    } else {
        printf("%s create record failed. uri is empty. fileInfo:%s\n",
            STR_FAIL.c_str(), fileInfo.ToStr().c_str());
        return Media::E_FAIL;
    }
    fileInfo.id = fileId;
    return Media::E_OK;
}

static int32_t WriteFile(const ExecEnv &env, FileInfo &fileInfo, bool isRestart)
{
    if (!PathToRealPath(fileInfo.path, fileInfo.path)) {
        printf("%s path issue. errno:%d, path:%s.\n", STR_FAIL.c_str(), errno, fileInfo.path.c_str());
        return Media::E_ERR;
    }
    int32_t rfd = open(fileInfo.path.c_str(), O_RDONLY | O_CLOEXEC);
    if (rfd < 0) {
        printf("%s open file failed. rfd:%d, path:%s\n", STR_FAIL.c_str(), rfd, fileInfo.path.c_str());
        return Media::E_ERR;
    }
    int32_t wfd = UserFileClientEx::Open(fileInfo.uri, Media::MEDIA_FILEMODE_WRITETRUNCATE, isRestart);
    if (wfd < 0) {
        printf("%s open failed. wfd:%d, uri:%s\n", STR_FAIL.c_str(), wfd, fileInfo.uri.c_str());
        close(rfd);
        return Media::E_ERR;
    }
    int32_t ret = MediaFileUtils::CopyFile(rfd, wfd);
    if (!ret) {
        printf("%s send data failed. rfd:%d, wfd:%d\n", STR_FAIL.c_str(), rfd, wfd);
        close(rfd);
        close(wfd);
        return Media::E_ERR;
    }
    if (env.sendParam.isCreateThumbSyncInSend) {
        ret = UserFileClientEx::Close(fileInfo.uri, wfd, Media::MEDIA_FILEMODE_WRITETRUNCATE, true, isRestart);
    } else {
        ret = UserFileClientEx::Close(fileInfo.uri, wfd, Media::MEDIA_FILEMODE_WRITETRUNCATE, false, isRestart);
    }
    if (ret != E_OK) {
        printf("close file has err [%d]\n", ret);
    }

    close(rfd);
    return ret;
}

static constexpr int MAX_RESTART_TIME = 5;
static void RestartMediaLibraryServer(int32_t restartTime)
{
    printf("Run operations failed, restart Time %d\n", restartTime);
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_NOTIFY_TIME));
}

static int32_t SendFile(const ExecEnv &env, FileInfo &fileInfo)
{
    int32_t restartTime = 0;

    while (restartTime < MAX_RESTART_TIME) {
        fileInfo.result = CreateRecord(env, fileInfo, (restartTime > 0) ? true : false);
        if (fileInfo.result != Media::E_OK) {
            restartTime++;
            RestartMediaLibraryServer(restartTime);
            continue;
        } else {
            break;
        }
    }
    if (fileInfo.result != Media::E_OK) {
        return Media::E_ERR;
    }

    std::string tableName = UserFileClientEx::GetTableNameByUri(fileInfo.uri);
    if (tableName.empty()) {
        printf("%s uri issue. uri:%s\n", STR_FAIL.c_str(), fileInfo.uri.c_str());
        return Media::E_ERR;
    }

    while (restartTime < MAX_RESTART_TIME) {
        fileInfo.result = WriteFile(env, fileInfo, (restartTime > 0) ? true : false);
        if (fileInfo.result != Media::E_OK) {
            restartTime++;
            RestartMediaLibraryServer(restartTime);
            continue;
        } else {
            break;
        }
    }
    if (fileInfo.result != Media::E_OK) {
        return Media::E_ERR;
    }
    fileInfo.result = Media::E_OK;
    return Media::E_OK;
}

static int32_t SendFiles(const ExecEnv &env, std::vector<FileInfo> &fileInfos)
{
    int count = 0;
    int correctCount = 0;
    for (auto &fileInfo : fileInfos) {
        ++count;
        int32_t ret = SendFile(env, fileInfo);
        if (ret != Media::E_OK) {
            printf("%s send uri [%s] failed.\n", STR_FAIL.c_str(), fileInfo.uri.c_str());
        } else {
            fileInfo.toBeRemove = true;
            printf("%s\n", fileInfo.uri.c_str());
            ++correctCount;
        }
    }

    if (count > ARGS_ONE) {
        if (count == correctCount) {
            printf("%s send %d and %d is successful.\n", STR_SUCCESS.c_str(), count, correctCount);
            return Media::E_OK;
        } else {
            printf("%s send %d and %d is successful.\n", STR_FAIL.c_str(), count, correctCount);
            return Media::E_FAIL;
        }
    } else {
        if (correctCount == count) {
            return Media::E_OK;
        } else {
            return Media::E_FAIL;
        }
    }
}

int32_t SendCommandV10::Start(const ExecEnv &env)
{
    std::vector<FileInfo> fileInfos;
    auto ret = (env.sendParam.isFile) ? GetFileInfo(env, env.sendParam.sendPath, fileInfos) : GetDirInfo(env,
        env.sendParam.sendPath, fileInfos);
    if (ret != Media::E_OK) {
        printf("%s get file information failed. ret:%d\n", STR_FAIL.c_str(), ret);
        return ret;
    }
    ret = SendFiles(env, fileInfos);
    if (env.sendParam.isRemoveOriginFileInSend) {
        RemoveFiles(env, fileInfos);
    }
    return ret;
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
