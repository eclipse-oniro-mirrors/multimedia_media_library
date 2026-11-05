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
#include <map>
#include <memory>
#include <vector>

#include <unicode/brkiter.h>
#include <unicode/normalizer2.h>
#include <unicode/unistr.h>

#include "media_string_utils.h"
#include "media_log.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"
#include <sys/xattr.h>
#include <unordered_map>

namespace OHOS::Media {
const char DOT = '.';
const char SLASH = '/';
const std::string EMPTY_STRING = "";

static const std::string STORAGE_CLOUD_DIR = "/storage/cloud/";

static const std::string STORAGE_MEDIA_DIR = "/storage/media/";
static const std::string LOCAL_MIDFIX = "local/";
static const std::string STORAGE_LOCAL_DIR = STORAGE_MEDIA_DIR + LOCAL_MIDFIX;

static const std::string FILES_MIDFIX = "files/";

static const std::string PHOTO_BUCKET = "Photo";
static const std::string EDITDATA_BUCKET = ".editData";
static const std::string THUMB_BUCKET = ".thumbs";
static const std::string CACHE_BUCKET = ".cache";
static const std::string CUSTOM_RESTORE_BUCKET = "custom_restore";
static const std::string AUDIO_BUCKET = "Audio";
static const std::string AUDIOS_BUCKET = "Audios";
static const std::string DOCS_BUCKET = "Docs";
static const std::string CAMERA_BUCKET = "Camera";
static const std::string VIDEOS_BUCKET = "Videos";
static const std::string PICTURES_BUCKET = "Pictures";
static const std::string BACK_UP_BUCKET = ".backup";
static const std::map<FileBucketType, std::string> BUCKET_FIX_MAP = {
    { FileBucketType::PHOTO,            PHOTO_BUCKET },
    { FileBucketType::EDIT_DATA,        EDITDATA_BUCKET },
    { FileBucketType::THUMB,            THUMB_BUCKET },
    { FileBucketType::CACHE,            CACHE_BUCKET },
    { FileBucketType::CUSTOM_RESTORE,   CUSTOM_RESTORE_BUCKET },
    { FileBucketType::AUDIO,            AUDIO_BUCKET },
    { FileBucketType::AUDIOS,           AUDIOS_BUCKET },
    { FileBucketType::DOCS,             DOCS_BUCKET },
    { FileBucketType::CAMERA,           CAMERA_BUCKET },
    { FileBucketType::VIDEOS,           VIDEOS_BUCKET },
    { FileBucketType::PICTURES,         PICTURES_BUCKET },
    { FileBucketType::BACK_UP,          BACK_UP_BUCKET },
};

static const std::string LCD_FILE_NAME = "/LCD.jpg";
static const std::string LCD_EX_FILE_NAME = "/THM_EX/LCD.jpg";
static const std::map<ThumbnailFileType, std::string> THUMBNAIL_FILE_MAP = {
    { ThumbnailFileType::LCD, LCD_FILE_NAME },
    { ThumbnailFileType::LCD_EX, LCD_EX_FILE_NAME },
};

static const std::unordered_map<MediaPathType, PathEntry> PATH_TABLE = {
    // cloud
    {MediaPathType::CLOUD_ROOT,             {STORAGE_CLOUD_DIR, "", ""}},
    {MediaPathType::CLOUD_FILES,            {STORAGE_CLOUD_DIR, FILES_MIDFIX, ""}},
    {MediaPathType::CLOUD_PHOTO,            {STORAGE_CLOUD_DIR, FILES_MIDFIX, PHOTO_BUCKET}},
    {MediaPathType::CLOUD_EDIT_DATA,        {STORAGE_CLOUD_DIR, FILES_MIDFIX, EDITDATA_BUCKET}},
    {MediaPathType::CLOUD_THUMB,            {STORAGE_CLOUD_DIR, FILES_MIDFIX, THUMB_BUCKET}},
    {MediaPathType::CLOUD_CACHE,            {STORAGE_CLOUD_DIR, FILES_MIDFIX, CACHE_BUCKET}},
    {MediaPathType::CLOUD_CUSTOM_RESTORE,   {STORAGE_CLOUD_DIR, FILES_MIDFIX, CUSTOM_RESTORE_BUCKET}},
    {MediaPathType::CLOUD_AUDIO,            {STORAGE_CLOUD_DIR, FILES_MIDFIX, AUDIO_BUCKET}},
    {MediaPathType::CLOUD_AUDIOS,           {STORAGE_CLOUD_DIR, FILES_MIDFIX, AUDIOS_BUCKET}},
    {MediaPathType::CLOUD_DOCS,             {STORAGE_CLOUD_DIR, FILES_MIDFIX, DOCS_BUCKET}},
    {MediaPathType::CLOUD_CAMERA,           {STORAGE_CLOUD_DIR, FILES_MIDFIX, CAMERA_BUCKET}},
    {MediaPathType::CLOUD_VIDEOS,           {STORAGE_CLOUD_DIR, FILES_MIDFIX, VIDEOS_BUCKET}},
    {MediaPathType::CLOUD_PICTURES,         {STORAGE_CLOUD_DIR, FILES_MIDFIX, PICTURES_BUCKET}},
    {MediaPathType::CLOUD_BACK_UP,          {STORAGE_CLOUD_DIR, FILES_MIDFIX, BACK_UP_BUCKET}},
    {MediaPathType::CLOUD_EDIT_DATA_PHOTO,  {STORAGE_CLOUD_DIR, FILES_MIDFIX, EDITDATA_BUCKET + "/" + PHOTO_BUCKET}},
    {MediaPathType::CLOUD_THUMB_PHOTO,      {STORAGE_CLOUD_DIR, FILES_MIDFIX, THUMB_BUCKET + "/" + PHOTO_BUCKET}},

    // local
    {MediaPathType::LOCAL_STORAGE_ROOT,     {STORAGE_MEDIA_DIR, "", ""}},
    {MediaPathType::LOCAL_ROOT,             {STORAGE_LOCAL_DIR, "", ""}},
    {MediaPathType::LOCAL_FILES,            {STORAGE_LOCAL_DIR, FILES_MIDFIX, ""}},
    {MediaPathType::LOCAL_PHOTO,            {STORAGE_LOCAL_DIR, FILES_MIDFIX, PHOTO_BUCKET}},
    {MediaPathType::LOCAL_EDIT_DATA,        {STORAGE_LOCAL_DIR, FILES_MIDFIX, EDITDATA_BUCKET}},
    {MediaPathType::LOCAL_THUMB,            {STORAGE_LOCAL_DIR, FILES_MIDFIX, THUMB_BUCKET}},
    {MediaPathType::LOCAL_CACHE,            {STORAGE_LOCAL_DIR, FILES_MIDFIX, CACHE_BUCKET}},
    {MediaPathType::LOCAL_CUSTOM_RESTORE,   {STORAGE_LOCAL_DIR, FILES_MIDFIX, CUSTOM_RESTORE_BUCKET}},
    {MediaPathType::LOCAL_AUDIO,            {STORAGE_LOCAL_DIR, FILES_MIDFIX, AUDIO_BUCKET}},
    {MediaPathType::LOCAL_AUDIOS,           {STORAGE_LOCAL_DIR, FILES_MIDFIX, AUDIOS_BUCKET}},
    {MediaPathType::LOCAL_DOCS,             {STORAGE_LOCAL_DIR, FILES_MIDFIX, DOCS_BUCKET}},
    {MediaPathType::LOCAL_CAMERA,           {STORAGE_LOCAL_DIR, FILES_MIDFIX, CAMERA_BUCKET}},
    {MediaPathType::LOCAL_VIDEOS,           {STORAGE_LOCAL_DIR, FILES_MIDFIX, VIDEOS_BUCKET}},
    {MediaPathType::LOCAL_PICTURES,         {STORAGE_LOCAL_DIR, FILES_MIDFIX, PICTURES_BUCKET}},
    {MediaPathType::LOCAL_BACK_UP,          {STORAGE_LOCAL_DIR, FILES_MIDFIX, BACK_UP_BUCKET}},
    {MediaPathType::LOCAL_EDIT_DATA_PHOTO,  {STORAGE_LOCAL_DIR, FILES_MIDFIX, EDITDATA_BUCKET + "/" + PHOTO_BUCKET}},
    {MediaPathType::LOCAL_THUMB_PHOTO,      {STORAGE_LOCAL_DIR, FILES_MIDFIX, THUMB_BUCKET + "/" + PHOTO_BUCKET}},
};

static const std::string ROOT_PREFERENCES_DIR = "/data/storage/el2/base/preferences/";
static const std::string XML_EXTENSION = "xml";

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
GetForegroundAccountIdHandler MediaPathUtils::handler_;
#endif

std::string MediaPathUtils::GetDir(MediaPathType pathType, AdaptAccount adaptAccount, AppendSlash appendSlash)
{
    auto it = PATH_TABLE.find(pathType);
    if (it == PATH_TABLE.end()) {
        MEDIA_ERR_LOG("invalid pathType=%{public}d", static_cast<int32_t>(pathType));
        return "";
    }
    const PathEntry& entry = it->second;
    std::string path = entry.rootDir;

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    // 满足条件时在 midfix 与 bucket 之间插入 "accountId/"，否则留空
    std::string accountSeg;
    if (adaptAccount == AdaptAccount::YES && !entry.midfix.empty()) {
        // AccountScope 优先（用于跨账号操作场景，如资产所属账号非前台账号时的 Close），
        // 回退到前台账号，与 RdbStore 的账号路由模式保持一致
        int32_t accountId = AccountScope::Get();
        if (accountId == INVALID_ACCOUNT_ID) {
            if (GetForegroundAccountId(accountId)) {
                accountSeg = std::to_string(accountId) + SLASH;
            }
        } else {
            accountSeg = std::to_string(accountId) + SLASH;
        }
    }
    path = path + entry.midfix + accountSeg + entry.bucket;
#else
    path = path + entry.midfix + entry.bucket;
#endif
    if (appendSlash == AppendSlash::YES) {
        if (path.empty() || path.back() != SLASH) {
            path += SLASH;
        }
    } else {
        if (!path.empty() && path.back() == SLASH) {
            path.pop_back();
        }
    }
    return path;
}

bool MediaPathUtils::CheckFilesPath(MediaPathType pathType, const std::string& path, AdaptAccount adaptAccount)
{
    std::string filesDir = GetDir(pathType, adaptAccount);
    return path.length() >= filesDir.length() && MediaStringUtils::StartsWith(path, filesDir);
}

std::string MediaPathUtils::GetLocalThumbnailFilePath(const std::string &photoPath, ThumbnailFileType thumbnailFileType)
{
    if (!CheckFilesPath(MediaPathType::CLOUD_FILES, photoPath, AdaptAccount::YES)) {
        return "";
    }
    if (photoPath.find("..") != std::string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return "";
    }
    std::string localThumbnailDir = GetDir(MediaPathType::LOCAL_THUMB, AdaptAccount::YES);
    std::string cloudFilesDir = GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    return localThumbnailDir + photoPath.substr(cloudFilesDir.length()) + GetThumbnailFileFix(thumbnailFileType);
}

std::string MediaPathUtils::GetBucketFix(FileBucketType bucketType)
{
    auto it = BUCKET_FIX_MAP.find(bucketType);
    if (it == BUCKET_FIX_MAP.end()) {
        MEDIA_ERR_LOG("invalid bucketType=%{public}d", static_cast<int32_t>(bucketType));
        return "";
    }
    return it->second + "/";
}

std::string MediaPathUtils::GetThumbnailFileFix(ThumbnailFileType thumbnailFileType)
{
    auto it = THUMBNAIL_FILE_MAP.find(thumbnailFileType);
    if (it == THUMBNAIL_FILE_MAP.end()) {
        MEDIA_ERR_LOG("invalid bucketType=%{public}d", static_cast<int32_t>(thumbnailFileType));
        return "";
    }
    return it->second;
}

bool MediaPathUtils::CheckPreferencePath(const std::string& xmlPath)
{
    return xmlPath.length() >= ROOT_PREFERENCES_DIR.length() &&
           MediaStringUtils::StartsWith(xmlPath, ROOT_PREFERENCES_DIR);
}

namespace {
// 与 media_file_utils 中的常量保持一致，脱敏只处理云路径前缀
const UChar MASK_CHAR = 0x002A;  // '*'
const UChar DOT_CHAR = 0x002E;   // '.'
} // namespace

std::string MediaPathUtils::AppendUserId(const std::string& path, int32_t userId)
{
    if (userId < 0 || !CheckFilesPath(MediaPathType::CLOUD_FILES, path, AdaptAccount::YES)) {
        return path;
    }

    return STORAGE_CLOUD_DIR + std::to_string(userId) + "/" + path.substr(STORAGE_CLOUD_DIR.length());
}

bool MediaPathUtils::CheckPhotoPath(const std::string& photoPath)
{
    if (photoPath.find("..") != std::string::npos) {
        MEDIA_ERR_LOG("Invalid photoPath with path traversal: %{private}s", photoPath.c_str());
        return false;
    }
    const std::string CLOUD_FILES_DIR = STORAGE_CLOUD_DIR + FILES_MIDFIX;
    return photoPath.length() >= CLOUD_FILES_DIR.length() && MediaStringUtils::StartsWith(photoPath, CLOUD_FILES_DIR);
}

std::string MediaPathUtils::AdaptPreferencesPathForAllDevice(const std::string& xmlPath, bool addAccount)
{
#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
    CHECK_AND_RETURN_RET_LOG(!xmlPath.empty(), xmlPath, "xmlPath is empty");
    if (!CheckPreferencePath(xmlPath)) {
        MEDIA_ERR_LOG("xmlPath %{private}s is not under the preferences dir", xmlPath.c_str());
        return xmlPath;
    }

    std::string fileName = GetFileName(xmlPath);
    CHECK_AND_RETURN_RET_LOG(!fileName.empty(), xmlPath, "xmlPath %{private}s has no file name", xmlPath.c_str());
    CHECK_AND_RETURN_RET_LOG(GetExtension(fileName) == XML_EXTENSION, xmlPath, "xmlPath %{private}s is not an xml file",
        xmlPath.c_str());

    constexpr size_t XML_EXTENSION_LENGTH = 4; // ".xml"
    std::string nameWithoutExtension = fileName.substr(0, fileName.length() - XML_EXTENSION_LENGTH);

    // 优先获取前台账号 id；返回 false 表示当前业务不支持多账号处理，直接返回原始路径
    int32_t foregroundAccountId = INVALID_ACCOUNT_ID;
    if (!GetForegroundAccountId(foregroundAccountId)) {
        return xmlPath;
    }
    // AccountScope::Get() 有效则优先使用，否则使用前台账号 id
    int32_t subProfileId = AccountScope::Get();
    if (subProfileId == INVALID_ACCOUNT_ID) {
        subProfileId = foregroundAccountId;
    }
    // 检查文件名是否已添加过 accountId 后缀，避免重复添加
    std::string accountSuffix = "_" + std::to_string(subProfileId);
    if (nameWithoutExtension.length() >= accountSuffix.length() &&
        nameWithoutExtension.substr(nameWithoutExtension.length() - accountSuffix.length()) == accountSuffix) {
        return ROOT_PREFERENCES_DIR + fileName;
    }
    std::string newFileName = nameWithoutExtension + accountSuffix + ".xml";
    return ROOT_PREFERENCES_DIR + newFileName;
#else
    return xmlPath;
#endif
}

std::string MediaPathUtils::GetPreferencesDir()
{
    return ROOT_PREFERENCES_DIR;
}

void MediaPathUtils::PrintStatInformation(const std::string& path)
{
    struct stat statInfo {};
    if (stat(path.c_str(), &statInfo) != E_SUCCESS) {
        MEDIA_WARN_LOG("path:%{public}s is not exist", DesensitizePath(path).c_str());
        return;
    }
    MEDIA_INFO_LOG("path:%{public}s uid:%{public}d, gid:%{public}d, mode:%{public}d",
        DesensitizePath(path).c_str(), statInfo.st_uid, statInfo.st_gid, statInfo.st_mode);
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
    if (!MediaStringUtils::StartsWith(cloudPath, STORAGE_CLOUD_DIR)) {
        MEDIA_ERR_LOG("cloudPath %{public}s not start with %{public}s", cloudPath.c_str(), STORAGE_CLOUD_DIR.c_str());
        return cloudPath;
    }
    return STORAGE_LOCAL_DIR + cloudPath.substr(STORAGE_CLOUD_DIR.length());
}

std::string MediaPathUtils::ConvertLocalPathToCloudPath(const std::string &localPath)
{
    CHECK_AND_RETURN_RET_LOG(!localPath.empty(), EMPTY_STRING, "localPath is empty");
    if (!MediaStringUtils::StartsWith(localPath, STORAGE_LOCAL_DIR)) {
        MEDIA_ERR_LOG("localPath %{public}s not start with %{public}s", localPath.c_str(), STORAGE_LOCAL_DIR.c_str());
        return localPath;
    }
    return STORAGE_CLOUD_DIR + localPath.substr(STORAGE_LOCAL_DIR.length());
}

std::string MediaPathUtils::DesensitizePath(const std::string &path)
{
    std::string result = path;
    // 优先用含 accountId 的前缀脱敏（隐藏 accountId）；未匹配则回退到无 accountId 前缀脱敏
    std::string accountDir = GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::YES);
    if (path.length() >= accountDir.length() && MediaStringUtils::StartsWith(path, accountDir)) {
        return result.replace(0, accountDir.length(), "*");
    }
    std::string filesDir = GetDir(MediaPathType::CLOUD_FILES, AdaptAccount::NO);
    if (path.length() >= filesDir.length() && MediaStringUtils::StartsWith(path, filesDir)) {
        return result.replace(0, filesDir.length(), "*");
    }
    return path;
}

