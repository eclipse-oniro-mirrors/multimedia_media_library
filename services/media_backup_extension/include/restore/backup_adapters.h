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
#ifndef OHOS_MEDIA_BACKUP_ADAPTERS
#define OHOS_MEDIA_BACKUP_ADAPTERS

#include <string>
#include <vector>

#include "backup_const.h"

namespace OHOS::Media {
class FileAdapter {
public:
    static bool IsLakeFile(const FileInfo &fileInfo);
    static bool IsFileManagerFile(const FileInfo &fileInfo);
    static bool IsFromDocsLpath(const std::string &lPath);
    static std::string GetOriginalFilePath(const FileInfo &fileInfo);
};
}  // namespace OHOS::Media
#endif