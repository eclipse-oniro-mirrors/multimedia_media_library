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

#ifndef COMMON_UTILS_MEDIA_PATH_UTILS_H_
#define COMMON_UTILS_MEDIA_PATH_UTILS_H_

#include <string>
#include <sys/stat.h>

#include "userfile_manager_types.h"

namespace OHOS::Media {
#define EXPORT __attribute__ ((visibility ("default")))

/**
 * media path utils which the ability process path
 */
class MediaPathUtils {
public:
    EXPORT MediaPathUtils();
    EXPORT ~MediaPathUtils();
    EXPORT static std::string GetFileName(const std::string &filePath);
    EXPORT static std::string GetExtension(const std::string &path);
};
} // namespace OHOS::Media

#endif // COMMON_UTILS_MEDIA_PATH_UTILS_H_