#ifdef MEDIALIBRARY_MULTI_ACCOUNT_SUPPORT
void MediaPathUtils::RegisterGetAccountId(GetForegroundAccountIdHandler handler)
{
    if (handler != nullptr) {
        handler_ = handler;
    }
}

bool MediaPathUtils::GetForegroundAccountId(int32_t& accountId)
{
    if (handler_ == nullptr) {
        MEDIA_ERR_LOG("handler_ is null, GetForegroundAccountId failed");
        return false;
    }
    return handler_(accountId);
}

std::string MediaPathUtils::RemoveAccountId(const std::string& path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), path, "path is empty");

    if (!MediaStringUtils::StartsWith(path, STORAGE_CLOUD_DIR) &&
        !MediaStringUtils::StartsWith(path, STORAGE_LOCAL_DIR)) {
        return path;
    }

    const std::string FILES_MARKER = "/files/";
    size_t filesPos = path.find(FILES_MARKER);
    if (filesPos == std::string::npos || filesPos == 0) {
        return path;
    }

    // accountId 在 /files/ 之后。
    size_t segStart = filesPos + FILES_MARKER.length();
    size_t segEnd = path.find('/', segStart);
    if (segEnd == std::string::npos) {
        return path;
    }

    std::string accountSeg = path.substr(segStart, segEnd - segStart);
    if (!MediaStringUtils::IsAccountId(accountSeg)) {
        return path;
    }

    return path.substr(0, segStart) + path.substr(segEnd + 1);
}

