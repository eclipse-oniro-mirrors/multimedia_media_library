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

#include "media_uri_utils.h"

#include <algorithm>
#include <cctype>

#include "media_log.h"
#include "media_string_utils.h"
#include "media_path_utils.h"
#include "media_pure_file_utils.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
const int ASSET_IN_BUCKET_NUM_MAX = 1000;
const int ASSET_DIR_START_NUM = 16;
const int ASSET_MAX_NUM = 10000000;
const int DEFAULT_USER_ID_INNER = -1;

const std::string PHOTO_BUCKET_INNER = "Photo";
const std::string ROOT_MEDIA_DIR = "/storage/cloud/files";
const std::string URI_PREFIX_INNER = "file://media/";
const std::string PHOTO_URI_PREFIX_INNER = "file://media/Photo/";
const std::string MULTI_USER_URI_FLAG = "user";
const std::string GARBLE = "*";
const std::string SPLIT_PATH = "/";
const std::string DOT = ".";
const std::string MEDIA_URI_FILE_SCHEME = "file";
const std::string MEDIA_URI_DATA_SHARE_SCHEME = "datashare";
const std::string MEDIA_URI_AUTHORITY = "media";
const std::string MEDIA_URI_PATH_IDENTIFIER = "/media";
constexpr uint32_t GARBLE_SMALL = 3;
constexpr uint32_t GARBLE_LARGE = 8;
constexpr uint32_t GARBLE_LAST_TWO = 2;
constexpr uint32_t GARBLE_LAST_ONE = 1;

void MediaUriUtils::AppendKeyValue(std::string &uri, const std::string &key, std::string value)
{
    std::string uriKey = key + '=';
    if (uri.find(uriKey) != std::string::npos) {
        return;
    }

    char queryMark = (uri.find('?') == std::string::npos) ? '?' : '&';
    std::string append = queryMark + key + '=' + value;

    size_t pos = uri.find('#');
    if (pos == std::string::npos) {
        uri += append;
    } else {
        uri.insert(pos, append);
    }
}

int32_t MediaUriUtils::CreateAssetBucket(int32_t fileId, int32_t &bucketNum)
{
    if (fileId < 0) {
        MEDIA_ERR_LOG("input fileId [%{public}d] is invalid", fileId);
        return E_INVALID_FILEID;
    }
    fileId = fileId % ASSET_MAX_NUM;
    int start = ASSET_DIR_START_NUM;
    int divider = ASSET_DIR_START_NUM;
    while (fileId > start * ASSET_IN_BUCKET_NUM_MAX) {
        divider = start;
        start *= 2; // 2: Left shift offset
    }

    int fileIdRemainder = fileId % divider;
    if (fileIdRemainder == 0) {
        bucketNum = start + fileIdRemainder;
    } else {
        bucketNum = (start - divider) + fileIdRemainder;
    }
    return E_OK;
}

std::string MediaUriUtils::GetUriByExtrConditions(const std::string &prefix, const std::string &fileId,
    const std::string &suffix)
{
    return prefix + fileId + suffix;
}

std::string MediaUriUtils::GetPathFromUri(const std::string &uri)
{
    size_t index = uri.rfind('/');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("index invalid %{private}s", uri.c_str());
        return "";
    }
    std::string realTitle = uri.substr(0, index);
    index = realTitle.rfind('/');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("invalid realTitle %{private}s", uri.c_str());
        return "";
    }
    realTitle = realTitle.substr(index + 1);
    index = realTitle.rfind('_');
    if (index == std::string::npos) {
        MEDIA_ERR_LOG("realTitle can not find _ %{private}s", uri.c_str());
        return "";
    }
    std::string fileId = realTitle.substr(index + 1);
    int32_t fileUniqueId = 0;
    bool result = MediaStringUtils::ConvertToInt(fileId, fileUniqueId);
    if (!result) {
        MEDIA_ERR_LOG("invalid fileuri %{private}s", uri.c_str());
        return "";
    }
    int32_t bucketNum = 0;
    CreateAssetBucket(fileUniqueId, bucketNum);

    std::string ext = MediaPathUtils::GetExtension(uri);
    if (ext.empty()) {
        MEDIA_ERR_LOG("invalid ext %{private}s", uri.c_str());
        return "";
    }

    std::string path = ROOT_MEDIA_DIR + "/" + PHOTO_BUCKET_INNER + "/" + std::to_string(bucketNum) + "/" + realTitle +
        "." + ext;
    if (!MediaPureFileUtils::IsFileExists(path)) {
        MEDIA_ERR_LOG("file not exist, path=%{private}s", path.c_str());
        return "";
    }
    return path;
}

