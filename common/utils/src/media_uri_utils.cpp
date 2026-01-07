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
const std::string PHOTO_URI_PREFIX_INNER = "file://media/Photo/";
const std::string MULTI_USER_URI_FLAG = "user";

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
    const static std::string ERROR = "-1";
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
    if (fileIdStr.empty()) {
        MEDIA_ERR_LOG("fileId is empty");
        return ERROR;
    }
    return fileIdStr;
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
} // namespace OHOS::Media