std::string MediaPathUtils::AppendAccountId(const std::string& path, int32_t accountId)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), path, "path is empty");

    if (!MediaStringUtils::StartsWith(path, STORAGE_CLOUD_DIR) &&
        !MediaStringUtils::StartsWith(path, STORAGE_LOCAL_DIR)) {
        return path;
    }

    const std::string FILES_MARKER = "/files/";
    size_t filesPos = path.find(FILES_MARKER);
    if (filesPos == std::string::npos || filesPos == 0) {
        return path;
    }

    int32_t actualAccountId = accountId;
    if (accountId == -1) {
        if (!GetForegroundAccountId(actualAccountId)) {
            MEDIA_ERR_LOG("failed to get foreground account id");
            return path;
        }
    }

    // accountId 插在 /files/ 之后。先检查是否已存在 accountId。
    size_t segStart = filesPos + FILES_MARKER.length();
    size_t segEnd = path.find('/', segStart);
    if (segEnd != std::string::npos) {
        std::string existingSeg = path.substr(segStart, segEnd - segStart);
        if (MediaStringUtils::IsAccountId(existingSeg)) {
            return path; // 已存在 accountId，跳过
        }
    }
    // path = prefix + files/ + accountId/ + rest
    return path.substr(0, segStart) + std::to_string(actualAccountId) + "/" + path.substr(segStart);
}