int32_t MediaUriUtils::GetFileId(const std::string &uri)
{
    const static int ERROR = -1;
    if (PHOTO_URI_PREFIX_INNER.size() >= uri.size()) {
        MEDIA_ERR_LOG("photo uri is too short");
        return ERROR;
    }
    if (uri.substr(0, PHOTO_URI_PREFIX_INNER.size()) != PHOTO_URI_PREFIX_INNER) {
        MEDIA_ERR_LOG("only photo uri is valid");
        return ERROR;
    }
    std::string tmp = uri.substr(PHOTO_URI_PREFIX_INNER.size());
    std::string fileIdStr = tmp.substr(0, tmp.find_first_of('/'));
    int32_t value = 0;
    bool result = MediaStringUtils::ConvertToInt(fileIdStr, value);
    if (!result) {
        MEDIA_ERR_LOG("asset fileId is invalid");
        return ERROR;
    }
    return value;
}

std::string MediaUriUtils::GetFileIdStr(const std::string &uri)
{
    return std::to_string(GetFileId(uri));
}

Uri MediaUriUtils::GetMultiUri(Uri &uri, int32_t userId)
{
    if (userId == DEFAULT_USER_ID_INNER) {
        return uri;
    }
    std::string uriString = uri.ToString();
    AppendKeyValue(uriString, MULTI_USER_URI_FLAG, std::to_string(userId));
    return Uri(uriString);
}

bool MediaUriUtils::CheckUri(const std::string &uri)
{
    if (uri.find("../") != std::string::npos) {
        return false;
    }
    std::string uriprex = "file://media";
    return uri.substr(0, uriprex.size()) == uriprex;
}

// 媒体库 uri 统一形态：{scheme}://{authority}[/media]/{Type}/{fileId}[/{title}[/{displayName}]]，
// fileId 固定落在类型段之后。段名与 PhotoColumn::PHOTO_TYPE_URI、AudioColumn::AUDIO_TYPE_URI、
// PhotoColumn::HIGHTLIGHT_COVER_URI 等常量保持一致。
// fileIdOffset 表示 fileId 相对类型段的偏移：1 为紧跟类型段，2 为中间隔一层。
struct LibraryUriTypeSegment {
    const char *segment;
    size_t fileIdOffset;
};

static const LibraryUriTypeSegment LIBRARY_URI_TYPE_SEGMENTS[] = {
    { "/Photo/", 1 },
    { "/Audio/", 1 },
    { "/hiddenPhoto/", 1 },
    { "/trashedPhoto/", 1 },
    // highlight 固定为 /highlight/video/{assetId}/... 或 /highlight/cover/{id}/...
    { "/highlight/", 2 },
};

static bool IsAllDigits(const std::string &str)
{
    // char 可能是有符号类型，UTF-8 等多字节字符的字节值可能为负，直接传给 isdigit 属于未定义行为
    return !str.empty() && std::all_of(str.begin(), str.end(),
        [](unsigned char c) { return std::isdigit(c) != 0; });
}

