/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MLOG_TAG "Media_Client"

#include "cloud_media_path_utils.h"

#include <string>

#include "media_string_utils.h"
#include "medialibrary_db_const.h"

namespace OHOS::Media::CloudSync {
const int32_t FILE_SOURCE_TYPE_DOCS = 1;
const int32_t FILE_SOURCE_TYPE_LAKE = 3;
const std::string LAKE_STORAGE_PATH_PREFIX = "/storage/media/local/files/Docs/HO_DATA_EXT_MISC/";
const std::string LAKE_STORAGE_PATH_PATTERN = "/mnt/data/{0}/HO_MEDIA/{1}";
const std::string DOCS_STORAGE_PATH_PREFIX = "/storage/media/local/files/Docs/";
const std::string DOCS_STORAGE_PATH_PATTERN = "/storage/media/{0}/local/files/Docs/{1}";
const std::string CLOUD_STORAGE_PATH_PREFIX = "/storage/cloud/files/Photo/";
const std::string CLOUD_STORAGE_PATH_PATTERN = "/data/service/el2/{0}/hmdfs/account/files/Photo/{1}";
std::string CloudMediaPathUtils::FindStoragePath(const std::string &storagePath, const int32_t userId)
{
    if (MediaStringUtils::StartsWith(storagePath, LAKE_STORAGE_PATH_PREFIX)) {
        std::string relativePath = storagePath.substr(LAKE_STORAGE_PATH_PREFIX.length());
        return MediaStringUtils::FillParams(LAKE_STORAGE_PATH_PATTERN, {std::to_string(userId), relativePath});
    }

    if (MediaStringUtils::StartsWith(storagePath, DOCS_STORAGE_PATH_PREFIX)) {
        std::string relativePath = storagePath.substr(DOCS_STORAGE_PATH_PREFIX.length());
        return MediaStringUtils::FillParams(DOCS_STORAGE_PATH_PATTERN, {std::to_string(userId), relativePath});
    }

    if (MediaStringUtils::StartsWith(storagePath, CLOUD_STORAGE_PATH_PREFIX)) {
        std::string relativePath = storagePath.substr(CLOUD_STORAGE_PATH_PREFIX.length());
        return MediaStringUtils::FillParams(CLOUD_STORAGE_PATH_PATTERN, {std::to_string(userId), relativePath});
    }

    // if the storagePath doesn't match any known pattern, return empty string to indicate it's not a valid path.
    return std::string();
}

/**
 * 根据 fileSourceType ，返回带 userId 的 cloudPath 或者 storagePath 路径信息
 * @return 文件路径信息（带userId）
 * 数据场景：
 * | fileSourceType | 返参 |
 * |----------------|-----|
 * | LAKE(3)        | storagePath |
 * | FILE_MANAGER(1)| storagePath |
 * | 其他            | data |
 *
 * 说明：此处不考虑纯云湖内资产(非隐藏&非回收站)的数据场景；
 * | storagePath          | position | fileSourceType | 文件存储 |
 * |----------------------|----------|----------------|---------|
 * | ./HO_DATA_EXT_MISC/. | 2        | MEDIA(0)       | storagePath |
 */
std::string CloudMediaPathUtils::FindStoragePath(
    const int32_t fileSourceType, const std::string &cloudPath, const std::string &storagePath, const int32_t userId)
{
    bool isStoragePathValid = (fileSourceType == FILE_SOURCE_TYPE_LAKE);
    isStoragePathValid = isStoragePathValid || (fileSourceType == FILE_SOURCE_TYPE_DOCS);
    if (isStoragePathValid) {
        return FindStoragePath(storagePath, userId);
    }
    return FindStoragePath(cloudPath, userId);
}
}  // namespace OHOS::Media::CloudSync