/*
* Copyright (C) 2025 Huawei Device Co., Ltd.
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

#define MLOG_TAG "FolderScannerUtils"

#include <filesystem>
#include <iostream>

#include "folder_scanner_utils.h"
#include "scanner_utils.h"
#include "lake_const.h"
#include "lake_file_utils.h"

namespace OHOS::Media {
namespace fs = std::filesystem;

bool FolderScannerUtils::IsSkipCurrentFile(const std::string &filePath)
{
    std::string fileName = fs::path(filePath).filename().string();
    if (fileName.empty() || fileName[0] == '.') {
        MEDIA_INFO_LOG("hidden file: %{public}s", LakeFileUtils::GarbleFile(fileName).c_str());
        return true;
    }
    return false;
}

int32_t FolderScannerUtils::BatchInsertAssets(const std::string &tableName,
    std::vector<NativeRdb::ValuesBucket> &values,
    std::shared_ptr<AccurateRefresh::AssetAccurateRefresh> assetRefresh)
{
    int64_t changeRows = -1;
    int rdbError = 0;
    int32_t ret = assetRefresh->BatchInsert(changeRows, tableName, values, rdbError,
        NativeRdb::ConflictResolution::ON_CONFLICT_REPLACE);
    CHECK_AND_RETURN_RET_LOG(ret == NativeRdb::E_OK, ERR_FAIL,
        "Batch insert assets fail, ret = %{public}d, changeRows = %{public}lld, tableName: %{public}s",
        ret, changeRows, tableName.c_str());
    assetRefresh->RefreshAlbum();
    return ERR_SUCCESS;
}

bool FolderScannerUtils::shouldScanDirectory(const std::string &filePath)
{
    CHECK_AND_RETURN_RET_LOG(!filePath.empty() && filePath.back() != '/', true,
        "Current directory path is invalid, filePath is %{public}s", LakeFileUtils::GarbleFilePath(filePath).c_str());
    std::string nomediaPath = filePath + "/.nomedia";
    std::error_code errorCode;
    //CASE 1
    if (std::regex_match(filePath, PATTERN_VISIBLE)) {
        // case1  error_code > 0: skip it
        // case2 error_code = 0 and not exist: need to scan
        // case3  exist and not error : need to delete .nomedia and scan
        bool ret = fs::exists(nomediaPath, errorCode);
        CHECK_AND_RETURN_RET_LOG(!errorCode, false,
            "failed to access %{public}s, error message is %{public}s",
            LakeFileUtils::GarbleFilePath(nomediaPath).c_str(), errorCode.message().c_str());
        CHECK_AND_RETURN_RET_INFO_LOG(ret, true,
            "Current path not exists .nomedia, path is %{public}s", LakeFileUtils::GarbleFilePath(filePath).c_str());
        std::remove(nomediaPath.c_str());
        return true;
    }
    // CASE 2
    CHECK_AND_RETURN_RET_WARN_LOG(!std::regex_match(filePath, PATTERN_TENCENT_CACHE), false,
        "Current directory match /Tencent/MicroMsg, path is %{public}s",
        LakeFileUtils::GarbleFilePath(filePath).c_str());
    // CASE 3
    if (std::regex_match(filePath, PATTERN_INVISIBLE)) {
        // Try create .nomedia file if not exists
        bool ret = fs::exists(nomediaPath, errorCode);
        CHECK_AND_RETURN_RET_LOG(!errorCode, false,
            "failed to detemine nomedia %{public}s exist, error message is %{public}s",
            LakeFileUtils::GarbleFilePath(filePath).c_str(),
            errorCode.message().c_str());
        CHECK_AND_RETURN_RET_LOG(!ret, false,
            ".nomedia has exists, nomediaPath is %{public}s", LakeFileUtils::GarbleFilePath(nomediaPath).c_str());
        std::ofstream ofs(nomediaPath);  // create if not exists
        return false;
    }
    return true;
}

std::string GetCanonicalPath(const std::string &path)
{
    std::error_code errorCode;
    fs::path canonicalPath = fs::canonical(path, errorCode);
    CHECK_AND_RETURN_RET_LOG(!errorCode, "", "Failed to canonicalize path : %{public}s, message: %{public}s",
        LakeFileUtils::GarbleFilePath(path).c_str(), errorCode.message().c_str());
    return canonicalPath.string();
}

std::string ExtractRelativePath(const std::string &path)
{
    CHECK_AND_RETURN_RET(!path.empty(), "");
    std::string dirPath = GetCanonicalPath(path);
    MEDIA_DEBUG_LOG("Trans path %{public}s to canonical path %{public}s", LakeFileUtils::GarbleFilePath(path).c_str(),
        LakeFileUtils::GarbleFilePath(dirPath).c_str());
    CHECK_AND_RETURN_RET(!dirPath.empty(), "");
    
    std::smatch matcher;
    CHECK_AND_RETURN_RET(std::regex_search(dirPath, matcher, PATTERN_RELATIVE_PATH), "");
    size_t lastSlash = dirPath.find_last_of('/');
    size_t matchEnd = matcher.position(0) + matcher.length(0);
    if (lastSlash == std::string::npos || lastSlash < matchEnd) {
        return "/";
    } else {
        return dirPath.substr(matchEnd);
    }
}

std::vector<std::string> SanitizePath(const std::string &path)
{
    std::vector<std::string> segments;
    CHECK_AND_RETURN_RET(!path.empty(), segments);

    size_t start = 0;
    size_t end = path.find('/');
    while (end != std::string::npos) {
        if (end > start) {
            segments.push_back(path.substr(start, end - start));
        }
        start = end + 1;
        end = path.find('/', start);
    }
    if (start < path.size()) {
        segments.push_back(path.substr(start));
    }

    if (segments.empty()) {
        segments.push_back("");
    }
    return segments;
}

void FolderScannerUtils::CleanNomediaInDefaultDirs(const std::string &dirPath)
{
    std::error_code errCode;
    CHECK_AND_RETURN_LOG(!dirPath.empty() && dirPath.back() != '/',
        "Current directory path is invalid, dirPath is %{public}s", LakeFileUtils::GarbleFilePath(dirPath).c_str());
    std::string nomediaPath = dirPath + "/.nomedia";
    CHECK_AND_RETURN(fs::exists(nomediaPath, errCode));
    std::string relativePath = ExtractRelativePath(dirPath);
    CHECK_AND_RETURN_LOG(!relativePath.empty(),
        "Current directory path not in root path, dirPath is %{public}s",
        LakeFileUtils::GarbleFilePath(dirPath).c_str());
    // 1. 拆分路径组件
    std::vector<std::string> segments = SanitizePath(relativePath);
    CHECK_AND_RETURN_LOG(!segments.empty(),
        "Segments is invalid, dirPath is %{public}s", LakeFileUtils::GarbleFilePath(dirPath).c_str());
    // 情况1：当前目录是根目录
    if (relativePath == "/" && segments == std::vector<std::string>{""}) {
        fs::remove(nomediaPath, errCode);
        MEDIA_INFO_LOG("Root path %{public}s need to delete .nomedia", LakeFileUtils::GarbleFilePath(dirPath).c_str());
        return;
    }

    // 情况2：当前目录是根目录下的特定一级目录
    if (segments.size() == 1 && DEFAULT_FOLDER_NAMES.count(segments[0])) {
        fs::remove(nomediaPath, errCode);
        MEDIA_INFO_LOG("Top level path %{public}s need to delete .nomedia",
            LakeFileUtils::GarbleFilePath(dirPath).c_str());
        return;
    }

    // 情况3：当前目录是根目录下的DCIM/Camera
    size_t numberTwo = 2;
    if (segments.size() == numberTwo && segments[0] == "DCIM" && segments[1] == "Camera") {
        fs::remove(nomediaPath, errCode);
        MEDIA_INFO_LOG("DCIM/Camera path %{public}s need to delete .nomedia",
            LakeFileUtils::GarbleFilePath(dirPath).c_str());
        return;
    }

    // 情况4：当前目录是特定一级目录下的Screenshots或根目录下的Screenshots
    bool isTargetFirstLevelChild = (segments.size() == 2 && DEFAULT_FOLDER_NAMES.count(segments[0])
        && segments[1] == "Screenshots");
    bool isRootScreenshots = (segments.size() == 1 && segments[0] == "Screenshots");
    if (isTargetFirstLevelChild || isRootScreenshots) {
        fs::remove(nomediaPath, errCode);
        MEDIA_INFO_LOG("Screenshots path %{public}s need to delete .nomedia",
            LakeFileUtils::GarbleFilePath(dirPath).c_str());
        return;
    }
    return;
}

bool FolderScannerUtils::IsSkipCurrentDirectory(const std::string &currentDir)
{
    MEDIA_INFO_LOG("check path:%{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
    if (currentDir.empty()) {
        MEDIA_INFO_LOG("empty path: %{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
        return true;
    }
    if (BLACK_LIST.find(currentDir) != BLACK_LIST.end()) {
        MEDIA_INFO_LOG("black path: %{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
        return true;
    }

    fs::path currentPath(currentDir);
    std::string folderName = currentPath.filename().string();
    if (folderName.empty() || folderName[0] == '.') {
        MEDIA_INFO_LOG("hidden currentDir: %{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
        return true;
    }

    CleanNomediaInDefaultDirs(currentDir);

    std::string nomediaFilePath = currentDir + "/.nomedia";
    std::error_code errorCode;
    if (fs::exists(nomediaFilePath, errorCode)) {
        MEDIA_INFO_LOG("contain nomedia, nomediaFilePath: %{public}s",
            LakeFileUtils::GarbleFilePath(nomediaFilePath).c_str());
        return true;
    }

    return false;
}

bool FolderScannerUtils::IsSkipDirectory(const std::string &dir)
{
    std::string currentDir = dir;
    while (currentDir != LAKE_SCAN_DIR) {
        MEDIA_INFO_LOG("check path:%{public}s", LakeFileUtils::GarbleFilePath(currentDir).c_str());
        if (IsSkipCurrentDirectory(currentDir)) {
            return true;
        }
        currentDir = fs::path(currentDir).parent_path().string();
    }
    return false;
}

}