// 条件3：fileId 必须是整数，且不为表示无效的 -1（'-' 不是数字，天然被排除）。
// 有类型段时按类型段定位 fileId；没有类型段的 uri（如相册）末段即 fileId。
static bool IsLibraryFileIdValid(const std::string &path)
{
    if (path.empty() || path.back() == '/') {
        return false;
    }
    for (const auto &type : LIBRARY_URI_TYPE_SEGMENTS) {
        std::string typeSegment(type.segment);
        size_t pos = path.find(typeSegment);
        if (pos == std::string::npos) {
            continue;
        }
        size_t begin = pos + typeSegment.size();
        for (size_t level = 1; level < type.fileIdOffset; ++level) {
            begin = path.find('/', begin);
            if (begin == std::string::npos) {
                return false;
            }
            ++begin;
        }
        if (begin >= path.size()) {
            return false;
        }
        size_t end = path.find('/', begin);
        std::string fileId = (end == std::string::npos) ? path.substr(begin) : path.substr(begin, end - begin);
        return IsAllDigits(fileId);
    }
    size_t index = path.rfind('/');
    if (index == std::string::npos) {
        return false;
    }
    return IsAllDigits(path.substr(index + 1));
}

bool MediaUriUtils::IsLibraryUri(const std::string &uri)
{
    // 条件1：不含 ../，避免借媒体库 uri 形态绕过沙箱路径约束
    if (uri.find("../") != std::string::npos) {
        return false;
    }
    Uri parsedUri(uri);
    // 条件2：scheme 只允许 datashare / file
    std::string scheme = parsedUri.GetScheme();
    if (scheme != MEDIA_URI_FILE_SCHEME && scheme != MEDIA_URI_DATA_SHARE_SCHEME) {
        return false;
    }
    // 条件3：fileId 为整数，且不为 -1
    if (!IsLibraryFileIdValid(parsedUri.GetPath())) {
        return false;
    }
    // 条件4：authority 为 media，或 path 以 /media 开头
    return parsedUri.GetAuthority() == MEDIA_URI_AUTHORITY ||
        MediaStringUtils::StartsWith(parsedUri.GetPath(), MEDIA_URI_PATH_IDENTIFIER);
}

static std::string GetTitleFromDisplayName(const std::string &displayName)
{
    std::string title;
    if (!displayName.empty()) {
        std::string::size_type pos = displayName.find_last_of('.');
        if (pos == std::string::npos) {
            return "";
        }
        title = displayName.substr(0, pos);
    }
    return title;
}

std::string MediaUriUtils::GetSafeDiaplayName(const std::string &displayName)
{
    if (displayName == "") {
        return displayName;
    }
    std::string extension;
    size_t splitIndex = displayName.find_last_of(DOT);
    if (splitIndex == std::string::npos) {
        extension = "";
    } else {
        extension = displayName.substr(splitIndex);
    }
    std::string title = GetTitleFromDisplayName(displayName);
    if (title == "") {
        return title;
    }
    uint32_t length = title.size();
    std::string safeDisplayName;
    if (length <= GARBLE_SMALL) {
        safeDisplayName = GARBLE + title.substr(length - GARBLE_LAST_ONE) + extension;
    } else if (length > GARBLE_LARGE) {
        safeDisplayName = GARBLE + title.substr(GARBLE_LARGE) + extension;
    } else {
        safeDisplayName = GARBLE + title.substr(length - GARBLE_LAST_TWO) + extension;
    }
    return safeDisplayName;
}

std::string MediaUriUtils::GetSafeUri(const std::string &uri)
{
    std::string safeUri = uri;
    if (uri == "") {
        return safeUri;
    }
    size_t splitIndex = safeUri.find_last_of(SPLIT_PATH);
    std::string displayName;
    if (splitIndex == std::string::npos) {
        return safeUri;
    } else {
        displayName = safeUri.substr(splitIndex + 1);
    }
    std::string safeDisplayName = GetSafeDiaplayName(displayName);
    safeUri = safeUri.substr(0, splitIndex) + "/" + safeDisplayName;
    return safeUri;
}
} // namespace OHOS::Media