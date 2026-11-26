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
#include "command/recv_command_v10.h"
#include "media_pure_file_utils.h"

#include <cerrno>
#include <fcntl.h>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "constant.h"
#include "datashare_result_set.h"
#include "datashare_predicates.h"
#include "directory_ex.h"
#include "media_file_uri.h"
#include "media_file_utils.h"
#include "media_log.h"
#include "medialibrary_errno.h"
#include "mediatool_uri.h"
#include "utils/mediatool_command_utils.h"
#include "userfile_client.h"
#include "userfile_client_ex.h"
#include "media_audio_column.h"
#include "media_path_utils.h"

namespace OHOS {
namespace Media {
namespace MediaTool {

using namespace std;

constexpr mode_t OPEN_MODE = 0644;

static bool GetWriteFilePath(const ExecEnv& env, const string& displayName, std::string& wFilePath)
{
    wFilePath = env.recvParam.recvPath;
    if (!MediaPureFileUtils::IsDirectory(wFilePath)) {
        if (env.recvParam.isRecvAll) {
            MEDIA_ERR_LOG("RecvFilePath:%{public}s is not a directory.", wFilePath.c_str());
            printf("RecvFilePath:%s is not a directory.\n", wFilePath.c_str());
            return false;
        }
    } else {
        wFilePath = IncludeTrailingPathDelimiter(wFilePath);
        if (displayName.empty()) {
            MEDIA_ERR_LOG("RecvFile displayName is null.");
            printf("RecvFile displayName is null.\n");
            return false;
        }
        wFilePath += displayName;
    }
    return true;
}

static int32_t RecvFile(const ExecEnv &env, const FileAsset &fileAsset, bool isRecvMovingPhotoVideo = false)
{
    std::string wFilePath;
    string displayName = fileAsset.GetDisplayName();
    if (isRecvMovingPhotoVideo) {
        string title = fileAsset.GetTitle();
        displayName = title + ".mp4";
    }
    if (!GetWriteFilePath(env, displayName, wFilePath)) {
        return Media::E_ERR;
    }
    auto wfd = open(wFilePath.c_str(), O_CREAT | O_EXCL | O_NOFOLLOW | O_WRONLY | O_CLOEXEC, OPEN_MODE);
    if (wfd <= 0) {
        MEDIA_ERR_LOG("Open write path failed. errno:%{public}d, path name:%{public}s", errno, wFilePath.c_str());
        printf("%s open write path failed. errno:%d, path name:%s\n", STR_FAIL.c_str(), errno, wFilePath.c_str());
        return Media::E_ERR;
    }

    std::string openUri = fileAsset.GetUri();
    if (isRecvMovingPhotoVideo) {
        MediaFileUtils::UriAppendKeyValue(openUri, CONST_MEDIA_MOVING_PHOTO_OPRN_KEYWORD,
            CONST_OPEN_MOVING_PHOTO_VIDEO);
    }
    auto rfd = UserFileClientEx::Open(openUri, Media::MEDIA_FILEMODE_READONLY);
    if (rfd < 0) {
        MEDIA_ERR_LOG("Open source media file failed. uri:%{public}s", openUri.c_str());
        printf("%s open source media file failed.\n", STR_FAIL.c_str());
        close(wfd);
        return Media::E_ERR;
    }
    auto ret = MediaFileUtils::CopyFile(rfd, wfd);
    if (!ret) {
        MEDIA_ERR_LOG("Receive data failed. uri: %{public}s", openUri.c_str());
        printf("%s receive data failed.\n", STR_FAIL.c_str());
    } else {
        printf("%s\n", wFilePath.c_str());
    }
    UserFileClientEx::Close(fileAsset.GetUri(), rfd, Media::MEDIA_FILEMODE_READONLY);
    close(wfd);
    return ret ? Media::E_OK : Media::E_ERR;
}

static bool IsRoot()
{
    constexpr int rootUid = 0;
    return getuid() == rootUid;
}

static string ConvertToCloudPath(const string& inputPath)
{
    // The client only converts compatible paths. The server selects the account.
    const auto localPrefix = MediaPathUtils::GetDir(MediaPathType::LOCAL_PHOTO, AdaptAccount::NO);
    const auto cloudPrefix = MediaPathUtils::GetDir(MediaPathType::CLOUD_PHOTO, AdaptAccount::NO);
    if (localPrefix.empty() || cloudPrefix.empty() || inputPath.size() <= localPrefix.size() ||
        inputPath.compare(0, localPrefix.size(), localPrefix) != 0) {
        MEDIA_ERR_LOG("Convert to cloud path failed, input path format incorrect");
        return "";
    }
    return cloudPrefix + inputPath.substr(localPrefix.size());
}

int32_t RecvCommandV10::QueryAssets(std::shared_ptr<DataShare::DataShareResultSet>& resultSet,
    const std::string& tableName)
{
    if (!UserFileClientEx::CheckTableName(tableName)) {
        MEDIA_ERR_LOG("tableName %{public}s is Invalid", tableName.c_str());
        return Media::E_ERR;
    }
    resultSet = nullptr;
    std::string queryUriStr = UserFileClientEx::GetQueryUri(tableName);
    if (queryUriStr.empty()) {
        MEDIA_ERR_LOG("query failed. queryUriStr:empty, tableName:%{public}s", tableName.c_str());
        return Media::E_ERR;
    }
    OHOS::DataShare::DataSharePredicates predicates;
    if (!isRecvAll_) {
        if (!uri_.empty()) {
            MediaFileUri fileUri(uri_);
            std::string id = fileUri.GetFileId();
            predicates.EqualTo(MediaColumn::MEDIA_ID, id);
        } else if (!srcPath_.empty()) {
            const auto cloudPath = ConvertToCloudPath(srcPath_);
            if (cloudPath.empty()) {
                return Media::E_ERR;
            }
            predicates.EqualTo(MediaColumn::MEDIA_FILE_PATH, cloudPath);
        }
    }

    Uri queryUri(queryUriStr);
    predicates.And()->NotEqualTo(PhotoColumn::PHOTO_FILE_SOURCE_TYPE,
        static_cast<int32_t>(FileSourceTypes::TEMP_FILE_MANAGER));
    if (!IsRoot() && tableName == PhotoColumn::PHOTOS_TABLE) {
        predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, 0);
    }
    std::vector<std::string> columns;
    int errCode = 0;
    MEDIA_INFO_LOG("query. queryUri: %{public}s, tableName: %{public}s, uri: %{public}s, "
        "path: %{public}s", queryUri.ToString().c_str(), tableName.c_str(), uri_.c_str(), srcPath_.c_str());
    resultSet = UserFileClient::Query(queryUri, predicates, columns, errCode);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query failed. resultSet:null, errCode:%{public}d.", errCode);
        return Media::E_ERR;
    }
    if (errCode != Media::E_OK) {
        MEDIA_ERR_LOG("query failed. errCode:%{public}d.", errCode);
        resultSet->Close();
        return Media::E_ERR;
    }
    return Media::E_OK;
}

bool RecvCommandV10::QueryMovingPhotoAsset(const string& filePath, unique_ptr<FileAsset>& movingPhotoAsset)
{
    std::string queryUriStr = UserFileClientEx::GetQueryUri(tableName_);
    if (queryUriStr.empty()) {
        MEDIA_ERR_LOG("query failed. queryUriStr:empty, tableName:%{public}s", tableName_.c_str());
        return false;
    }

    string queryPath = ConvertToCloudPath(filePath);
    if (queryPath.empty()) {
        return false;
    }
    string::size_type lastDotPos = queryPath.find_last_of('.');
    if (lastDotPos == string::npos) {
        MEDIA_ERR_LOG("filePath does not have an extension: %{public}s", filePath.c_str());
        return false;
    }
    string pathPrefix = queryPath.substr(0, lastDotPos + 1);

    OHOS::DataShare::DataSharePredicates predicates;
    predicates.Like(MediaColumn::MEDIA_FILE_PATH, pathPrefix + "%");
    predicates.And()->EqualTo(PhotoColumn::PHOTO_SUBTYPE, static_cast<int32_t>(PhotoSubType::MOVING_PHOTO));
    if (!IsRoot()) {
        predicates.And()->EqualTo(MediaColumn::MEDIA_HIDDEN, 0);
    }

    Uri queryUri(queryUriStr);
    std::vector<std::string> columns;
    int errCode = 0;
    auto resultSet = UserFileClient::Query(queryUri, predicates, columns, errCode);
    if (resultSet == nullptr) {
        MEDIA_ERR_LOG("query failed. resultSet:null, errCode:%{public}d.", errCode);
        return false;
    }
    if (errCode != Media::E_OK) {
        MEDIA_ERR_LOG("query failed. errCode:%{public}d.", errCode);
        resultSet->Close();
        return false;
    }
    std::shared_ptr<FetchResult<FileAsset>> fetchResult = std::make_shared<FetchResult<FileAsset>>(resultSet);
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    auto count = fetchResult->GetCount();
    if (count <= 0) {
        fetchResult->Close();
        return false;
    }
    movingPhotoAsset = fetchResult->GetFirstObject();
    fetchResult->Close();
    return true;
}

bool RecvCommandV10::IsMovingPhotoVideoPath(unique_ptr<FileAsset>& movingPhotoAsset)
{
    if (srcPath_.empty()) {
        return false;
    }

    string extension = MediaFileUtils::GetExtensionFromPath(srcPath_);
    if (!MediaFileUtils::CheckMovingPhotoVideoExtension(extension)) {
        return false;
    }

    if (QueryMovingPhotoAsset(srcPath_, movingPhotoAsset)) {
        return true;
    }
    return false;
}

static bool PathExists(const string& path)
{
    std::string lsUriStr = CONST_TOOL_LS_PHOTO;
    Uri lsUri(lsUriStr);
    DataShare::DataShareValuesBucket values;
    values.Put(MediaColumn::MEDIA_FILE_PATH, path);
    string outString;
    UserFileClient::InsertExt(lsUri, values, outString);
    return !outString.empty();
}

static int32_t RecvResultFiles(const ExecEnv &env, const std::shared_ptr<FetchResult<FileAsset>> &fetchResult,
    int32_t count)
{
    bool hasError = false;
    for (int32_t index = 0; index < count; index++) {
        auto fileAsset = fetchResult->GetObjectAtPosition(index);
        if (fileAsset == nullptr) {
            MEDIA_ERR_LOG("Get media asset failed. index:%{public}d", index);
            printf("%s get media asset failed. index:%d\n", STR_FAIL.c_str(), index);
            hasError = true;
            continue;
        }
        if (RecvFile(env, *fileAsset) != Media::E_OK) {
            hasError = true;
        }
    }
    fetchResult->Close();
    return hasError ? Media::E_ERR : Media::E_OK;
}

int32_t RecvCommandV10::RecvAssets(const ExecEnv& env, const std::string& tableName)
{
    printf("Table Name: %s\n", tableName.c_str());
    if (!srcPath_.empty() && !PathExists(srcPath_)) {
        MEDIA_ERR_LOG("Path does not exist, path: %{public}s",
            srcPath_.c_str());
        printf("%s This path does not refer to a valid media asset \n",
            STR_FAIL.c_str());
        return Media::E_ERR;
    }
    std::shared_ptr<DataShare::DataShareResultSet> resultSet;
    auto res = QueryAssets(resultSet, tableName);
    if (res != Media::E_OK || resultSet == nullptr) {
        MEDIA_ERR_LOG("query issue. tableName:%{public}s", tableName.c_str());
        printf("%s query issue. tableName:%s\n", STR_FAIL.c_str(), tableName.c_str());
        return Media::E_ERR;
    }
    auto fetchResult = std::make_shared<FetchResult<FileAsset>>(resultSet);
    if (fetchResult == nullptr) {
        return Media::E_ERR;
    }
    fetchResult->SetResultNapiType(ResultNapiType::TYPE_USERFILE_MGR);
    auto count = fetchResult->GetCount();
    if (count < 0) {
        MEDIA_ERR_LOG("Get media asset count failed. count: %{public}d.", count);
        printf("%s Get media asset count failed. count:%d\n", STR_FAIL.c_str(), count);
        return Media::E_ERR;
    }
    if (count == 0 && !isRecvAll_) {
        fetchResult->Close();
        unique_ptr<FileAsset> movingPhotoAsset;
        if (IsMovingPhotoVideoPath(movingPhotoAsset)) {
            return RecvFile(env, *movingPhotoAsset, true);
        }
        MEDIA_ERR_LOG("No valid media asset found. uri: %{public}s, path: %{public}s",
            uri_.c_str(), srcPath_.c_str());
        printf("%s This %s does not refer to a valid media asset \n",
            STR_FAIL.c_str(), srcPath_.empty() ? "uri" : "path");
        return Media::E_ERR;
    }
    return RecvResultFiles(env, fetchResult, count);
}

bool RecvCommandV10::CheckArgs(const ExecEnv& env)
{
    isRecvAll_ = env.recvParam.isRecvAll;
    if (isRecvAll_) {
        return true;
    }

    string recvTarget = env.recvParam.recvTarget;
    string reformattedPath;
    if (MediatoolCommandUtils::CheckAndReformatPathParam(recvTarget, reformattedPath)) {
        inputPath_ = recvTarget;
        srcPath_ = reformattedPath;
        tableName_ = PhotoColumn::PHOTOS_TABLE;
        return true;
    }

    MediaFileUri fileUri(recvTarget);
    if (fileUri.IsValid()) {
        uri_ = recvTarget;
        tableName_ = UserFileClientEx::GetTableNameByUri(uri_);
        if (tableName_.empty()) {
            MEDIA_ERR_LOG("Failed to get table name from uri. uri: %{public}s", uri_.c_str());
            printf("%s uri issue. uri: %s\n", STR_FAIL.c_str(), uri_.c_str());
            return false;
        }
        return true;
    }

    return false;
}

int32_t RecvCommandV10::Start(const ExecEnv &env)
{
    if (!CheckArgs(env)) {
        MEDIA_ERR_LOG("recv target invalid: %{public}s", env.recvParam.recvTarget.c_str());
        printf("%s recv target invalid: %s\n", STR_FAIL.c_str(), env.recvParam.recvTarget.c_str());
        return Media::E_ERR;
    }
    if (isRecvAll_) {
        bool hasError = false;
        auto tables = UserFileClientEx::GetSupportTables();
        for (auto tableName : tables) {
            if (RecvAssets(env, tableName) != Media::E_OK) {
                hasError = true;
            }
            printf("\n");
        }
        return hasError ? Media::E_ERR : Media::E_OK;
    } else {
        return RecvAssets(env, tableName_);
    }
}
} // namespace MediaTool
} // namespace Media
} // namespace OHOS