bool MediaPathUtils::CheckAccountId(const std::string& path, int32_t accountId)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), false, "path is empty");

    if (!MediaStringUtils::StartsWith(path, STORAGE_CLOUD_DIR) &&
        !MediaStringUtils::StartsWith(path, STORAGE_LOCAL_DIR)) {
        return false;
    }

    const std::string FILES_MARKER = "/files/";
    size_t filesPos = path.find(FILES_MARKER);
    if (filesPos == std::string::npos || filesPos == 0) {
        return false;
    }

    int32_t expectedAccountId = accountId;
    if (accountId == -1) {
        if (!GetForegroundAccountId(expectedAccountId)) {
            MEDIA_ERR_LOG("failed to get foreground account id");
            return false;
        }
    }

    // accountId 在 /files/ 之后。
    size_t segStart = filesPos + FILES_MARKER.length();
    size_t segEnd = path.find('/', segStart);
    if (segEnd == std::string::npos) {
        return false;
    }
    std::string accountSeg = path.substr(segStart, segEnd - segStart);
    if (!MediaStringUtils::IsAccountId(accountSeg)) {
        return false;
    }
    return accountSeg == std::to_string(expectedAccountId);
}

int32_t MediaPathUtils::ParseAccountIdFromPath(const std::string& path)
{
    CHECK_AND_RETURN_RET_LOG(!path.empty(), INVALID_ACCOUNT_ID, "path is empty");

    if (!MediaStringUtils::StartsWith(path, STORAGE_CLOUD_DIR) &&
        !MediaStringUtils::StartsWith(path, STORAGE_LOCAL_DIR)) {
        return INVALID_ACCOUNT_ID;
    }

    const std::string FILES_MARKER = "/files/";
    size_t filesPos = path.find(FILES_MARKER);
    if (filesPos == std::string::npos || filesPos == 0) {
        return INVALID_ACCOUNT_ID;
    }

    // accountId 在 /files/ 之后。
    size_t segStart = filesPos + FILES_MARKER.length();
    size_t segEnd = path.find('/', segStart);
    if (segEnd == std::string::npos) {
        return INVALID_ACCOUNT_ID;
    }
    std::string segment = path.substr(segStart, segEnd - segStart);
    if (!MediaStringUtils::IsAccountId(segment)) {
        return INVALID_ACCOUNT_ID;
    }
    return std::stoi(segment);
}
#endif

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

