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

#include "media_log.h"
#include "medialibrary_errno.h"

namespace OHOS::Media {
const char DOT = '.';
const std::string EMPTY_STRING = "";

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

} // namespace OHOS::Media