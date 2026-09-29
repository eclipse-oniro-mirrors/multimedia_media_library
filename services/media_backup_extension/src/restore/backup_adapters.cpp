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
#include "backup_adapters.h"

#include <algorithm>
#include <cctype>

#include "medialibrary_db_const.h"
#include "media_log.h"

namespace OHOS::Media {
static bool IsSourceTypeValid(const FileInfo &fileInfo, FileSourceType expectedType)
{
    return fileInfo.fileSourceType == expectedType && !fileInfo.storagePath.empty();
}

bool FileAdapter::IsLakeFile(const FileInfo &fileInfo)
{
    return IsSourceTypeValid(fileInfo, FileSourceType::MEDIA_HO_LAKE);
}

bool FileAdapter::IsFileManagerFile(const FileInfo &fileInfo)
{
    return IsSourceTypeValid(fileInfo, FileSourceType::FILE_MANAGER);
}

// Case-insensitive prefix check. Returns true when `str` starts with `prefix` regardless of case.
// Used because file-system / lPath comparisons are case-insensitive on the device.
static bool HasCaseInsensitivePrefix(const std::string &str, const std::string &prefix)
{
    if (str.size() < prefix.size()) {
        return false;
    }
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(str[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

bool FileAdapter::IsFromDocsLpath(const std::string &lPath)
{
    // Strict match: exact "/FromDocs" or starts with "/FromDocs/" (case-insensitive).
    // Reject "/FromDocsBackup", "/FromDocs2", etc.
    if (lPath.size() == FROMDOCS_LPATH_PREFIX.size() &&
        HasCaseInsensitivePrefix(lPath, FROMDOCS_LPATH_PREFIX)) {
        return true;
    }
    static const std::string FROMDOCS_SLASH = FROMDOCS_LPATH_PREFIX + "/";
    return HasCaseInsensitivePrefix(lPath, FROMDOCS_SLASH);
}

std::string FileAdapter::GetOriginalFilePath(const FileInfo &fileInfo)
{
    CHECK_AND_RETURN_RET(!IsLakeFile(fileInfo), fileInfo.storagePath);
    return fileInfo.cloudPath;
}
}