namespace {
// 单个目录项的拷贝动作：目录补建对应目录，普通文件拷贝文件，其余按 stat 结果报错
int32_t CopyDirEntry(const std::filesystem::directory_entry &entry, const std::string &srcFilePath,
    const std::string &dstFilePath)
{
    std::error_code entryEc;
    if (entry.is_directory(entryEc)) {
        if (!MediaPureFileUtils::CreateDirectory(dstFilePath)) {
            MEDIA_ERR_LOG("Create dir:%{public}s failed", MediaPathUtils::DesensitizePath(dstFilePath).c_str());
            return E_FAIL;
        }
        return E_OK;
    }
    if (!entryEc && entry.is_regular_file(entryEc)) {
        if (MediaPathUtils::IsTranscodeFile(srcFilePath)) {
            return E_OK;
        }
        if (!MediaPureFileUtils::CopyFileUtil(srcFilePath, dstFilePath)) {
            MEDIA_ERR_LOG("Copy file from %{public}s to %{public}s failed.",
                MediaPathUtils::DesensitizePath(srcFilePath).c_str(),
                MediaPathUtils::DesensitizePath(dstFilePath).c_str());
            return E_FAIL;
        }
        return E_OK;
    }
    if (entryEc) {
        MEDIA_ERR_LOG("Stat %{public}s failed: %{public}s", MediaPathUtils::DesensitizePath(srcFilePath).c_str(),
            entryEc.message().c_str());
        return E_FAIL;
    }
    MEDIA_ERR_LOG("Unhandled path type, path:%{public}s", MediaPathUtils::DesensitizePath(srcFilePath).c_str());
    return E_FAIL;
}

// 遍历目录并逐项拷贝，调用方需已完成入参校验
int32_t CopyDirectoryByEntry(const std::string &srcDir, const std::string &dstDir)
{
    std::error_code ec;
    auto iter = std::filesystem::recursive_directory_iterator(srcDir, ec);
    CHECK_AND_RETURN_RET_LOG(!ec, E_FAIL, "Iterator creation failed: %{public}s", ec.message().c_str());
    for (; iter != std::filesystem::recursive_directory_iterator(); iter.increment(ec)) {
        if (ec) {
            // 这里如果有错误码，迭代器会置为end
            MEDIA_ERR_LOG("Iteration error: %{public}s", ec.message().c_str());
            continue;
        }
        const auto &entry = *iter;
        std::string srcFilePath = entry.path();
        std::string tmpFilePath = srcFilePath;
        std::string dstFilePath = tmpFilePath.replace(0, srcDir.length(), dstDir);
        int32_t ret = CopyDirEntry(entry, srcFilePath, dstFilePath);
        CHECK_AND_RETURN_RET(ret == E_OK, ret);
    }
    CHECK_AND_RETURN_RET_LOG(!ec, E_FAIL, "Iterate %{public}s failed: %{public}s",
        MediaPathUtils::DesensitizePath(srcDir).c_str(), ec.message().c_str());
    return E_OK;
}
} // namespace

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
    return CopyDirectoryByEntry(srcDir, dstDir);
}
} // namespace OHOS::Media
