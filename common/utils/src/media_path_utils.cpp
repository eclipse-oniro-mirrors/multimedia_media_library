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

#include "media_path_utils.h"

#include <filesystem>
#include <memory>
#include <vector>

#include <unicode/brkiter.h>
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>

#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include "media_string_utils.h"
#include <sys/xattr.h>

namespace OHOS::Media {
const char DOT = '.';
const std::string EMPTY_STRING = "";
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files/";
const std::string ROOT_MEDIA_LOCAL_DIR = "/storage/media/local/files/";

namespace {
// 与 media_file_utils 中的常量保持一致，脱敏只处理云路径前缀
const std::string CLOUD_FILE_PATH = "/storage/cloud/files";
const UChar MASK_CHAR = 0x002A;  // '*'
const UChar DOT_CHAR = 0x002E;   // '.'
} // namespace

std::string MediaPathUtils::AppendUserId(const std::string& path, int32_t userId)
{
    if (userId < 0 || !MediaStringUtils::StartsWith(path, ROOT_MEDIA_DIR)) {
        return path;
    }

    return "/storage/cloud/" + std::to_string(userId) + "/files/" + path.substr(ROOT_MEDIA_DIR.length());
}

bool MediaPathUtils::CheckPhotoPath(const std::string& photoPath)
{
    if (photoPath.find("..") != std::string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return false;
    }
    return photoPath.length() >= ROOT_MEDIA_DIR.length() && MediaStringUtils::StartsWith(photoPath, ROOT_MEDIA_DIR);
}

bool MediaPathUtils::CheckPathTraversal(const std::string& path)
{
    return path.find("..") != std::string::npos;
}

std::string MediaPathUtils::GetFileName(const std::string &filePath)
{
    std::string fileName;

    if (!filePath.empty()) {
        size_t lastSlash = filePath.rfind('/');
        if (lastSlash != std::string::npos) {
            if (filePath.size() > (lastSlash + 1)) {
                fileName = filePath.substr(lastSlash + 1);
            }
        }
    }

    return fileName;
}

std::string MediaPathUtils::GetExtension(const std::string &path)
{
    size_t splitIndex = path.find_last_of(DOT);
    if (splitIndex == std::string::npos || splitIndex == 0) {
        return EMPTY_STRING;
    }
    std::string extension = path.substr(splitIndex + 1);
    if (!extension.empty()) {
        transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    }
    return extension;
}

bool MediaPathUtils::CheckIsCloudFile(const std::string &sandboxPath)
{
    constexpr size_t MAX_ATTR_NAME = 64;
    constexpr const char* CLOUD_LOCATION_ATTR = "user.cloud.location";
    char cloudvalue[MAX_ATTR_NAME] = {'\0'};
    auto valueLen = getxattr(sandboxPath.c_str(), CLOUD_LOCATION_ATTR, cloudvalue, MAX_ATTR_NAME);
    CHECK_AND_RETURN_RET_LOG(valueLen > 0, false, "failed to getxattr, sandboxPath: %{public}s", sandboxPath.c_str());
    constexpr const char FILE_POSITION_CLOUD = '2';
    return cloudvalue[0] == FILE_POSITION_CLOUD;
}

std::string MediaPathUtils::ConvertCloudPathToLocalPath(const std::string &cloudPath)
{
    CHECK_AND_RETURN_RET_LOG(!cloudPath.empty(), EMPTY_STRING, "cloudPath is empty");
    CHECK_AND_RETURN_RET_LOG(MediaStringUtils::StartsWith(cloudPath, ROOT_MEDIA_DIR), EMPTY_STRING,
        "cloudPath %{public}s not start with %{public}s", cloudPath.c_str(), ROOT_MEDIA_DIR.c_str());
    std::string localPath = cloudPath;
    localPath.replace(0, ROOT_MEDIA_DIR.length(), ROOT_MEDIA_LOCAL_DIR);
    return localPath;
}

std::string MediaPathUtils::GetParentPath(const std::string &path)
{
    std::string name;
    size_t slashIndex = path.rfind("/");
    if (slashIndex != std::string::npos) {
        name = path.substr(0, slashIndex);
    }

    return name;
}

std::string MediaPathUtils::DesensitizePath(const std::string &path)
{
    std::string result = path;
    CHECK_AND_RETURN_RET(result.length() > CLOUD_FILE_PATH.length(), result);
    return result.replace(0, CLOUD_FILE_PATH.length(), "*");
}

std::string MediaPathUtils::DesensitizeName(const std::string &name)
{
    CHECK_AND_RETURN_RET_LOG(!name.empty(), name, "name is empty");

    UErrorCode status = U_ZERO_ERROR;
    const icu::Normalizer2 *normalizer = icu::Normalizer2::getNFCInstance(status);
    CHECK_AND_RETURN_RET_LOG(
        normalizer != nullptr && !U_FAILURE(status), name, "getNFCInstance failed, status:%{public}d", status);

    icu::UnicodeString uName = icu::UnicodeString::fromUTF8(icu::StringPiece(name.data(), (int32_t)name.length()));
    CHECK_AND_RETURN_RET_LOG(!uName.isBogus(), name, "uName is Bogus");

    icu::UnicodeString norm;
    normalizer->normalize(uName, norm, status);
    CHECK_AND_RETURN_RET_LOG(
        !U_FAILURE(status) && !norm.isBogus(), name, "normalize failed, status:%{public}d", status);

    const int32_t dotPos = norm.lastIndexOf(DOT_CHAR);
    const bool hasExt = (dotPos > 0 && dotPos < norm.length() - 1);
    const int32_t baseEnd = hasExt ? dotPos : norm.length();

    std::unique_ptr<icu::BreakIterator> it(icu::BreakIterator::createCharacterInstance(icu::Locale::getRoot(), status));
    CHECK_AND_RETURN_RET_LOG(
        it != nullptr && !U_FAILURE(status), name, "create character iterator failed, status:%{public}d", status);

    it->setText(norm);

    std::vector<int32_t> boundaries;
    for (int32_t pos = it->first(); pos != icu::BreakIterator::DONE && pos <= baseEnd; pos = it->next()) {
        boundaries.push_back(pos);
    }
    const int32_t clusterCount = static_cast<int32_t>(boundaries.size()) - 1;
    CHECK_AND_RETURN_RET_LOG(clusterCount > 0, name, "clusterCount is zero");

    // desensitization strategy (1,2,3 -> 1*; 4 -> 2*; 5+ -> 3*)
    const int32_t maskCount = (clusterCount >= 5) ? 3 : (clusterCount == 4 ? 2 : 1);
    const int32_t displayCount = clusterCount - maskCount;
    const int32_t prefixCount = displayCount / 2;
    const int32_t suffixCount = displayCount - prefixCount;

    icu::UnicodeString out;
    if (prefixCount > 0) {
        out.append(norm.tempSubStringBetween(0, boundaries[prefixCount]));
    }

    out.append(icu::UnicodeString(maskCount, MASK_CHAR, maskCount));

    if (suffixCount > 0) {
        out.append(norm.tempSubStringBetween(boundaries[clusterCount - suffixCount], baseEnd));
    }

    if (hasExt) {
        out.append(norm.tempSubString(dotPos));
    }

    std::string result;
    out.toUTF8String(result);
    return result;
}

bool MediaPathUtils::IsTranscodeFile(const std::string &filePath)
{
    std::string displayName = filePath;
    size_t pos = filePath.find_last_of("/");
    if (pos != std::string::npos) {
        displayName = filePath.substr(pos + 1);
    }
    if (displayName == "transcode.jpg" && filePath.find(".editData") != std::string::npos) {
        MEDIA_INFO_LOG("displayname %{public}s, success", DesensitizeName(displayName).c_str());
        return true;
    }
    return false;
}

int32_t MediaPathUtils::CopyDirectory(const std::string &srcDir, const std::string &dstDir)
{
    if (srcDir.empty() || dstDir.empty()) {
        MEDIA_ERR_LOG("Failed to copy directory, srcDir:%{public}s or newPath:%{public}s is empty!",
            DesensitizePath(srcDir).c_str(), DesensitizePath(dstDir).c_str());
        return E_MODIFY_DATA_FAIL;
    }
    if (!MediaPureFileUtils::IsFileExists(srcDir)) {
        MEDIA_ERR_LOG("SrcDir:%{public}s is not exist", DesensitizePath(srcDir).c_str());
        return E_NO_SUCH_FILE;
    }
    if (!MediaPureFileUtils::IsDirectory(srcDir)) {
        MEDIA_ERR_LOG("SrcDir:%{public}s is not directory", DesensitizePath(srcDir).c_str());
        return E_FAIL;
    }
    if (MediaPureFileUtils::IsFileExists(dstDir)) {
        MEDIA_ERR_LOG("DstDir:%{public}s exists", DesensitizePath(dstDir).c_str());
        return E_FILE_EXIST;
    }
    if (!MediaPureFileUtils::CreateDirectory(dstDir)) {
        MEDIA_ERR_LOG("Create dstDir:%{public}s failed", DesensitizePath(dstDir).c_str());
        return E_FAIL;
    }
    std::error_code ec;
    auto iter = std::filesystem::recursive_directory_iterator(srcDir, ec);
    CHECK_AND_RETURN_RET_LOG(!ec, E_FAIL, "Iterator creation failed: %{public}s", ec.message().c_str());
    for (; iter != std::filesystem::recursive_directory_iterator(); iter.increment(ec)) {
        if (ec) {
            // 这里如果有错误码，迭代器会置为end
            MEDIA_ERR_LOG("Iteration error: %{public}s", ec.message().c_str());
            continue;
        }
        const auto& entry = *iter;
        std::string srcFilePath = entry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        std::error_code entryEc;
        if (entry.is_directory(entryEc)) {
            if (!MediaPureFileUtils::CreateDirectory(dstFilePath)) {
                MEDIA_ERR_LOG("Create dir:%{public}s failed", DesensitizePath(dstFilePath).c_str());
                return E_FAIL;
            }
        } else if (!entryEc && entry.is_regular_file(entryEc)) {
            CHECK_AND_CONTINUE(!IsTranscodeFile(srcFilePath));
            if (!MediaPureFileUtils::CopyFileUtil(srcFilePath, dstFilePath)) {
                MEDIA_ERR_LOG("Copy file from %{public}s to %{public}s failed.",
                    DesensitizePath(srcFilePath).c_str(), DesensitizePath(dstFilePath).c_str());
                return E_FAIL;
            }
        } else if (entryEc) {
            MEDIA_ERR_LOG("Stat %{public}s failed: %{public}s", DesensitizePath(srcFilePath).c_str(),
                entryEc.message().c_str());
            return E_FAIL;
        } else {
            MEDIA_ERR_LOG("Unhandled path type, path:%{public}s", DesensitizePath(srcFilePath).c_str());
            return E_FAIL;
        }
    }
    CHECK_AND_RETURN_RET_LOG(!ec, E_FAIL, "Iterate %{public}s failed: %{public}s",
        DesensitizePath(srcDir).c_str(), ec.message().c_str());
    return E_OK;
}
} // namespace OHOS::Media
